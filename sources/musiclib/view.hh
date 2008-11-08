#ifndef MPX_ALBUM_VIEW_HH
#define MPX_ALBUM_VIEW_HH

#include "config.h"
#include <gtkmm.h>
#include <gtk/gtktreeview.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/format.hpp>
#include "glib-marshalers.h"
#include "model.hh"
#include <iostream>

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace MPX
{
        typedef sigc::signal<void, gint64, bool> SignalTrackActivated;

        class View : public Gtk::DrawingArea
        {
                Glib::RefPtr<Gdk::Pixbuf>           m_image;
                ViewModel                         * m_model;
                int                                 m_row_height;
                int                                 m_visible_height;
                int                                 m_previousdrawnrow;
                PropAdj                             m_prop_vadj;
                PropAdj                             m_prop_hadj;
                guint                               m_signal0; 
                GtkWidget                         * m_treeview;
                int                                 m_click_row_1;
                int                                 m_sel_size_was;
                bool                                m_highlight;

                SignalTrackActivated                m_trackactivated;

                void
                initialize_metrics ()
                {
                    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (gobj()));

                    PangoFontMetrics *metrics = pango_context_get_metrics (context,
                                                                            GTK_WIDGET (gobj())->style->font_desc, 
                                                                            pango_context_get_language (context));

                    m_row_height = (pango_font_metrics_get_ascent (metrics)/PANGO_SCALE) + 
                                   (pango_font_metrics_get_descent (metrics)/PANGO_SCALE) + 8;
                }

                void
                on_vadj_value_changed ()
                {
                    m_model->scroll_to(m_prop_vadj.get_value()->get_value()); 
                    queue_draw ();
                }

            protected:

                virtual void 
                on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context)
                {
                }

                virtual void
                on_drag_end (const Glib::RefPtr<Gdk::DragContext>& context)
                {
                } 

                virtual void
                on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time)
                {
                }

                virtual bool
                on_focus (Gtk::DirectionType direction)
                { 
                    return false;
                }

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    return false;
                }

                bool
                on_button_press_event (GdkEventButton * event)
                {
                    return false;
                }

                bool
                on_button_release_event (GdkEventButton * event)
                {
                    return false;
                }


                bool
                on_configure_event (GdkEventConfigure * event)        
                {
                    double upper = double(m_model->find_totalheight() - get_allocation().get_height());
                    if( m_prop_vadj.get_value() )
                    {
                        g_object_set(G_OBJECT(m_prop_vadj.get_value()->gobj()), "page-size", 1., "step-increment", 10., "upper", upper, NULL); 
                    }

                    return false;
                }

                bool
                on_expose_event (GdkEventExpose *event)
                {
                    Gdk::Rectangle const& alloc = get_allocation();

                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context();

                    /* TEST RECTANGLE
                    cairo->set_operator(Cairo::OPERATOR_SOURCE);
                    cairo->set_source_rgba(1., 0., 0., 1.);
                    cairo->rectangle( alloc.get_x(), alloc.get_y(), alloc.get_width(), alloc.get_height() );
                    cairo->fill();
                    */

                    cairo->set_operator(Cairo::OPERATOR_CLEAR);
                    cairo->paint();

                    cairo->set_operator(Cairo::OPERATOR_SOURCE);
                    cairo->set_source_rgba(1., 1., 1., 1.);
                    cairo->paint();

                    if( !m_model )
                    {
                        g_message("No model");
                        return true;
                    }
   
                    PositionData_t pd = m_model->find_row_at_current_position();
                    Rows_t const& rows = m_model->get_rows();

                    if( pd.Parent_Index >= rows.size() )
                    {
                        g_message("Parent index too large");
                        return true;
                    }

                    int x = 0, y = 0, w = alloc.get_width(), h = alloc.get_height();
                    y -= pd.Offset;

                    cairo->set_source_rgba(0., 0., 0., 1.);
                    cairo->set_line_width(0.5);

                    do{
                            Row_p row = rows[pd.Parent_Index];

                            if( !pd.Child_Index )
                            {
                                    cairo->set_operator(Cairo::OPERATOR_ATOP);

                                    if( rows[pd.Parent_Index]->Cover )
                                    {
                                            cairo->set_source( row->Cover, x, y );
                                            cairo->move_to( x, y );
                                            cairo->rectangle( x , y, 90, 90 );
                                            cairo->fill_preserve();
                                            cairo->stroke();
                                    }
                                    else
                                    {
                                            Gdk::Cairo::set_source_pixbuf( cairo, m_image, x, y );
                                            cairo->move_to( x, y );
                                            cairo->rectangle( x , y, 90, 90 );
                                            cairo->fill_preserve();
                                            cairo->stroke();
                                    }

                                    /*
                                    {
                                            Glib::RefPtr<Pango::Layout> layout = create_pango_layout(boost::get<std::string>(row->AlbumData["album"]));
                                            cairo->move_to( x + 98, y );
                                            pango_cairo_show_layout(cairo->cobj(), layout->gobj());
                                    }       

                                    {
                                            Glib::RefPtr<Pango::Layout> layout = create_pango_layout(boost::get<std::string>(row->AlbumData["album_artist"]));
                                            cairo->move_to( x + 98, y + 30 );
                                            pango_cairo_show_layout(cairo->cobj(), layout->gobj());
                                    }
                                    */

                                    y += m_model->m_rowheight_parent;
                                    pd.Child_Index = 0;
                            }

                            cairo->set_operator(Cairo::OPERATOR_SOURCE);

                            for( int i = pd.Child_Index.get(); (i < row->ChildData.size()) && (y < h); ++ i )
                            {
                                    Glib::RefPtr<Pango::Layout> layout = create_pango_layout(boost::get<std::string>(row->ChildData[i][ATTRIBUTE_TITLE].get()));
                                    cairo->move_to( x + 24, y + 2 );
                                    pango_cairo_show_layout(cairo->cobj(), layout->gobj());

                                    y += m_model->m_rowheight_child;
                            }

                            pd.Parent_Index++;    
                            pd.Child_Index.reset();

                    }while( y < h && pd.Parent_Index < rows.size() );

                    return true;
                }

                void
                on_model_changed()
                {
                }

                static gboolean
                list_view_set_adjustments(
                    GtkWidget*obj,
                    GtkAdjustment*hadj,
                    GtkAdjustment*vadj, 
                    gpointer data
                )
                {
                    View & view = *(reinterpret_cast<View*>(data));

                    g_object_set(G_OBJECT(obj), "vadjustment", vadj, NULL); 
                    g_object_set(G_OBJECT(obj), "hadjustment", hadj, NULL);

                    if( view.m_model )
                    {
                            double upper = double(view.m_model->find_totalheight() - view.get_allocation().get_height());
                            g_object_set(G_OBJECT(view.m_prop_vadj.get_value()->gobj()), "page-size", 1., "step-increment", 10., "upper", upper, NULL); 
                    }

                    view.m_prop_vadj.get_value()->signal_value_changed().connect(
                        sigc::mem_fun(
                            view,
                            &View::on_vadj_value_changed
                    ));

                    return TRUE;
                }

            public:
    
                void
                set_highlight(bool highlight)
                {
                    m_highlight = highlight;
                    queue_draw ();
                }

                void
                set_model(ViewModel * model)
                {
                    m_model = model;

                    if( m_prop_vadj.get_value() )
                    {
                            double upper = double(m_model->find_totalheight() - get_allocation().get_height());
                            g_object_set(G_OBJECT(m_prop_vadj.get_value()->gobj()), "page-size", 1., "step-increment", 10., "upper", upper, NULL); 
                    }

                    queue_draw ();
                }

                SignalTrackActivated&
                signal_track_activated()
                {
                    return m_trackactivated;
                }

                View ()
                : ObjectBase            ("MPXView")
                , m_model               (0)
                , m_previousdrawnrow    (0)
                , m_prop_vadj           (*this, "vadjustment", (Gtk::Adjustment*)(0))
                , m_prop_hadj           (*this, "hadjustment", (Gtk::Adjustment*)(0))
                , m_click_row_1         (0)
                , m_sel_size_was        (0)
                , m_highlight           (false)
                {
                    m_image = Gdk::Pixbuf::create_from_file(Glib::build_filename(DATA_DIR, "images" G_DIR_SEPARATOR_S "disc.png"))->scale_simple(90,90, Gdk::INTERP_HYPER);
                    m_treeview = gtk_tree_view_new();
                    gtk_widget_realize(GTK_WIDGET(m_treeview));

                    set_flags(Gtk::CAN_FOCUS);
                    add_events(Gdk::EventMask(GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK | GDK_EXPOSURE_MASK));

                    ((GtkWidgetClass*)(G_OBJECT_GET_CLASS(G_OBJECT(gobj()))))->set_scroll_adjustments_signal = 
                            g_signal_new ("set_scroll_adjustments",
                                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                      GSignalFlags (G_SIGNAL_RUN_FIRST),
                                      0,
                                      NULL, NULL,
                                      psm_glib_marshal_VOID__OBJECT_OBJECT, G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

                    g_signal_connect(G_OBJECT(gobj()), "set_scroll_adjustments", G_CALLBACK(list_view_set_adjustments), this);

                    gtk_widget_realize(GTK_WIDGET(gobj()));
                    initialize_metrics();
                }

                ~View ()
                {
                }
        };
}

#endif
