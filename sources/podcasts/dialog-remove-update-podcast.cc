//  BMP
//  Copyright (C) 2005-2008 BMP development.
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
#include "dialog-remove-update-podcast.hh"
using namespace Gtk;

namespace Bmp
{

  DialogRemoveUpdatePodcast::~DialogRemoveUpdatePodcast ()
  {
  }

  DialogRemoveUpdatePodcast*
  DialogRemoveUpdatePodcast::create ()
  {
    const std::string path = BMP_GLADE_DIR G_DIR_SEPARATOR_S "dialog-remove-update-podcast.glade";
    Glib::RefPtr<Gnome::Glade::Xml> glade_xml = Gnome::Glade::Xml::create (path);
    DialogRemoveUpdatePodcast * p = 0;
    glade_xml->get_widget_derived ("dialog", p);
    return p;
  }

  DialogRemoveUpdatePodcast::DialogRemoveUpdatePodcast (BaseObjectType                 * cobj,
                                                        RefPtr<Gnome::Glade::Xml> const& xml)
  : Dialog    (cobj)
  , m_ref_xml (xml)
  {
    Util::window_set_icon_list (*this, "player");
  }

  int
  DialogRemoveUpdatePodcast::run (Podcast & cast, Episode & item)
  {
    Label * label;
    m_ref_xml->get_widget( "label-info", label );

    static boost::format title_f ("%s / %s - BMP");
    set_title ((title_f % Markup::escape_text(item.item["title"].c_str())
                        % Markup::escape_text(cast.podcast["title"].c_str())).str());

    static boost::format message_f ("<big>%s</big>");
    label->set_markup ((message_f % Markup::escape_text (_("Do you wish to Re-Download this Episode, Remove it, or Cancel the request?"))).str());
    return Dialog::run();
  }

} // namespace Bmp
