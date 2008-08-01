//
//  mpx-source-handling.cc
//
//  Authors:
//      Milosz Derezynski <milosz@backtrace.info>
//
//  (C) 2008 MPX Project
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

// PyGTK NO_IMPORT
#define NO_IMPORT

#include "config.h"
#include <boost/optional.hpp>
#include <boost/python.hpp>
#include <boost/algorithm/string.hpp>
#include <glibmm.h>
#include <Python.h>
#include <pygobject.h>
#include "mpx/util-string.hh"

#include "mpx.hh"
#include "play.hh"
#include "sidebar.hh"
#include "infoarea.hh"

using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace Gnome::Glade;
using namespace MPX::Util;
using namespace boost::python;
using boost::get;
using boost::algorithm::split;
using boost::algorithm::is_any_of;

namespace
{
  inline bool
  is_module (std::string const& basename)
  {
    return MPX::Util::str_has_suffix_nocase(basename.c_str (), G_MODULE_SUFFIX);
  } 
}

namespace MPX
{
    bool
    Player::source_load (std::string const& path)
    {
		enum
		{
			LIB_BASENAME,
			LIB_PLUGNAME,
			LIB_SUFFIX
		};

		const std::string type = "mpxsource";

		std::string basename (path_get_basename (path));
		std::string pathname (path_get_dirname  (path));

		if (!is_module (basename))
			return false;

		StrV subs; 
		split (subs, basename, is_any_of ("-."));
		std::string name  = type + std::string("-") + subs[LIB_PLUGNAME];
		std::string mpath = Module::build_path (build_filename(PLUGIN_DIR, "sources"), name);

		Module module (mpath, ModuleFlags (0)); 
		if (!module)
		{
			g_message("Source plugin load FAILURE '%s': %s", mpath.c_str (), module.get_last_error().c_str());
			return false;
		}

		g_message("LOADING Source plugin: %s", mpath.c_str ());

		module.make_resident();

		SourcePluginPtr plugin = SourcePluginPtr (new SourcePlugin());
		if (!g_module_symbol (module.gobj(), "get_instance", (gpointer*)(&plugin->get_instance)))
		{
          g_message("Source plugin load FAILURE '%s': get_instance hook missing", mpath.c_str ());
		  return false;
		}

        if (!g_module_symbol (module.gobj(), "del_instance", (gpointer*)(&plugin->del_instance)))
		{
          g_message("Source plugin load FAILURE '%s': del_instance hook missing", mpath.c_str ());
		  return false;
		}

		
        PlaybackSource * p = plugin->get_instance(m_ui_manager, *this); 
        Gtk::Alignment * a = new Gtk::Alignment;

        if(p->get_ui()->get_parent())
            p->get_ui()->reparent(*a);
        else
            a->add(*p->get_ui());
        a->show();

        m_Sidebar->addItem(
            p->get_name(),
            a,  
            p,
            p->get_icon()->scale_simple(
                    20,
                    20,
                    Gdk::INTERP_BILINEAR
            ),
            m_NextSourceId
        );

        ItemKey key (boost::optional<gint64>(), m_NextSourceId++);
		m_Sources[key] = p;
		source_install(key);

		return false;
    }

	void
	Player::source_install (ItemKey const& source_id)
	{
        g_message("%s: Installing Source '%s'", G_STRLOC, m_Sources[source_id]->get_name().c_str());

        m_Sources[source_id]->set_key(source_id);

		m_Sources[source_id]->signal_caps().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_caps), source_id));

		m_Sources[source_id]->signal_flags().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_flags), source_id));

		m_Sources[source_id]->signal_playback_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_play_request), source_id));

		m_Sources[source_id]->signal_stop_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_stop), source_id));

		m_Sources[source_id]->signal_play_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::play_async_cb), source_id));

		m_Sources[source_id]->signal_next_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::next_async_cb), source_id));

		m_Sources[source_id]->signal_prev_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::prev_async_cb), source_id));

		m_Sources[source_id]->signal_segment().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_segment), source_id));

		m_Sources[source_id]->signal_track_metadata().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_track_metadata), source_id));

		m_Sources[source_id]->signal_items().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_items), source_id));

		m_Sources[source_id]->send_caps();
		m_Sources[source_id]->send_flags();

		UriSchemes v = m_Sources[source_id]->get_uri_schemes ();
		if(v.size())
		{
		  for(UriSchemes::const_iterator i = v.begin(); i != v.end(); ++i)
		  {
				if(m_UriMap.find(*i) != m_UriMap.end())
				{
				  g_warning("%s: Source '%s' tried to override URI handler for scheme '%s'!",
					  G_STRLOC,
					  m_Sources[source_id]->get_name().c_str(),
					  i->c_str());	
				  continue;
				}
				g_message("%s: Source '%s' registers for URI scheme '%s'", G_STRLOC, m_Sources[source_id]->get_name().c_str(), i->c_str()); 
				m_UriMap.insert (std::make_pair (std::string(*i), source_id));
		  }
		}

        m_Sources[source_id]->post_install();

#if 0
        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
        try{
            using namespace boost::python;
            object obj(boost::ref(m_Sources[source_id]));
    		g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_SOURCE], 0, obj.ptr());
        } catch( boost::python::error_already_set )
        {
            PyErr_Print();
        }
        pyg_gil_state_release(state);
#endif
	}

    void
    Player::on_source_items(gint64 count, ItemKey const& key)
    {
        m_Sidebar->setItemCount(key, count);
    }

    PyObject*
    Player::get_source(std::string const& uuid)
    {
        for(SourcesMap::iterator i = m_Sources.begin(); i != m_Sources.end(); ++i)
        {
            if((*i).second->get_guid() == uuid)
            {
                return (*i).second->get_py_obj();
            }
        }

        Py_INCREF(Py_None);
        return Py_None;
    }

    PyObject*
    Player::get_sources_by_class(std::string const& uuid)
    {
        using namespace boost::python;
        boost::python::list l;
        for(SourcesMap::iterator i = m_Sources.begin(); i != m_Sources.end(); ++i)
        {
            if((*i).second->get_class_guid() == uuid)
            {
                l.append(object(handle<>(borrowed((*i).second->get_py_obj()))));
            }
        }

        Py_INCREF(l.ptr());
        return l.ptr();
    }

    void
    Player::add_subsource(PlaybackSource* p, ItemKey const& parent, gint64 id)
    {
        Gtk::Alignment * a = new Gtk::Alignment;

        if(p->get_ui()->get_parent())
            p->get_ui()->reparent(*a);
        else
            a->add(*p->get_ui());
        a->show();

        m_Sidebar->addSubItem(
            p->get_name(),
            a,  
            p,
            p->get_icon()->scale_simple(
                    20,
                    20,
                    Gdk::INTERP_BILINEAR
            ),
            parent.second,
            id
        );
        ItemKey key (parent.second, id);
		m_Sources[key] = p;
		source_install(key);
    }

	void
	Player::on_source_caps (Caps caps, ItemKey const& source_id)
	{
        Mutex::Lock L (m_SourceCFLock);

        m_source_c[source_id] = caps;

        if( m_Sidebar->getVisibleId() == source_id )
        {
          set_caps(C_CAN_PLAY, caps & C_CAN_PLAY);
        }

        if( m_ActiveSource && source_id == m_ActiveSource.get() )
        {
              set_caps(C_CAN_GO_PREV, caps & C_CAN_GO_PREV);
              set_caps(C_CAN_GO_NEXT, caps & C_CAN_GO_NEXT);
        }
        else
        {
              del_caps(C_CAN_GO_PREV);
              del_caps(C_CAN_GO_NEXT);
        }
	}

	void
	Player::on_source_flags (Flags flags, ItemKey const& source_id)
	{
        Mutex::Lock L (m_SourceCFLock);
        m_source_f[source_id] = flags;
	}

	void
	Player::on_source_track_metadata (Metadata const& metadata, ItemKey const& source_id)
	{
        set_metadata(metadata, source_id);
	}

	void
	Player::on_source_play_request (ItemKey const& source_id)
	{
        if( m_ActiveSource ) 
        {
            track_played();
            m_Sources[m_ActiveSource.get()]->stop ();
            m_Sources[m_ActiveSource.get()]->send_caps ();
            m_ActiveSource.reset();
        }

        PlaybackSource* source = m_Sources[source_id];

        if( m_source_f[source_id] & F_ASYNC)
        {
              set_caps(C_CAN_STOP);
              source->play_async ();
        }
        else
        {
                if( source->play () )
                {
                  m_PlayDirection   = PD_PLAY;
                  m_PreparingSource = source_id;
                  switch_stream (source->get_uri(), source->get_type());
                  return;
                }
        }

        stop();
	}

	void
	Player::on_source_segment (ItemKey const& source_id)
	{
        g_return_if_fail( m_ActiveSource == source_id );
        m_Sources[source_id]->segment ();
	}

	void
	Player::on_source_stop (ItemKey const& source_id)
	{
        if(m_ActiveSource == source_id || m_PreparingSource == source_id)
        {
            stop ();
        }
	}

	void
	Player::on_source_changed (ItemKey const& source_id)
	{
	    Mutex::Lock L (m_SourceCFLock);

        if(m_SourceUI)
        {
            m_ui_manager->remove_ui(m_SourceUI);
        }

        Caps caps = m_source_c[source_id];

        if( m_Sources.count(source_id) ) 
        {
            m_SourceUI = m_Sources[source_id]->add_menu();

            if( (m_Play.property_status() == PLAYSTATUS_PLAYING)
                && ( m_ActiveSource && m_ActiveSource.get() == source_id ))
            {
                Caps caps = m_source_c[source_id];
                set_caps(C_CAN_GO_PREV, caps & C_CAN_GO_PREV);
                set_caps(C_CAN_GO_NEXT, caps & C_CAN_GO_NEXT);
            }
            else
            {
                del_caps(C_CAN_GO_PREV);
                del_caps(C_CAN_GO_NEXT);
            }

            set_caps(C_CAN_PLAY, caps & C_CAN_PLAY);
        }
	}

	void
	Player::play_async_cb (ItemKey const& source_id)
	{
        PlaybackSource* source = m_Sources[source_id];

        m_PreparingSource = source_id;
        m_PlayDirection = PD_PLAY;

        switch_stream(source->get_uri(), source->get_type());
	}

	void
	Player::next_async_cb (ItemKey const& source_id)
	{
        PlaybackSource* source = m_Sources[source_id];
        Flags f = m_source_f[source_id];

        if( (f & F_PHONY_NEXT) == 0 )
        {
              m_PlayDirection = PD_NEXT;
              switch_stream (source->get_uri(), source->get_type());
        }
	}

	void
	Player::prev_async_cb (ItemKey const& source_id)
	{
        PlaybackSource* source = m_Sources[source_id];
        Flags f = m_source_f[source_id];

        if( (f & F_PHONY_PREV) == 0 )
        {
              m_PlayDirection = PD_PREV;
              switch_stream (source->get_uri(), source->get_type());
        }
	}
}
