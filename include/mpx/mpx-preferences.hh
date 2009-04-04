//  MPX
//  Copyright (C) 2005-2007 MPX development.
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

#ifndef MPX_PREFERENCES_HH
#define MPX_PREFERENCES_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#ifndef PLUGIN_BUILD
#include "mpx/mpx-audio.hh"
#endif

#include "mpx/mpx-services.hh"
#include "mpx/widgets/widgetloader.hh"

#include <set>
#include <string>
#include <vector>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <mcs/mcs.h>
#include <mcs/gtk-bind.h>

namespace MPX
{
  class CoverArtSourceView;
  class FileFormatPrioritiesView;

  /** Preferences dialog
   *
   * MPX::Preferences is a complex dialog for adjusting run time parameters
   * of MPX trough the GUI instead of having to manipulate the configuration
   * file.
   */
  class Preferences
  : public Gnome::Glade::WidgetLoader<Gtk::Window>
  , public Service::Base
  {
    public:

        Preferences(
            const Glib::RefPtr<Gnome::Glade::Xml>&
        ) ;

        static Preferences*
        create(
        ) ;

        virtual
        ~Preferences(
        ) ;

        virtual void
        present(
        ) ;

        void
        add_page(
              Gtk::Widget*
            , const std::string&
        ) ;

#ifndef PLUGIN_BUILD
    protected:

        virtual bool
        on_delete_event(GdkEventAny*) ;

        virtual void
        hide() ;

    private:

        Gtk::Notebook               * m_notebook_preferences ;

        // Misc
        Gtk::SpinButton             * m_Radio_MinimalBitrate ;

      	CoverArtSourceView          * m_Covers_CoverArtSources ; 

        // File Formats
        FileFormatPrioritiesView    * m_Fmts_PrioritiesView ;
        Gtk::CheckButton            * m_Fmts_PrioritizeByFileType ;
        Gtk::CheckButton            * m_Fmts_PrioritizeByBitrate ;

        // Library
        Gtk::CheckButton            * m_Library_RescanAtStartup ;
        Gtk::CheckButton            * m_Library_RescanInIntervals ;
        Gtk::SpinButton             * m_Library_RescanInterval ; 
        Gtk::HBox                   * m_Library_RescanIntervalBox ;
        Gtk::ToggleButton           * m_Library_RescanAlwaysVacuum ;
#endif // PLUGIN_BUILD

#ifdef HAVE_HAL

        void
        on_library_use_hal_toggled() ;

        Gtk::RadioButton            * m_Library_UseHAL_Yes ;
        Gtk::RadioButton            * m_Library_UseHAL_No ;

#endif // hAVE_HAL

  } ; // class Preferences
} // namespace MPX

#endif // MPX_PREFERENCES_HH
