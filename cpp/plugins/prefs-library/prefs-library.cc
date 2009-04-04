//  MPX
//  Copyright (C) 2003-2007 MPX Development
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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#include "config.h"

#include <utility>
#include <iostream>

#include <glibmm.h>
#include <glib/gi18n.h>
#include <gtk/gtkstock.h>
#include <gtkmm.h>
#include <libglademm.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <mcs/mcs.h>

#include "mpx/mpx-preferences.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/widgets/widgetloader.hh"

#include "prefs-library.hh"

using namespace Glib;
using namespace Gtk;

namespace MPX
{
    PrefsLibrary*
        PrefsLibrary::create(gint64 id)
    {
        return new PrefsLibrary (Gnome::Glade::Xml::create (build_filename(DATA_DIR, "glade" G_DIR_SEPARATOR_S "cppmod-prefs-library.glade")), id );
    }

    PrefsLibrary::~PrefsLibrary ()
    {
    }

    PrefsLibrary::PrefsLibrary(
          const Glib::RefPtr<Gnome::Glade::Xml>&    xml
        , gint64                                    id
    )
        : Gnome::Glade::WidgetLoader<Gtk::VBox>(xml, "cppmod-prefs-library")
        , PluginHolderBase()
    {
        show() ;

        m_Name = "PreferencesModule LIBRARY" ;
        m_Description = "This plugin provides library preferences" ;
        m_Authors = "M. Derezynski" ;
        m_Copyright = "(C) 2009 MPX Project" ;
        m_IAge = 0 ;
        m_Website = "http://redmine.sivashs.org/projects/mpx" ;
        m_Active = false ;
        m_HasGUI = false ;
        m_CanActivate = false ;
        m_Id = id ;

        boost::shared_ptr<Preferences> p = services->get<Preferences>("mpx-service-preferences") ;

        p->add_page(
              this
            , _("Music Library")
        ) ;

        //// LIBRARY
        mcs_bind->bind_filechooser(
            *dynamic_cast<Gtk::FileChooser*>( m_Xml->get_widget( "preferences-fc-music-import-path" ))
            , "mpx"
            , "music-import-path"
            );

        m_Xml->get_widget("rescan-at-startup", m_Library_RescanAtStartup);
        m_Xml->get_widget("rescan-in-intervals", m_Library_RescanInIntervals);
        m_Xml->get_widget("rescan-interval", m_Library_RescanInterval);
        m_Xml->get_widget("hbox-rescan-interval", m_Library_RescanIntervalBox);

        mcs_bind->bind_spin_button(
            *m_Library_RescanInterval
            , "library"
            , "rescan-interval"
            );

        mcs_bind->bind_toggle_button(
            *m_Library_RescanAtStartup
            , "library"
            , "rescan-at-startup"
            );

        mcs_bind->bind_toggle_button(
            *m_Library_RescanInIntervals
            , "library"
            , "rescan-in-intervals"
            );

        m_Library_RescanInIntervals->signal_toggled().connect(
            sigc::compose(
            sigc::mem_fun(*m_Library_RescanIntervalBox, &Gtk::Widget::set_sensitive),
            sigc::mem_fun(*m_Library_RescanInIntervals, &Gtk::ToggleButton::get_active)
            ));

#ifdef HAVE_HAL
        m_Xml->get_widget("lib-use-hal-rb1", m_Library_UseHAL_Yes);
        m_Xml->get_widget("lib-use-hal-rb2", m_Library_UseHAL_No);

        if( mcs->key_get<bool>("library","use-hal") )
            m_Library_UseHAL_Yes->set_active();
        else
            m_Library_UseHAL_No->set_active();

        m_Library_UseHAL_Yes->signal_toggled().connect(
            sigc::mem_fun(
            *this,
            &PrefsLibrary::on_library_use_hal_toggled
            ));

#else
        m_Xml->get_widget("vbox135")->hide();
#endif // HAVE_HAL
    }

#ifdef HAVE_HAL
    void
        PrefsLibrary::on_library_use_hal_toggled()
    {
        boost::shared_ptr<Library> l = services->get<Library>("mpx-service-library");

        if( m_Library_UseHAL_Yes->get_active() )
        {
            m_Xml->get_widget("vbox135")->set_sensitive( false );
            g_message("%s: Switching to HAL mode", G_STRLOC);
            //l->switch_mode( true );
            mcs->key_set("library","use-hal", true);
            m_Xml->get_widget("vbox135")->set_sensitive( true );
        }
        else
        if( m_Library_UseHAL_No->get_active() )
        {
            m_Xml->get_widget("vbox135")->set_sensitive( false );
            g_message("%s: Switching to NO HAL mode", G_STRLOC);
            //l->switch_mode( false );
            mcs->key_set("library","use-hal", false);
            m_Xml->get_widget("vbox135")->set_sensitive( true );
        }
    }
#endif

}  // namespace MPX
