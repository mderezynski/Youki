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

#ifndef MPX_UI_SPLASH_HH
#define MPX_UI_SPLASH_HH

#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/window.h>
#include <pangomm/layout.h>

namespace MPX
{
  class Splashscreen : public Gtk::Window
  {
    public:

        Splashscreen ();
        void set_message (char const * message, double percent);

    protected:

        virtual void
        on_realize ();

        virtual bool
        on_expose_event (GdkEventExpose *event);

    private:

        Glib::RefPtr<Gdk::Pixbuf>     m_image;
        Glib::RefPtr<Pango::Layout>   m_layout;

        unsigned int  m_image_w;
        unsigned int  m_image_h;
        unsigned int  m_bar_w;
        unsigned int  m_bar_h;
        unsigned int  m_bar_x;
        unsigned int  m_bar_y;
        bool          m_has_alpha;
        double        m_percent;
        Glib::ustring m_message;

  };

} // MPX

#endif // !MPX_UI_SPLASH_HH
