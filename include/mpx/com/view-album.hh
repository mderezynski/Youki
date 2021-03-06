#ifndef YOUKI_VIEW_ALBUMS__HH
#define YOUKI_VIEW_ALBUMS__HH

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

#include "mpx/algorithm/limiter.hh"
#include "mpx/algorithm/interval.hh"

#include "mpx/aux/glibaddons.hh"

#include "mpx/widgets/cairo-extensions.hh"

#include "mpx/mpx-main.hh"
#include "mpx/i-youki-theme-engine.hh"

#include "glib-marshalers.h"

#include "mpx/com/indexed-list.hh"

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace
{
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
namespace View
{
namespace Albums
{
        const double rounding = 4. ; 

        // Album

        struct Album
        {
            Cairo::RefPtr<Cairo::ImageSurface>      coverart ;
            gint64                                  album_id ;
            gint64                                  artist_id ;
            std::string                             album ;
            std::string                             album_artist ;
            std::string                             type ;
            std::string                             year ;
            std::string                             mbid ;
            std::string                             mbid_artist ;
            std::string                             label ;
            gint64                                  track_count ;
            gdouble                                 album_playscore ; 
        };

        typedef boost::shared_ptr<Album> Album_sp ;

        typedef IndexedList<Album_sp>                       Model_t ;
        typedef boost::shared_ptr<Model_t>                  Model_sp_t ;
        typedef std::map<gint64, Model_t::iterator>         IdIterMap_t ;
        typedef std::vector<Model_t::iterator>              RowRowMapping_t ;
        typedef sigc::signal<void, std::size_t, bool>       Signal_1 ;

        struct OrderFunc
        : public std::binary_function<Album_sp, Album_sp, bool>
        {
            bool operator() (
                  const Album_sp&  a
                , const Album_sp&  b
            )
            {
                if( a->album_id == -1 ) 
                {
                    return true ;
                }

                if( b->album_id == -1 )
                {     
                    return false ;
                }

                if( a->album_artist < b->album_artist )
                    return true ;

                if( b->album_artist < b->album_artist )
                    return false ;

                if( a->year < b->year )
                    return true ;

                if( b->year < a->year )
                    return false ;

                if( a->album < b->album )
                    return true ;

                if( a->album < b->album )
                    return false ;

                return false ;
            }
        } ;

        struct DataModel
        : public sigc::trackable
        {
                Model_sp_t                      m_realmodel ;
                IdIterMap_t                     m_iter_map ;
                std::size_t                     m_top_row ;
                boost::optional<gint64>         m_selected ;
                Signal_1                        m_changed ;

                DataModel()
                : m_top_row( 0 )
                {
                    m_realmodel = Model_sp_t(new Model_t); 
                }

                DataModel(Model_sp_t model)
                : m_top_row( 0 )
                {
                    m_realmodel = model; 
                }

                virtual void
                clear()
                {
                    m_realmodel->clear () ;
                    m_iter_map.clear() ;
                    m_top_row = 0 ;
                } 

                virtual Signal_1&
                signal_changed ()
                {
                    return m_changed ;
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

                virtual Album_sp
                row(std::size_t row)
                {
                    return (*m_realmodel)[row] ;
                }

                virtual void
                set_current_row(
                    std::size_t row
                )
                {
                    m_top_row = row ;
                }

                virtual void
                set_selected(
                    const boost::optional<gint64>& id = boost::optional<gint64>()
                )
                {
                    m_selected = id ;
                }

                virtual void
                append_album(
                    const Album_sp album
                )
                {
                    m_realmodel->push_back( album ) ;
                    Model_t::iterator i = m_realmodel->end() ;
                    std::advance( i, -1 ) ;
                    m_iter_map.insert( std::make_pair( album->album_id, i )) ; 
                }

                virtual void
                insert_album(
                    const Album_sp album
                )
                {
                    static OrderFunc order ;

                    Model_t::iterator i = m_realmodel->insert(
                          std::lower_bound(
                              m_realmodel->begin()
                            , m_realmodel->end()
                            , album
                            , order
                          )
                        , album
                    ) ;
                }

                void
                erase_album(
                    gint64 id_album
                )
                {
                    IdIterMap_t::iterator i = m_iter_map.find( id_album ) ;

                    if( i != m_iter_map.end() )
                    {
                        m_realmodel->erase( i->second );
                        m_iter_map.erase( i );
                    }
                }

                void
                update_album(
                    const Album_sp album
                )
                {
                    IdIterMap_t::iterator i = m_iter_map.find( album->album_id ) ;
                    *(i->second) = album ;
                    m_changed.emit( m_top_row, false) ;
                }

                void
                update_album_cover(
                      gint64      id
                    , Cairo::RefPtr<Cairo::ImageSurface> is
                )
                {
                    IdIterMap_t::iterator i = m_iter_map.find( id ) ;
                    Album_sp & album = *i->second ;
                    album->coverart = is ;
                    m_changed.emit( m_top_row, false ) ;
                }
        };

        typedef boost::shared_ptr<DataModel> DataModel_sp_t;

        struct DataModelFilter
        : public DataModel
        {
            typedef std::vector<gint64>                     IdVector_t ;
            typedef boost::shared_ptr<IdVector_t>           IdVector_sp ;

            public:

                RowRowMapping_t   m_mapping ;
                IdVector_sp       m_constraints_albums ;
                IdVector_sp       m_constraints_artist ;

            public:

                DataModelFilter(
                      DataModel_sp_t model
                )

                    : DataModel( model->m_realmodel )

                {
                    regen_mapping() ;
                }

                virtual
                ~DataModelFilter()
                {
                }

                virtual void
                set_constraints_albums(
                    IdVector_sp&  constraint
                )
                {
                    if( constraint != m_constraints_albums )
                    {
                        m_constraints_albums = constraint ;
                    }
                }

                virtual void
                clear_constraints_album(
                )
                {
                    m_constraints_albums.reset() ;
                }

                virtual void
                set_constraints_artist(
                    IdVector_sp& constraint
                )
                {
                    if( constraint != m_constraints_artist )
                    {
                        m_constraints_artist = constraint ;
                        regen_mapping() ;
                    }
                }

                virtual void
                clear_constraints_artist(
                )
                {
                    m_constraints_artist.reset() ;
                }

                virtual void
                clear()
                {
                    DataModel::clear() ;
                    m_mapping.clear() ;
                    m_changed.emit( m_top_row, true ) ;
                } 

                virtual std::size_t 
                size()
                {
                    return m_mapping.size();
                }

                virtual Album_sp 
                row(
                      std::size_t   row
                )
                {
                    return *(m_mapping[row]);
                }

                virtual RowRowMapping_t::const_iterator
                row_iter(
                      std::size_t   row
                )
                {
                    RowRowMapping_t::const_iterator i = m_mapping.begin() ;
                    std::advance( i, row ) ;
                    return i ;
                }

                void
                swap(
                      std::size_t   p1
                    , std::size_t   p2
                )
                {
                    std::swap( m_mapping[p1], m_mapping[p2] ) ;
                    m_changed.emit( m_top_row, false ) ;
                }

                virtual void
                append_album(
                    const Album_sp album
                )
                {
                    DataModel::append_album( album ) ;
                }
                
                void
                erase_album(
                      gint64 id_album
                )
                {
                    DataModel::erase_album( id_album );
                }

                virtual void
                insert_album(
                    const Album_sp album
                )
                {
                    DataModel::insert_album(
                        album
                    ) ;
                }

                virtual void
                update_album(
                    const Album_sp album
                )
                {
                    DataModel::update_album(
                        album
                    ) ;

                    regen_mapping() ;
                }

                virtual void
                regen_mapping(
                )
                {
                    using boost::get;

                    if( m_realmodel->empty() )
                    {
                        return ;
                    } 

                    RowRowMapping_t new_mapping ;
                    new_mapping.reserve( m_realmodel->size() ) ;

                    boost::optional<gint64> id_top ;
                    boost::optional<gint64> id_sel ;

                    if( m_top_row < m_mapping.size() )
                    {
                        id_top = (row( m_top_row ))->album_id ;
                    }

                    if( m_selected )
                    {
                        id_sel = m_selected ; 
                    }

                    m_selected.reset() ;
                    m_top_row = 0 ;
    
                    typedef Model_t::iterator Iter ;
                    Iter i = m_realmodel->begin() ;

                    new_mapping.push_back( i++ ) ;

                    IdVector_t * constraints_albums = m_constraints_albums.get() ;
                    IdVector_t * constraints_artist = m_constraints_artist.get() ;

                    for( ; i != m_realmodel->end(); ++i )
                    {
                        int truth = 
                                    (!constraints_albums || ((*constraints_albums)[(*i)->album_id] > 0))
                                                                    &&
                                    (!constraints_artist || ((*constraints_artist)[(*i)->artist_id] > 0))
                        ; 

                        if( truth )
                        {
                            gint64 id_row = (*i)->album_id ;

                            if( id_top && id_row == id_top.get() )
                            {
                                m_top_row = new_mapping.size() ;
                            }

                            if( id_sel && id_row == id_sel.get() )
                            {
                                m_selected = id_sel ; 
                            }

                            new_mapping.push_back( i ) ;
                        }
                    }

                    if( new_mapping != m_mapping )
                    {
                        std::swap( new_mapping, m_mapping ) ;
                        m_changed.emit( m_top_row, m_mapping.size() != new_mapping.size() );
                    }                
                }
        };

        typedef boost::shared_ptr<DataModelFilter> DataModelFilter_sp_t;

        class Column
        {
                int m_width ;
                int m_column ;

            public:

                Column(
                )
                    : m_width( 0 )
                    , m_column( 0 )
                {
                }

                ~Column ()
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
                    , Cairo::RefPtr<Cairo::ImageSurface>&   disc
                    , const Album_sp                        album
                    , Gtk::Widget&                          widget
                    , std::size_t                           row
                    , std::size_t                           xpos
                    , int                                   ypos
                    , std::size_t                           row_height
                    , bool                                  selected
                    , const ThemeColor&                     color
                    , bool                                  album_name_only
                    , std::size_t                           track_count
                    , std::size_t                           album_count
                )
                {
                    using boost::get;

                    GdkRectangle r ;
                    r.y = ypos ; 

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    if( row > 0 ) 
                    {
                            r.x = 8 ; 

                            cairo->set_source(
                                  album->coverart ? album->coverart : disc
                                , r.x
                                , ypos + 2 
                            ) ; 
                            RoundedRectangle(
                                  cairo
                                , r.x
                                , ypos + 2 
                                , 64 
                                , 64 
                                , rounding
                            ) ;
                            cairo->fill() ;

                            if( album->coverart )
                            {
                                cairo->set_source_rgba(
                                      0.3 
                                    , 0.3
                                    , 0.3
                                    , 0.5
                                ) ; 

                                RoundedRectangle(
                                      cairo
                                    , r.x
                                    , ypos + 2 
                                    , 64 
                                    , 64 
                                    , rounding
                                ) ;

                                cairo->set_line_width( .75 ) ;
                                cairo->stroke() ;
                            }


                            ReleaseType rt = get_rt( album->type ) ; 
                            std::string release_type ;

                            if( rt == RT_SINGLE )
                                release_type = "SINGLE" ;

                            if( rt == RT_ALBUM )
                                release_type = "ALBUM" ;

                            if( rt == RT_SOUNDTRACK )
                                release_type = "OST" ;

                            if( rt == RT_COMPILATION )
                                release_type = "COMPIL." ;

                            if( rt == RT_LIVE )
                                release_type = "LIVE" ;

                            if( rt == RT_EP )
                                release_type = "EP" ;

                            if( rt == RT_REMIX )
                                release_type = "REMIX" ;

                            if( !release_type.empty() )
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
                                const int text_size_pt = static_cast<int> ((text_size_px * 72) 
                                                            / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;

                                int width, height;

                                Pango::FontDescription font_desc =  widget.get_style()->get_font() ;
                                font_desc.set_size( text_size_pt * PANGO_SCALE ) ;
                                font_desc.set_weight( Pango::WEIGHT_BOLD ) ;

                                Glib::RefPtr<Pango::Layout> layout = Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) ;
                                layout->set_font_description( font_desc ) ;
                                layout->set_ellipsize( Pango::ELLIPSIZE_NONE ) ;
                                layout->set_width( 64 * PANGO_SCALE ) ;
                                layout->set_text( release_type ) ; 
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

                            if( track_count > 0 ) 
                            {
                                std::size_t total_track_count = album->track_count ; 
    
                                if( track_count != total_track_count )
                                {
                                    cairo->save() ;

                                    RoundedRectangle(
                                          cairo
                                        , r.x + 4
                                        , ypos + 2 + 4
                                        , 56
                                        , 16 
                                        , rounding
                                    ) ;


                                    cairo->set_source_rgba(
                                          0. 
                                        , 0.
                                        , 0.
                                        , 0.65
                                    ) ; 

                                    cairo->fill() ;

                                    const int text_size_px = 10 ;
                                    const int text_size_pt = static_cast<int> ((text_size_px * 72) 
                                    / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ; 

                                    int width, height;

                                    Pango::FontDescription font_desc =  widget.get_style()->get_font() ;
                                    font_desc.set_size( text_size_pt * PANGO_SCALE ) ;
                                    font_desc.set_weight( Pango::WEIGHT_BOLD ) ;

                                    Glib::RefPtr<Pango::Layout> layout = Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) ;
                                    layout->set_font_description( font_desc ) ;
                                    layout->set_ellipsize( Pango::ELLIPSIZE_NONE ) ;
                                    layout->set_width( 64 * PANGO_SCALE ) ;
                                    layout->set_text((boost::format ("%u %s %u") % track_count % ("of") % total_track_count).str()) ;
                                    layout->get_pixel_size (width, height) ;
                                    cairo->move_to(
                                          r.x + ((64 - width)/2)
                                        , ypos+2+4+((16 - height)/2)
                                    ) ;

                                    pango_cairo_layout_path( cairo->cobj (), layout->gobj () ) ;

                                    cairo->set_source_rgba(
                                          1. 
                                        , 1. 
                                        , 1. 
                                        , 1. 
                                    ) ;
                                    cairo->fill() ;

                                    cairo->restore() ;
                                }
                            }
                    }
                
                    cairo->save() ;
            
                    enum { L1, L2, N_LS } ;

                    const int text_size_px[N_LS] = { 11, 12 } ; 
                    const int text_size_pt[N_LS] = {   static_cast<int> ((text_size_px[L1] * 72)
                                                            / Util::screen_get_y_resolution (Gdk::Screen::get_default ()))
                                                     , static_cast<int> ((text_size_px[L2] * 72)
                                                            / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) } ;

                    int width, height;

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    Pango::FontDescription font_desc[N_LS] ;

                    font_desc[L1] = widget.get_style()->get_font() ;
                    font_desc[L1].set_size( text_size_pt[L1] * PANGO_SCALE ) ;
                    font_desc[L1].set_weight( Pango::WEIGHT_BOLD ) ;
                    font_desc[L1].set_style( Pango::STYLE_ITALIC ) ;

                    font_desc[L2] =  widget.get_style()->get_font() ;
                    font_desc[L2].set_size( text_size_pt[L2] * PANGO_SCALE ) ;
                    font_desc[L2].set_weight( Pango::WEIGHT_BOLD ) ;

                    Glib::RefPtr<Pango::Layout> layout[N_LS] = { Glib::wrap( pango_cairo_create_layout( cairo->cobj() )),
                                                                 Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) } ;

                    layout[L1]->set_font_description( font_desc[L1] ) ;
                    layout[L1]->set_ellipsize( Pango::ELLIPSIZE_END ) ;
                    layout[L1]->set_width(( m_width - 88 ) * PANGO_SCALE ) ;

                    layout[L2]->set_font_description( font_desc[L2] ) ;
                    layout[L2]->set_ellipsize( Pango::ELLIPSIZE_END ) ;
                    layout[L2]->set_width(( m_width - 88 ) * PANGO_SCALE ) ;

                    if( row > 0 )
                    {
                            xpos += 8 + 64 ; 

                            int yoff  = 2 ;

                            //// ALBUM
                            layout[L2]->set_text( album->album )  ;
                            layout[L2]->get_pixel_size( width, height ) ;
                            cairo->move_to(
                                  xpos + 8 
                                , ypos + yoff 
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , .8
                            ) ;
                            pango_cairo_show_layout( cairo->cobj(), layout[L2]->gobj() ) ;

                            yoff = 18 ;

                            //// ARTIST
                            layout[L1]->set_text( album->album_artist )  ;
                            layout[L1]->get_pixel_size( width, height ) ;
                            cairo->move_to(
                                  xpos + 8 
                                , ypos + yoff
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , .6
                            ) ;
                            pango_cairo_show_layout( cairo->cobj(), layout[L1]->gobj() ) ;


                            //// YEAR + LABEL
                            font_desc[L1].set_style( Pango::STYLE_NORMAL ) ;
                            layout[L1]->set_font_description( font_desc[L1] ) ;

                            if( album->label.empty() )
                            {
                                layout[L1]->set_text( (boost::format("PlayScore: %.2f") % album->album_playscore).str() ) ; 
                            }
                            else
                            {
                                layout[L1]->set_markup( /*(boost::format("%s / %s") % Glib::Markup::escape_text(album->year.c_str()).c_str() % Glib::Markup::escape_text(album->label).c_str()).str()*/ (boost::format("PlayScore: %.2f") % album->album_playscore).str() ) ;
                            }
                                
                            layout[L1]->get_pixel_size( width, height ) ;
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
                            pango_cairo_show_layout( cairo->cobj(), layout[L1]->gobj() ) ;
                    }
                    else // display album count only
                    {
                            font_desc[L1] = widget.get_style()->get_font() ;
                            font_desc[L1].set_size( text_size_pt[L1] * PANGO_SCALE * 1.5 ) ;
                            font_desc[L1].set_weight( Pango::WEIGHT_BOLD ) ;

                            layout[L1]->set_font_description( font_desc[L1] ) ;
                            layout[L1]->set_ellipsize( Pango::ELLIPSIZE_END ) ;
                            layout[L1]->set_width( m_width * PANGO_SCALE ) ;

                            layout[L1]->set_text( (boost::format(("%lld %s")) % album_count % ( (album_count > 1)
                                                                                                 ? ("Albums") : ("Album"))).str()) ;
                            layout[L1]->get_pixel_size( width, height ) ;
                            cairo->move_to(
                                  xpos + (m_width - width) / 2
                                , r.y + (row_height - height) / 2
                            ) ;
                            cairo->set_source_rgba(
                                  color.r
                                , color.g
                                , color.b
                                , 0.65
                            ) ;
                            pango_cairo_show_layout( cairo->cobj(), layout[L1]->gobj() ) ;
                    }

                    cairo->restore() ;
                }
        };

        typedef boost::shared_ptr<Column>       Column_sp_t ;
        typedef std::vector<Column_sp_t>        Column_sp_t_vector_t ;
        typedef sigc::signal<void>              Signal_void ;

        class Class
        : public Gtk::DrawingArea
        {
            public:

                DataModelFilter_sp_t                m_model ;

            private:

                std::size_t                         m_row_height ;
                std::size_t                         m_visible_height ;
                std::size_t                         m_dest_position ;

                enum ScrollDirection
                {
                      SCROLL_DIRECTION_UP
                    , SCROLL_DIRECTION_DOWN
                } ;

                sigc::connection                    m_scroll_sigc_connection ;
                 
                ScrollDirection                     m_scroll_direction ; 

                Column_sp_t_vector_t                m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                boost::optional<boost::tuple<Model_t::iterator, gint64, std::size_t> >  m_selection ; 

                Signal_void                         m_SIGNAL_selection_changed ;
                Signal_void                         m_SIGNAL_find_accepted ;
                Signal_void                         m_SIGNAL_start_playback ;

                Interval<std::size_t>               m_Model_I ;

                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                sigc::connection                    m_search_changed_conn ;
                bool                                m_search_active ;

                Cairo::RefPtr<Cairo::ImageSurface>  m_disc ;

                Glib::RefPtr<Gtk::UIManager> m_refUIManager ;
                Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup ;
                Gtk::Menu* m_pMenuPopup ;

                typedef sigc::signal<void, const std::string&> SignalMBID ;

                SignalMBID _signal_0 ; 
                SignalMBID _signal_1 ; 

                bool
                scroll_timeout_func()
                {
                    int adj_value = m_prop_vadj.get_value()->get_value() ;
    
                    if( m_scroll_direction == SCROLL_DIRECTION_UP )
                    {
                        adj_value -= 4 ;
                        adj_value = std::max<int>( adj_value, m_dest_position ) ;
                    }
                    else
                    {
                        adj_value += 4 ;
                        adj_value = std::min<int>( adj_value, m_dest_position ) ;
                    }

                    m_prop_vadj.get_value()->set_value( adj_value ) ;

                    bool done = ( static_cast<int>(adj_value) == static_cast<int>(m_dest_position) ) ;

                    if( done )
                    {
                        m_scroll_sigc_connection.disconnect() ;
                        return false ;
                    }

                    return true ;
                }

                void
                init_scroll(
                      std::size_t       dest_pos
                    , ScrollDirection   scroll_direction
                )
                {
                    if( m_scroll_sigc_connection )
                    {
                        m_scroll_sigc_connection.disconnect() ;
                    }

                    m_dest_position = dest_pos ;
                    m_scroll_direction = scroll_direction ;

                    m_scroll_sigc_connection = Glib::signal_timeout().connect(
                          sigc::mem_fun(
                              *this
                            , &Class::scroll_timeout_func
                    ), 10 ) ;
                }

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
                    if( m_prop_vadj.get_value() && m_row_height )
                    {
                        std::size_t upper = m_prop_vadj.get_value()->get_value() / m_row_height ;
                        return upper ;
                    }
                    else
                        return 0 ;
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

                bool
                key_press_event (GdkEventKey * event)
                {
                    if( event->is_modifier )
                        return false ;

                    if( !m_model->size() )
                        return false ;

                    if( m_search_active )
                    {
                        switch( event->keyval )
                        {
                            case GDK_Up:
                            case GDK_KP_Up:
                                find_prev_match() ;
                                return true ;

                            case GDK_Down:
                            case GDK_KP_Down:
                                find_next_match() ;
                                return true ;

                            case GDK_Escape:
                                cancel_search() ;
                                return true ;
        
                            case GDK_Tab:
                                cancel_search() ;
                                return false ;

                            default: ;
                        }

                        GdkEvent *new_event = gdk_event_copy( (GdkEvent*)(event) ) ;
                        g_object_unref( ((GdkEventKey*)new_event)->window ) ;
                        ((GdkEventKey *) new_event)->window = GDK_WINDOW(g_object_ref(G_OBJECT(GTK_WIDGET(m_SearchWindow->gobj())->window))) ;
                        gtk_widget_event(GTK_WIDGET(m_SearchEntry->gobj()), new_event) ;
                        gdk_event_free(new_event) ;

                        return true ;
                    }

                    int step = 0 ; 
                    int row = 0 ;

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
                                step = - (m_visible_height / m_row_height) ; 
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
                                int origin = boost::get<2>(m_selection.get()) ;

                                if( origin > 0 )
                                {
                                    if( get_row_is_visible( origin ) ) 
                                    {
                                        row = std::max<int>( origin+step, 0 ) ;
                                        select_row( row ) ;

                                        double adj_value = m_prop_vadj.get_value()->get_value() ;

                                        if( (row * m_row_height) < adj_value )
                                        {
                                            if( event->keyval == GDK_Page_Up )
                                            {
                                                m_prop_vadj.get_value()->set_value( std::max<int>( adj_value + (step*int(m_row_height)), 0 )) ; 
                                            }
                                            else
                                            {
                                                scroll_to_row( row ) ; 
                                            }
                                        }
                                    }
                                    else
                                    {
                                        goto mark_first_row_up ;
                                    }
                                }
                                else
                                {
                                    scroll_to_row( 0 ) ;
                                }
                            }

                            return true;

                        case GDK_Home:
                        {
                            select_row( 0 ) ;
                            scroll_to_row( 0 ) ;

                            return true ;
                        }

                        case GDK_End:
                        {
                            select_row( m_model->size() - 1 ) ;
                            scroll_to_row( m_model->size() -1 ) ; 

                            return true ;
                        }

                        case GDK_Down:
                        case GDK_KP_Down:
                        case GDK_Page_Down:

                            if( event->keyval == GDK_Page_Down )
                            {
                                step = (m_visible_height / m_row_height) ; 
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
                                int origin = boost::get<2>(m_selection.get()) ;

                                if( get_row_is_visible( origin )) 
                                {
                                    row = std::min<int>( origin+step, m_model->size() - 1 ) ;
                                    select_row( row ) ;

                                    double adj_value = m_prop_vadj.get_value()->get_value() ;

                                    if( event->keyval == GDK_Page_Down )
                                    {
                                        std::size_t new_val = adj_value + (step*m_row_height) ;

                                        if( new_val > (m_prop_vadj.get_value()->get_upper()-m_visible_height))
                                        {
                                            scroll_to_row( row ) ;
                                        }
                                        else
                                        {
                                            m_prop_vadj.get_value()->set_value( adj_value + (step*m_row_height)) ; 
                                        }
                                    }
                                    else
                                    {
                                        std::size_t offset = adj_value - ((std::size_t(adj_value)/m_row_height) * m_row_height) ;
                                        std::size_t excess = (((m_visible_height/m_row_height)+1)*m_row_height) - m_visible_height ;

                                        if( offset == 0 )
                                        {
                                            if( (row-get_upper_row()+1)*m_row_height > m_visible_height )
                                            {
                                                m_prop_vadj.get_value()->set_value( 
                                                      adj_value + excess
                                                ) ;
                                            }
                                        }
                                        else
                                        {
                                            std::size_t endpos = ((64-offset) + ((row-get_upper_row())*m_row_height)) ;

                                            if( endpos > m_visible_height )
                                            {
                                                m_prop_vadj.get_value()->set_value( 
                                                      (adj_value + (m_row_height-offset) + excess)
                                                ) ;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    goto mark_first_row_down ;
                                }
                            }

                            return true;

                        default:

                            if( !Gtk::DrawingArea::on_key_press_event( event ))
                            { 
                                if( !m_search_active ) 
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

                                    return false ;
                                }
                            }
                    }

                    return false ;
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

                    {
                        cancel_search() ;
                        grab_focus() ;

                        std::size_t row  = double(m_prop_vadj.get_value()->get_value()) / double(m_row_height) ; 
                        std::size_t off  = m_row_height - (m_prop_vadj.get_value()->get_value() - (row*m_row_height)) ;

                        if( event->y > off || off == 0 )
                        {
                            std::size_t row2 = row + (event->y + (off ? (m_row_height-off) : 0)) / m_row_height ; 

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
                                select_row( row ) ;
                            }
                        }
                    }

                    if( event->button == 3 )
                    {
                        m_pMenuPopup->popup(event->button, event->time) ;                            
                        return true ;
                    }

                    if( event->button == 1 && event->type == GDK_2BUTTON_PRESS )
                    {
                        m_SIGNAL_start_playback.emit() ;
                        return true ;
                    }

                    return true ;
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

                void
                configure_vadj(
                      std::size_t   upper
                    , std::size_t   page_size
                    , std::size_t   step_increment
                )
                {
                    if( m_prop_vadj.get_value() )
                    {
                        m_prop_vadj.get_value()->set_upper( upper ) ; 
                        m_prop_vadj.get_value()->set_page_size( page_size ) ; 
                        m_prop_vadj.get_value()->set_step_increment( step_increment ) ; 
                    }
                }

                bool
                on_configure_event(
                    GdkEventConfigure* event
                )        
                {
                    m_visible_height = event->height ; 

                    if( m_row_height )
                    {
                        configure_vadj(
                              m_model->m_mapping.size() * m_row_height
                            , m_visible_height
                            , m_row_height
                        ) ; 
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

                    std::size_t row     = get_upper_row() ; 
                    int offset          = m_prop_vadj.get_value()->get_value() - (row*m_row_height) ;
                    int ypos            = 0 ;
                    std::size_t xpos    = 0 ;
                    std::size_t count   = m_visible_height / m_row_height + 2 ;
            
                    if( offset ) 
                    {
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

                    bool album_name_only = m_model->m_constraints_artist && (m_model->m_constraints_artist->size() == 1) ;

                    std::valarray<double> dashes ( 3 ) ;
                    dashes[0] = 0. ;
                    dashes[1] = 1. ;
                    dashes[2] = 0. ;

                    RowRowMapping_t::const_iterator row_iter = m_model->row_iter( row ) ;

                    while( m_model->is_set() && count && m_Model_I.in( row )) 
                    {
                        xpos = 0 ;

                        Model_t::iterator  selected           = *row_iter ; 
                        bool               iter_is_selected   = ( m_selection && boost::get<2>(m_selection.get()) == row ) ;

                        if( iter_is_selected )
                        {
                            GdkRectangle r ;

                            r.x         = inner_pad ; 
                            r.y         = inner_pad + ypos ; 
                            r.width     = a.get_width() - 2*inner_pad ;  
                            r.height    = m_row_height - 2*inner_pad - 2 ;

                            theme->draw_selection_rectangle(
                                  cairo
                                , r
                                , has_focus()
                            ) ;
                        }

                        for( Column_sp_t_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i )
                        {
                            std::size_t count = 0 ;

                            if( m_model->m_constraints_albums )
                            { 
                                gint64 id = (**row_iter)->album_id ;
                                count = (*m_model->m_constraints_albums)[id] ;
                            }

                            (*i)->render(
                                  cairo
                                , m_disc
                                , **row_iter 
                                , *this
                                , row
                                , xpos
                                , ypos + 4
                                , m_row_height
                                , iter_is_selected
                                , iter_is_selected ? c_text_sel : c_text
                                , album_name_only 
                                , count 
                                , m_model->size() - 1
                            ) ;

                            xpos += (*i)->get_width() ; 
                        }

                        ypos += m_row_height;

                        if( m_Model_I.in( row+1 ))
                        {
                            cairo->save(); 

/*
                            gint64 id_1 = get<2>(**row_iter) ;
                            gint64 id_2 = get<2>(**m_model->row_iter(row+1)) ;  
*/

//                            if( id_1 != id_2 )
                            {
                                cairo->set_operator( Cairo::OPERATOR_OVER ) ;

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

                                cairo->set_line_width( 0.75 ) ; 

                                cairo->move_to(
                                      0
                                    , ypos - 1 
                                ) ; 

                                cairo->line_to(
                                      a.get_width() - 2
                                    , ypos - 1 
                                ) ;

                                cairo->stroke() ;
                            }

                            cairo->restore() ; 
                        }

                        row ++ ;
                        count -- ;
                        row_iter ++ ;
                    }

                    return true;
                }

                void
                on_model_changed(
                      std::size_t   position
                    , bool          size_changed
                )
                {
                    if( size_changed ) 
                    {
                            configure_vadj(
                                  m_model->m_mapping.size() * m_row_height
                                , m_visible_height
                                , m_row_height
                            ) ; 

                            m_Model_I = Interval<std::size_t>(
                                  Interval<std::size_t>::IN_EX
                                , 0
                                , m_model->size()
                            ) ;
                    }

                    boost::optional<std::size_t> row ;

                    if( m_selection )
                    {
                        row = boost::get<2>(m_selection.get()) ;
                    }

                    if( row )
                    {
                        scroll_to_row( row.get() ) ; 
                        select_row( row.get(), true ) ;
                    }
                    else
                    {
                        scroll_to_row( position ) ; 
                        select_row( position, true ) ; 
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

                            Class & view = *(reinterpret_cast<Class*>(data));

                            view.m_prop_vadj.get_value()->signal_value_changed().connect(
                                sigc::mem_fun(
                                    view,
                                    &Class::on_vadj_value_changed
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

                        for( RowRowMapping_t::iterator i = m_model->m_mapping.begin(); i != m_model->m_mapping.end(); ++i )
                        {
                            if( real_id == (**i)->album_id )
                            {
                                std::size_t n = std::distance( m_model->m_mapping.begin(), i ) ;
                                select_row( n ) ;
                                scroll_to_row( n ) ;
                                return ;
                            }
                        }
                    }
                
                    clear_selection() ;
                }

                void
                scroll_to_row(
                      std::size_t row
                )
                {
                    if( m_visible_height && m_row_height && m_prop_vadj.get_value() )
                    {
                        if( m_model->m_mapping.size() < std::size_t(m_visible_height/m_row_height) )
                        {
                            m_prop_vadj.get_value()->set_value( 0 ) ; 
                        }
                        else
                        {
                            Limiter<std::size_t> d ( 
                                  Limiter<std::size_t>::ABS_ABS
                                , 0
                                , (m_model->size() * m_row_height) - m_visible_height
                                , (row*m_row_height)
                            ) ;

                            m_prop_vadj.get_value()->set_value( d ) ; 
                        }
                    }
                }

                void
                select_row(
                      std::size_t   row
                    , bool          quiet = false
                )
                {
                    if( m_Model_I.in( row )) 
                    {
                        const gint64& id = (*m_model->m_mapping[row])->album_id ;

                        m_selection = boost::make_tuple( m_model->m_mapping[row], id, row ) ;

                        m_model->set_selected( id ) ;

                        if( !quiet )
                        {
                            m_SIGNAL_selection_changed.emit() ;
                        }

                        queue_draw();
                    }
                }

                Signal_void&
                signal_selection_changed()
                {
                    return m_SIGNAL_selection_changed ;
                }

                Signal_void&
                signal_find_accepted()
                {
                    return m_SIGNAL_find_accepted ;
                }

                Signal_void&
                signal_start_playback()
                {
                    return m_SIGNAL_start_playback ;
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
                set_model(DataModelFilter_sp_t model)
                {
                    m_model = model;

                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &Class::on_model_changed
                    ));

                    on_model_changed( 0, true ) ;
                }

                void
                append_column(
                      Column_sp_t   column
                )
                {
                    m_columns.push_back(column);
                }

            protected:

                void
                find_next_match()
                {
                    using boost::get ;

                    Glib::ustring text = m_SearchEntry->get_text().casefold() ;

                    if( text.empty() )
                    {
                        return ;
                    }

                    RowRowMapping_t::iterator i = m_model->m_mapping.begin(); 

                    if( m_selection )
                    {
                        std::advance( i, get<2>(m_selection.get()) ) ;
                        ++i ;
                    }

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        Glib::ustring match = Glib::ustring((**i)->album).casefold() ;

                        if( Util::match_keys( match, text )) 
                        {
                            std::size_t d = std::distance( m_model->m_mapping.begin(), i ) ; 
                            scroll_to_row( d ) ;
                            select_row( d ) ;
                            return ;
                        }
                    }
                }

                void
                find_prev_match()
                {
                    using boost::get ;

                    Glib::ustring text = m_SearchEntry->get_text().casefold() ;

                    if( text.empty() )
                    {
                        return ;
                    }

                    RowRowMapping_t::iterator i = m_model->m_mapping.begin(); 

                    if( m_selection )
                    {
                        std::advance( i, get<2>(m_selection.get()) ) ;
                        --i ; 
                    }

                    for( ; i >= m_model->m_mapping.begin(); --i )
                    {
                        Glib::ustring match = Glib::ustring((**i)->album).casefold() ;

                        if( Util::match_keys( match, text )) 
                        {
                            std::size_t d = std::distance( m_model->m_mapping.begin(), i ) ; 
                            scroll_to_row( d ) ;
                            select_row( d ) ;
                            return ;
                        }
                    }
                }

                void
                on_search_entry_changed()
                {
                    using boost::get ;

                    Glib::ustring text = m_SearchEntry->get_text().casefold() ;

                    if( text.empty() )
                    {
                        return ;
                    }

                    RowRowMapping_t::iterator i = m_model->m_mapping.begin(); 
                    ++i ;
              
                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        Glib::ustring match = Glib::ustring((**i)->album).casefold() ;

                        if( Util::match_keys( match, text )) 
                        {
                            std::size_t d = std::distance( m_model->m_mapping.begin(), i ) ; 
                            scroll_to_row( d ) ;
                            select_row( d ) ;
                            return ;
                        }
                    }

                    clear_selection( false ) ;
                    scroll_to_row( 0 ) ;
                }

                void
                on_search_entry_activated()
                {
                    cancel_search() ;
                    m_SIGNAL_find_accepted.emit() ;
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
                on_show_only_this_album() 
                {
                    if( m_selection )
                    {
                        Album_sp album = *(boost::get<0>(m_selection.get())) ;
                        _signal_0.emit( album->mbid ) ;
                    }
                }

                void
                on_show_only_this_artist() 
                {
                    if( m_selection )
                    {
                        Album_sp album = *(boost::get<0>(m_selection.get())) ;
                        _signal_1.emit( album->mbid_artist ) ;
                    }
                }

            public:

                void
                cancel_search()
                {
                    if( !m_search_active )
                        return ;

                    send_focus_change( *m_SearchEntry, false ) ;

                    m_SearchWindow->hide() ;
                    m_search_changed_conn.block () ;
                    m_SearchEntry->set_text("") ;
                    m_search_changed_conn.unblock () ;
                    m_search_active = false ;
                }
    
            protected:

                virtual void
                on_realize()
                {
                    Gtk::DrawingArea::on_realize() ;
                    initialize_metrics();
                    queue_resize();
                }

            public:

                void
                clear_selection(
                      bool quiet = true
                )
                {
                    m_model->m_selected.reset() ;
                    m_selection.reset() ;

                    if( !quiet )
                    {
                        queue_draw() ;
                    }
                }

                SignalMBID&
                signal_only_this_album_mbid()
                {
                    return _signal_0 ;
                }

                SignalMBID&
                signal_only_this_artist_mbid()
                {
                    return _signal_1 ;
                }

                Class ()

                        : ObjectBase( "YoukiViewAlbums" )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_search_active( false )

                {
                    m_disc = Util::cairo_image_surface_from_pixbuf( Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "disc.png" ))->scale_simple(64, 64, Gdk::INTERP_BILINEAR)) ;

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
                    gtk_widget_realize( GTK_WIDGET(m_SearchEntry->gobj() )) ;
                    m_SearchEntry->show() ;

                    m_search_changed_conn = m_SearchEntry->signal_changed().connect(
                            sigc::mem_fun(
                                  *this
                                , &Class::on_search_entry_changed
                    )) ;
    
                    m_SearchEntry->signal_activate().connect(
                            sigc::mem_fun(
                                  *this
                                , &Class::on_search_entry_activated
                    )) ;

                    m_SearchWindow = new Gtk::Window( Gtk::WINDOW_POPUP ) ;
                    m_SearchWindow->set_decorated( false ) ;

                    m_SearchWindow->signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &Class::on_search_window_focus_out
                    )) ;

                    signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &Class::on_search_window_focus_out
                    )) ;

                    m_SearchWindow->add( *m_SearchEntry ) ;
                    m_SearchEntry->show() ;

                    signal_key_press_event().connect(
                        sigc::mem_fun(
                              *this
                            , &Class::key_press_event
                    ), true ) ;

                    m_refActionGroup = Gtk::ActionGroup::create() ;
                    m_refActionGroup->add( Gtk::Action::create("ContextMenu", "Context Menu")) ;

                    m_refActionGroup->add( Gtk::Action::create("ContextShowAlbum", "Show only this Album"),
                        sigc::mem_fun(*this, &Class::on_show_only_this_album)) ;
                    m_refActionGroup->add( Gtk::Action::create("ContextShowArtist", "Show only this Artist"),
                        sigc::mem_fun(*this, &Class::on_show_only_this_artist)) ;
    
                    m_refUIManager = Gtk::UIManager::create() ;
                    m_refUIManager->insert_action_group(m_refActionGroup) ;

                    std::string ui_info =
                    "<ui>"
                    "   <popup name='PopupMenu'>"
                    "       <menuitem action='ContextShowAlbum'/>"
                    "       <menuitem action='ContextShowArtist'/>"
                    "   </popup>"
                    "</ui>" ;

                    m_refUIManager->add_ui_from_string( ui_info ) ;

                    m_pMenuPopup = dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu")) ;
               }

                virtual ~Class ()
                {
                }
        };
}
}
}

#endif // _YOUKI_ALBUM_LIST_HH
