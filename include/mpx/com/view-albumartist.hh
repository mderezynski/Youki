#ifndef YOUKI_LISTVIEW_AA_HH
#define YOUKI_LISTVIEW_AA_HH

#include <gtkmm.h>
#include <gtk/gtktreeview.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "mpx/aux/glibaddons.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/algorithm/aque.hh"

#include "glib-marshalers.h"

using boost::get ;

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace
{
    const double rounding_aa = 4. ; 
}

namespace MPX
{
        typedef boost::tuple<std::string, gint64>           Row2 ;
        typedef std::vector<Row2>                           ModelAA_t ;
        typedef boost::shared_ptr<ModelAA_t>                ModelAA_SP_t ;
        typedef sigc::signal<void>                          SignalAA_0 ;
        typedef std::map<gint64, ModelAA_t::iterator>       IdIterMapAA_t ;

        struct DataModelAA
        {
                ModelAA_SP_t        m_realmodel ;
                SignalAA_0          m_changed ;
                IdIterMapAA_t       m_iter_map ;

                DataModelAA()
                {
                    m_realmodel = ModelAA_SP_t( new ModelAA_t ) ; 
                }

                DataModelAA(
                    ModelAA_SP_t model
                )
                {
                    m_realmodel = model; 
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear () ;
                    m_iter_map.clear() ;
                    m_changed.emit () ;
                } 

                virtual SignalAA_0&
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

                virtual Row2&
                row (int row)
                {
                    return (*m_realmodel)[row];
                }

                virtual void
                append_artist(
                      const std::string&        artist
                    , gint64                    artist_id
                )
                {
                    Row2 row ( artist, artist_id ) ;
                    m_realmodel->push_back(row);

                    ModelAA_t::iterator i = m_realmodel->end();
                    std::advance( i, -1 );
                    m_iter_map.insert(std::make_pair(artist_id, i)); 
                }
        };

        typedef boost::shared_ptr<DataModelAA> DataModelAA_SP_t;

        struct DataModelFilterAA : public DataModelAA
        {
                typedef std::vector<ModelAA_t::iterator> RowRowMapping;

                RowRowMapping           m_mapping ;
                std::set<gint64>        m_constraint_artist ;

                DataModelFilterAA(DataModelAA_SP_t & model)
                : DataModelAA(model->m_realmodel)
                {
                    regen_mapping ();
                }

                virtual void
                set_constraint(
                    const std::set<gint64>& constraint
                )
                {
                    m_constraint_artist = constraint ;
                }

                virtual void
                clear_constraint(
                )
                {
                    m_constraint_artist.clear() ;
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear () ;
                    m_mapping.clear () ;
                    m_iter_map.clear() ;
                    m_changed.emit () ;
                } 

                virtual int 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row2&
                row (int row)
                {
                    return *(m_mapping[row]);
                }

                virtual void
                append_artist(
                      const std::string&        artist
                    , gint64                    artist_id
                )
                {
                    DataModelAA::append_artist( artist, artist_id ) ;
                    regen_mapping();
                }
                
                virtual void
                append_artist_quiet(
                      const std::string&        artist
                    , gint64                    artist_id
                )
                {
                    DataModelAA::append_artist( artist, artist_id ) ;
                }

                void
                regen_mapping(
                )
                {
                    if( !m_realmodel->size() )
                    {
                        return ;
                    }

                    using boost::get;

                    RowRowMapping new_mapping;

                    ModelAA_t::iterator i = m_realmodel->begin() ; 
                    new_mapping.push_back( i ) ;
                    ++i ;

                    for( ; i != m_realmodel->end(); ++i )
                    {
                        const Row2& row = *i;

                        int truth = (m_constraint_artist.empty() || m_constraint_artist.count( get<1>(row) )) ;

                        if( truth )
                        {
                            new_mapping.push_back( i ) ;
                        }
                    }

                    if( new_mapping != m_mapping )
                    {
                        std::swap(m_mapping, new_mapping);
                        m_changed.emit();
                    }
                }
        };

        typedef boost::shared_ptr<DataModelFilterAA> DataModelFilterAA_SP_t;

        class ColumnAA
        {
                int                 m_width ;
                int                 m_column ;
                std::string         m_title ;
                Pango::Alignment    m_alignment ;

            public:

                ColumnAA (std::string const& title)
                : m_width( 0 )
                , m_column( 0 )
                , m_title( title )
                , m_alignment( Pango::ALIGN_LEFT )
                {
                }

                ~ColumnAA ()
                {
                }

                void
                set_width (int width)
                {
                    m_width = width; 
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
                set_title (std::string const& title)
                {
                    m_title = title;
                }

                std::string const&
                get_title ()
                {
                    return m_title;
                }

                void
                set_alignment(
                    Pango::Alignment align
                )
                {
                    m_alignment = align ;
                }


                Pango::Alignment
                get_alignment(
                )
                {
                    return m_alignment ;
                }

                void
                render(
                      Cairo::RefPtr<Cairo::Context>   cairo
                    , const Row2&                     datarow
                    , Gtk::Widget&                    widget
                    , int                             row
                    , int                             xpos
                    , int                             ypos
                    , int                             rowheight
                    , bool                            selected
                )
                {
                    using boost::get;

                    cairo->set_operator(Cairo::OPERATOR_ATOP);
                    cairo->set_source_rgba( 1., 1., 1., 1. ) ;

                    cairo->rectangle(
                          xpos
                        , ypos
                        , m_width
                        , rowheight
                    ) ;
                    cairo->clip();
                    cairo->move_to(
                          xpos + 6
                        , ypos + 4
                    ) ;

                    std::string str;
                    switch( m_column )
                    {
                        case 0:
                            str = get<0>(datarow);
                            break;
                    }

                    Glib::RefPtr<Pango::Layout> layout; 

                    if( row == 0 )
                    {
                        layout = widget.create_pango_layout("");
                        layout->set_markup("<b>" + Glib::Markup::escape_text(str) + "</b>") ;
                    }
                    else
                    {
                        layout = widget.create_pango_layout(str);
                    }

                    layout->set_ellipsize(
                          Pango::ELLIPSIZE_END
                    ) ;

                    layout->set_width(
                          (m_width - 8) * PANGO_SCALE
                    ) ;

                    layout->set_alignment(
                          m_alignment
                    ) ;

                    pango_cairo_show_layout(
                          cairo->cobj()
                        , layout->gobj()
                    ) ;

                    cairo->reset_clip();
                }
        };

        typedef boost::shared_ptr<ColumnAA> ColumnAA_SP_t ;
        typedef std::vector<ColumnAA_SP_t>  ColumnAA_SP_vector_t ;
        typedef sigc::signal<void>          SignalSelectionChanged ;

        class ListViewAA : public Gtk::DrawingArea
        {
                int                                 m_row_height ;
                int                                 m_visible_height ;
                int                                 m_previous_drawn_row ;

                DataModelFilterAA_SP_t              m_model ;
                ColumnAA_SP_vector_t                m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                guint                               m_signal0 ; 

                boost::optional<std::pair<ModelAA_t::iterator, gint64> > m_selection ;

                bool                                m_highlight ;

                std::set<int>                       m_collapsed ;
                std::set<int>                       m_fixed ;
                int                                 m_fixed_total_width ;
        
                SignalSelectionChanged              m_SIGNAL_selection_changed ;

                GtkWidget                         * m_treeview ;

                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                void
                initialize_metrics ()
                {
                    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (gobj()));

                    PangoFontMetrics *metrics = pango_context_get_metrics(
                          context
                        , GTK_WIDGET (gobj())->style->font_desc
                        , pango_context_get_language (context)
                    ) ;

                    m_row_height =
                        (pango_font_metrics_get_ascent (metrics)/PANGO_SCALE) + 
                        (pango_font_metrics_get_descent (metrics)/PANGO_SCALE) + 8;

                    const int visible_area_pad = 2 ;
                }

                void
                on_vadj_value_changed ()
                {
                    int row = (double(m_prop_vadj.get_value()->get_value()-m_row_height) / double(m_row_height));
                    if( m_previous_drawn_row != row )
                    {
                        queue_draw ();
                    }
                }

                int
                get_upper_row ()
                {
                    int row_upper = (m_prop_vadj.get_value()->get_value() / m_row_height); 
                    return row_upper;
                }

                bool
                get_row_is_visible (int row)
                {
                    int row_upper = (m_prop_vadj.get_value()->get_value() / m_row_height); 
                    int row_lower = row_upper + m_visible_height/m_row_height;
                    return ( row >= row_upper && row <= row_lower);
                }

            protected:

               virtual bool
                on_focus (Gtk::DirectionType direction)
                { 
                    grab_focus() ;
                    return false ;
                }

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    int step; 

                    switch( event->keyval )
                    {
                        case GDK_Up:
                        case GDK_KP_Up:
                        case GDK_Page_Up:

                            if( event->keyval == GDK_Page_Up )
                            {
                                step = - (m_visible_height / m_row_height) + 1;
                            }
                            else
                            {
                                step = -1;
                            }

                            if( !m_selection )
                            {
                                mark_first_row_up:
    
                                int row = get_upper_row();
                                m_selection = std::make_pair(m_model->m_mapping[row], get<1>(*m_model->m_mapping[row])) ;
                                m_SIGNAL_selection_changed.emit() ;
                            }
                            else
                            {
                                int row = m_selection.get().second;

                                if( get_row_is_visible( row ))
                                {
                                    ModelAA_t::iterator i = m_selection.get().first;

                                    std::advance(i, step);
                                    row += step;

                                    if( row >= 0 )
                                    {
                                        m_selection = std::make_pair(m_model->m_mapping[row], get<1>(*m_model->m_mapping[row])) ;
                                        m_SIGNAL_selection_changed.emit() ;

                                        if( row < get_upper_row()) 
                                        {
                                            double value = m_prop_vadj.get_value()->get_value();
                                            value += step*m_row_height;
                                            m_prop_vadj.get_value()->set_value( value );
                                        }
                                    }
                                }
                                else
                                {
                                    goto mark_first_row_up;
                                }
                            }
                            queue_draw();
                            return true;

                        case GDK_Down:
                        case GDK_KP_Down:
                        case GDK_Page_Down:

                            if( event->keyval == GDK_Page_Down )
                            {
                                step = (m_visible_height / m_row_height) - 1;
                            }
                            else
                            {
                                step = 1;
                            }

                            if( !m_selection ) 
                            {
                                mark_first_row_down:

                                int row = get_upper_row();
                                m_selection = std::make_pair(m_model->m_mapping[row], get<1>(*m_model->m_mapping[row])) ;
                                m_SIGNAL_selection_changed.emit() ;
                            }
                            else
                            {
                                int row = m_selection.get().second;

                                if( get_row_is_visible( row ) )
                                {
                                    ModelAA_t::iterator i = m_selection.get().first;
    
                                    std::advance(i, step);
                                    row += step;

                                    if( row < m_model->m_mapping.size() )
                                    {
                                        m_selection = std::make_pair(m_model->m_mapping[row], get<1>(*m_model->m_mapping[row])) ;
                                        m_SIGNAL_selection_changed.emit() ;

                                        if( row >= (get_upper_row() + (m_visible_height/m_row_height)))
                                        {
                                            double value = m_prop_vadj.get_value()->get_value();
                                            value += step*m_row_height;
                                            m_prop_vadj.get_value()->set_value( value );
                                        }
                                    }
                                }
                                else
                                {
                                    goto mark_first_row_down;
                                }
                            }
                            queue_draw();
                            return true;

                        default:

                            int x, y, x_root, y_root ;

                            dynamic_cast<Gtk::Window*>(get_toplevel())->get_position( x_root, y_root ) ;

                            x = x_root + get_allocation().get_x() ;
                            y = y_root + get_allocation().get_y() + get_allocation().get_height() ;

                            m_SearchWindow->set_size_request( get_allocation().get_width(), -1 ) ;
                            m_SearchWindow->move( x, y ) ;

                            m_SearchWindow->show() ;
                    }

                    return false;
                }

                bool
                on_button_press_event (GdkEventButton * event)
                {
                    using boost::get;

                    if(event->type == GDK_BUTTON_PRESS)
                    {
                            int row = get_upper_row() + (int(event->y) / m_row_height);

                            if( row < m_model->m_mapping.size() )
                            {
                                m_selection = std::make_pair(m_model->m_mapping[row], get<1>(*m_model->m_mapping[row])) ;
                                m_SIGNAL_selection_changed.emit() ;
                                queue_draw();
                            }
                    }
                
                    return true;
                }

                bool
                on_button_release_event (GdkEventButton * event)
                {
                   return false;
                }

                bool
                on_leave_notify_event(
                    GdkEventCrossing* G_GNUC_UNUSED
                )
                {
                    queue_draw () ;

                    return true ;
                }

                bool
                on_motion_notify_event(
                    GdkEventMotion* event
                )
                {
                    return true ;
                }

                bool
                on_configure_event(
                    GdkEventConfigure* event
                )        
                {
                    m_visible_height = event->height ;

                    m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;
                    m_prop_vadj.get_value()->set_page_size( (m_visible_height/m_row_height)*int(m_row_height) ) ; 

                    double column_width = (double(event->width) - m_fixed_total_width - (40*m_collapsed.size()) ) / double(m_columns.size()-m_collapsed.size()-m_fixed.size());

                    for(int n = 0; n < m_columns.size(); ++n)
                    {
                        if( m_fixed.count( n ) )
                        {
                            continue ;
                        } 
                        if( m_collapsed.count( n ) )
                        {
                            m_columns[n]->set_width( 40 ) ;
                        }
                        else
                        {
                            m_columns[n]->set_width( column_width ) ;
                        }
                    }

                    queue_draw() ;

                    return false;
                }

                bool
                on_expose_event (GdkEventExpose *event)
                {
                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context(); 

                    cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
                    cairo->set_source_rgba(
                          0.12    
                        , 0.12
                        , 0.12
                        , 1.
                    ) ;
                    cairo->paint();

                    const Gtk::Allocation& alloc = get_allocation();

                    int row = get_upper_row() ;
                    m_previous_drawn_row = row;

                    int ypos    = 0 ;
                    int xpos    = 0 ;
                    int col     = 0 ;
                    int cnt     = m_visible_height / m_row_height + 1 ;

                    cairo->set_operator(Cairo::OPERATOR_ATOP);
    
                    while( m_model->is_set() && cnt && (row < m_model->m_mapping.size()) ) 
                    {
                        const int inner_pad  = 1 ;

                        xpos = 0 ;

                        bool iter_is_selected = ( m_selection && m_selection.get().second == get<1>(*m_model->m_mapping[row])) ;

                        if( !(row % 2) ) 
                        {
                            GdkRectangle r ;

                            r.x       = inner_pad ;
                            r.y       = ypos + inner_pad ;
                            r.width   = alloc.get_width() - 2 * inner_pad ; 
                            r.height  = m_row_height - 2 * inner_pad ;

                            RoundedRectangle(
                                  cairo
                                , r.x
                                , r.y
                                , r.width
                                , r.height
                                , rounding_aa
                            ) ;

                            cairo->set_source_rgba(
                                  0.2
                                , 0.2
                                , 0.2
                                , 1.
                            ) ;

                            cairo->fill() ;
                        }

                        if( iter_is_selected ) 
                        {
                            Gdk::Color c = get_style()->get_base( Gtk::STATE_SELECTED ) ;

                            GdkRectangle r ;

                            r.x         = inner_pad ;
                            r.y         = ypos + inner_pad ;
                            r.width     = alloc.get_width() - 2*inner_pad ;  
                            r.height    = m_row_height - 2*inner_pad ;
        
                            cairo->save () ;

                            Cairo::RefPtr<Cairo::LinearGradient> background_gradient_ptr = Cairo::LinearGradient::create(
                                  r.x + r.width / 2
                                , r.y  
                                , r.x + r.width / 2
                                , r.y + r.height
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  0
                                , c.get_red_p() 
                                , c.get_green_p()
                                , c.get_blue_p()
                                , 0.90 
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  .40
                                , c.get_red_p() 
                                , c.get_green_p()
                                , c.get_blue_p()
                                , 0.75 
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  1. 
                                , c.get_red_p() 
                                , c.get_green_p()
                                , c.get_blue_p()
                                , 0.45 
                            ) ;

                            cairo->set_source( background_gradient_ptr ) ;
                            cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

                            RoundedRectangle(
                                  cairo
                                , r.x 
                                , r.y 
                                , r.width 
                                , r.height 
                                , rounding_aa 
                            ) ;

                            cairo->fill_preserve (); 

                            cairo->set_source_rgb(
                                  c.get_red_p()
                                , c.get_green_p()
                                , c.get_blue_p()
                            ) ;

                            cairo->set_line_width( 0.8 ) ;
                            cairo->stroke () ;

                            cairo->restore () ;
                        }

                        for(ColumnAA_SP_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
                        {
                            (*i)->render(cairo, m_model->row(row), *this, row, xpos, ypos, m_row_height, iter_is_selected);
                            xpos += (*i)->get_width();
                        }

                        ypos += m_row_height;
                        row ++;
                        cnt --;
                    }

                    if( has_focus() )
                    {
                        get_style()->paint_focus(
                            get_window(),
                            Gtk::STATE_NORMAL,
                            get_allocation(),
                            *this,
                            "treeview",
                            0, 
                            0, 
                            get_allocation().get_width(), 
                            get_allocation().get_height()
                        );
                    }

                    return true;
                }

                void
                on_model_changed()
                {
                    int view_count = m_visible_height / m_row_height;

                    if( m_model->size() < view_count )
                    {
                        m_prop_vadj.get_value()->set_value(0.);
                    } 

                    m_selection.reset();

                    m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;
                    m_prop_vadj.get_value()->set_value(0.);

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
                    if( vadj )
                    {
                            g_object_set(G_OBJECT(obj), "vadjustment", vadj, NULL); 
                            g_object_set(G_OBJECT(obj), "hadjustment", hadj, NULL);
                            g_object_set(G_OBJECT(vadj), "page-size", 0.05, "upper", 1.0, NULL);

                            ListViewAA & view = *(reinterpret_cast<ListViewAA*>(data));

                            view.m_prop_vadj.get_value()->signal_value_changed().connect(
                                sigc::mem_fun(
                                    view,
                                    &ListViewAA::on_vadj_value_changed
                            ));
                    }

                    return TRUE;
                }

            public:

                SignalSelectionChanged&
                signal_selection_changed()
                {
                    return m_SIGNAL_selection_changed ;
                }

                gint64
                get_selected()
                {
                    return m_selection ? m_selection.get().second : -1 ;
                }
    
                void
                clear_selection()
                {
                    if( m_selection )
                    {
                        m_selection.reset() ;
                        // so we ESPECIALLY don't signal out if there is no selection anyway
                    }
                }

                void
                set_highlight(bool highlight)
                {
                    m_highlight = highlight;
                    queue_draw ();
                }

                void
                set_model(DataModelFilterAA_SP_t model)
                {
                    m_model = model;
                    set_size_request(200, 8 * m_row_height);
                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &ListViewAA::on_model_changed
                    ));
                }

                void
                append_column (ColumnAA_SP_t column)
                {
                    m_columns.push_back(column);
                }

                void
                column_set_collapsed(
                      int       column
                    , bool      collapsed
                )
                {
                    if( collapsed )
                    {
                        m_collapsed.insert( column ) ;
                        queue_resize () ;
                        queue_draw () ;
                    }
                    else
                    {
                        m_collapsed.erase( column ) ;
                        queue_resize () ;
                        queue_draw () ;
                    }
                }

                void
                column_set_fixed(
                      int       column
                    , bool      fixed
                    , int       width = 0
                )
                {
                    if( fixed )
                    {
                        m_fixed.insert( column ) ;
                        m_fixed_total_width += width ;
                        m_columns[column]->set_width( width ) ;
                        queue_resize () ;
                        queue_draw () ;
                    }
                    else
                    {
                        m_fixed.erase( column ) ;
                        m_fixed_total_width -= m_columns[column]->get_width() ; 
                        queue_resize () ;
                        queue_draw () ;
                    }
                }

                ListViewAA ()

                        : ObjectBase( "YoukiListViewAA" )
                        , m_previous_drawn_row( 0 )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_highlight( false )
                        , m_fixed_total_width( 0 )

                {
                    m_treeview = gtk_tree_view_new();
                    gtk_widget_realize(GTK_WIDGET(m_treeview));

                    set_flags(Gtk::CAN_FOCUS);
                    add_events(Gdk::EventMask(GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK ));

                    ((GtkWidgetClass*)(G_OBJECT_GET_CLASS(G_OBJECT(gobj()))))->set_scroll_adjustments_signal = 
                            g_signal_new ("set_scroll_adjustments",
                                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                      GSignalFlags (G_SIGNAL_RUN_FIRST),
                                      0,
                                      NULL, NULL,
                                      g_cclosure_user_marshal_VOID__OBJECT_OBJECT, G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

                    g_signal_connect(G_OBJECT(gobj()), "set_scroll_adjustments", G_CALLBACK(list_view_set_adjustments), this);

                    gtk_widget_realize(GTK_WIDGET(gobj()));
                    initialize_metrics();

                    m_SearchEntry = Gtk::manage( new Gtk::Entry ) ;
                    m_SearchWindow = Gtk::manage( new Gtk::Window( Gtk::WINDOW_TOPLEVEL )) ;

                    m_SearchWindow->add( *m_SearchEntry ) ;
                }

                ~ListViewAA ()
                {
                }
        };
}

#endif
