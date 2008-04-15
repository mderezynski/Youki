#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include "mpx/tagview.hh"
#include <tr1/cmath>

namespace MPX
{
        void
        TagView::update_global_extents ()
        {
            m_Layout.RowHeight = 0;
            for(LayoutList::const_iterator i = m_Layout.List.begin(); i != m_Layout.List.end(); ++i)
            {
                if((*i)->m_Logical.get_height() > m_Layout.RowHeight)    
                {
                    m_Layout.RowHeight = (*i)->m_Logical.get_height();
                }
            }
        }

        inline void
        TagView::push_back_row (LayoutList const& row, int x)
        {
            m_Layout.Rows.push_back(row);
            m_Layout.Widths.push_back(x - (TAG_SPACING / m_Layout.Scale));
        }

        void
        TagView::layout ()
        {
            m_Layout.Scale = 1.;

            retry_pack:

            m_Layout.Rows.clear();
            m_Layout.Widths.clear();

            if(m_Layout.List.empty())
                return;

            if(get_allocation().get_height() < m_Layout.RowHeight)
                return;
    
            LayoutList row;
            double x = 0;
            int mw = 0;            
            for(LayoutList::const_iterator i = m_Layout.List.begin(); i != m_Layout.List.end(); ++i)
            {
                LayoutSP sp = *i;

                if((x+sp->m_Logical.get_width()) > (get_allocation().get_width() / m_Layout.Scale)) 
                {
                    if((x - (TAG_SPACING / m_Layout.Scale)) > mw)
                    {
                        mw = x - (TAG_SPACING / m_Layout.Scale);
                    }
                    push_back_row (row, x);
                    row = LayoutList();
                    x = 0;
                }
   
                x += sp->m_Logical.get_width() + (TAG_SPACING / m_Layout.Scale);
                row.push_back(sp);
            }

            push_back_row (row, x);

            if((m_Layout.Rows.size() * m_Layout.RowHeight) > (get_allocation().get_height() / m_Layout.Scale)) 
            {
                if(m_Layout.Scale >= ACCEPTABLE_MIN_SCALE)
                {
                    m_Layout.Scale -= SCALE_STEP;
                    goto retry_pack;
                }
            }

#if 0
            else
            if((m_Layout.Rows.size() * m_Layout.RowHeight) <= (get_allocation().get_height() / m_Layout.Scale)) 
            {
                double diff = (get_allocation().get_height() / m_Layout.Scale) - (m_Layout.Rows.size() * m_Layout.RowHeight);
                if(diff > m_Layout.RowHeight) // arbitrary?
                {
                    m_Layout.Scale += SCALE_STEP*2.;
                    goto retry_pack;
                }
            }
#endif
        
            if(mw > (get_allocation().get_width() / m_Layout.Scale)) 
            {
                if(m_Layout.Scale >= ACCEPTABLE_MIN_SCALE)
                {
                    m_Layout.Scale -= SCALE_STEP;
                    goto retry_pack;
                }
            }

            double heightcorrection = (((get_allocation().get_height() - (m_Layout.Rows.size() * (m_Layout.RowHeight * m_Layout.Scale)))) / m_Layout.Scale) / 2.;
            double height_for_row = (heightcorrection * 2.) / (m_Layout.Rows.size() - 1);
            double ry = 0;

            int rowcounter = 0;

            WidthsT::const_iterator wi = m_Layout.Widths.begin(); 

            for(RowListT::iterator i = m_Layout.Rows.begin(); i != m_Layout.Rows.end(); ++i)
            {
                LayoutList & l = *i;

                double rx = (get_allocation().get_width() / 2.) - (((*wi) * m_Layout.Scale) / 2.); // center row 
                rx = std::tr1::fmax(0, rx);
                rx /= m_Layout.Scale;
                for(LayoutList::const_iterator r = l.begin(); r != l.end(); ++r) 
                {
                    LayoutSP sp = *r;
                    sp->x = rx;

                    if(m_Layout.Scale < 1.0)
                        sp->y = ry + height_for_row*rowcounter + ((m_Layout.RowHeight - sp->m_Logical.get_height())/2.); 
                    else
                        sp->y = ry + heightcorrection + ((m_Layout.RowHeight - sp->m_Logical.get_height())/2.); 

                    rx += sp->m_Logical.get_width() + (TAG_SPACING / m_Layout.Scale);
                }

                ry += m_Layout.RowHeight;
                wi++;
                rowcounter++;
            }
        }

        bool
        TagView::on_leave_notify_event (GdkEventCrossing * event)
        {
            motion_x = -1;
            motion_y = -1;
            queue_draw ();
        }

        bool
        TagView::on_motion_notify_event (GdkEventMotion * event)
        {
            GdkModifierType state;
            int x, y;

            if (event->is_hint)
            {
              gdk_window_get_pointer (event->window, &x, &y, &state);
            }
            else
            {
              x = int (event->x);
              y = int (event->y);
              state = GdkModifierType (event->state);
            }

            motion_x = x;
            motion_y = y;

            m_ActiveTagName = std::string();
            int rowcounter = 0;
            for(RowListT::const_iterator i = m_Layout.Rows.begin(); i != m_Layout.Rows.end(); ++i)
            {
                LayoutList const& l = *i;
                for(LayoutList::const_iterator r = l.begin(); r != l.end(); ++r) 
                {
                    LayoutSP sp = *r;

                    if(((motion_x / m_Layout.Scale) >= sp->x) && ((motion_y / m_Layout.Scale) >= sp->y) && ((motion_x / m_Layout.Scale) < (sp->x + sp->m_Logical.get_width())) && ((motion_y / m_Layout.Scale) < (sp->y + sp->m_Logical.get_height())))
                    {
                        sp->active = true;
                        m_ActiveTagName = sp->m_Text;
                        m_ActiveRow = rowcounter;
                    }
                    else
                    {
                        sp->active = false;
                    }
                }
                rowcounter++;
            }

            queue_draw ();

            return false;
        }

        bool
        TagView::on_configure_event (GdkEventConfigure * event)
        {
            update_global_extents ();
            layout ();
            queue_draw ();
    
            return false;
        }

        bool
        TagView::on_expose_event (GdkEventExpose * event)
        {
            using namespace Gtk;

            Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context();

            int x, y, w, h;
            Gtk::Allocation a = get_allocation();
            x = a.get_x();
            y = a.get_y();
            w = a.get_width();
            h = a.get_height();

            cr->set_operator(Cairo::OPERATOR_SOURCE);
            cr->set_source_rgb(0., 0., 0.);
            cr->rectangle(x, y, w, h);
            cr->fill();

            cr->scale( m_Layout.Scale, m_Layout.Scale );

            int rowcounter = 0;
            int tagcounter = 0;
            for(RowListT::const_iterator i = m_Layout.Rows.begin(); i != m_Layout.Rows.end(); ++i)
            {
                LayoutList const& l = *i;
                for(LayoutList::const_iterator r = l.begin(); r != l.end(); ++r) 
                {
                    LayoutSP sp = *r;

                    if(sp->active)
                        cr->set_source_rgb(1., 0., 0.);
                    else
                    {
                        if((tagcounter % 2) == 0)
                            cr->set_source_rgb(0.22, 0.66, 0.36);
                        else
                            cr->set_source_rgb(0.10, 0.87, 0.30);
                    }

                    cr->move_to( sp->x , sp->y ); 
                    pango_cairo_show_layout(cr->cobj(), sp->m_Layout->gobj());
                    tagcounter++;
                }
                rowcounter++;
            }

            return true;
        }

        std::string const&
        TagView::get_active_tag () const
        {
            return m_ActiveTagName;
        }

        void
        TagView::clear ()
        {
            m_Layout.reset();
            queue_draw ();
        }

        void
        TagView::add_tag (std::string const& text, double amplitude)
        {
            LayoutSP sp (new Layout);                        
            sp->m_Text = text;
            sp->amplitude = amplitude;
            sp->m_Layout = create_pango_layout(text);

            Pango::AttrList list;
            Pango::Attribute attr1 = Pango::Attribute::create_attr_scale(amplitude);
            list.insert(attr1);
            sp->m_Layout->set_attributes(list);

            sp->m_Layout->get_pixel_extents(sp->m_Ink, sp->m_Logical);
            m_Layout.List.push_back(sp);

            update_global_extents ();
            layout ();
        }

        TagView::TagView (const Glib::RefPtr<Gnome::Glade::Xml>& xml, std::string const& widget_name)
        : WidgetLoader<Gtk::DrawingArea>(xml, widget_name)
        , motion_x(0)
        , motion_y(0)
        , m_ActiveRow(0)
        {
            gtk_widget_add_events(GTK_WIDGET(gobj()), Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::LEAVE_NOTIFY_MASK);
        }

        TagView::~TagView ()
        {
        }

        double TagView::TAG_SPACING = 12;
        double TagView::ACCEPTABLE_MIN_SCALE = 0.0;
        double TagView::SCALE_STEP = 0.02;
}
