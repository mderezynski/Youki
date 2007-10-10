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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.
#ifndef MPX_HH
#define MPX_HH
#include "config.h"
#include <gio/gasyncresult.h>
#include <gio/gfile.h>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include "library.hh"
#include "widgetloader.h"

using namespace Gnome::Glade;

namespace Gtk
{
    class Statusbar;
}

namespace MPX
{
    class Sources;
    class InfoArea;
    class Player
      : public WidgetLoader<Gtk::Window>
    {
      protected:

        virtual bool on_key_press_event (GdkEventKey*);

      private:

        Glib::RefPtr<Gnome::Glade::Xml>   m_ref_xml;
        Glib::RefPtr<Gtk::ActionGroup>    m_actions;
        Glib::RefPtr<Gtk::UIManager>      m_ui_manager;

        Sources *m_Sources;
        InfoArea *m_InfoArea;
        Gtk::Statusbar *m_Statusbar;

        Library & m_Library;
    
        void
        on_library_scan_start();

        void
        on_library_scan_run(gint64,gint64);

        void
        on_library_scan_end(gint64,gint64);



        GFile *m_MountFile;
        GMountOperation *m_MountOperation;
        Glib::ustring m_Share, m_ShareName;

        static void
        mount_ready_callback (GObject*,
                              GAsyncResult*,
                              gpointer);

        static void
        unmount_ready_callback (GObject*,
                              GAsyncResult*,
                              gpointer);

        void
        on_import_folder();

        void
        on_import_share();

      public:

        Player (const Glib::RefPtr<Gnome::Glade::Xml>&, MPX::Library &);
        virtual ~Player ();

        static Player*
        create (MPX::Library&);
    };
}
#endif
