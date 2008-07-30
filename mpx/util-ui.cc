//  MPX
//  Copyright (C) 2005-2007 MPX Project 
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
#include "config.h"
#include <string>
#include <boost/format.hpp>
#include <glibmm/ustring.h>
#include <glibmm/i18n.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/widget.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm.h>
#include "mpx/util-ui.hh"
#include "mpx/mpx-minisoup.hh"
#include "mpx/mpx-uri.hh"
using namespace Gtk;

namespace MPX
{
    bool
    Util::ui_manager_add_ui(Glib::RefPtr<Gtk::UIManager>& uimanager,
                            char const* ui,
                            Gtk::Window& window,
                            const Glib::ustring& name)
    {
        try{        
            uimanager->add_ui_from_string(ui);
            return true;
        }
        catch(Glib::MarkupError & cxe)
        {
            MessageDialog dialog (window, ((boost::format (_("An Error occured parsing the Menu markup for '%s'")) % name.c_str()).str()));
            dialog.run ();
            return false;
        }
    }

    Gtk::Widget *
    Util::get_popup (Glib::RefPtr<Gtk::UIManager> ui_manager, Glib::ustring const& menupath)
    {
      Gtk::Widget * menuitem = 0;
      menuitem = ui_manager->get_widget (menupath);
      return (menuitem ? dynamic_cast <Gtk::MenuItem *> (menuitem)->get_submenu() : 0);
    }

    Glib::RefPtr<Gdk::Pixbuf>
    Util::get_image_from_uri (ustring const& uri)
    {
      Glib::RefPtr<Gdk::Pixbuf> image = Glib::RefPtr<Gdk::Pixbuf>(0);

      try{
        URI u (uri);
        if (u.get_protocol() == URI::PROTOCOL_HTTP)
        {
          Soup::RequestSyncRefP request = Soup::RequestSync::create (ustring (u));
          guint code = request->run (); 

          if (code == 200)
          try{
                Glib::RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
                loader->write (reinterpret_cast<const guint8*>(request->get_data_raw()), request->get_data_size());
                loader->close ();
                image = loader->get_pixbuf();
            }
          catch (Gdk::PixbufError & cxe)
            {
                g_message("%s: Gdk::PixbufError: %s", G_STRLOC, cxe.what().c_str());
            }
        }
        else if (u.get_protocol() == URI::PROTOCOL_FILE)
        {
          image = Gdk::Pixbuf::create_from_file (filename_from_uri (uri));
        }
      }
      catch (MPX::URI::ParseError & cxe)
      { 
          g_message("%s: URI Parse Error: %s", G_STRLOC, cxe.what());
      }
      return image;
    }

} // MPX
