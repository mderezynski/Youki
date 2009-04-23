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
#include "mpx/algorithm/limiter.hh"
#include "mpx/algorithm/interval.hh"

#include "mpx/aux/glibaddons.hh"

#include "mpx/widgets/cairo-extensions.hh"

#include "glib-marshalers.h"

#include "mpx/mpx-main.hh"
#include "mpx/i-youki-theme-engine.hh"

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace
{
    const double rounding_albums = 4. ; 

    enum ReleaseType
    {
        RT_NONE             =   0,
        RT_ALBUM            =   1 << 0,
        RT_SINGLE           =   1 << 1,
        RT_COMPILATION      =   1 << 2,
        RT_EP               =   1 << 3,
        RT_LIVE             =   1 << 4,
        RT_REMIX            =   1 << 5,
        RT_SOUNDTRACK       =   1 << 6,
        RT_OTHER            =   1 << 7,
        RT_ALL              =   (RT_ALBUM|RT_SINGLE|RT_COMPILATION|RT_EP|RT_LIVE|RT_REMIX|RT_SOUNDTRACK|RT_OTHER)
    };

    ReleaseType
    get_rt(
          const std::string& type
    )
    {
        if( type == "album" )
                return RT_ALBUM;

        if( type == "single" )
                return RT_SINGLE;

        if( type == "compilation" )
                return RT_COMPILATION;

        if( type == "ep" )
                return RT_EP;

        if( type == "live" )
                return RT_LIVE;

        if( type == "remix" )
                return RT_REMIX;

        if( type == "soundtrack" )
                return RT_SOUNDTRACK;

        return RT_OTHER;
    }
}

namespace MPX
{
        typedef boost::tuple<Cairo::RefPtr<Cairo::ImageSurface>, gint64, gint64, std::string, std::string, std::string, ReleaseType, std::string>  RowAlbum ;

        typedef std::vector<RowAlbum>                                                                       ModelAlbums_t ;
        typedef boost::shared_ptr<ModelAlbums_t>                                                            ModelAlbums_SP_t ;
        typedef std::map<gint64, ModelAlbums_t::iterator>                                                   IdIterMapAlbums_t ;

        typedef sigc::signal<void>                                                                          SignalAlbums_0 ;
        typedef sigc::signal<void, std::size_t, bool>                                                       SignalAlbums_1 ;

        struct DataModelAlbums : public sigc::trackable
        {
                ModelAlbums_SP_t        m_realmodel ;
                IdIterMapAlbums_t       m_iter_map ;
                std::size_t             m_position ;
                boost::optional<gint64> m_selected ;

                SignalAlbums_0          m_select ;
                SignalAlbums_1          m_changed ;

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
                    m_changed.emit( 0, true ) ;
                } 

                virtual SignalAlbums_1&
                signal_changed ()
                {
                    return m_changed ;
                }

                virtual SignalAlbums_0&
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

                virtual RowAlbum&
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
                    , const std::string&                    mbid
                    , const std::string&                    type
                    , const std::string&                    year
                )
                {
                    RowAlbum row ( surface, id_album, id_artist, album, album_artist, mbid, get_rt( type ), year ) ; 
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
                    m_changed.emit( 0, true ) ;
                } 

                virtual std::size_t 
                size ()
                {
                    return m_mapping.size();
                }

                virtual RowAlbum&
                row (std::size_t row)
                {
                    return *(m_mapping[row]);
                }

                void
                swap( std::size_t p1, std::size_t p2 )
                {
                    std::swap( m_mapping[p1], m_mapping[p2] ) ;
                    m_changed.emit( m_position, false ) ;
                }

                virtual void
                append_album(
                      Cairo::RefPtr<Cairo::ImageSurface>    surface
                    , gint64                                id_album
                    , gint64                                id_artist
                    , const std::string&                    album
                    , const std::string&                    album_artist
                    , const std::string&                    mbid
                    , const std::string&                    type
                    , const std::string&                    year
                )
                {
                    DataModelAlbums::append_album(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                        , mbid
                        , type
                        , year
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
                    , const std::string&                        mbid
                    , const std::string&                        type
                    , const std::string&                        year
                )
                {
                    DataModelAlbums::append_album(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                        , mbid
                        , type
                        , year
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

                    boost::optional<gint64> id_cur = ( m_position < m_mapping.size()) ? get<1>(row( m_position )) : boost::optional<gint64>() ; 
                    boost::optional<gint64> id_sel = m_selected ; 

                    m_position = 0 ;
    
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
                       }

                    if( new_mapping != m_mapping )
                    {
                        RowAlbum & row = *(m_realmodel->begin()) ;

                        long long int sz = new_mapping.size() - 1 ;
    
                        get<4>(row) = (boost::format(_("All %lld %s")) % sz % ((sz > 1) ? _("Albums") : _("Album"))).str() ;

                        std::swap( new_mapping, m_mapping ) ;
                        m_changed.emit( m_position, new_mapping.size() != m_mapping.size() ) ;
                        m_select.emit() ;
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
                      Cairo::RefPtr<Cairo::Context>&        cairo
                    , Cairo::RefPtr<Cairo::ImageSurface>&   s_fallback
                    , const RowAlbum&                       data_row
                    , Gtk::Widget&                          widget
                    , int                                   row
                    , int                                   xpos
                    , int                                   ypos
                    , int                                   row_height
                    , bool                                  selected
                    , const ThemeColor&                     color
                )
                {
                    using boost::get;

                    GdkRectangle r ;
                    r.y = ypos ; 

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    Cairo::RefPtr<Cairo::ImageSurface> s = get<0>(data_row) ;

                    int off = 0 ;

                    if( row > 0 ) 
                    {
                            off = 64 ; 

                            r.x = xpos + m_width - off - 6 ; 

                            cairo->set_source(
                                  s ? s : s_fallback
                                , r.x
                                , ypos + 2 
                            ) ; 

                            RoundedRectangle(
                                  cairo
                                , r.x
                                , ypos + 2 
                                , 64 
                                , 64 
                                , 4.
                            ) ;

                            cairo->fill() ;

                            ReleaseType rt = get<6>(data_row) ;

                            if( rt == RT_SINGLE )
                            {
                                cairo->save() ;

                                cairo->rectangle(
                                      r.x
                                    , ypos+2+54
                                    , 64 
                                    , 10
                                ) ;

                                cairo->clip() ;

                                cairo->set_source_rgba(
                                      0. 
                                    , 0.
                                    , 0.
                                    , 0.6 
                                ) ; 

                                RoundedRectangle(
                                      cairo
                                    , r.x
                                    , ypos+2+50 
                                    , 64 
                                    , 14 
                                    , 3.
                                ) ;

                                cairo->fill() ;

                                cairo->restore() ;

                                const int text_size_px = 6 ; 
                                const int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;

                                int width, height;

                                Pango::FontDescription font_desc =  widget.get_style()->get_font() ;
                                font_desc.set_size( text_size_pt * PANGO_SCALE ) ;
                                font_desc.set_weight( Pango::WEIGHT_BOLD ) ;

                                Glib::RefPtr<Pango::Layout> layout = Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) ;
                                layout->set_font_description( font_desc ) ;
                                layout->set_ellipsize( Pango::ELLIPSIZE_MIDDLE ) ;
                                layout->set_width( 64 * PANGO_SCALE ) ;
                                layout->set_text( _("SINGLE")) ; 
                                layout->get_pixel_size (width, height) ;
                                cairo->move_to(
                                      r.x + ((64 - width)/2)
                                    , ypos+2+54+((10 - height)/2)
                                ) ;
                                cairo->set_source_rgba(
                                      1. 
                                    , 1. 
                                    , 1. 
                                    , 1. 
                                ) ;
                                pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
                            }

                            if( s )
                            {
                                cairo->set_source_rgba(
                                      .2 
                                    , .2 
                                    , .2 
                                    , 1.
                                ) ; 

                                RoundedRectangle(
                                      cairo
                                    , r.x
                                    , ypos + 2 
                                    , 64 
                                    , 64 
                                    , 4.
                                ) ;

                                cairo->set_line_width( 0.5 ) ;
                                cairo->stroke() ;
                            }
                    }
                
                    cairo->save() ;

                    const int text_size_px = 9 ; 
                    const int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;

                    int width, height;

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    Pango::FontDescription font_desc =  widget.get_style()->get_font() ;
                    font_desc.set_size( text_size_pt * PANGO_SCALE ) ;
                    font_desc.set_weight( Pango::WEIGHT_BOLD ) ;

                    Glib::RefPtr<Pango::Layout> layout = Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) ;
                    layout->set_font_description( font_desc ) ;
                    layout->set_ellipsize( Pango::ELLIPSIZE_MIDDLE ) ;
                    layout->set_width( (m_width-off-20) * PANGO_SCALE ) ;

                    if( row > 0 )
                    {
                            //// ARTIST
                            layout->set_text( get<4>(data_row) )  ;
                            layout->get_pixel_size (width, height) ;
                            cairo->move_to(
                                  xpos + 8 
                                , ypos + 2
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , .6
                            ) ;
                            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;

                            //// ALBUM
                            layout->set_text( get<3>(data_row) )  ;
                            layout->get_pixel_size (width, height) ;
                            cairo->move_to(
                                  xpos + 8 
                                , ypos + 16 
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , .8
                            ) ;
                            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;

                            //// YEAR
                            layout->set_text( get<7>(data_row) )  ;
                            layout->get_pixel_size (width, height) ;
                            cairo->move_to(
                                  xpos + 8 
                                , r.y + row_height - height - 14
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , .4
                            ) ;
                            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
                    }
                    else
                    {
                            //// ARTIST
                            Pango::FontDescription desc = layout->get_font_description() ;
                            desc.set_weight( Pango::WEIGHT_BOLD ) ;
                            layout->set_font_description( desc ) ;
                            layout->set_text( get<4>(data_row) )  ;
                            layout->get_pixel_size (width, height) ;
                            cairo->move_to(
                                  xpos + (m_width - width) / 2
                                , r.y + (row_height - height) / 2
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , 1.
                            ) ;
                            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
                    }

                    cairo->restore() ;
                }
        };

        typedef boost::shared_ptr<ColumnAlbums>                                                 ColumnAlbums_SP_t ;
        typedef std::vector<ColumnAlbums_SP_t>                                                  ColumnAlbums_SP_t_vector_t ;

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

                boost::optional<boost::tuple<ModelAlbums_t::iterator, gint64, std::size_t> >  m_selection ; 

                SignalSelectionChanged              m_SIGNAL_selection_changed ;

                Interval<std::size_t>               m_Model_I ;

                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                sigc::connection                    m_search_changed_conn ;
                bool                                m_search_active ;
                int                                 m_search_idx ;

                Cairo::RefPtr<Cairo::ImageSurface>  m_disc ;

                void
                initialize_metrics ()
                {
                   m_row_height = 78 ; 
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

                std::size_t
                get_upper_row ()
                {
                    return double(m_prop_vadj.get_value()->get_value()) / double(m_row_height) ;
                }

                bool
                get_row_is_visible(
                      std::size_t row
                )
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

                    int step; 

                    Limiter<int64_t> row ;
                    Interval<std::size_t> i ;

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
                                step = - (m_visible_height / m_row_height) + 1 ; 
                            }
                            else
                            {
                                step = - 1 ;
                            }

                            if( !m_selection ) 
                            {
                                mark_first_row_up:
                                select_row( get_upper_row() ) ;
                            }
                            else
                            {
                                if( get_row_is_visible( get<2>(m_selection.get()) ))
                                {
                                    row = Limiter<int64_t> (
                                          Limiter<int64_t>::ABS_ABS
                                        , 0
                                        , m_model->size()
                                        , get<2>(m_selection.get()) + step
                                    ) ;

                                    select_row( row ) ;

                                    Interval<std::size_t> i (
                                          Interval<std::size_t>::IN_EX
                                        , 0
                                        , get_upper_row()
                                    ) ;

                                    if( i.in( row )) 
                                    {
                                        m_prop_vadj.get_value()->set_value( row * m_row_height ) ;
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
                                step = (m_visible_height / m_row_height) - 1 ;  
                            }
                            else
                            {
                                step = 1 ;
                            }

                            if( !m_selection ) 
                            {
                                mark_first_row_down:
                                select_row( get_upper_row() ) ;
                            }
                            else
                            {
                                if( get_row_is_visible( get<2>(m_selection.get()) ))
                                {
                                    row = Limiter<int64_t> (
                                          Limiter<int64_t>::ABS_ABS
                                        , 0
                                        , m_model->size()
                                        , get<2>(m_selection.get()) + step
                                    ) ;

                                    select_row( row ) ;

                                    Interval<std::size_t> i (
                                          Interval<std::size_t>::IN_EX
                                        , get_upper_row() + (m_visible_height/m_row_height)
                                        , m_model->size() 
                                    ) ;

                                    if( i.in( row )) 
                                    {
                                        m_prop_vadj.get_value()->set_value( (get_upper_row()+1) * m_row_height ) ;
                                    }
                                }
                                else
                                {
                                    goto mark_first_row_down;
                                }
                            }
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

                    return false;
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

                    if( event->type == GDK_BUTTON_PRESS )
                    {
                        grab_focus() ;

                        int row = double(m_prop_vadj.get_value()->get_value()) / double(m_row_height) ; 
                        int off = m_row_height - (m_prop_vadj.get_value()->get_value() - (row*m_row_height)) ;

                        if( event->y > off )
                        {
                            int row2 = row + (event->y + (off ? (m_row_height-off) : 0)) / m_row_height ; 
                            if( m_Model_I.in( row2 )) 
                            {
                                if( row2 >= (row + m_visible_height/m_row_height))
                                {
                                }
                                select_row( row2 ) ;
                            }
                        }
                        else
                        {
                            if( m_Model_I.in( row )) 
                            {
                                m_prop_vadj.get_value()->set_value( row * m_row_height ) ;
                                select_row( row ) ;
                            }
                        }

                        queue_draw() ;
                        m_SIGNAL_selection_changed.emit() ;
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

                    if( m_row_height )
                    {
                        m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;
                        m_prop_vadj.get_value()->set_page_size( m_visible_height ) ;
                        m_prop_vadj.get_value()->set_step_increment( m_row_height ) ; 
                    }

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
                    const Gtk::Allocation& a = get_allocation() ;

                    boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;

                    const ThemeColor& c_text        = theme->get_color( THEME_COLOR_TEXT ) ;
                    const ThemeColor& c_text_sel    = theme->get_color( THEME_COLOR_TEXT_SELECTED ) ;
                    const ThemeColor& c_treelines   = theme->get_color( THEME_COLOR_TREELINES ) ;

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    std::size_t row     = m_prop_vadj.get_value()->get_value() / m_row_height ;
                    int offset          = m_prop_vadj.get_value()->get_value() - (row*m_row_height) ;
                    int ypos            = 0 ;
                    std::size_t xpos    = 0 ;
                    std::size_t cnt     = m_visible_height / m_row_height + 1 ;
            
                    if( row && offset ) 
                    {
//                        ypos = -(m_row_height-offset) ;
                        ypos -= offset ;
                    }

                    cairo->rectangle(
                          0 
                        , 0 
                        , a.get_width()
                        , a.get_height()
                    ) ;
                    cairo->clip() ;

                    const std::size_t inner_pad = 1 ;

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    while( m_model->is_set() && cnt && m_Model_I.in( row )) 
                    {
                        xpos = 0 ;

                        ModelAlbums_t::iterator  selected           = m_model->m_mapping[row] ;
                        bool                     iter_is_selected   = ( m_selection && get<2>(m_selection.get()) == row ) ;

                        if( iter_is_selected )
                        {
                            GdkRectangle r ;

                            r.x         = inner_pad + 2 ; 
                            r.y         = inner_pad + ypos ;
                            r.width     = a.get_width() - 2*inner_pad  - 4 ;  
                            r.height    = m_row_height - 2*inner_pad - 2 ;

                            theme->draw_selection_rectangle(
                                  cairo
                                , r
                                , has_focus()
                            ) ;
                        }

                        for( ColumnAlbums_SP_t_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i )
                        {
                            (*i)->render(
                                    cairo
                                  , m_disc
                                  , m_model->row(row)
                                  , *this
                                  , row
                                  , xpos
                                  , ypos + 4
                                  , m_row_height
                                  , iter_is_selected
                                  , iter_is_selected ? c_text_sel : c_text
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
                              c_treelines.r 
                            , c_treelines.g
                            , c_treelines.b
                            , c_treelines.a 
                        ) ;

                        cairo->move_to(
                              0
                            , ypos - 1 
                        ) ; 

                        cairo->line_to(
                              a.get_width()
                            , ypos - 1 
                        ) ;

                        cairo->stroke() ;
                        cairo->restore(); 

                        row ++;
                        cnt --;
                    }

                    return true;
                }

                void
                on_model_changed(
                      std::size_t   position
                    , bool          size_changed
                )
                {
                    if( size_changed && m_prop_vadj.get_value() && m_visible_height && m_row_height )
                    {
                            std::size_t view_count = m_visible_height / m_row_height ;

                            m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;
                            m_prop_vadj.get_value()->set_page_size( m_visible_height ) ;
                            m_prop_vadj.get_value()->set_step_increment( m_row_height ) ; 

                            if( m_model->size() < view_count )
                                m_prop_vadj.get_value()->set_value(0.) ;
                            else
                                m_prop_vadj.get_value()->set_value( position * m_row_height ) ;

                            m_Model_I = Interval<std::size_t>(
                                  Interval<std::size_t>::IN_EX
                                , 0
                                , m_model->size()
                            ) ;
                    }

                    clear_selection() ;
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
                    if( m_Model_I.in( row )) 
                    {
                        const gint64& id = get<1>(*m_model->m_mapping[row]) ;

                        m_selection = boost::make_tuple( m_model->m_mapping[row], id, row ) ;
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
                            &ListViewAlbums::clear_selection
                    ));

                    clear_selection() ;
                    queue_resize() ;
                    on_model_changed( 0, true ) ;
                }

                void
                append_column (ColumnAlbums_SP_t column)
                {
                    m_columns.push_back(column);
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

                    DataModelFilterAlbums::RowRowMapping::iterator i = m_model->m_mapping.begin(); 
                    ++i ; // first row is "All" FIXME this sucks

                    int idx = m_search_idx ;

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        const RowAlbum& row = **i ;
                        Glib::ustring match = Glib::ustring(get<3>(row)).casefold() ;

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

                ListViewAlbums ()

                        : ObjectBase( "YoukiListViewAlbums" )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_search_active( false )
                        , m_search_idx( 0 )

                {
                    m_disc = Util::cairo_image_surface_from_pixbuf( Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "disc.png" ))) ;

                    boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
                    const ThemeColor& c = theme->get_color( THEME_COLOR_BASE ) ;
                    Gdk::Color cgdk ;
                    cgdk.set_rgb_p( c.r, c.g, c.b ) ; 
                    modify_bg( Gtk::STATE_NORMAL, cgdk ) ;
                    modify_base( Gtk::STATE_NORMAL, cgdk ) ;

                    set_flags(Gtk::CAN_FOCUS);

                    add_events(Gdk::EventMask(GDK_KEY_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK ));

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
                                , &ListViewAlbums::on_search_entry_changed
                    )) ;
    
                    m_SearchWindow = new Gtk::Window( Gtk::WINDOW_POPUP ) ;
                    m_SearchWindow->set_decorated( false ) ;

                    m_SearchWindow->signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewAlbums::on_search_window_focus_out
                    )) ;

                    signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewAlbums::on_search_window_focus_out
                    )) ;

                    m_SearchWindow->add( *m_SearchEntry ) ;
                    m_SearchEntry->show() ;
               }

                virtual ~ListViewAlbums ()
                {
                }
        };
}

#endif // _YOUKI_ALBUM_LIST_HH
