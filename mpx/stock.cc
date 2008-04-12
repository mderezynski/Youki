//  MPX
//  Copyright (C) 2005-2007 MPX development.
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
#include <gtkmm.h>
#include "stock.hh"
#include <string>

namespace MPX
{
  using namespace Gdk;
  using namespace Glib;
  using namespace Gtk;

  void
  register_stock_icons ()
  {
    RefPtr<Gtk::IconFactory> factory = IconFactory::create ();

    struct StockIconSpec
    {
      char const* filename;
      char const* stock_id;
    };

    StockIconSpec theme_icon_list[] =
    {   
      { "lastfm.png",                   MPX_STOCK_LASTFM            },
      { "plugin.png",                   MPX_STOCK_PLUGIN            },
      { "plugin-disabled.png",          MPX_STOCK_PLUGIN_DISABLED   },
    };

    for (unsigned int n = 0; n < G_N_ELEMENTS (theme_icon_list); ++n)
    {
      std::string filename = build_filename(build_filename (DATA_DIR, G_DIR_SEPARATOR_S "icons"
																 G_DIR_SEPARATOR_S "hicolor"
																 G_DIR_SEPARATOR_S "24x24"
																 G_DIR_SEPARATOR_S "stock"), theme_icon_list[n].filename);
      RefPtr<Pixbuf> pixbuf = Pixbuf::create_from_file (filename);
      factory->add (StockID (theme_icon_list[n].stock_id), IconSet(pixbuf));
    }

    factory->add_default ();
  }
}
