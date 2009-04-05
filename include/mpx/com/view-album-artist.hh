#ifndef YOUKI_LISTVIEW_Artist_HH
#define YOUKI_LISTVIEW_Artist_HH

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

#include <set>

using boost::get ;

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace
{
    const double rounding_aa = 4. ; 
}

namespace MPX
{
        typedef boost::tuple<std::string, gint64>           Row2 ;
        typedef std::vector<Row2>                           ModelArtist_t ;
        typedef boost::shared_ptr<ModelArtist_t>            ModelArtist_SP_t ;
        typedef sigc::signal<void, std::size_t>             SignalArtist_1 ;
        typedef std::map<gint64, ModelArtist_t::iterator>   IdIterMapArtist_t ;

        struct DataModelArtist : public sigc::trackable
        {
                ModelArtist_SP_t        m_realmodel ;
                SignalArtist_1          m_changed ;
                SignalArtist_1          m_select ;
                IdIterMapArtist_t       m_iter_map ;
                std::size_t             m_position ;
                boost::optional<gint64> m_selected ;

                DataModelArtist()
                : m_position( 0 )
                {
                    m_realmodel = ModelArtist_SP_t( new ModelArtist_t ) ; 
                }

                DataModelArtist(
                    ModelArtist_SP_t model
                )
                : m_position( 0 )
                {
                    m_realmodel = model; 
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear () ;
                    m_iter_map.clear() ;
                    m_changed.emit( m_position ) ;
                } 

                virtual SignalArtist_1&
                signal_changed ()
                {
                    return m_changed ;
                }

                virtual SignalArtist_1&
                signal_select ()
                {
                    return m_select ;
                }


                virtual bool
                is_set ()
                {
                    return bool(m_realmodel);
                }

                virtual std::size_t
                size ()
                {
                    return m_realmodel->size();
                }

                virtual Row2&
                row (std::size_t row)
                {
                    return (*m_realmodel)[row];
                }

                virtual void
                set_current_row(
                    std::size_t row
                )
                {
                    m_position = row ;
                }

                virtual void
                set_selected(
                    boost::optional<gint64> row
                )
                {
                    m_selected = row ;
                }

                virtual void
                append_artist(
                      const std::string&        artist
                    , gint64                    artist_id
                )
                {
                    Row2 row ( artist, artist_id ) ;
                    m_realmodel->push_back(row);

                    ModelArtist_t::iterator i = m_realmodel->end();
                    std::advance( i, -1 );
                    m_iter_map.insert(std::make_pair(artist_id, i)); 
                }
        };

        typedef boost::shared_ptr<DataModelArtist> DataModelArtist_SP_t;

        struct DataModelFilterArtist : public DataModelArtist
        {
                typedef std::vector<ModelArtist_t::iterator> RowRowMapping;

                RowRowMapping                       m_mapping ;

                boost::optional<std::set<gint64> >  m_constraint_id_artist ;

                DataModelFilterArtist(DataModelArtist_SP_t & model)
                : DataModelArtist(model->m_realmodel)
                {
                    regen_mapping ();
                }

                virtual ~DataModelFilterArtist()
                {
                }

                virtual void
                set_constraint_artist(
                    const boost::optional<std::set<gint64> >& constraint
                )
                {
                    m_constraint_id_artist = constraint ;
                }

                virtual void
                clear_constraint_artist(
                )
                {
                    m_constraint_id_artist.reset() ;
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear( ) ;
                    m_mapping.clear( ) ;
                    m_iter_map.clear( ) ;
                    m_changed.emit( m_position ) ;
                } 

                virtual std::size_t 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row2&
                row (std::size_t row)
                {
                    return *(m_mapping[row]);
                }

                virtual void
                append_artist(
                      const std::string&        artist
                    , gint64                    artist_id
                )
                {
                    DataModelArtist::append_artist( artist, artist_id ) ;
                    regen_mapping();
                }
                
                virtual void
                append_artist_quiet(
                      const std::string&        artist
                    , gint64                    artist_id
                )
                {
                    DataModelArtist::append_artist( artist, artist_id ) ;
                }

                void
                regen_mapping(
                )
                {
                    using boost::get;

                    if( m_realmodel->size() == 0 )
                    {
                        return ;
                    }

                    RowRowMapping new_mapping;

                    if( !m_constraint_id_artist )
                    {
                        m_selected.reset() ;
                    }

                    boost::optional<gint64> id_cur = ( m_position < m_mapping.size()) ? get<1>(row( m_position )) : boost::optional<gint64>() ;
                    boost::optional<gint64> id_sel = m_selected ;

                    m_position = 0 ;

                    std::size_t new_sel_pos = 0 ;

                    ModelArtist_t::iterator i = m_realmodel->begin() ; 
                    new_mapping.push_back( i++ ) ;

                    for( ; i != m_realmodel->end(); ++i )
                    {
                        int truth = !m_constraint_id_artist || m_constraint_id_artist.get().count( get<1>(*i)) ;

                        if( truth )
                        {
                            new_mapping.push_back( i ) ;
                        }

                        if( id_cur && get<1>(*i) == id_cur.get() )
                        {
                            m_position = new_mapping.size()  - 1 ;
                        }

                        if( id_sel && get<1>(*i) == id_sel.get() )
                        {
                            new_sel_pos = new_mapping.size()  - 1 ;
                        }
                    }

                    if( new_mapping != m_mapping )
                    {
                        std::swap(m_mapping, new_mapping);
                        m_changed.emit( m_position );
                        m_select.emit( new_sel_pos ) ;
                    }
                }
        };

        typedef boost::shared_ptr<DataModelFilterArtist> DataModelFilterArtist_SP_t;

        class ColumnArtist
        {
                int                 m_width ;
                int                 m_column ;
                std::string         m_title ;
                Pango::Alignment    m_alignment ;

            public:

                ColumnArtist (std::string const& title)
                : m_width( 0 )
                , m_column( 0 )
                , m_title( title )
                , m_alignment( Pango::ALIGN_LEFT )
                {
                }

                ~ColumnArtist ()
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

        typedef boost::shared_ptr<ColumnArtist> ColumnArtist_SP_t ;
        typedef std::vector<ColumnArtist_SP_t>  ColumnArtist_SP_vector_t ;
        typedef sigc::signal<void>              SignalSelectionChanged ;

        class ListViewArtist : public Gtk::DrawingArea
        {
                int                                 m_row_height ;
                int                                 m_visible_height ;

                DataModelFilterArtist_SP_t          m_model ;
                ColumnArtist_SP_vector_t            m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                guint                               m_signal0 ; 

                boost::optional<boost::tuple<ModelArtist_t::iterator, gint64, std::size_t> > m_selection ;

                bool                                m_highlight ;

                std::set<int>                       m_collapsed ;
                std::set<int>                       m_fixed ;
                int                                 m_fixed_total_width ;
        
                SignalSelectionChanged              m_SIGNAL_selection_changed ;

                GtkWidget                         * m_treeview ;

                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                sigc::connection                    m_search_changed_conn ;
                bool                                m_search_active ;

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
                }

                void
                on_vadj_value_changed ()
                {
                    if( m_model->m_mapping.size() )
                    {
                        m_model->set_current_row( get_upper_row() ) ;
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
                    if( !m_selection )
                    {
                        select_row( get_upper_row() ) ;
                    }
                    return true ;
                }

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    if( m_search_active )
                    {
                        switch( event->keyval )
                        {
                            case GDK_Escape:
                                cancel_search() ;
                                return true ;
        
                            default: ;
                        }

                        GdkEvent *new_event = gdk_event_copy( (GdkEvent*)(event) ) ;
                        g_object_unref( ((GdkEventKey*)new_event)->window ) ;
                        ((GdkEventKey *) new_event)->window = GDK_WINDOW(g_object_ref(G_OBJECT(GTK_WIDGET(m_SearchWindow->gobj())->window))) ;
                        gtk_widget_event(GTK_WIDGET(m_SearchEntry->gobj()), new_event) ;
                        gdk_event_free(new_event) ;

                        return false ;
                    }

                    int row ; 

                    switch( event->keyval )
                    {
                        case GDK_Up:
                        case GDK_KP_Up:
                        case GDK_Page_Up:

                            if( !m_selection ) 
                            {
                                select_row( get_upper_row() ) ;
                                break ;
                            }

                            row = boost::get<2>(m_selection.get()) ;

                            if( !get_row_is_visible( row ) )
                            {
                                m_prop_vadj.get_value()->set_value( row * m_row_height ) ;
                            }

                            if( event->keyval == GDK_Page_Up )
                            {
                                row = std::max( 0, row - m_visible_height/m_row_height ) ;

                                if( row < get_upper_row()) 
                                {
                                    double value = m_prop_vadj.get_value()->get_value() - m_visible_height ;
                                    m_prop_vadj.get_value()->set_value( value ) ; 
                                }
                            }
                            else
                            {
                                row = std::max( 0, row - 1 ) ;

                                if( row < get_upper_row()) 
                                {
                                    m_prop_vadj.get_value()->set_value( (get_upper_row()-1) * m_row_height ) ; 
                                }
                            }

                            select_row( row ) ;


                            queue_draw();
                            return true;

                        case GDK_Down:
                        case GDK_KP_Down:
                        case GDK_Page_Down:

                            if( !m_selection ) 
                            {
                                select_row( get_upper_row() ) ;
                                break ;
                            }

                            row = boost::get<2>(m_selection.get()) ;

                            if( !get_row_is_visible( row ) )
                            {
                                m_prop_vadj.get_value()->set_value( row * m_row_height ) ;
                            }

                            if( event->keyval == GDK_Page_Down )
                            {
                                row = std::min( m_model->m_mapping.size(), std::size_t(row + (m_visible_height/m_row_height)) ) ;

                                double value = m_prop_vadj.get_value()->get_value() ; 
                                m_prop_vadj.get_value()->set_value( value + m_visible_height ) ;
                            }
                            else
                            {
                                row = std::min( m_model->m_mapping.size(), std::size_t(row + 1) ) ;

                                if( row > (get_upper_row()+(m_visible_height/m_row_height))) 
                                {
                                    m_prop_vadj.get_value()->set_value( (get_upper_row()+1) * m_row_height ) ;
                                }
                            }

                            select_row( row ) ;

                            queue_draw();
                            return true;

                        case GDK_Left:
                        case GDK_KP_Left:
                        case GDK_Right:
                        case GDK_KP_Right:
                        case GDK_Home:
                        case GDK_KP_Home:
                        case GDK_End:
                        case GDK_KP_End:
                        case GDK_KP_Enter:
                        case GDK_Return:
                            return false ;

                        case GDK_Tab:
                                Gtk::DrawingArea::on_key_press_event( event ) ;
                            return true ;

                        default:

                            if( !Gtk::DrawingArea::on_key_press_event( event ))
                            { 
                                    int x, y, x_root, y_root ;

                                    dynamic_cast<Gtk::Window*>(get_toplevel())->get_position( x_root, y_root ) ;

                                    x = x_root + get_allocation().get_x() ;
                                    y = y_root + get_allocation().get_y() + get_allocation().get_height() ;

                                    m_SearchWindow->set_size_request( get_allocation().get_width(), -1 ) ;
                                    m_SearchWindow->move( x, y ) ;
                                    m_SearchWindow->show() ;

                                    send_focus_change( *m_SearchEntry, true ) ;

                                    GdkEvent *new_event = gdk_event_copy( (GdkEvent*)(event) ) ;
                                    g_object_unref( ((GdkEventKey*)new_event)->window ) ;
                                    gtk_widget_realize( GTK_WIDGET(m_SearchWindow->gobj()) ) ;
                                    ((GdkEventKey *) new_event)->window = GDK_WINDOW(g_object_ref(G_OBJECT(GTK_WIDGET(m_SearchWindow->gobj())->window))) ;
                                    gtk_widget_event(GTK_WIDGET(m_SearchEntry->gobj()), new_event) ;
                                    gdk_event_free(new_event) ;

                                    m_search_active = true ;
                            }
                    }

                    return true ;
                }

                void
                send_focus_change(
                      Gtk::Widget&  w
                    , bool          in
                    )
                {
                    GtkWidget * widget = w.gobj() ;

                    GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

                    g_object_ref (widget);

                   if( in )
                      GTK_WIDGET_SET_FLAGS( widget, GTK_HAS_FOCUS ) ;
                    else
                      GTK_WIDGET_UNSET_FLAGS( widget, GTK_HAS_FOCUS ) ;

                    fevent->focus_change.type   = GDK_FOCUS_CHANGE;
                    fevent->focus_change.window = GDK_WINDOW(g_object_ref( widget->window )) ;
                    fevent->focus_change.in     = in;

                    gtk_widget_event( widget, fevent ) ;

                    g_object_notify(
                          G_OBJECT (widget)
                        , "has-focus"
                    ) ;

                    g_object_unref( widget ) ;
                    gdk_event_free( fevent ) ;
                }


                bool
                on_button_press_event (GdkEventButton * event)
                {
                    using boost::get;

                    if(event->type == GDK_BUTTON_PRESS)
                    {
                            grab_focus() ;

                            std::size_t row = get_upper_row() + (event->y / m_row_height);

                            if( row < m_model->m_mapping.size() )
                            {
                                select_row( row ) ;
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

                    for( std::size_t n = 0; n < m_columns.size(); ++n)
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

                    std::size_t row     = get_upper_row() ;
                    std::size_t ypos    = 0 ;
                    std::size_t xpos    = 0 ;
                    std::size_t cnt     = m_visible_height / m_row_height + 1 ;

                    const std::size_t inner_pad  = 1 ;

                    cairo->set_operator(Cairo::OPERATOR_ATOP);
    
                    while( m_model->is_set() && cnt && (row < m_model->m_mapping.size()) ) 
                    {
                        xpos = 0 ;

                        bool iter_is_selected = ( m_selection && boost::get<1>(m_selection.get()) == get<1>(*m_model->m_mapping[row])) ;

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

                            double factor = has_focus() ? 1. : 0.3 ;

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
                                , 0.90 * factor  
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  .40
                                , c.get_red_p() 
                                , c.get_green_p()
                                , c.get_blue_p()
                                , 0.75 * factor
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  1. 
                                , c.get_red_p() 
                                , c.get_green_p()
                                , c.get_blue_p()
                                , 0.45 * factor 
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

                        for(ColumnArtist_SP_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
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
                              get_window()
                            , Gtk::STATE_NORMAL
                            , get_allocation()
                            , *this
                            , "treeview"
                            , 0 
                            , 0
                            , get_allocation().get_width()
                            , get_allocation().get_height()
                        ) ;
                    }

                    return true;
                }

                void
                on_model_changed( std::size_t position )
                {
                    std::size_t view_count = m_visible_height / m_row_height;

                    m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;

                    if( m_model->size() < view_count )
                    {
                        m_prop_vadj.get_value()->set_value(0.);
                    } 
                    else
                    {
                        m_prop_vadj.get_value()->set_value( position * m_row_height ) ;
                    }

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

                            ListViewArtist & view = *(reinterpret_cast<ListViewArtist*>(data));

                            view.m_prop_vadj.get_value()->signal_value_changed().connect(
                                sigc::mem_fun(
                                    view,
                                    &ListViewArtist::on_vadj_value_changed
                            ));
                    }

                    return TRUE;
                }

            public:

                void
                select_id(
                    boost::optional<gint64> id
                )
                {
                    using boost::get;

                    if( id )
                    {
                            const gint64& real_id = id.get() ;

                            for( DataModelFilterArtist::RowRowMapping::iterator i = m_model->m_mapping.begin(); i != m_model->m_mapping.end(); ++i )
                            {
                                if( real_id == get<1>(**i))
                                {
                                    select_row( std::distance( m_model->m_mapping.begin(), i )) ;
                                    return ;
                                }
                            }
                    }
                
                    clear_selection() ;
                }

                void
                select_row(
                      std::size_t row
                )
                {
                    if( row < m_model->m_mapping.size() )
                    {
                        const gint64& id = get<1>(*m_model->m_mapping[row]) ;

                        m_selection = boost::make_tuple (m_model->m_mapping[row], id, row ) ;
                        m_model->set_selected( id ) ;
                        m_SIGNAL_selection_changed.emit() ;
                        queue_draw();
                    }
                }

                SignalSelectionChanged&
                signal_selection_changed()
                {
                    return m_SIGNAL_selection_changed ;
                }

                boost::optional<gint64>
                get_selected()
                {
                    if( m_selection )
                    {
                            const gint64& sel_id = boost::get<1>(m_selection.get()) ;

                            if( sel_id != -1 )
                            {
                                return boost::optional<gint64>( sel_id ) ;
                            }
                    }

                    return boost::optional<gint64>() ;
                }
    
                void
                clear_selection()
                {
                    if( m_selection )
                    {
                        m_selection.reset() ;
                        m_model->set_selected( boost::optional<gint64>() ) ;

                        if( m_model->m_mapping.size()) 
                        {
                            m_selection = boost::make_tuple(m_model->m_mapping[0], get<1>(*m_model->m_mapping[0]), 0) ;
                        }
                    }
                }

                void
                set_highlight(bool highlight)
                {
                    m_highlight = highlight;
                    queue_draw ();
                }

                void
                set_model(DataModelFilterArtist_SP_t model)
                {
                    m_model = model;

                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &ListViewArtist::on_model_changed
                    ));

                    m_model->signal_select().connect(
                        sigc::mem_fun(
                            *this,
                            &ListViewArtist::select_row
                    ));

                    clear_selection() ;
                }

                void
                append_column (ColumnArtist_SP_t column)
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

            protected:

                void
                on_search_entry_changed()
                {
                    using boost::get ;

                    Glib::ustring text = m_SearchEntry->get_text() ;

                    if( text.empty() )
                    {
                        select_row( 0 ) ;
                        return ;
                    }

                    DataModelFilterArtist::RowRowMapping::iterator i = m_model->m_mapping.begin(); 
                    ++i ; // first row is "All" FIXME this sucks

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        const Row2& row = **i ;
                        Glib::ustring match = get<0>(row) ;

                        if( match.substr( 0, std::min( text.length(), match.length())) == text.substr( 0, std::min( text.length(), match.length())) )   
                        {
                            int row = std::distance( m_model->m_mapping.begin(), i ) ; 
                            m_prop_vadj.get_value()->set_value( row * m_row_height ) ; 
                            select_row( row ) ;
                            break ;
                        }
                    }
                }

                void
                cancel_search()
                {
                    send_focus_change( *m_SearchEntry, false ) ;
                    m_SearchWindow->hide() ;
                    m_search_changed_conn.block () ;
                    m_SearchEntry->set_text("") ;
                    m_search_changed_conn.unblock () ;
                    m_search_active = false ;
                }

                bool
                on_search_window_focus_out(
                      GdkEventFocus* G_GNUC_UNUSED
                )
                {
                    cancel_search() ;
                    return false ;
                }

            public:

                ListViewArtist ()

                        : ObjectBase( "YoukiListViewArtist" )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_highlight( false )
                        , m_fixed_total_width( 0 )
                        , m_search_active( false )

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
                    m_search_changed_conn = m_SearchEntry->signal_changed().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewArtist::on_search_entry_changed
                    )) ;
    
                    m_SearchWindow = Gtk::manage( new Gtk::Window( Gtk::WINDOW_POPUP )) ;
                    m_SearchWindow->set_decorated( false ) ;

                    m_SearchWindow->signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewArtist::on_search_window_focus_out
                    )) ;

                    signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewArtist::on_search_window_focus_out
                    )) ;

                    m_SearchWindow->add( *m_SearchEntry ) ;
                    m_SearchEntry->show() ;

                    property_can_focus() = true ;
                }

                virtual ~ListViewArtist ()
                {
                }
        };
}

#endif
