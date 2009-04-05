#ifndef _YOUKI_ALBUM_LIST_HH
#define _YOUKI_ALBUM_LIST_HH

#include <gtkmm.h>
#include <gtk/gtktreeview.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>

#include "mpx/util-string.hh"
#include "mpx/util-graphics.hh"

#include "mpx/mpx-types.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-covers.hh"

#include "mpx/algorithm/aque.hh"
#include "mpx/algorithm/ntree.hh"

#include "mpx/aux/glibaddons.hh"

#include "mpx/widgets/cairo-extensions.hh"

#include "glib-marshalers.h"

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace
{
    const double rounding_albums = 4. ; 
}

namespace MPX
{
        typedef boost::tuple<Cairo::RefPtr<Cairo::ImageSurface>, gint64, gint64, std::string, std::string>  Row5 ;
        typedef std::vector<Row5>                                                                           ModelAlbums_t ;
        typedef boost::shared_ptr<ModelAlbums_t>                                                            ModelAlbums_SP_t ;
        typedef sigc::signal<void, std::size_t>                                                             SignalAlbums_1 ;
        typedef std::map<gint64, ModelAlbums_t::iterator>                                                   IdIterMapAlbums_t ;

        struct DataModelAlbums : public sigc::trackable
        {
                ModelAlbums_SP_t        m_realmodel ;
                SignalAlbums_1          m_changed ;
                SignalAlbums_1          m_select ;
                IdIterMapAlbums_t       m_iter_map ;
                std::size_t             m_position ;
                boost::optional<gint64> m_selected ;

                DataModelAlbums()
                : m_position( 0 )
                {
                    m_realmodel = ModelAlbums_SP_t(new ModelAlbums_t); 
                }

                DataModelAlbums(ModelAlbums_SP_t model)
                : m_position( 0 )
                {
                    m_realmodel = model; 
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear () ;
                    m_iter_map.clear() ;
                    m_changed.emit( 0 ) ;
                } 

                virtual SignalAlbums_1&
                signal_changed ()
                {
                    return m_changed ;
                }

                virtual SignalAlbums_1&
                signal_select ()
                {
                    return m_select ;
                }


                virtual bool
                is_set ()
                {
                    return bool(m_realmodel) ;
                }

                virtual std::size_t
                size ()
                {
                    return m_realmodel->size() ;
                }

                virtual Row5&
                row (std::size_t row)
                {
                    return (*m_realmodel)[row] ;
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
                append_album(
                      Cairo::RefPtr<Cairo::ImageSurface>    surface
                    , gint64                                id_album
                    , gint64                                id_artist
                    , const std::string&                    album
                    , const std::string&                    album_artist
                )
                {
                    Row5 row ( surface, id_album, id_artist, album, album_artist ) ; 
                    m_realmodel->push_back( row ) ;

                    ModelAlbums_t::iterator i = m_realmodel->end() ;
                    std::advance( i, -1 ) ;
                    m_iter_map.insert( std::make_pair( id_album, i) ) ; 
                }

                void
                erase_album(
                    gint64 id_album
                )
                {
                    IdIterMapAlbums_t::iterator i = m_iter_map.find( id_album ) ;

                    if( i != m_iter_map.end() )
                    {
                        m_realmodel->erase( i->second );
                        m_iter_map.erase( i );
                    }
                }
        };

        typedef boost::shared_ptr<DataModelAlbums> DataModelAlbums_SP_t;

        struct DataModelFilterAlbums : public DataModelAlbums
        {
            public:

                typedef std::vector<ModelAlbums_t::iterator> RowRowMapping ;

                RowRowMapping                           m_mapping ;

                boost::optional<std::set<gint64> >      m_constraint_id_album ;
                boost::optional<std::set<gint64> >      m_constraint_id_artist ;

            public:

                DataModelFilterAlbums(DataModelAlbums_SP_t & model)
                : DataModelAlbums(model->m_realmodel)
                {
                    regen_mapping() ;
                }

                virtual ~DataModelFilterAlbums()
                {
                }

                virtual void
                set_constraint_albums(
                    const boost::optional<std::set<gint64> >& constraint
                )
                {
                    m_constraint_id_album = constraint ;
                }

                virtual void
                clear_constraint_album(
                )
                {
                    m_constraint_id_album.reset() ;
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
                    m_realmodel->clear () ;
                    m_mapping.clear() ;
                    m_iter_map.clear() ;
                    m_changed.emit( 0 ) ;
                } 

                virtual std::size_t 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row5&
                row (std::size_t row)
                {
                    return *(m_mapping[row]);
                }

                void
                swap( std::size_t p1, std::size_t p2 )
                {
                    std::swap( m_mapping[p1], m_mapping[p2] ) ;
                    m_changed.emit( m_position ) ;
                }

                virtual void
                append_album(
                      Cairo::RefPtr<Cairo::ImageSurface>        surface
                    , gint64                                    id_album
                    , gint64                                    id_artist
                    , const std::string&                        album
                    , const std::string&                        album_artist
                )
                {
                    DataModelAlbums::append_album(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                    ) ;

                    regen_mapping();
                }
                
                virtual void
                append_album_quiet(
                      Cairo::RefPtr<Cairo::ImageSurface>        surface
                    , gint64                                    id_album
                    , gint64                                    id_artist
                    , const std::string&                        album
                    , const std::string&                        album_artist
                )
                {
                    DataModelAlbums::append_album(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                    ) ;
                }

                void
                erase_album(
                      gint64 id_album
                )
                {
                    DataModelAlbums::erase_album( id_album );
                    regen_mapping();
                }

                virtual void
                regen_mapping(
                )
                {
                    using boost::get;

                    if( m_realmodel->size() == 0 )
                    {
                        return ;
                    } 

                    RowRowMapping new_mapping ;

                    if( !m_constraint_id_album && !m_constraint_id_artist )
                    {
                        m_selected.reset() ;
                    }

                    boost::optional<gint64> id_cur = ( m_position < m_mapping.size()) ? get<1>(row( m_position )) : boost::optional<gint64>() ; 
                    boost::optional<gint64> id_sel = m_selected ; 

                    m_position = 0 ;

                    std::size_t new_sel_pos = 0 ;
    
                    ModelAlbums_t::iterator i = m_realmodel->begin() ;
                    new_mapping.push_back( i++ ) ;

                    for( ; i != m_realmodel->end(); ++i )
                    {
                            int truth = 
                                        (!m_constraint_id_album  || m_constraint_id_album.get().count( get<1>(*i)) )
                                                                        &&
                                        (!m_constraint_id_artist || m_constraint_id_artist.get().count( get<2>(*i)) )
                            ; 

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
                        std::swap( new_mapping, m_mapping ) ;
                        m_changed.emit( m_position ) ;
                        m_select.emit( new_sel_pos ) ;
                    }                
                }
        };

        typedef boost::shared_ptr<DataModelFilterAlbums> DataModelFilterAlbums_SP_t;

        class ColumnAlbums
        {
                int m_width ;
                int m_column ;

            public:

                ColumnAlbums(
                )
                    : m_width( 0 )
                    , m_column( 0 )
                {
                }

                ~ColumnAlbums ()
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
                render(
                      Cairo::RefPtr<Cairo::Context>   cairo
                    , const Row5&                     data_row
                    , Gtk::Widget&                    widget
                    , int                             row
                    , int                             xpos
                    , int                             ypos
                    , int                             row_height
                    , bool                            selected
                )
                {
                    using boost::get;

                    GdkRectangle r ;
                    r.y = ypos ; 

                    cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

                    Cairo::RefPtr<Cairo::ImageSurface> s = get<0>(data_row) ;

                    if( row > 0 && s )
                    {
                            r.x         = xpos + (m_width - s->get_width()) / 2 ; 
                            r.y         = ypos ;
                            r.width     = m_width ;
                            r.height    = row_height ;


                            cairo->set_source(
                                  s
                                , r.x
                                , r.y + 3
                            ) ; 

                            RoundedRectangle(
                                  cairo
                                , r.x
                                , r.y + 3
                                , s->get_width() 
                                , s->get_height() 
                                , 4.
                            ) ;

                            cairo->fill() ;
                    }
                
                    cairo->save() ;

                    const int text_size_px = 9 ;
                    const int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;

                    int width, height;

                    cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

                    Pango::FontDescription font_desc( "sans" ) ;
                    font_desc.set_size( text_size_pt * PANGO_SCALE ) ;
                    font_desc.set_weight( Pango::WEIGHT_BOLD ) ;

                    Glib::RefPtr<Pango::Layout> layout = Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) ;
                    layout->set_font_description( font_desc ) ;
                    layout->set_ellipsize( Pango::ELLIPSIZE_MIDDLE ) ;
                    layout->set_width( (m_width-8) * PANGO_SCALE ) ;

                    //// ARTIST

                    layout->set_text( get<4>(data_row) )  ;
                    layout->get_pixel_size (width, height) ;

                    cairo->move_to(
                          xpos + (m_width - width) / 2.
                        , r.y + 71 
                    ) ;
                    cairo->set_source_rgba( 1., 1., 1., .7 ) ;
                    pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;

                    //// ALBUM

                    if( row > 0 )
                    {
                            layout->set_text( get<3>(data_row) )  ;
                            layout->get_pixel_size (width, height) ;
                            cairo->move_to(
                                  xpos + (m_width - width) / 2.
                                , r.y + 84 
                            ) ;
                            cairo->set_source_rgba( 1., 1., 1., .8 ) ;
                            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
                    }

                    cairo->restore() ;
                }
        };

        typedef boost::shared_ptr<ColumnAlbums>                                                 ColumnAlbums_SP_t ;
        typedef std::vector<ColumnAlbums_SP_t>                                                  ColumnAlbums_SP_t_vector_t ;
        typedef boost::optional<boost::tuple<ModelAlbums_t::iterator, std::size_t, gint64> >    SelectionAlbums ;
        typedef sigc::signal<void>                                                              SignalSelectionChanged ;

        class ListViewAlbums : public Gtk::DrawingArea
        {
            public:

                DataModelFilterAlbums_SP_t          m_model ;

            private:

                int                                 m_row_height ;
                int                                 m_visible_height ;

                ColumnAlbums_SP_t_vector_t          m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                guint                               m_signal0 ; 

                SelectionAlbums                     m_selection ;
                SignalSelectionChanged              m_SIGNAL_selection_changed ;

                GtkWidget                         * m_treeview ;

                void
                initialize_metrics ()
                {
                   m_row_height = 106 ; 
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
                    return double(m_prop_vadj.get_value()->get_value()) / double(m_row_height) ;
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
                    grab_focus();
                    return true;
                }

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    int step; 

                    switch( event->keyval )
                    {
                        case GDK_Return:
                        case GDK_KP_Enter:
                        case GDK_ISO_Enter:
                        case GDK_3270_Enter:
                            if( m_selection )
                            {
                            }
                            return true;

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
                                select_row( get_upper_row() ) ;
                            }
                            else
                            {
                                if( get_row_is_visible( get<1>(m_selection.get()) ))
                                {
                                    ModelAlbums_t::iterator i   = get<0>(m_selection.get()) ;
                                    int                     row = get<1>(m_selection.get()) ; 

                                    std::advance( i, step ) ;
                                    row += step;

                                    if( row >= 0 )
                                    {
                                        select_row( row ) ;

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
                                select_row( get_upper_row() ) ;
                            }
                            else
                            {
                                if( get_row_is_visible( get<1>(m_selection.get())) )
                                {
                                    ModelAlbums_t::iterator i   = get<0>(m_selection.get()) ;
                                    std::size_t             row = get<1>(m_selection.get()) ;
    
                                    std::advance( i, step ) ;
                                    row += step;

                                    if( row < m_model->m_mapping.size() )
                                    {
                                        select_row( row ) ;

                                        if( row >= std::size_t(get_upper_row() + (m_visible_height/m_row_height)))
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
                    }

                    return false;
                }

                bool
                on_button_press_event (GdkEventButton * event)
                {
                    using boost::get;

                    if( event->type == GDK_BUTTON_PRESS )
                    {
                        grab_focus() ;

                        std::size_t row = get_upper_row() + (event->y / m_row_height) ;

                        if( row < m_model->size() )
                        {
                            select_row( row ) ;
                            queue_draw() ;
                            m_SIGNAL_selection_changed.emit() ;
                        }
                    }
                
                    return true;
                }

                bool
                on_button_release_event (GdkEventButton * event)
                {
                    return true ;
                }

                bool
                on_leave_notify_event(
                    GdkEventCrossing* G_GNUC_UNUSED
                )
                {
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

                    double n                       = m_columns.size() ; 
                    double column_width_calculated = event->width / n ;

                    for( std::size_t n = 0; n < m_columns.size(); ++n )
                    {
                        m_columns[n]->set_width( column_width_calculated ) ; 
                    }

                    queue_draw() ;

                    return true ;
                }

                bool
                on_expose_event (GdkEventExpose *event)
                {
                    Cairo::RefPtr<Cairo::Context>   cairo = get_window()->create_cairo_context() ;
                    const Gtk::Allocation&          alloc = get_allocation() ;

                    cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

                    std::size_t row     = get_upper_row() ;
                    std::size_t ypos    = 0 ;
                    std::size_t xpos    = 0 ;
                    std::size_t cnt     = m_visible_height / m_row_height ; 

                    const std::size_t inner_pad = 1 ;

                    cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

                    while( m_model->is_set() && cnt && (row < m_model->m_mapping.size()) ) 
                    {
                        xpos = 0 ;

                        ModelAlbums_t::iterator  selected           = m_model->m_mapping[row] ;
                        bool                     iter_is_selected   = ( m_selection && get<1>(m_selection.get()) == row ) ;

                        double factor = has_focus() ? 1. : 0.3 ;

                        if( iter_is_selected )
                        {
                            Gdk::Color c = get_style()->get_base( Gtk::STATE_SELECTED ) ;

                            GdkRectangle r ;

                            r.x         = inner_pad + 2 ; 
                            r.y         = inner_pad + ypos ;
                            r.width     = alloc.get_width() - 2*inner_pad  - 2 ;  
                            r.height    = m_row_height - 2*inner_pad - 2 ;

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
                                , rounding_albums
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

                            iter_is_selected = true;
                        }

                        for( ColumnAlbums_SP_t_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i )
                        {
                            (*i)->render(
                                    cairo
                                  , m_model->row(row)
                                  , *this
                                  , row
                                  , xpos
                                  , ypos + 4
                                  , 112
                                  , iter_is_selected
                            ) ;

                            xpos += (*i)->get_width() ; 
                        }

                        ypos += m_row_height;

                        std::valarray<double> dashes ( 3 ) ;
                        dashes[0] = 0. ;
                        dashes[1] = 3. ;
                        dashes[2] = 0. ;

                        cairo->save(); 
                        cairo->set_line_width(
                              .5
                        ) ;

                        cairo->set_dash(
                              dashes
                            , 0
                        ) ;

                        cairo->set_source_rgba(
                              1.
                            , 1.
                            , 1.
                            , .5
                        ) ;

                        cairo->move_to(
                              0
                            , ypos - 1 
                        ) ; 

                        cairo->line_to(
                              alloc.get_width()
                            , ypos - 1 
                        ) ;

                        cairo->stroke() ;
                        cairo->restore(); 

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
                        );
                    }

                    return true;
                }

                void
                on_model_changed( std::size_t position )
                {
                    std::size_t view_count = m_visible_height / m_row_height ;

                    m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;

                    if( m_model->size() < view_count )
                    {
                        m_prop_vadj.get_value()->set_value(0.) ;
                    } 
                    else
                    {
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

                            ListViewAlbums & view = *(reinterpret_cast<ListViewAlbums*>(data));

                            view.m_prop_vadj.get_value()->set_value(0.);
                            view.m_prop_vadj.get_value()->set_upper( view.m_model->size() * view.m_row_height ) ;
                            view.m_prop_vadj.get_value()->set_page_size( (view.m_visible_height/view.m_row_height)*int(view.m_row_height) ) ; 

                            view.m_prop_vadj.get_value()->signal_value_changed().connect(
                                sigc::mem_fun(
                                    view,
                                    &ListViewAlbums::on_vadj_value_changed
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

                            for( DataModelFilterAlbums::RowRowMapping::iterator i = m_model->m_mapping.begin(); i != m_model->m_mapping.end(); ++i )
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

                        m_selection = boost::make_tuple( m_model->m_mapping[row], row, id ) ;
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
                            const gint64& sel_id = boost::get<2>(m_selection.get()) ;
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
                        if( m_model->m_mapping.size() )
                        {
                            m_selection = boost::make_tuple(m_model->m_mapping[0], 0, get<1>(*m_model->m_mapping[0])) ;
                        }
                    }
                }
    
                void
                set_model(DataModelFilterAlbums_SP_t model)
                {
                    m_model = model;

                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &ListViewAlbums::on_model_changed
                    ));

                    m_model->signal_select().connect(
                        sigc::mem_fun(
                            *this,
                            &ListViewAlbums::select_row
                    ));

                    clear_selection() ;
                }

                void
                append_column (ColumnAlbums_SP_t column)
                {
                    m_columns.push_back(column);
                }

                ListViewAlbums ()

                        : ObjectBase( "YoukiListViewAlbums" )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))

                {
                    Gdk::Color c ;
    
                    c.set_rgb_p( .10, .10, .10 ) ;

                    modify_bg( Gtk::STATE_NORMAL, c ) ;
                    modify_base( Gtk::STATE_NORMAL, c ) ;

                    m_treeview = gtk_tree_view_new();
                    gtk_widget_hide(GTK_WIDGET(m_treeview)) ;
                    gtk_widget_realize(GTK_WIDGET(m_treeview));

                    set_flags(Gtk::CAN_FOCUS);
                    add_events(Gdk::EventMask(GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK ));

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

                    property_can_focus() = true ;
               }

                virtual ~ListViewAlbums ()
                {
                }
        };
}

#endif // _YOUKI_ALBUM_LIST_HH
