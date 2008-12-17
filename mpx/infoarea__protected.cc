#include "config.h"
#include "infoarea.hh"
#include <boost/algorithm/string.hpp>

namespace MPX 
{
        bool
                InfoArea::on_button_press_event (GdkEventButton * event)
                {
                        int x = int (event->x);
                        int y = int (event->y);

                        if ((event->window == get_window()->gobj()) && (x >= 6) && (x <= (m_cover_pos+72)) && (y >= 3) && (y <= 75))
                        {
                                //m_pressed = true;
                                queue_draw ();
                        }

                        return false;
                }

        bool
                InfoArea::on_button_release_event (GdkEventButton * event)
                {
                        int x = int (event->x);
                        int y = int (event->y);

                        //m_pressed = false;
                        queue_draw ();

                        if ((event->window == get_window()->gobj()) && (x >= 6) && (x <= (m_cover_pos+72)) && (y >= 3) && (y <= 75))
                        {
                                m_SignalCoverClicked.emit ();
                        }

                        return false;
                }

        bool
                InfoArea::on_drag_drop (Glib::RefPtr<Gdk::DragContext> const& context,
                                int                                   x,
                                int                                   y,
                                guint                                 time)
                {
                        Glib::ustring target (drag_dest_find_target (context));
                        if( !target.empty() )
                        {
                                drag_get_data (context, target, time);
                                context->drag_finish  (true, false, time);
                                return true;
                        }
                        else
                        {
                                context->drag_finish  (false, false, time);
                                return false;
                        }
                }

        void
                InfoArea::on_drag_data_received (Glib::RefPtr<Gdk::DragContext> const& context,
                                int                                   x,
                                int                                   y,
                                Gtk::SelectionData const&             data,
                                guint                                 info,
                                guint                                 time)
                {
                        if( data.get_data_type() == "text/uri-list")
                        {
                                Util::FileList u = data.get_uris();
                                m_SignalUrisDropped.emit (u);
                        }
                        else
                                if( data.get_data_type() == "text/plain")
                                {
                                        using boost::algorithm::split;
                                        using boost::algorithm::is_any_of;
                                        using boost::algorithm::replace_all;

                                        std::string text = data.get_data_as_string ();
                                        replace_all (text, "\r", "");

                                        StrV v;
                                        split (v, text, is_any_of ("\n"));

                                        if( v.empty ()) // we're taking chances here
                                        {
                                                v.push_back (text);
                                        }

                                        Util::FileList u;

                                        for(StrV::const_iterator i = v.begin(); i != v.end(); ++i)
                                        {
                                                try{
                                                        URI uri (*i);
                                                        u.push_back (*i);
                                                }
                                                catch (URI::ParseError & cxe)
                                                {
                                                        // seems like not it
                                                }
                                        }

                                        if( !u.empty() )
                                        {
                                                m_SignalUrisDropped.emit (u);
                                        }
                                }
                }

        bool
                InfoArea::on_expose_event (GdkEventExpose * event)
                {
                        Cairo::RefPtr<Cairo::Context> cr = get_window ()->create_cairo_context ();

                        draw_background (cr);
                        draw_cover (cr);
                        draw_spectrum (cr);
                        draw_text (cr);
                        draw_info (cr);

                        const Gtk::Allocation& allocation = get_allocation();

                        cr->set_line_width(0.75);
                        Util::cairo_rounded_rect(cr, 0.5, 0.5, allocation.get_width()-1, allocation.get_height()-1, 7.);
                        cr->set_source_rgb(0.6, 0.6, 0.6);
                        cr->stroke();

                        Util::cairo_rounded_rect(cr, 0, 0, allocation.get_width(), allocation.get_height(), 7.);
                        cr->set_source_rgb(0.05, 0.05, 0.05);
                        cr->stroke();

                        return true;
                }

}
