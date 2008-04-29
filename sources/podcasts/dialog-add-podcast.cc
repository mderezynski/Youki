//  BMP
//  Copyright (C) 2005-2007 BMP development.
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
//  The BMPx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //!HAVE_CONFIG_H

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <libglademm.h>
#include <cstring>
#include <string>

#include "paths.hh"
#include "stock.hh"
#include "ui-tools.hh"

#include "dialog-add-podcast.hh"

namespace Bmp
{

  DialogAddPodcast::~DialogAddPodcast ()
  {
  }

  DialogAddPodcast*
  DialogAddPodcast::create ()
  {
    const std::string path = BMP_GLADE_DIR G_DIR_SEPARATOR_S "dialog-add-podcast.glade";
    Glib::RefPtr<Gnome::Glade::Xml> glade_xml = Gnome::Glade::Xml::create (path);
    DialogAddPodcast * p = 0;
    glade_xml->get_widget_derived ("dialog", p);
    return p;
  }

  DialogAddPodcast::DialogAddPodcast (BaseObjectType                       * obj,
                                      Glib::RefPtr<Gnome::Glade::Xml> const& xml)
  : Gtk::Dialog     (obj),
    m_ref_xml       (xml)
  {
    Util::window_set_icon_list (*this, "player");
    dynamic_cast<Gtk::Image *>(m_ref_xml->get_widget("image-add"))->set (Gtk::StockID (BMP_STOCK_FEED_ADD), Gtk::ICON_SIZE_SMALL_TOOLBAR);
  }

  int DialogAddPodcast::run (Glib::ustring & uri)
  {
    dynamic_cast<Gtk::Entry *>(m_ref_xml->get_widget("entry"))->set_text(Gtk::Clipboard::get()->wait_for_text());
    int response = Gtk::Dialog::run ();
    uri = dynamic_cast<Gtk::Entry *>(m_ref_xml->get_widget("entry"))->get_text();
    return response;
  }

} // namespace Bmp
