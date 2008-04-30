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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <libglademm.h>
#include <map>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include "mpx/mpx-public.hh"
#include "mpx/library.hh"
#include "mpx/types.hh"
#include "mpx/minisoup.hh"
#include "source-podcasts.hh"

using namespace Glib;
using namespace Gtk;

namespace
{
    const char ACTION_ADD_PODCAST [] = "action-add-podcast";
    
    char const * ui_source =
    "<ui>"
    ""
    "<menubar name='MenubarMain'>"
    "   <placeholder name='placeholder-source'>"
    "     <menu action='menu-source-podcasts'>"
    "         <menuitem action='action-add-podcast'/>"
    "     </menu>"
    "   </placeholder>"
    "</menubar>"
    ""
    "</ui>"
    "";
}

namespace MPX
{
namespace Source
{
    Glib::RefPtr<Gdk::Pixbuf>
    Podcasts::get_icon ()
    {
        try{
            return IconTheme::get_default()->load_icon("source-podcasts", 64, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    Podcasts::get_ui ()
    {
        return m_UI;
    }

    Podcasts::Podcasts (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
    : PlaybackSource(ui_manager, _("Podcasts"))
    {
		const std::string path (build_filename(DATA_DIR, build_filename("glade","source-podcasts.glade")));
		m_ref_xml = Gnome::Glade::Xml::create (path);
		m_UI = m_ref_xml->get_widget("source-podcasts");

        StockIconSpecV v;
        v.push_back(StockIconSpec("stock-feed-add.png", "mpx-stock-feed-add"));
        register_stock_icons(v, default_stock_path());
    }

    std::string
    Podcasts::get_guid ()
    {
        return "bb8b9a8e-e534-4192-bfce-5ff7f9262643";
    }



    std::string
    Podcasts::get_uri ()
    {
    }

    bool
    Podcasts::go_next ()
    {
		return false;
    }

    bool
    Podcasts::go_prev ()
    {
		return false;
    }

    void
    Podcasts::stop ()
    {
    }

    bool
    Podcasts::play ()
    {
    }

	void
	Podcasts::send_metadata ()
	{
	}

    void
    Podcasts::play_post ()
    {
    }

    void
    Podcasts::restore_context ()
    {
    }

  } // Source
} // MPX
