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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifndef KOBO_TITLEINFO_HH
#define KOBO_TITLEINFO_HH

#include <glibmm/timer.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/drawingarea.h>
#include <sigc++/connection.h>

namespace MPX
{
  class KoboTitleInfo : public Gtk::DrawingArea
  {
    public:

        KoboTitleInfo ();

        void
        set_info(
            const std::vector<std::string>&
        ) ;

        void
        clear () ;
  
    protected:

        virtual bool
        on_expose_event (GdkEventExpose *event);

    private:

        double total_animation_time ;
        double start_time ;
        double end_time ; 

        Glib::Timer                 m_timer;
        sigc::connection            m_update_connection;
        std::vector<std::string>    m_info ;

        void
        draw_frame ();

        bool
        update_frame ();

        double
        cos_smooth (double x) ;

        std::string 
        get_text_at_time (double time) ;

        double
        get_text_alpha_at_time (double time) ;
  };

} // MPX

#endif // !MPX_UI_DIALOG_ABOUT_HH
