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
#include <gtkmm.h>
#include <sigx/sigx.h>
#include "widgetloader.h"
#include "mpx/paccess.hh"
#include "mpx/i-playbacksource.hh"

using namespace Gnome::Glade;

namespace Gtk
{
    class Statusbar;
}

namespace MPX
{
	class Library;
	class Covers;
    class Play;
    class HAL;

    class Player
      : public WidgetLoader<Gtk::Window>
      , public sigx::glib_auto_dispatchable
    {
      public:

		void
		add_widget (Gtk::Widget*);

        void
        add_info_widget(Gtk::Widget*, std::string const&);

        void
        add_subsource(PlaybackSource*, ItemKey const&, gint64 id);

		void
		remove_widget (Gtk::Widget*);

		void
		remove_info_widget (Gtk::Widget*);

		void
		get_object (PAccess<MPX::Library>&);

		void	
		get_object (PAccess<MPX::Covers>&);

        void
        get_object (PAccess<MPX::Play>&);

#ifdef HAVE_HAL
        void
        get_object (PAccess<MPX::HAL>&);
#endif // HAVE_HAL

        virtual ~Player ();

      protected:

        Player (const Glib::RefPtr<Gnome::Glade::Xml>&, MPX::Library&, MPX::Covers&);
    };
}
#endif
