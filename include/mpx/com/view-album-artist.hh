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

#include "mpx/algorithm/aque.hh"
#include "mpx/algorithm/interval.hh"
#include "mpx/algorithm/limiter.hh"

#include "mpx/aux/glibaddons.hh"
#include "mpx/mpx-types.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/util-string.hh"
#include "mpx/util-graphics.hh"

#include "glib-marshalers.h"

#include "mpx/mpx-main.hh"
#include "mpx/i-youki-theme-engine.hh"

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
        typedef std::map<gint64, ModelArtist_t::iterator>   IdIterMapArtist_t ;

        typedef sigc::signal<void>                          SignalArtist_0 ;
        typedef sigc::signal<void, std::size_t, bool>       SignalArtist_1 ;

        struct DataModelArtist : public sigc::trackable
        {
                ModelArtist_SP_t        m_realmodel ;
                IdIterMapArtist_t       m_iter_map ;
                std::size_t             m_position ;
                boost::optional<gint64> m_selected ;

                SignalArtist_0          m_select ;
                SignalArtist_1          m_changed ;

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
                    m_changed.emit( m_position, true ) ;
                } 

                virtual SignalArtist_1&
                signal_changed ()
                {
                    return m_changed ;
                }

                virtual SignalArtist_0&
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
                    m_changed.emit( m_position, true ) ;
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
                    }

                    if( new_mapping != m_mapping )
                    {
                        Row2 & row = *(m_realmodel->begin()) ;

                        long long int sz = new_mapping.size() - 1 ;
    
                        get<0>(row) = (boost::format(_("All %lld %s")) % sz % ((sz > 1) ? _("Artists") : _("Artist"))).str() ;

                        std::swap(m_mapping, new_mapping);
                        m_changed.emit( m_position, m_mapping.size() != new_mapping.size() );
                        m_select.emit() ;
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
                      Cairo::RefPtr<Cairo::Context>     cairo
                    , const Row2&                       datarow
                    , Gtk::Widget&                      widget
                    , int                               row
                    , int                               xpos
                    , int                               ypos
                    , int                               rowheight
                    , bool                              selected
                    , const ThemeColor&                 color
                )
                {
                    using boost::get;

                    cairo->set_operator(Cairo::OPERATOR_ATOP);

                    cairo->set_source_rgba(
                          color.r
                        , color.g
                        , color.b
                        , color.a
                    ) ;

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

                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                sigc::connection                    m_search_changed_conn ;
                bool                                m_search_active ;
                int                                 m_search_idx ;

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
                    std::size_t up = get_upper_row() ;

                    Interval<std::size_t> i (
                          Interval<std::size_t>::IN_IN
                        , up 
                        , up + (m_visible_height/m_row_height)
                    ) ;
            
                    return i.in( row ) ;
                }

            protected:

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    if( event->is_modifier )
                        return false ;

                    if( m_search_active )
                    {
                        switch( event->keyval )
                        {
                            case GDK_Up:
                            case GDK_KP_Up:
                                m_search_idx = Limiter<int> (
                                      Limiter<int>::ABS_ABS
                                    , 0
                                    , std::numeric_limits<int>::max() 
                                    , m_search_idx - 1 
                                ) ;
                                on_search_entry_changed() ;
                                return true ;

                            case GDK_Down:
                            case GDK_KP_Down:
                                m_search_idx = Limiter<int> (
                                      Limiter<int>::ABS_ABS
                                    , 0
                                    , std::numeric_limits<int>::max() 
                                    , m_search_idx + 1 
                                ) ;
                                on_search_entry_changed() ;
                                return true ;

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

                    Limiter<int64_t> row ;
                    Interval<std::size_t> i ;
                    int64_t origin = boost::get<2>(m_selection.get()) ; 

                    switch( event->keyval )
                    {
                        case GDK_Up:
                        case GDK_KP_Up:
                        case GDK_Page_Up:

                            if( !origin )
                                break ;

                            if( !m_selection ) 
                            {
                                select_row( get_upper_row() ) ;
                                break ;
                            }

                            if( event->keyval == GDK_Page_Up )
                            {
                                row = Limiter<int64_t>( 
                                      Limiter<int64_t>::ABS_ABS
                                    , 0
                                    , m_model->size() - 1 
                                    , origin - (m_visible_height/m_row_height)
                                ) ;

                                g_message("Row: %lld", int64_t(row)) ;

                                select_row( row ) ;
                            }
                            else
                            {
                                row = Limiter<int64_t> ( 
                                      Limiter<int64_t>::ABS_ABS
                                    , 0
                                    , m_model->size() - 1 
                                    , origin - 1
                                ) ;
                                select_row( row ) ;
                            }

                            i = Interval<std::size_t> (
                                  Interval<std::size_t>::IN_EX
                                , 0
                                , get_upper_row()
                            ) ;

                            if( i.in( row )) 
                            {
                                m_prop_vadj.get_value()->set_value( row * m_row_height ) ; 
                            }

                            return true;

                        case GDK_Down:
                        case GDK_KP_Down:
                        case GDK_Page_Down:

                            if( origin == (m_model->size() - 1))
                                break ;

                            if( !m_selection ) 
                            {
                                select_row( get_upper_row() ) ;
                                break ;
                            }

                            if( event->keyval == GDK_Page_Down )
                            {
                                row = Limiter<int64_t> ( 
                                      Limiter<int64_t>::ABS_ABS
                                    , 0 
                                    , m_model->size() - 1 
                                    , origin + (m_visible_height/m_row_height) 
                                ) ;
                                select_row( row ) ;
                            }
                            else
                            {
                                row = Limiter<int64_t> ( 
                                      Limiter<int64_t>::ABS_ABS
                                    , 0 
                                    , m_model->size() - 1
                                    , origin + 1
                                ) ;
                                select_row( row ) ;
                            }

                            i = Interval<std::size_t> (
                                  Interval<std::size_t>::EX_EX
                                , get_upper_row() + (m_visible_height/m_row_height) 
                                , m_model->size() 
                            ) ;

                            if( i.in( row )) 
                            {
                                m_prop_vadj.get_value()->set_value( row * m_row_height ) ; 
                            }

                            return true ;

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
                        case GDK_Escape:
                        case GDK_Tab:
                            return false ;

                        default:

                            if( !Gtk::DrawingArea::on_key_press_event( event ))
                            { 
                                if( (event->state & GDK_CONTROL_MASK)
                                                ||
                                    (event->state & GDK_MOD1_MASK)
                                                ||
                                    (event->state & GDK_SUPER_MASK)
                                                ||
                                    (event->state & GDK_HYPER_MASK)
                                                ||
                                    (event->state & GDK_META_MASK)
                                )
                                {
                                    return false ;
                                }

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

                            std::size_t row = 0 ; 

                            if( event->y > m_row_height )
                            {
                                std::size_t upper = get_upper_row() ;
                                row = (upper + (event->y / m_row_height)) - ((upper > 0) ? 1 : 0)  ;
                            }

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

                    if( m_row_height )
                    {
                        m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height + m_row_height ) ;
                        m_prop_vadj.get_value()->set_page_size( (m_visible_height/m_row_height)*m_row_height ) ; 
                    }

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
                    const Gtk::Allocation& a = get_allocation();

                    boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context(); 

                    const ThemeColor& c_base            = theme->get_color( THEME_COLOR_BASE ) ;
                    const ThemeColor& c_base_rules_hint = theme->get_color( THEME_COLOR_BASE_ALTERNATE ) ;
                    const ThemeColor& c_text            = theme->get_color( THEME_COLOR_TEXT ) ;
                    const ThemeColor& c_text_sel        = theme->get_color( THEME_COLOR_TEXT_SELECTED ) ;
                    const ThemeColor& c_sel             = theme->get_color( THEME_COLOR_SELECT ) ;

                    cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
                    RoundedRectangle(
                          cairo
                        , 0
                        , 0
                        , a.get_width()
                        , a.get_height()
                        , 4.
                    ) ;
                    cairo->set_source_rgba(
                          c_base.r
                        , c_base.g
                        , c_base.b
                        , c_base.a
                    ) ;
                    cairo->fill() ;

                    const std::size_t inner_pad  = 1 ;

                    std::size_t row_origin  = std::max<std::size_t>( get_upper_row() , 1 ) ;

                    std::size_t cnt         = Limiter<std::size_t>(
                                                  Limiter<std::size_t>::ABS_ABS
                                                , 0
                                                , m_model->size()
                                                , m_visible_height/m_row_height
                                              ) ;

                    std::size_t ypos        = 0 ;
                    std::size_t xpos        = 0 ;

                    std::vector<std::size_t> render_rows ;
                    render_rows.resize( cnt ) ;
                    render_rows.push_back( 0 ) ;

                    for( std::size_t n = 0 ; n < cnt ; ++n ) 
                    {
                        render_rows[n+1] = n + row_origin ;
                    }

                    cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
    
                    for( std::vector<std::size_t>::const_iterator i = render_rows.begin(); i != render_rows.end(), *i < m_model->size() ; ++i )
                    {
                        std::size_t row = *i ;

                        xpos = 0 ;

                        bool iter_is_selected = ( m_selection && boost::get<1>(m_selection.get()) == get<1>(*m_model->m_mapping[row])) ;

                        if( !(row % 2) && row > 0 ) 
                        {
                            GdkRectangle r ;

                            r.x       = inner_pad ;
                            r.y       = inner_pad + ypos ;
                            r.width   = a.get_width() - 2 * inner_pad ; 
                            r.height  = m_row_height  - 2 * inner_pad ;

                            RoundedRectangle(
                                  cairo
                                , r.x
                                , r.y
                                , r.width
                                , r.height
                                , rounding_aa
                            ) ;

                            cairo->set_source_rgba(
                                  c_base_rules_hint.r 
                                , c_base_rules_hint.g 
                                , c_base_rules_hint.b 
                                , c_base_rules_hint.a 
                            ) ;

                            cairo->fill() ;
                        }

                        if( iter_is_selected ) 
                        {
                            GdkRectangle r ;

                            r.x         = inner_pad ;
                            r.y         = inner_pad + ypos ;
                            r.width     = a.get_width() - 2*inner_pad ;  
                            r.height    = m_row_height  - 2*inner_pad ;
        
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
                                , c_sel.r 
                                , c_sel.g 
                                , c_sel.b 
                                , 0.90 * factor  
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  .40
                                , c_sel.r 
                                , c_sel.g 
                                , c_sel.b 
                                , 0.75 * factor
                            ) ;
                            
                            background_gradient_ptr->add_color_stop_rgba(
                                  1. 
                                , c_sel.r 
                                , c_sel.g 
                                , c_sel.b 
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
                                  c_sel.r
                                , c_sel.g
                                , c_sel.b
                            ) ;

                            cairo->set_line_width( 0.8 ) ;
                            cairo->stroke () ;

                            cairo->restore () ;
                        }

                        for( ColumnArtist_SP_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i )
                        {
                            (*i)->render(
                                  cairo
                                , m_model->row(row)
                                , *this
                                , row
                                , xpos
                                , ypos
                                , m_row_height
                                , iter_is_selected
                                , iter_is_selected ? c_text_sel : c_text
                            ) ;

                            xpos += (*i)->get_width();
                        }

                        ypos += m_row_height;
                        row ++;
                        cnt --;
                    }

                    return true;
                }

                void
                on_model_changed(
                      std::size_t       position
                    , bool              size_changed
                )
                {
                    if( size_changed && m_prop_vadj.get_value() && m_visible_height && m_row_height )
                    {
                        std::size_t view_count = m_visible_height / m_row_height ;

                        m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height + m_row_height ) ;
                        m_prop_vadj.get_value()->set_page_size( (m_visible_height/m_row_height)*m_row_height ) ; 

                        if( m_model->size() < view_count )
                            m_prop_vadj.get_value()->set_value(0.);
                        else
                            m_prop_vadj.get_value()->set_value( position * m_row_height ) ;
                    }

                    queue_draw() ;
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
                    if( m_model->m_mapping.size() && (!m_selection || boost::get<2>(m_selection.get()) != 0) ) 
                    {
                        select_row( 0 ) ;
                        return ;
                    }

                    m_model->set_selected( boost::optional<gint64>() ) ;
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
                            &ListViewArtist::clear_selection
                    ));

                    on_model_changed( 0, true ) ;
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

                    Glib::ustring text = m_SearchEntry->get_text().casefold() ;

                    if( text.empty() )
                    {
                        select_row( 0 ) ;
                        return ;
                    }

                    DataModelFilterArtist::RowRowMapping::iterator i = m_model->m_mapping.begin(); 
                    ++i ; // first row is "All" FIXME this sucks

                    int idx = m_search_idx ;

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        const Row2& row = **i ;

                        Glib::ustring match = Glib::ustring(get<0>(row)).casefold() ;

                        if( match.substr( 0, std::min( text.length(), match.length())) == text.substr( 0, std::min( text.length(), match.length())) )   
                        {
                            if( idx <= 0 )
                            {
                                std::size_t row = std::distance( m_model->m_mapping.begin(), i ) ; 
                                m_prop_vadj.get_value()->set_value( row * m_row_height ) ; 
                                select_row( row ) ;
                                break ;
                            }
                            else
                            {
                                --idx ;
                            }
                        }
                    }
                }

                bool
                on_search_window_focus_out(
                      GdkEventFocus* G_GNUC_UNUSED
                )
                {
                    cancel_search() ;
                    return false ;
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
                    m_search_idx = 0 ;
                }

            protected:

                virtual void
                on_realize()
                {
                    Gtk::DrawingArea::on_realize() ;
                    initialize_metrics();
                }

            public:

                ListViewArtist ()

                        : ObjectBase( "YoukiListViewArtist" )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_highlight( false )
                        , m_fixed_total_width( 0 )
                        , m_search_active( false )
                        , m_search_idx( 0 )

                {
                    boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
                    const ThemeColor& c = theme->get_color( THEME_COLOR_BASE ) ;
                    Gdk::Color cgdk ;
                    cgdk.set_rgb_p( c.r, c.g, c.b ) ; 
                    modify_bg( Gtk::STATE_NORMAL, cgdk ) ;
                    modify_base( Gtk::STATE_NORMAL, cgdk ) ;

                    set_flags(Gtk::CAN_FOCUS);
                    add_events(Gdk::EventMask(GDK_KEY_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK ));

                    ((GtkWidgetClass*)(G_OBJECT_GET_CLASS(G_OBJECT(gobj()))))->set_scroll_adjustments_signal = 
                            g_signal_new ("set_scroll_adjustments",
                                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                      GSignalFlags (G_SIGNAL_RUN_FIRST),
                                      0,
                                      NULL, NULL,
                                      g_cclosure_user_marshal_VOID__OBJECT_OBJECT, G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

                    g_signal_connect(G_OBJECT(gobj()), "set_scroll_adjustments", G_CALLBACK(list_view_set_adjustments), this);

                    m_SearchEntry = Gtk::manage( new Gtk::Entry ) ;
                    m_search_changed_conn = m_SearchEntry->signal_changed().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewArtist::on_search_entry_changed
                    )) ;
    
                    m_SearchWindow = new Gtk::Window( Gtk::WINDOW_POPUP ) ;
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
                }

                virtual ~ListViewArtist ()
                {
                }
        };
}

#endif
