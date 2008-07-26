#ifndef MPX_LISTVIEW_HH
#define MPX_LISTVIEW_HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include "mpx/glibaddons.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/cairoextensions.hh"
#include "glib-marshalers.h"

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace MPX
{
        typedef boost::tuple<std::string, std::string, std::string, gint64> Row4;
        typedef std::vector<std::string>                                    Row1;
        typedef std::vector<Row4>                                           ModelT;
        typedef boost::shared_ptr<ModelT>                                   ModelP;

        typedef sigc::signal<void> Signal0;

        struct DataModel
        {
                ModelP          m_realmodel;
                Signal0         m_changed;

                DataModel(ModelP model)
                {
                    m_realmodel = model;
                }

                virtual Signal0&
                signal_changed ()
                {
                    return m_changed;
                }

                virtual bool
                is_set ()
                {
                    return bool(m_realmodel);
                }

                virtual int
                size ()
                {
                    return m_realmodel->size();
                }

                virtual Row4 const&
                row (int row)
                {
                    return (*m_realmodel)[row];
                }
        };

        typedef boost::shared_ptr<DataModel> DataModelP;

        struct DataModelFilter : public DataModel
        {
                typedef std::vector<ModelT::iterator> RowRowMapping;

                RowRowMapping   m_mapping;
                std::string     m_filter;

                DataModelFilter(DataModelP & model)
                : DataModel(model->m_realmodel)
                {
                    regen_mapping ();
                }

                virtual int 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row4 const&
                row (int row)
                {
                    return *(m_mapping[row]);
                }

                virtual void
                set_filter(std::string const& filter)
                {
                    if(!m_filter.empty() && filter.substr(0, filter.size()-1) == m_filter)
                    {
                        m_filter = Glib::ustring(filter).lowercase();
                        regen_mapping_iterative();
                    }
                    else
                    {
                        m_filter = Glib::ustring(filter).lowercase();
                        regen_mapping();
                    }

                    m_changed.emit();
                }

                void
                regen_mapping ()
                {
                    using boost::get;

                    m_mapping.clear();
                    
                    for(ModelT::iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i)
                    {
                        Row4 const& row = *i;
                    
                        if( m_filter.empty() || 
                            Util::match_keys( Glib::ustring(get<0>(row)).lowercase(), m_filter ) ||
                            Util::match_keys( Glib::ustring(get<1>(row)).lowercase(), m_filter ) ||
                            Util::match_keys( Glib::ustring(get<2>(row)).lowercase(), m_filter ))
                        {
                            m_mapping.push_back(i);
                        }
                    }
                }

                void
                regen_mapping_iterative ()
                {
                    using boost::get;

                    RowRowMapping new_mapping;
                    
                    for(RowRowMapping::const_iterator i = m_mapping.begin(); i != m_mapping.end(); ++i)
                    {
                        Row4 const& row = *(*i);
                   
                        if( m_filter.empty() || 
                            Util::match_keys( Glib::ustring(get<0>(row)).lowercase(), m_filter ) ||
                            Util::match_keys( Glib::ustring(get<1>(row)).lowercase(), m_filter ) ||
                            Util::match_keys( Glib::ustring(get<2>(row)).lowercase(), m_filter ))
                        {
                            new_mapping.push_back(*i);
                        }
                    }

                    std::swap(m_mapping, new_mapping);
                }

        };

        typedef boost::shared_ptr<DataModelFilter> DataModelFilterP;

        class Column
        {
                int m_width;
                int m_column;

            public:

                Column ()
                : m_width(200)
                , m_column(0)
                {
                }

                ~Column ()
                {
                }

                void
                set_width (int width)
                {
                    m_width = width - 8;
                }

                int
                get_width ()
                {
                    return m_width;
                }

                void
                set_column (int column)
                {
                    m_column = column;
                }

                int
                get_column ()
                {
                    return m_column;
                }

                void
                render(Cairo::RefPtr<Cairo::Context> cr, Row4 const& datarow, Gtk::Widget& widget, int row, int x_pos, int y_pos, int rowheight)
                {
                    using boost::get;

                    cr->rectangle( x_pos, y_pos, m_width, rowheight);
                    cr->clip();
                    cr->move_to( x_pos, y_pos + 4);

                    Glib::RefPtr<Pango::Layout> layout; 
                    switch( m_column )
                    {
                        case 0:
                            layout = widget.create_pango_layout(get<0>(datarow));
                            break;
                        case 1:
                            layout = widget.create_pango_layout(get<1>(datarow));
                            break;
                        case 2:
                            layout = widget.create_pango_layout(get<2>(datarow));
                            break;
                            
                    }
                    layout->set_ellipsize(Pango::ELLIPSIZE_END);
                    layout->set_width(m_width*PANGO_SCALE);
                    pango_cairo_show_layout(cr->cobj(), layout->gobj());

                    cr->reset_clip();
                }
        };

        typedef boost::shared_ptr<Column> ColumnP;
        typedef std::vector<ColumnP> Columns;

        typedef sigc::signal<void, gint64> SignalTrackActivated;

        class ListView : public Gtk::DrawingArea
        {
                int                                 m_rowheight;
                int                                 m_visibleheight;
                int                                 m_previousdrawnrow;
                DataModelFilterP                    m_model;
                Columns                             m_columns;
                PropAdj                             m_prop_vadj;
                PropAdj                             m_prop_hadj;
                guint                               m_signal0; 
                std::vector<ModelT::iterator>       m_selection;
                boost::optional<ModelT::iterator>   m_selected;
                SignalTrackActivated                m_trackactivated;
                GtkWidget                         * m_treeview;

                void
                initialize_metrics ()
                {
                    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (gobj()));

                    PangoFontMetrics *metrics = pango_context_get_metrics (context,
                                                                            GTK_WIDGET (gobj())->style->font_desc, 
                                                                            pango_context_get_language (context));

                    m_rowheight = (pango_font_metrics_get_ascent (metrics)/PANGO_SCALE) + 
                                   (pango_font_metrics_get_descent (metrics)/PANGO_SCALE) + 8;
                }

                void
                on_vadj_value_changed ()
                {
                    int row = double(m_model->size()) * double(m_prop_vadj.get_value()->get_value());
                    if( m_previousdrawnrow != row )
                    {
                        queue_draw ();
                    }
                }

            protected:

                virtual void
                on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time)
                {
                    using boost::get;

                    if(!m_selected)
                        return;

                    Row4 const& r = *(m_selected.get());
                    gint64 * id = new gint64(get<3>(r));
                    selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(id), 8);
                }

                bool
                on_button_press_event (GdkEventButton * event)
                {
                    using boost::get;

                    if(event->type == GDK_2BUTTON_PRESS)
                    {
                        Row4 const& r = *(m_selected.get());
                        gint64 id = get<3>(r);
                        m_trackactivated.emit(id);
                    }
                    else
                    {
                        int row = (double(m_model->size() - (m_visibleheight/m_rowheight)) * double(m_prop_vadj.get_value()->get_value())) + std::floor(double(event->y) / double(m_rowheight));
                        m_selected = m_model->m_mapping[row];
                        queue_draw ();
                    }
                
                    return false;
                }

                bool
                on_configure_event (GdkEventConfigure * event)        
                {
                    m_prop_vadj.get_value()->set_page_size(double(event->height/m_rowheight - 2) / double(m_model->size()));

                    m_visibleheight = event->height;

                    double columnwidth = double(event->width) / double(m_columns.size());

                    for(int n = 0; n < m_columns.size(); ++n)
                    {
                        m_columns[n]->set_width(columnwidth);
                    }
                    return false;
                }

                bool
                on_expose_event (GdkEventExpose *event)
                {
                    Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context(); 

                    cr->set_operator(Cairo::OPERATOR_SOURCE);
                    Gdk::Cairo::set_source_color(cr, get_style()->get_base(Gtk::STATE_NORMAL));
                    cr->paint();

                    Gtk::Allocation const& alloc = get_allocation();

                    int row = double(m_model->size() - (m_visibleheight/m_rowheight)) * double(m_prop_vadj.get_value()->get_value());
                    int y_pos = 0;

                    m_previousdrawnrow = row;

                    Gdk::Cairo::set_source_color(cr, get_style()->get_text(Gtk::STATE_NORMAL));

                    while(m_model->is_set() && (y_pos < m_visibleheight) && (row < m_model->size())) 
                    {
                        int x_pos = 8;

                        if( row % 2 )
                        {
                          GdkRectangle background_area;
                          background_area.x = 0;
                          background_area.y = y_pos;
                          background_area.width = alloc.get_width();
                          background_area.height = m_rowheight;

                          gtk_paint_flat_box(get_style()->gobj(),
                                  event->window,
                                  GTK_STATE_NORMAL,
                                  GTK_SHADOW_NONE,
                                  &event->area,
                                  m_treeview,
                                  "cell_odd_ruled",
                                  background_area.x,
                                  background_area.y,
                                  background_area.width,
                                  background_area.height);
                        }


                        ModelT::iterator selected = m_model->m_mapping[row];

                        if( m_selected && m_selected.get() == selected) 
                        {
                            cr->set_operator(Cairo::OPERATOR_ATOP);

                            Gdk::Color c = get_style()->get_base(Gtk::STATE_SELECTED);

                            cr->set_source_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p(), 0.8);

                            RoundedRectangle(cr, 4, y_pos+2, alloc.get_width()-8, m_rowheight-4, 4.);

                            cr->fill_preserve(); 

                            cr->set_source_rgb(c.get_red_p(), c.get_green_p(), c.get_blue_p());

                            cr->stroke();

                            cr->set_operator(Cairo::OPERATOR_SOURCE);
                            Gdk::Cairo::set_source_color(cr, get_style()->get_text(Gtk::STATE_NORMAL));
                        }

                        for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
                        {
                            (*i)->render(cr, m_model->row(row), *this, row, x_pos, y_pos, m_rowheight);
                            x_pos += (*i)->get_width();
                        }

                        y_pos += m_rowheight;
                        row ++;
                    }

                    return true;
                }

                void
                on_model_changed()
                {
                    int view_count = m_visibleheight / m_rowheight;
                    if( m_model->size() < view_count )
                    {
                        m_prop_vadj.get_value()->set_value(0.);
                    } 
                    m_selected.reset();
                    queue_draw();
                }

                static gboolean
                list_view_set_adjustments(
                    GtkWidget*obj,
                    GtkAdjustment*hadj,
                    GtkAdjustment*vadj, 
                    gpointer data
                )
                {
                    g_object_set(G_OBJECT(obj), "vadjustment", vadj, NULL); 
                    g_object_set(G_OBJECT(obj), "hadjustment", hadj, NULL);
                    g_object_set(G_OBJECT(vadj), "page-size", 0.05, "upper", 1.0, NULL);

                    ListView & view = *(reinterpret_cast<ListView*>(data));

                    view.m_prop_vadj.get_value()->signal_value_changed().connect(
                        sigc::mem_fun(
                            view,
                            &ListView::on_vadj_value_changed
                    ));

                    return TRUE;
                }

            public:

                void
                set_model(DataModelFilterP model)
                {
                    m_model = model;
                    set_size_request(200, 8 * m_rowheight);
                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &ListView::on_model_changed
                    ));
                }

                void
                append_column (ColumnP column)
                {
                    m_columns.push_back(column);
                }

                SignalTrackActivated&
                signal_track_activated()
                {
                    return m_trackactivated;
                }

                ListView ()
                : ObjectBase    ("MPXListView")
                , m_previousdrawnrow(0)
                , m_prop_vadj   (*this, "vadjustment", (Gtk::Adjustment*)(0))
                , m_prop_hadj   (*this, "hadjustment", (Gtk::Adjustment*)(0))
                {
                    m_treeview = gtk_tree_view_new();
                    gtk_widget_realize(GTK_WIDGET(m_treeview));

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

                    std::vector<Gtk::TargetEntry> Entries;
                    Entries.push_back(Gtk::TargetEntry("mpx-track", Gtk::TARGET_SAME_APP, 0x81));
                    drag_source_set(Entries); 
                }

                ~ListView ()
                {
                }
        };
}

#endif
