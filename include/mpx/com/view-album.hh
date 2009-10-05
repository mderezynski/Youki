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

#include "mpx/algorithm/aque.hh"
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
namespace View
{
namespace Albums
{
        typedef boost::tuple<Cairo::RefPtr<Cairo::ImageSurface>, gint64, gint64, std::string, std::string, std::string, ReleaseType, std::string, std::string, std::string> Row_t ;

        typedef IndexedList<Row_t>                                                      Model_t ;
        typedef boost::shared_ptr<Model_t>                                              Model_SP_t ;
        typedef std::map<gint64, Model_t::iterator>                                     IdIterMap_t ;

        typedef std::vector<Model_t::iterator>                                          RowRowMapping_t ;

        typedef sigc::signal<void>                                                      Signal_0 ;
        typedef sigc::signal<void, std::size_t, bool>                                   Signal_1 ;

        struct OrderFunc
        : public std::binary_function<Row_t, Row_t, bool>
        {
            bool operator() (
                  const Row_t&  a
                , const Row_t&  b
            )
            {
                int64_t id[2] = { get<1>(a), get<2>(b) } ;

                if( id[0] == -1 ) 
                {
                    return true ;
                }

                if( id[1] == -1 )
                {     
                    return false ;
                }

                const std::string&  c_a_1 = boost::get<4>(a) ;
                const std::string&  c_a_2 = boost::get<7>(a) ;
                const std::string&  c_a_3 = boost::get<3>(a) ;
            
                const std::string&  c_b_1 = boost::get<4>(b) ;
                const std::string&  c_b_2 = boost::get<7>(b) ;
                const std::string&  c_b_3 = boost::get<3>(b) ;

                if( c_a_1 < c_b_1 )
                    return true ;

                if( c_b_1 < c_a_1 )
                    return false ;


                if( c_a_2 < c_b_2 )
                    return true ;

                if( c_b_2 < c_a_2 )
                    return false ;


                if( c_a_3 < c_b_3 )
                    return true ;

                if( c_b_3 < c_a_3 )
                    return false ;

                return false ;
            }
        } ;

        struct DataModel
        : public sigc::trackable
        {
                Model_SP_t                      m_realmodel ;
                IdIterMap_t                     m_iter_map ;
                std::size_t                     m_top_row ;
                boost::optional<gint64>         m_selected ;
                boost::optional<std::size_t>    m_selected_row ;

                Signal_1                        m_changed ;

                DataModel()
                : m_top_row( 0 )
                {
                    m_realmodel = Model_SP_t(new Model_t); 
                }

                DataModel(Model_SP_t model)
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

                virtual Row_t&
                row (std::size_t row)
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
                    const boost::optional<gint64>& row = boost::optional<gint64>()
                )
                {
                    m_selected = row ;
                }

                virtual boost::optional<std::size_t>
                get_selected_row(
                )
                {
                    return m_selected_row ; 
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
                    , const std::string&                    label
                )
                {
                    Row_t row ( surface, id_album, id_artist, album, album_artist, mbid, get_rt( type ), year, label ) ; 
                    m_realmodel->push_back( row ) ;

                    Model_t::iterator i = m_realmodel->end() ;
                    std::advance( i, -1 ) ;
                    m_iter_map.insert( std::make_pair( id_album, i) ) ; 
                }

                virtual void
                insert_album(
                      Cairo::RefPtr<Cairo::ImageSurface>    surface
                    , gint64                                id_album
                    , gint64                                id_artist
                    , const std::string&                    album
                    , const std::string&                    album_artist
                    , const std::string&                    mbid
                    , const std::string&                    type
                    , const std::string&                    year
                    , const std::string&                    label
                )
                {
                    static OrderFunc order ;

                    Row_t row(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                        , mbid
                        , get_rt( type )
                        , year
                        , label
                    ) ; 

                    Model_t::iterator i = m_realmodel->insert(
                          std::lower_bound(
                              m_realmodel->begin()
                            , m_realmodel->end()
                            , row
                            , order
                          )
                        , row
                    ) ;

                    m_iter_map.insert( std::make_pair( id_album, i )) ; 
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
        };

        typedef boost::shared_ptr<DataModel> DataModel_SP_t;

        struct DataModelFilter
        : public DataModel
        {
            typedef std::vector<int>                        IdVector_t ;
            typedef boost::shared_ptr<IdVector_t>           IdVector_sp ;

            public:

                RowRowMapping_t   m_mapping ;
                IdVector_sp       m_constraints_albums ;
                IdVector_sp       m_constraints_artist ;

            public:

                DataModelFilter(
                      DataModel_SP_t& model
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
                    const IdVector_sp& constraint
                )
                {
                    m_constraints_albums = constraint ;
                }

                virtual void
                clear_constraints_album(
                )
                {
                    m_constraints_albums.reset() ;
                }

                virtual void
                set_constraints_artist(
                    const IdVector_sp& constraint
                )
                {
                    m_constraints_artist = constraint ;
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

                virtual Row_t&
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
                      Cairo::RefPtr<Cairo::ImageSurface>    surface
                    , gint64                                id_album
                    , gint64                                id_artist
                    , const std::string&                    album
                    , const std::string&                    album_artist
                    , const std::string&                    mbid
                    , const std::string&                    type
                    , const std::string&                    year
                    , const std::string&                    label
                )
                {
                    DataModel::append_album(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                        , mbid
                        , type
                        , year
                        , label
                    ) ;
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
                      Cairo::RefPtr<Cairo::ImageSurface>    surface
                    , gint64                                id_album
                    , gint64                                id_artist
                    , const std::string&                    album
                    , const std::string&                    album_artist
                    , const std::string&                    mbid
                    , const std::string&                    type
                    , const std::string&                    year
                    , const std::string&                    label
                )
                {
                    DataModel::insert_album(
                          surface
                        , id_album
                        , id_artist
                        , album
                        , album_artist
                        , mbid
                        , type
                        , year
                        , label
                    ) ;
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

                    RowRowMapping_t new_mapping ;
                    new_mapping.reserve( m_realmodel->size() ) ;

                    boost::optional<gint64> id_top ;
                    boost::optional<gint64> id_sel ;

                    if( m_top_row < m_mapping.size() )
                    {
                        id_top = get<1>( row( m_top_row )) ;
                    }

                    if( m_selected )
                    {
                        id_sel = m_selected ; 
                    }

                    m_selected.reset() ;
                    m_selected_row.reset() ;
                    m_top_row = 0 ;
    
                    Model_t::iterator i = m_realmodel->begin() ;

                    new_mapping.push_back( i++ ) ;

                    IdVector_t * constraints_albums = m_constraints_albums.get() ;
                    IdVector_t * constraints_artist = m_constraints_artist.get() ;

                    for( ; i != m_realmodel->end(); ++i )
                    {
                        int truth = 
                                    (!constraints_albums || (*constraints_albums)[get<1>(*i)] == 1 )
                                                                    &&
                                    (!constraints_artist || (*constraints_artist)[get<2>(*i)] == 1 )
                        ; 

                        if( truth )
                        {
                            gint64 id_row = get<1>( *i ) ;

                            if( id_top && id_row == id_top.get() )
                            {
                                m_top_row = new_mapping.size() ;
                            }

                            if( id_sel && id_row == id_sel.get() )
                            {
                                m_selected = id_sel ; 
                                m_selected_row = new_mapping.size() ;
                            }

                            new_mapping.push_back( i ) ;
                        }
                    }

                    if( m_selected_row )
                    {
                        m_top_row = m_selected_row.get() ;
                    }
                
                    if( new_mapping != m_mapping )
                    {
                        std::swap( new_mapping, m_mapping ) ;
                        update_count() ;
                        m_changed.emit( m_top_row, m_mapping.size() != new_mapping.size() );
                    }                
                }

                void
                update_count(
                )
                {
                    Row_t& row = **m_mapping.begin() ;
                    std::size_t model_size = m_mapping.size()-1 ;
                    get<4>(row) = (boost::format(_("<b>%lld %s</b>")) % model_size % ((model_size > 1) ? _("Albums") : _("Album"))).str() ;
                }
        };

        typedef boost::shared_ptr<DataModelFilter> DataModelFilter_SP_t;

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
                    , Cairo::RefPtr<Cairo::ImageSurface>&   s_fallback
                    , const Row_t&                          data_row
                    , Gtk::Widget&                          widget
                    , int                                   row
                    , int                                   xpos
                    , int                                   ypos
                    , int                                   row_height
                    , bool                                  selected
                    , const ThemeColor&                     color
                    , bool                                  album_name_only
                )
                {
                    using boost::get;

                    GdkRectangle r ;
                    r.y = ypos ; 

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    Cairo::RefPtr<Cairo::ImageSurface> s = get<0>(data_row) ;

                    if( row > 0 ) 
                    {
                            r.x = 8 ; 

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
                                layout->set_ellipsize( Pango::ELLIPSIZE_NONE ) ;
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
                                    , 4.
                                ) ;

                                cairo->set_line_width( .75 ) ;
                                cairo->stroke() ;
                            }
                    }
                
                    cairo->save() ;
            
                    enum { L1, L2, N_LS } ;

                    const int text_size_px[N_LS] = { 10, 12 } ; 
                    const int text_size_pt[N_LS] = {   static_cast<int> ((text_size_px[L1] * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ()))
                                                     , static_cast<int> ((text_size_px[L2] * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) } ;

                    int width, height;

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    Pango::FontDescription font_desc[N_LS] ;

                    font_desc[L1] =  widget.get_style()->get_font() ;
                    font_desc[L1].set_size( text_size_pt[L1] * PANGO_SCALE ) ;
                    font_desc[L1].set_weight( Pango::WEIGHT_BOLD ) ;

                    font_desc[L2] =  widget.get_style()->get_font() ;
                    font_desc[L2].set_size( text_size_pt[L2] * PANGO_SCALE ) ;
                    font_desc[L2].set_weight( Pango::WEIGHT_BOLD ) ;

                    Glib::RefPtr<Pango::Layout> layout[N_LS] = { Glib::wrap( pango_cairo_create_layout( cairo->cobj() )), Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) } ;

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

                            if( !album_name_only )
                            {
                                //// ARTIST
                                layout[L1]->set_text( get<4>(data_row) )  ;
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

                                yoff = 16 ;
                            }

                            //// ALBUM
                            layout[L2]->set_text( get<3>(data_row) )  ;
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

                            //// YEAR + LABEL

                            std::string year_label ; 

                            if( get<8>(data_row).empty() )
                                year_label = get<7>(data_row) ; 
                            else
                                year_label = (boost::format("%s <span color='#404040'>•</span> %s") % get<7>(data_row) % get<8>(data_row)).str() ;

                            layout[L1]->set_markup( year_label ) ; 
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
                    else
                    {
                            font_desc[L1] = widget.get_style()->get_font() ;
                            font_desc[L1].set_size( text_size_pt[L1] * PANGO_SCALE * 1.5 ) ;
                            font_desc[L1].set_weight( Pango::WEIGHT_BOLD ) ;

                            layout[L1]->set_font_description( font_desc[L1] ) ;
                            layout[L1]->set_ellipsize( Pango::ELLIPSIZE_END ) ;
                            layout[L1]->set_width( m_width * PANGO_SCALE ) ;

                            layout[L1]->set_markup( get<4>(data_row) )  ;
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

        typedef boost::shared_ptr<Column>       Column_SP_t ;
        typedef std::vector<Column_SP_t>        Column_SP_t_vector_t ;

        typedef sigc::signal<void>              Signal_void ;

        class Class : public Gtk::DrawingArea
        {
            public:

                DataModelFilter_SP_t                m_model ;

            private:

                int                                 m_row_height ;
                int                                 m_visible_height ;

                Column_SP_t_vector_t                m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                guint                               m_signal0 ; 

                boost::optional<boost::tuple<Model_t::iterator, gint64, std::size_t> >  m_selection ; 

                Signal_void                         m_SIGNAL_selection_changed ;
                Signal_void                         m_SIGNAL_find_accepted ;

                Interval<std::size_t>               m_Model_I ;

                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                sigc::connection                    m_search_changed_conn ;
                bool                                m_search_active ;

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
        
                            default: ;
                        }

                        GdkEvent *new_event = gdk_event_copy( (GdkEvent*)(event) ) ;
                        g_object_unref( ((GdkEventKey*)new_event)->window ) ;
                        ((GdkEventKey *) new_event)->window = GDK_WINDOW(g_object_ref(G_OBJECT(GTK_WIDGET(m_SearchWindow->gobj())->window))) ;
                        gtk_widget_event(GTK_WIDGET(m_SearchEntry->gobj()), new_event) ;
                        gdk_event_free(new_event) ;

                        return true ;
                    }

                    int step ; 
                    int row ;

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
                                int origin = get<2>(m_selection.get()) ;

                                if( origin > 0 )
                                {
                                    if( get_row_is_visible( origin ) ) 
                                    {
                                        row = std::max<int>( origin+step, 0 ) ;

                                        select_row( row ) ;

                                        double adj_value = m_prop_vadj.get_value()->get_value() ;

                                        if( (row * m_row_height) < m_prop_vadj.get_value()->get_value() )
                                        {
                                            if( event->keyval == GDK_Page_Up )
                                            {
                                                m_prop_vadj.get_value()->set_value( std::max<double>( adj_value + (step*m_row_height), 0 )) ; 
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
                                int origin = get<2>(m_selection.get()) ;

                                if( get_row_is_visible( origin )) 
                                {
                                    row = std::min<int>( origin+step, m_model->size() - 1 ) ;

                                    select_row( row ) ;

                                    std::size_t position     = (get_upper_row()+step) * m_row_height ; 
                                    std::size_t position_x   = (((m_visible_height/m_row_height)+1)*m_row_height) - m_visible_height ;
                                    
                                    double adj_value = m_prop_vadj.get_value()->get_value() ;

                                    std::size_t val1 = ((row-get_upper_row())*m_row_height+m_row_height) ;
                                    std::size_t val2 = m_visible_height ;

                                    if( val1 > val2 )
                                    {
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
                                            std::size_t row_in_view  = ((-position_x) + ((row-get_upper_row())*m_row_height)) / m_row_height ;
                                            std::size_t visible_rows = m_visible_height / m_row_height ;
                                            m_prop_vadj.get_value()->set_value( position + position_x - (visible_rows-row_in_view)*m_row_height ) ; 
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

                    if( event->type == GDK_BUTTON_PRESS )
                    {
                        cancel_search() ;
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
                                scroll_to_row( row ) ; 
                                select_row( row ) ;
                            }
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

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    std::size_t row     = m_prop_vadj.get_value()->get_value() / m_row_height ;
                    int offset          = m_prop_vadj.get_value()->get_value() - (row*m_row_height) ;
                    int ypos            = 0 ;
                    std::size_t xpos    = 0 ;
                    std::size_t cnt     = m_visible_height / m_row_height + 2 ;
            
                    if( offset ) 
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

                    RowRowMapping_t::const_iterator row_iter = m_model->row_iter( row ) ;

                    bool album_name_only = m_model->m_constraints_artist && (m_model->m_constraints_artist->size() == 1) ;

                    std::valarray<double> dashes ( 3 ) ;
                    dashes[0] = 0. ;
                    dashes[1] = 1. ;
                    dashes[2] = 0. ;

                    while( m_model->is_set() && cnt && m_Model_I.in( row )) 
                    {
                        xpos = 0 ;

                        Model_t::iterator  selected           = *row_iter ; 
                        bool               iter_is_selected   = ( m_selection && get<2>(m_selection.get()) == row ) ;

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

                        for( Column_SP_t_vector_t::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i )
                        {
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
                            ) ;

                            xpos += (*i)->get_width() ; 
                        }

                        ypos += m_row_height;

                        if( m_Model_I.in( row+1 ))
                        {
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
                                  a.get_width() - 2
                                , ypos - 1 
                            ) ;

                            cairo->stroke() ;
                            cairo->restore(); 
                        }

                        row ++ ;
                        cnt -- ;
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

                    boost::optional<std::size_t> row = m_model->get_selected_row() ;

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
                            if( real_id == get<1>(**i))
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
                        const gint64& id = get<1>(*m_model->m_mapping[row]) ;

                        m_selection = boost::make_tuple( m_model->m_mapping[row], id, row ) ;
                        m_model->set_selected( id ) ;

                        if( !quiet )
                        {
                            m_SIGNAL_selection_changed.emit() ;
                            queue_draw();
                        }
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
                clear_selection(
                      bool quiet = true
                )
                {
                    if( m_model->m_mapping.size() && (!m_selection || boost::get<2>(m_selection.get()) != 0) )
                    {
                        select_row( 0, quiet ) ;
                        return ;
                    }

                    m_model->set_selected() ; 
                }
    
                void
                set_model(DataModelFilter_SP_t model)
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
                append_column (Column_SP_t column)
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
                        Glib::ustring match = Glib::ustring(get<3>(**i)).casefold() ;

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
                        Glib::ustring match = Glib::ustring(get<3>(**i)).casefold() ;

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
                        Glib::ustring match = Glib::ustring(get<3>(**i)).casefold() ;

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
               }

                virtual ~Class ()
                {
                }
        };
}
}
}

#endif // _YOUKI_ALBUM_LIST_HH
