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

#ifndef BMP_PODCAST_ADD_HH
#define BMP_PODCAST_ADD_HH

#include <gtkmm/dialog.h>
#include <libglademm/xml.h>

namespace Bmp
{
  class DialogAddPodcast : public Gtk::Dialog
  {
      public:
          DialogAddPodcast (BaseObjectType                       * obj,
                            Glib::RefPtr<Gnome::Glade::Xml> const& xml);
          static DialogAddPodcast * create ();
          virtual ~DialogAddPodcast ();

          int run (Glib::ustring & uri);

      private:
        Glib::RefPtr<Gnome::Glade::Xml>	m_ref_xml;
  };
} // namespace Bmp

#endif // !BMP_PODCAST_ADD_HH

