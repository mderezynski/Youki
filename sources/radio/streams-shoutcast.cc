//  MPX
//  Copyright (C) 2005-2009 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <sstream>
#include <cstring>

#include <glibmm.h>
#include <glibmm/markup.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <gtk/gtkdialog.h>

#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>
#include "mpx/widgets/dialog-simple-entry.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/xml/xml.hh"
#include "streams-shoutcast.hh"
using namespace Glib;
using namespace Gtk;

#define SHOUTCAST_HOST        "www.shoutcast.com"
#define SHOUTCAST_PATH        "sbin/newxml.phtml"

namespace
{
  char const* CUSTOM_SEARCH = N_("Custom Search");
}

namespace MPX
{
  namespace RadioDirectory
  {
      Shoutcast::Shoutcast (BaseObjectType                 * obj,
                            RefPtr<Gnome::Glade::Xml> const& xml)
      : TreeView (obj)
      {
        Data.Cache = g_hash_table_new_full (g_str_hash,g_str_equal,GDestroyNotify(g_free),GDestroyNotify(xmlFreeDoc));

        CellRendererText *cell;
        cell = manage ( new CellRendererText() );
        append_column (_("Genre"), *cell);
        get_column (0)->set_resizable (false);
        get_column (0)->set_sizing (TREE_VIEW_COLUMN_AUTOSIZE);
        Data.Genres = ListStore::create (columns);
        set_model (Data.Genres);
        get_column (0)->set_cell_data_func (*cell, sigc::mem_fun (this, &MPX::RadioDirectory::Shoutcast::Shoutcast::genre_cell_data_func));
        get_selection()->set_mode (SELECTION_BROWSE);
        get_selection()->signal_changed().connect (sigc::mem_fun (this, &MPX::RadioDirectory::Shoutcast::Shoutcast::refresh_wrapper));

        refresh ();
      }

      Shoutcast::~Shoutcast ()
      {
        g_hash_table_destroy (Data.Cache);
      }

      SignalListUpdated&
      Shoutcast::signal_list_updated ()
      {
        return Signals.ListUpdated;
      }

      SignalStartStop&
      Shoutcast::signal_start ()
      {
        return Signals.Start;
      }

      SignalStartStop&
      Shoutcast::signal_stop ()
      {
        return Signals.Stop;
      }

      void
      Shoutcast::genre_cell_data_func (CellRenderer                *cell,
                                       TreeModel::iterator const&   iter)
      {
        CellRendererText *cell_text = reinterpret_cast<CellRendererText *>(cell);
        ustring genre = (*iter)[columns.name];

        if (g_hash_table_lookup (Data.Cache, genre.c_str()))
            cell_text->property_markup () = ((boost::format ("<b>%s</b>")) % std::string(Markup::escape_text(genre))).str();
        else
            cell_text->property_markup () = Markup::escape_text (genre);
      }

      void
      Shoutcast::refresh ()
      {
        xmlDocPtr             doc;
        xmlXPathObjectPtr     xpathobj;
        xmlNodeSetPtr         nv;

        MPX::URI u ((boost::format ("http://%s/%s") % SHOUTCAST_HOST % SHOUTCAST_PATH).str());
        Soup::RequestSyncRefP request = Soup::RequestSync::create (ustring (u));
        guint code = request->run ();
        if (code != 200) return;

        std::string data = request->get_data(); 
        doc = xmlParseMemory ((const char*)(data.c_str()), data.size());
        if (!doc) return;

        xpathobj = xpath_query (doc, BAD_CAST "//genre", NULL);
        nv = xpathobj->nodesetval;

        Data.Genres->clear();
        (*Data.Genres->append())[columns.name] = "Top500";
        (*Data.Genres->append())[columns.name] = _(CUSTOM_SEARCH);
        for (int n = 0; n < nv->nodeNr; n++)
        {
          xmlChar *prop = xmlGetProp (nv->nodeTab[n], BAD_CAST "name");
          (*Data.Genres->append())[columns.name] = reinterpret_cast<char*>(prop);
          g_free (prop);
        }

        xmlXPathFreeObject (xpathobj);
        xmlFreeDoc (doc);
      }

      void
      Shoutcast::refresh_wrapper ()
      {
        if( get_selection()->count_selected_rows() )
        {
            TreeIter iter = get_selection()->get_selected();
            TreePath path = Data.Genres->get_path (iter);

            if( (path.get_indices().data()[0] != 1 )
                 && (!Data.CurrentIter || (Data.CurrentIter != get_selection()->get_selected())))
            {
                Data.CustomSearch.reset();
                Data.CurrentIter = iter; 
                rebuild_list(false); 
            }
            else
            if( path.get_indices().data()[0] == 1 )
            {
                boost::shared_ptr<DialogSimpleEntry> p = boost::shared_ptr<DialogSimpleEntry>(DialogSimpleEntry::create());

                p->set_title (_("AudioSource: Custom Search (ShoutCast)"));
                p->set_heading (_("Please enter search terms to search for stations:"));

                ustring text;
                int response = p->run (text);
                p->hide ();

                if( response == GTK_RESPONSE_OK )
                { 
                  Data.CustomSearch = text;
                  Data.StreamList.clear();
                  Signals.Start.emit ();

                  (*iter)[columns.name] = (boost::format("%s: %s") % (_(CUSTOM_SEARCH)) % text.c_str()).str();

                  URI u ((boost::format ("http://%s/%s?search=%s") % SHOUTCAST_HOST % SHOUTCAST_PATH % URI::escape_string (text).c_str()).str());
                  Data.Request = Soup::Request::create (ustring (u));
                  Data.Request->request_callback().connect (sigc::bind (sigc::mem_fun (*this, &Shoutcast::refresh_callback), std::string()));
                  Data.Request->run ();
              }
            }
        }
      }

      void
      Shoutcast::refresh_parse_and_emit_updated (xmlDocPtr doc)
      {
        xmlXPathObjectPtr     xpathobj;
        xmlNodeSetPtr         nv;
        xmlNodePtr            node;

        char const * P_NAME     = "name";
        char const * P_ID       = "id";
        char const * P_BITRATE  = "br";
        char const * P_GENRE    = "genre";
        char const * P_CURRENT  = "ct";
 
        xpathobj = xpath_query (doc, BAD_CAST "//station", NULL);
        nv = xpathobj->nodesetval;
        for (int n = 0; n < nv->nodeNr; ++n) 
        {
          StreamInfo      info;
          xmlChar       * prop;

          node = nv->nodeTab[n];
          prop = xmlGetProp (node, BAD_CAST P_ID);
          info.uri = ustring ("http://www.shoutcast.com/sbin/tunein-station.pls?id=").append(Markup::escape_text((char*)prop));
          g_free (prop);

          prop = xmlGetProp (node, BAD_CAST P_GENRE);
          info.genre = ustring(reinterpret_cast<char*>(prop));
          g_free (prop);

          prop = xmlGetProp (node, BAD_CAST P_NAME);
          info.name = ustring(reinterpret_cast<char*>(prop));
          g_free (prop);

          prop = xmlGetProp (node, BAD_CAST P_BITRATE);
          info.bitrate = atoi(reinterpret_cast<char*>(prop));
          g_free (prop);

          prop = xmlGetProp (node, BAD_CAST P_CURRENT);
          info.current = ustring(reinterpret_cast<char*>(prop));
          g_free (prop);

          Data.StreamList.push_back (info);
          while (gtk_events_pending()) gtk_main_iteration();
        }
        xmlXPathFreeObject (xpathobj);
        Signals.ListUpdated.emit(Data.StreamList, Data.CustomSearch);
      }

      void
      Shoutcast::refresh_callback (char const* data, guint size, guint code, std::string const& genre)
      {
        if (code != 200)
        {
          Data.Request.clear();
          Signals.Stop.emit ();
          return;
        }

        xmlDocPtr doc = xmlParseMemory (data, size); 
        if (!doc)
        {
          Data.Request.clear();
          Signals.Stop.emit ();
          return;
        }

        Data.Request.clear();

        if( !genre.empty() )
        {
          g_hash_table_insert (Data.Cache, strdup (genre.c_str()), gpointer (doc));
        }

        refresh_parse_and_emit_updated (doc);
      }

      void
      Shoutcast::rebuild_list (bool force)
      {
        if (!get_selection()->count_selected_rows())
          return;

        Data.StreamList.clear ();
        Signals.Start.emit ();

        TreeModel::iterator iter = get_selection()->get_selected ();
        ustring genre = (*iter)[columns.name];
    
        if (g_hash_table_lookup (Data.Cache, genre.c_str()) && !force)
        {
          xmlDocPtr doc = static_cast<xmlDocPtr>(g_hash_table_lookup (Data.Cache, genre.c_str()));
          refresh_parse_and_emit_updated (doc);
          return;
        }

        if (g_hash_table_lookup (Data.Cache, genre.c_str()) && force)
        {
          g_hash_table_remove (Data.Cache, genre.c_str());
        }

        URI u ((boost::format ("http://%s/%s?genre=%s") % SHOUTCAST_HOST % SHOUTCAST_PATH % URI::escape_string (genre).c_str()).str());
        Data.Request = Soup::Request::create (ustring (u));
        Data.Request->request_callback().connect (sigc::bind (sigc::mem_fun (*this, &Shoutcast::refresh_callback), genre));
        Data.Request->run ();
      }

      void
      Shoutcast::on_row_activated (TreeModel::Path const& path, TreeViewColumn * column)
      {
        TreeModel::iterator iter (Data.Genres->get_iter (path));
        ustring genre = (*iter)[columns.name];
        if (g_hash_table_lookup (Data.Cache, genre.c_str()))
        {
          rebuild_list(true);
        }
      }
    }
}
