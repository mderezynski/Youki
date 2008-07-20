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
#include <string>

#include "mpx/stock.hh"

namespace MPX
{
  using namespace Gdk;
  using namespace Glib;
  using namespace Gtk;

  void
  register_stock_icons (StockIconSpecV const& icons, std::string const& base_path)
  {
    RefPtr<Gtk::IconFactory> factory = IconFactory::create ();
    for (StockIconSpecV::const_iterator i = icons.begin(); i != icons.end(); ++i)
    {
      StockIconSpec const& icon = *i;
      std::string filename = build_filename( base_path, icon.filename );
      RefPtr<Pixbuf> pixbuf = Pixbuf::create_from_file( filename );
      factory->add( StockID( icon.stock_id.c_str() ), IconSet( pixbuf ) );
    }
    factory->add_default ();
  }

  std::string
  default_stock_path (std::string const& size)
  {
   return build_filename (DATA_DIR, build_filename("icons",
						                build_filename("hicolor",
                                            build_filename(size.c_str(),
									            "stock"
          ))));
  }

  void
  register_default_stock_icons ()
  {
    StockIconSpecV v; 

    v.push_back(StockIconSpec( "audio-volume-muted.png",       "audio-volume-muted"        ));
    v.push_back(StockIconSpec( "audio-volume-low.png",         "audio-volume-low"          ));
    v.push_back(StockIconSpec( "audio-volume-medium.png",      "audio-volume-medium"       ));
    v.push_back(StockIconSpec( "audio-volume-high.png",        "audio-volume-high"         ));
    v.push_back(StockIconSpec( "lastfm.png",                   MPX_STOCK_LASTFM            ));
    v.push_back(StockIconSpec( "plugin.png",                   MPX_STOCK_PLUGIN            ));
    v.push_back(StockIconSpec( "plugin-disabled.png",          MPX_STOCK_PLUGIN_DISABLED   ));

    register_stock_icons (v, default_stock_path("24x24")); 

    v.clear();
    v.push_back(StockIconSpec( "error.png",                    MPX_STOCK_ERROR             ));

    register_stock_icons (v, default_stock_path("16x16")); 
  }
}
