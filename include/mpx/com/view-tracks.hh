#ifndef YOUKI_VIEW_TRACKS_HH
#define YOUKI_VIEW_TRACKS_HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <deque>
#include <boost/unordered_set.hpp>
#include <sigx/sigx.h>

#include "mpx/util-string.hh"
#include "mpx/util-graphics.hh"

#include "mpx/mpx-types.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-covers.hh"

#include "mpx/algorithm/aque.hh"
#include "mpx/algorithm/ntree.hh"
#include "mpx/algorithm/interval.hh"
#include "mpx/algorithm/limiter.hh"

#include "mpx/aux/glibaddons.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "src/library.hh"

#include "glib-marshalers.h"
// ugh

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace MPX
{
namespace View
{
namespace Tracks
{
        std::string
        RowGetArtistName(
              const MPX::SQL::Row&   r
        )
        {
            std::string name ;

            if( r.count("album_artist") ) 
            {
                Glib::ustring in_utf8 = boost::get<std::string>(r.find("album_artist")->second) ; 
                gunichar c = in_utf8[0] ;

                if( g_unichar_get_script( c ) != G_UNICODE_SCRIPT_LATIN && r.count("album_artist_sortname") ) 
                {
                        std::string in = boost::get<std::string>( r.find("album_artist_sortname")->second ) ; 

                        boost::iterator_range <std::string::iterator> match1 = boost::find_nth( in, ", ", 0 ) ;
                        boost::iterator_range <std::string::iterator> match2 = boost::find_nth( in, ", ", 1 ) ;

                        if( !match1.empty() && match2.empty() ) 
                        {
                            name = std::string (match1.end(), in.end()) + " " + std::string (in.begin(), match1.begin());
                        }
                        else
                        {
                            name = in ;
                        }

                        return name ;
                }

                name = in_utf8 ;
            }

            return name ;
        }

        namespace
        {
            const double rounding = 4. ; 
        }

        typedef boost::tuple<std::string, std::string, std::string, gint64, Track, gint64, gint64, std::string, std::string>  Row_t ;

/*
        bool operator<(const Row_t& a, const Row_t& b )
        {
            const Glib::ustring&    a1 = get<1>(a)
                                  , a2 = get<0>(a)
                                  , a3 = get<2>(a) ;

            const Glib::ustring&    b1 = get<1>(b)
                                  , b2 = get<0>(b)
                                  , b3 = get<2>(b) ;

            gint64 ta = get<5>(a) ;
            gint64 tb = get<5>(b) ;

            return (a1 < b1 && a2 < b2 && a3 < b3 && ta < tb ) ; 
        }
*/

        typedef std::vector<Row_t>                          Model_t ;
        typedef boost::shared_ptr<Model_t>                  Model_SP_t ;
        typedef std::map<gint64, Model_t::iterator>         IdIterMap_t ;
        typedef sigc::signal<void, std::size_t, bool>       Signal1 ;

        struct Model_t_iterator_equal
        : std::binary_function<Model_t::iterator, Model_t::iterator, bool>
        {
            bool operator()(
                  const Model_t::iterator& a
                , const Model_t::iterator& b
            ) const
            {
                return a == b ;
            }

            bool operator()(
                  const Model_t::const_iterator& a
                , const Model_t::const_iterator& b
            ) const
            {
                return a == b ;
            }

            bool operator()(
                  const Model_t::const_iterator& a
                , const Model_t::iterator& b
            ) const
            {
                return a == b ;
            }

            bool operator()(
                  const Model_t::iterator& a
                , const Model_t::const_iterator& b
            ) const
            {
                return a == b ;
            }
        } ;

        struct Model_t_iterator_hash
        {
            std::size_t operator()( const Model_t::iterator& i ) const
            {
                return GPOINTER_TO_INT(&(*i)) ;
            }

            std::size_t operator()( Model_t::iterator& i ) const
            {
                return GPOINTER_TO_INT(&(*i)) ;
            }

            std::size_t operator()( const Model_t::const_iterator& i ) const
            {
                return GPOINTER_TO_INT(&(*i)) ;
            }

            std::size_t operator()( Model_t::const_iterator& i ) const
            {
                return GPOINTER_TO_INT(&(*i)) ;
            }
        } ;

        struct OrderFunc
        : public std::binary_function<Row_t, Row_t, bool>
        {
            bool operator() (
                  const Row_t&  a
                , const Row_t&  b
            )
            {
                const std::string&  order_artist_a = get<7>( a ) ; 
                const std::string&  order_artist_b = get<7>( b ) ; 

                const std::string&  order_album_a  = get<2>( a ) ; 
                const std::string&  order_album_b  = get<2>( b ) ; 

                const std::string&  order_date_a   = get<8>( a ) ; 
                const std::string&  order_date_b   = get<8>( b ) ; 

                gint64 order_track [2] = {
                      get<5>( a )
                    , get<5>( b )
                } ;

                return (order_artist_a < order_artist_b) && (order_album_a < order_album_b) && (order_date_a < order_date_b) && (order_track[0] < order_track[1]) ;
            }
        } ;

        struct DataModel
        : public sigc::trackable
        {
                Model_SP_t      m_realmodel;
                Signal1         m_changed;
                IdIterMap_t     m_iter_map;
                std::size_t     m_position ;

                DataModel()
                : m_position( 0 )
                {
                    m_realmodel = Model_SP_t(new Model_t); 
                }

                DataModel(Model_SP_t model)
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

                virtual Signal1&
                signal_changed()
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

                virtual const Row_t&
                row (std::size_t row)
                {
                    return (*m_realmodel)[row] ;
                }

                virtual void
                set_current_row(
                    gint64 row
                )
                {
                    m_position = row ;
                }

                virtual void
                append_track(
                      SQL::Row & r
                    , const MPX::Track& track
                )
                {
                    using boost::get ;

                    std::string     artist
                                  , album
                                  , title
                                  , release_date
                    ;

                    gint64          id          = 0
                                  , track_n     = 0
                                  , id_artist   = 0
                    ;

                    if( r.count("id") )
                    { 
                        id = get<gint64>(r["id"]) ;
                    }
                    else
                        g_critical("%s: No id for track, extremely suspicious", G_STRLOC) ;

                    artist = get<std::string>(r["artist"]) ;

                    if( r.count("album") )
                        album = get<std::string>(r["album"]) ;

                    if( r.count("title") )
                        title = get<std::string>(r["title"]) ;

                    if( r.count("track") )
                    { 
                        track_n = get<gint64>(r["track"]) ;
                    }

                    if( r.count("mpx_album_artist_id") )
                    { 
                        id_artist = get<gint64>(r["mpx_album_artist_id"]) ;
                    }

                    if( r.count("mb_release_date") )
                    { 
                        release_date = get<std::string>(r["mb_release_date"]).substr( 0, 4 ) ;
                    }

                    const std::string&  order_artist = r.count("album_artist_sortname")
                                                       ? get<std::string>(r["album_artist_sortname"])
                                                       : get<std::string>(r["album_artist"]) ;

                    Row_t row ( title, artist, album, id, track, track_n, id_artist, order_artist, release_date ) ;
                    m_realmodel->push_back(row) ;

                    Model_t::iterator i = m_realmodel->end() ;
                    std::advance( i, -1 ) ;

                    m_iter_map.insert( std::make_pair( id, i )) ; 
                }

                void
                erase_track(gint64 id)
                {
                    IdIterMap_t::iterator i = m_iter_map.find(id);

                    if( i != m_iter_map.end() )
                    {
                        m_realmodel->erase( i->second );
                        m_iter_map.erase( i );
                    }
                }
        };

        typedef boost::shared_ptr<DataModel> DataModel_SP_t;

#if 0
        class DataModelInserter
        : public sigx::glib_threadable
        {
            public:

                    typedef std::vector<std::pair<Model_t::const_iterator, Row_t> >  SignalData_t ;
                    typedef sigc::signal<void, SignalData_t>                        SignalRows_t ;
                    typedef sigx::signal_f<SignalRows_t>                            signal_rows_x ;

                    sigx::request_f<const std::vector<int64_t>&> process ;

                    signal_rows_x signal_rows ; 

            protected:

                    Model_SP_t m_Model ;
                    boost::shared_ptr<MPX::Library> m_library ; 

                    struct ThreadData                      
                    {
                        SignalRows_t Rows ;
                    } ;

                    Glib::Private<ThreadData> m_ThreadData ;

                    sigc::connection m_conn_idle ;

            public:
    
                    DataModelInserter(
                          Model_SP_t model
                    )
                    : sigx::glib_threadable()
                    , process(sigc::mem_fun( *this, &DataModelInserter::on_process))
                    , signal_row( *this, m_ThreadData, &ThreadData::Row )
                    , m_Model( model )
                    , m_library( services->get<Library>("mpx-service-library") )
                    {
                    }

                    virtual
                    ~DataModelInserter(
                    )
                    {}

            protected:

                    virtual void
                    on_startup(
                    )
                    {
                        m_ThreadData.set( new ThreadData ) ;
                    }

                    virtual void
                    on_cleanup(
                    )
                    {
                    }

                    void
                    on_process(
                          const std::vector<int64_t>& v
                    )
                    {
                        ThreadData * pthreaddata = m_ThreadData.get() ;

                        SignalData_t data ;

                        const Model_t& m = *(m_Model.get()) ;

                        for( std::vector<int64_t>::const_iterator n = v.begin() ; n != v.end() ; ++n )
                        {
                            const int64_t& id = *n ;  
          
                            SQL::RowV v ;
                            m_library->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % id ).str()) ;

                            SQL::Row& r = v[0] ; 

                            MPX::Track_sp track = m_library->sqlToTrack( r, true, true ) ;
  
                            using boost::get ;

                            std::string artist, album, title ;
                            gint64 track_n = 0, artist_id = 0 ;

                            if(r.count("artist"))
                                artist = get<std::string>(r["artist"]) ;

                            if(r.count("album"))
                                album = get<std::string>(r["album"]) ;

                            if(r.count("title"))
                                title = get<std::string>(r["title"]) ;

                            if(r.count("track"))
                            { 
                                track_n = get<gint64>(r["track"]) ;
                            }

                            if(r.count("mpx_album_artist_id"))
                            { 
                                artist_id = get<gint64>(r["mpx_album_artist_id"]) ;
                            }

                            Row_t row ( title, artist, album, id, *track.get(), track_n, artist_id ) ;

                            Model_t::const_iterator i = std::lower_bound( m.begin(), m.end(), row ) ; 

                            if( i != m.end() )
                            {
                                data.push_back( std::make_pair( i, row )) ;
                            }
                        }

                        pthreaddata->Rows.emit( data ) ; 
                    }
        } ;
#endif

        struct DataModelFilter
        : public DataModel
        {
                typedef std::set<gint64>                        IdSet_t ;
                typedef boost::shared_ptr<IdSet_t>              IdSet_sp ;

                typedef std::set<Model_t::const_iterator>       ModelIteratorSet_t ;
                typedef boost::shared_ptr<ModelIteratorSet_t>   ModelIteratorSet_sp ;

                typedef std::vector<ModelIteratorSet_sp>        IntersectVector_t ;

                typedef std::vector<Model_t::const_iterator>    RowRowMapping_t;

                struct CachedResult
                {
                    ModelIteratorSet_sp ModelIteratorSet ;
                } ;

                typedef std::map<std::string, CachedResult>     FragmentCache_t ;

                struct IntersectSort
                    : std::binary_function<ModelIteratorSet_sp, ModelIteratorSet_sp, bool>
                {
                        bool operator() (
                              const ModelIteratorSet_sp&  a
                            , const ModelIteratorSet_sp&  b
                        )
                        {
                            return a->size() < b->size() ; 
                        }
                } ;

                RowRowMapping_t             m_mapping ;
                RowRowMapping_t             m_mapping_unfiltered ;
                FragmentCache_t             m_fragment_cache ;
                std::string                 m_current_filter ;
                StrV                        m_frags ;
                AQE::Constraints_t          m_constraints_ext ;
                AQE::Constraints_t          m_constraints_aqe ;
                boost::optional<gint64>     m_active_track ;
                boost::optional<gint64>     m_local_active_track ;
                IdSet_sp                    m_constraints_albums ;
                IdSet_sp                    m_constraints_artist ;
                bool                        m_cache_enabled ;

                DataModelFilter( DataModel_SP_t& model )

                    : DataModel( model->m_realmodel )
                    , m_cache_enabled( true )

                {
                    regen_mapping() ;
                }

                virtual ~DataModelFilter()
                {
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear () ;
                    m_mapping.clear() ;
                    m_mapping_unfiltered.clear() ;
                    m_iter_map.clear() ;
                    clear_active_track() ;
                    m_changed.emit( 0, true ) ;
                } 

                void
                clear_active_track()
                {
                    m_local_active_track.reset() ;
                }

                void
                clear_fragment_cache()
                {
                    m_fragment_cache.clear() ;
                }

                void
                disable_fragment_cache()
                {
                    m_cache_enabled = false ;
                }

                void
                enable_fragment_cache()
                {
                    m_cache_enabled = true ;
                }

                void
                set_active_track(gint64 id)
                {
                    m_active_track = id ;
                    scan_active () ;
                }

                boost::optional<gint64>
                get_local_active_track ()
                {
                    return m_local_active_track ;
                }

                void
                add_synthetic_constraint(
                    const AQE::Constraint_t& c
                )
                {
                    m_constraints_ext.push_back( c ) ;    
                    regen_mapping() ;
                }

                void
                add_synthetic_constraint_quiet(
                    const AQE::Constraint_t& c
                )
                {
                    m_constraints_ext.push_back( c ) ;    
                }

                virtual void
                clear_synthetic_constraints(
                )
                {
                    m_constraints_ext.clear() ;
                    regen_mapping() ;
                }

                virtual void
                clear_synthetic_constraints_quiet(
                )
                {
                    m_constraints_ext.clear() ;
                    scan_active() ;
                }

                virtual std::size_t 
                size ()
                {
                    return m_mapping.size();
                }

                virtual const Row_t&
                row (std::size_t row)
                {
                    return *(m_mapping[row]);
                }

                void
                swap( std::size_t p1, std::size_t p2 )
                {
                    std::swap( m_mapping[p1], m_mapping[p2] ) ;
                    scan_active() ;
                    m_changed.emit( m_position, false ) ;
                }

                void
                erase( std::size_t p )
                {
                    RowRowMapping_t::iterator i = m_mapping.begin() ;
                    std::advance( i, p ) ;
                    m_mapping.erase( i ) ;
                    scan_active() ;
                    m_changed.emit( m_position, true ) ;
                }

                virtual void
                append_track(SQL::Row& r, const MPX::Track& track)
                {
                    DataModel::append_track(r, track);
                }

                void
                erase_track(gint64 id)
                {
                    DataModel::erase_track( id );
                }

                virtual void
                insert_track(
                      SQL::Row&             r
                    , const MPX::Track&     track
                )
                {
                    const std::string&                    title             = get<std::string>(r["title"]) ;
                    const std::string&                    artist            = get<std::string>(r["artist"]) ;
                    const std::string&                    album             = get<std::string>(r["album"]) ; 

                    const std::string&                    release_date      = r.count("mb_release_date")
                                                                              ? get<std::string>(r["mb_release_date"]).substr( 0, 4 )
                                                                              : "" ;

                    gint64                                id                = get<gint64>(r["id"]) ;
                    gint64                                track_n           = get<gint64>(r["track"]) ;
                    gint64                                id_artist         = get<gint64>(r["mpx_album_artist_id"]) ;

                    const std::string&                    order_artist      = r.count("album_artist_sortname")
                                                                              ? get<std::string>(r["album_artist_sortname"])
                                                                              : get<std::string>(r["album_artist"]) ;
                    static OrderFunc order ;

                    Row_t row(
                          title
                        , artist
                        , album
                        , id
                        , track
                        , track_n 
                        , id_artist
                        , order_artist
                        , release_date
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

                    m_iter_map.insert( std::make_pair( id, i )) ; 
                }
 
                virtual void
                set_filter(
                    const std::string& text
                )
                { 
                    using boost::get ;
                    using boost::algorithm::split;
                    using boost::algorithm::is_any_of;
                    using boost::algorithm::find_first;

                    AQE::Constraints_t aqe = m_constraints_aqe ;

                    m_constraints_aqe.clear() ;
                    m_frags.clear() ;

                    AQE::parse_advanced_query( m_constraints_aqe, text, m_frags ) ;

                    bool aqe_diff = m_constraints_aqe != aqe ;

                    if( !aqe_diff && ( text.substr( 0, text.size() - 1 ) == m_current_filter ) )
                    {
                        m_current_filter = text ;
                        regen_mapping_iterative() ;
                    }
                    else
                    {
                        m_current_filter = text ;
                        regen_mapping() ;
                    }
                }

                void
                scan_active ()
                {
                    using boost::get;

                    if( !m_active_track )
                    {
                        m_local_active_track.reset() ;
                        return ;
                    }

                    for( RowRowMapping_t::iterator i = m_mapping.begin(); i != m_mapping.end(); ++i )
                    {
                        if( m_active_track && get<3>(**i) == m_active_track.get() )
                        {
                            m_local_active_track = std::distance( m_mapping.begin(), i ) ; 
                            return ;
                        }
                    }

                    m_local_active_track.reset() ;
                }

                void
                find_position( gint64 id )
                {
                    if( id >= 0 )
                    {
                        for( RowRowMapping_t::iterator i = m_mapping.begin() ; i != m_mapping.end() ; ++i )
                        {
                            const Row_t& row = **i ;
                            if( boost::get<3>(row) == id )
                            {
                                m_position = std::distance( m_mapping.begin(), i ) ;
                                return ;
                            }
                        } 
                    }

                    m_position = 0 ;
                }

                virtual void
                cache_current_fragments(
                )
                {
                    using boost::get;
                    using boost::algorithm::split;
                    using boost::algorithm::is_any_of;
                    using boost::algorithm::find_first;

                    if( !m_cache_enabled )
                    {
                        return ;
                    }

                    if( m_frags.empty() )
                    {
                        return ;
                    }

                    std::vector<std::string> vec( 3 ) ;

                    for( std::size_t n = 0 ; n < m_frags.size(); ++n )
                    {
                        if( m_frags[n].empty() ) // the fragment is an empty string, so just contine and do nothing (FIXME> Perhaps just add the track instead?)
                        {
                            continue ;
                        }

                        if( m_fragment_cache.count( m_frags[n] )) 
                        {
                            continue ;
                        }
                        else
                        {
                            ModelIteratorSet_sp model_iterator_set ( new ModelIteratorSet_t ) ;

                            for( Model_t::const_iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i ) // determine all the matches
                            {
                                const Row_t& row = *i;

                                vec[0] = Glib::ustring(boost::get<0>(row)).lowercase() ;
                                vec[1] = Glib::ustring(boost::get<1>(row)).lowercase() ;
                                vec[2] = Glib::ustring(boost::get<2>(row)).lowercase() ;

                                if( Util::match_vec( m_frags[n], vec) )
                                {
                                    model_iterator_set->insert( i ) ; 
                                }
                            }
    
                            CachedResult r ;
                            r.ModelIteratorSet = model_iterator_set ;

                            m_fragment_cache.insert( std::make_pair( m_frags[n], r )) ;
                        }
                    }
                }

                virtual void
                regen_mapping(
                )
                {
                    using boost::get;
                    using boost::algorithm::split;
                    using boost::algorithm::is_any_of;
                    using boost::algorithm::find_first;

                    RowRowMapping_t new_mapping, new_mapping_unfiltered ;

                    IntersectVector_t intersect_v ; 

                    gint64 id ;
        
                    if( m_active_track )
                    {
                        id = m_active_track.get() ;
                    }
                    else
                    if( m_position < m_mapping.size() )
                    {
                        id = get<3>(row( m_position )) ; 
                    }

                    if( m_frags.empty() && (m_constraints_ext.empty() && m_constraints_aqe.empty()) )
                    {
                        m_constraints_albums.reset() ;
                        m_constraints_artist.reset() ;

                        new_mapping.resize( m_realmodel->size() ) ;
                        new_mapping_unfiltered.resize( m_realmodel->size() ) ;

                        std::size_t n = 0 ;

                        for( Model_t::iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i )
                        {
                            new_mapping[n] = i ;
                            new_mapping_unfiltered[n++] = i ;
                        }
                    }
                    else
                    if( m_frags.empty() && !(m_constraints_ext.empty() && m_constraints_aqe.empty()) )
                    {
                        m_constraints_albums = IdSet_sp( new IdSet_t ) ; 
                        m_constraints_artist = IdSet_sp( new IdSet_t ) ; 

                        ModelIteratorSet_sp model_iterator_set ( new ModelIteratorSet_t ) ;

                        intersect_v.push_back( model_iterator_set ) ; 

                        new_mapping.resize( m_realmodel->size() ) ;
                        new_mapping_unfiltered.resize( m_realmodel->size() ) ;

                        std::size_t n = 0 ;
                        std::size_t u = 0 ;

                        for( Model_t::const_iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i ) // determine all the matches
                        {
                            const Row_t& row = *i;

                            const MPX::Track& track = get<4>(row);

                            int t1 = true, t2 = true ;

                            if( !m_constraints_aqe.empty() )
                                t2 = AQE::match_track( m_constraints_aqe, track ) ;

                            if( !t2 )
                                continue ;

                            if( t2 )
                            {
                                new_mapping_unfiltered[u++] = i ;

                                m_constraints_albums->insert( get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ID].get()) ) ;
                                m_constraints_artist->insert( get<6>(row) ) ;
                            }

                            if( !m_constraints_ext.empty() )
                                t1 = AQE::match_track( m_constraints_ext, track ) ;

                            if( !t1 )
                                continue ;

                            new_mapping[n++] = i ; 
                        }

                        new_mapping.resize( n ) ; 
                        new_mapping_unfiltered.resize( u ) ; 
                    }
                    else
                    {
                        std::vector<std::string> vec( 3 ) ;

                        for( std::size_t n = 0 ; n < m_frags.size(); ++n )
                        {
                            if( m_frags[n].empty() ) // the fragment is an empty string, so just contine and do nothing (FIXME> Perhaps just add the track instead?)
                            {
                                continue ;
                            }

                            FragmentCache_t::iterator it = m_fragment_cache.find( m_frags[n] ) ; // do we have this text fragment's result set cached?

                            if( m_cache_enabled && it != m_fragment_cache.end() ) // yes, this text fragment's result set is already cached
                            {
                                const CachedResult& result = it->second ;
                                intersect_v.push_back( result.ModelIteratorSet ) ;
                            }
                            else // no we don't: we need to determine the result set
                            {
                                ModelIteratorSet_sp model_iterator_set ( new ModelIteratorSet_t ) ;
                                intersect_v.push_back( model_iterator_set ) ; 

                                for( Model_t::const_iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i ) // determine all the matches
                                {
                                    const Row_t& row = *i;

                                    vec[0] = Glib::ustring(boost::get<0>(row)).lowercase() ;
                                    vec[1] = Glib::ustring(boost::get<1>(row)).lowercase() ;
                                    vec[2] = Glib::ustring(boost::get<2>(row)).lowercase() ;

                                    if( Util::match_vec( m_frags[n], vec ))
                                    {
                                        model_iterator_set->insert( i ) ; 
                                    }
                                }

                                if( m_cache_enabled && !m_fragment_cache.count( m_frags[n] ) && m_constraints_ext.empty() && m_constraints_aqe.empty() )
                                {
                                    CachedResult r ;
                                    r.ModelIteratorSet = model_iterator_set ;

                                    m_fragment_cache.insert( std::make_pair( m_frags[n], r )) ; // insert newly determined result set for fragment into the fragment cache
                                }
                            }
                        }

                        std::sort( intersect_v.begin(), intersect_v.end(), IntersectSort() ) ;
                        ModelIteratorSet_sp output ( new ModelIteratorSet_t ) ; 

                        if( !intersect_v.empty() )
                        { 
                            output = intersect_v[0] ; 

                            for( std::size_t n = 1 ; n < intersect_v.size() ; ++n )
                            {
                                ModelIteratorSet_sp tmp ( new ModelIteratorSet_t ) ;

                                for( ModelIteratorSet_t::const_iterator i = intersect_v[n]->begin(); i != intersect_v[n]->end(); ++i )
                                {
                                    if( output->find( *i ) != output->end() )
                                    {
                                        tmp->insert( *i ) ;
                                    }
                                }

                                output = tmp ; 
                            }
                        }

                        new_mapping.resize( output->size() ) ;
                        new_mapping_unfiltered.resize( output->size() ) ;
                        std::size_t n = 0 ;
                        std::size_t u = 0 ;

                        m_constraints_albums = IdSet_sp( new IdSet_t ) ; 
                        m_constraints_artist = IdSet_sp( new IdSet_t ) ; 

                        for( ModelIteratorSet_t::iterator i = output->begin() ; i != output->end(); ++i )
                        {
                            const MPX::Track& track = get<4>(**i);

                            int t1 = true, t2 = true ;

                            if( !m_constraints_aqe.empty() )
                                t2 = AQE::match_track( m_constraints_aqe, track ) ;

                            if( !t2 )
                                continue ;

                            if( t2 )
                            {
                                new_mapping_unfiltered[u++] = *i ;

                                m_constraints_albums->insert( get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ID].get()) ) ;
                                m_constraints_artist->insert( get<6>(**i) ) ;
                            }

                            if( !m_constraints_ext.empty() )
                                t1 = AQE::match_track( m_constraints_ext, track ) ;

                            if( !t1 )
                                continue ;

                            new_mapping[n++] = *i ;
                        }

                        new_mapping.resize( n ) ;
                        new_mapping_unfiltered.resize( u ) ;
                    }

                    std::swap( new_mapping, m_mapping ) ;
                    std::swap( new_mapping_unfiltered, m_mapping_unfiltered ) ;
                    scan_active() ;
                    find_position( id ) ;
                    m_changed.emit( m_position, new_mapping.size() != m_mapping.size() ) ; 
                }

                void
                regen_mapping_iterative(
                )
                {
                    using boost::get;
                    using boost::algorithm::split;
                    using boost::algorithm::is_any_of;
                    using boost::algorithm::find_first;

                    RowRowMapping_t new_mapping, new_mapping_unfiltered ;

                    IntersectVector_t intersect_v ; 

                    gint64 id = - 1 ;

                    if( m_active_track )
                    {
                        id = m_active_track.get() ;
                    }
                    else
                    if( m_position < m_mapping.size() )
                    {
                        id = get<3>(row( m_position )) ; 
                    }

                    if( m_frags.empty() && (m_constraints_ext.empty() && m_constraints_aqe.empty()) )
                    {
                        m_constraints_albums.reset() ;
                        m_constraints_artist.reset() ;

                        new_mapping.resize( m_mapping_unfiltered.size() ) ;
                        new_mapping_unfiltered.resize( m_mapping_unfiltered.size() ) ;

                        std::size_t n = 0 ;

                        for( RowRowMapping_t::iterator i = m_mapping_unfiltered.begin(); i != m_mapping_unfiltered.end(); ++i )
                        {
                            new_mapping_unfiltered[n] = *i ;
                            new_mapping[n++] = *i ;
                        }
                    }
                    else
                    if( m_frags.empty() && !(m_constraints_ext.empty() && m_constraints_aqe.empty()) )
                    {
                        m_constraints_albums = IdSet_sp( new IdSet_t ) ;
                        m_constraints_artist = IdSet_sp( new IdSet_t ) ;

                        new_mapping.resize( m_mapping_unfiltered.size() ) ;
                        new_mapping_unfiltered.resize( m_mapping_unfiltered.size() ) ;

                        std::size_t n = 0 ;
                        std::size_t u = 0 ;

                        for( RowRowMapping_t::const_iterator i = m_mapping_unfiltered.begin(); i != m_mapping_unfiltered.end(); ++i )
                        {
                            const Row_t& row = **i ;

                            int t1 = true, t2 = true ;

                            const MPX::Track& track = get<4>(row);

                            if( !m_constraints_aqe.empty() )
                                t2 = AQE::match_track( m_constraints_aqe, track ) ;

                            if( !t2 )
                                continue ;

                            if( t2 )
                            {
                                new_mapping_unfiltered[u++] = *i ;

                                m_constraints_albums->insert( get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ID].get()) ) ;
                                m_constraints_artist->insert( get<6>(row) ) ;
                            }

                            if( !m_constraints_ext.empty() )
                                t1 = AQE::match_track( m_constraints_ext, track ) ;

                            if( !t1 )
                                continue ;

                            new_mapping[n++] = *i ; 
                        }

                        new_mapping.resize( n ) ;
                        new_mapping_unfiltered.resize( u ) ;
                    }
                    else
                    {
                        std::vector<std::string> vec( 3 ) ;

                        for( std::size_t n = 0 ; n < m_frags.size(); ++n )
                        {
                            if( m_frags[n].empty() ) // the fragment is an empty string, so just contine and do nothing (FIXME> Perhaps just add the track instead?)
                            {
                                continue ;
                            }

                            FragmentCache_t::iterator it = m_fragment_cache.find( m_frags[n] ) ; // do we have this text fragment's result set cached?

                            if( m_cache_enabled && it != m_fragment_cache.end() ) // yes, this text fragment's result set is already cached
                            {
                                const CachedResult& result = it->second ;

                                intersect_v.push_back( result.ModelIteratorSet ) ;
                            }
                            else // no we don't: we need to determine the result set
                            {
                                ModelIteratorSet_sp model_iterator_set ( new ModelIteratorSet_t ) ;

                                intersect_v.push_back( model_iterator_set ) ; 

                                for( RowRowMapping_t::const_iterator i = m_mapping_unfiltered.begin(); i != m_mapping_unfiltered.end(); ++i )
                                {
                                    const Row_t& row = **i ;

                                    vec[0] = Glib::ustring(boost::get<0>(row)).lowercase() ;
                                    vec[1] = Glib::ustring(boost::get<1>(row)).lowercase() ;
                                    vec[2] = Glib::ustring(boost::get<2>(row)).lowercase() ;

                                    if( Util::match_vec( m_frags[n], vec ))
                                    {
                                        model_iterator_set->insert( *i ) ; 
                                    }
                                }

                                if( m_cache_enabled && m_frags.size() == 1 && !m_fragment_cache.count( m_frags[n] ) && m_constraints_ext.empty() && m_constraints_aqe.empty() )
                                {
                                    CachedResult r ;

                                    r.ModelIteratorSet = model_iterator_set ;
                                    m_fragment_cache.insert( std::make_pair( m_frags[n], r )) ; // insert newly determined result set for fragment into the fragment cache
                                }
                            }
                        }

                        std::sort( intersect_v.begin(), intersect_v.end(), IntersectSort() ) ;
                        ModelIteratorSet_sp output ( new ModelIteratorSet_t ) ; 

                        if( !intersect_v.empty() )
                        { 
                            output = intersect_v[0] ; 

                            for( std::size_t n = 1 ; n < intersect_v.size() ; ++n )
                            {
                                ModelIteratorSet_sp tmp ( new ModelIteratorSet_t ) ;

                                for( ModelIteratorSet_t::const_iterator i = intersect_v[n]->begin(); i != intersect_v[n]->end(); ++i )
                                {
                                    if( output->find( *i ) != output->end() )
                                    {
                                        tmp->insert( *i ) ;
                                    }
                                }

                                output = tmp ; 
                            }
                        }

                        new_mapping.resize( m_realmodel->size() ) ;
                        new_mapping_unfiltered.resize( m_realmodel->size() ) ; 

                        std::size_t n = 0 ;
                        std::size_t u = 0 ;

                        m_constraints_albums = IdSet_sp( new IdSet_t ) ;
                        m_constraints_artist = IdSet_sp( new IdSet_t ) ;

                        for( ModelIteratorSet_t::iterator i = output->begin() ; i != output->end(); ++i )
                        {
                            const MPX::Track& track = get<4>(**i);

                            int t1 = true, t2 = true ;

                            if( !m_constraints_aqe.empty() )
                                t2 = AQE::match_track( m_constraints_aqe, track ) ;

                            if( !t2 )
                                continue ;

                            if( t2 )
                            {
                                new_mapping_unfiltered[u++] = *i ;

                                m_constraints_albums->insert( get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ID].get()) ) ;
                                m_constraints_artist->insert( get<6>(**i) ) ;
                            }

                            if( !m_constraints_ext.empty() )
                                t1 = AQE::match_track( m_constraints_ext, track ) ;

                            if( !t1 )
                                continue ;

                            new_mapping[n++] = *i ;
                        }

                        new_mapping.resize( n ) ;
                        new_mapping_unfiltered.resize( u ) ;
                    }

                    std::swap( new_mapping, m_mapping ) ;
                    std::swap( new_mapping_unfiltered, m_mapping_unfiltered ) ;
                    scan_active() ;
                    find_position( id ) ;
                    m_changed.emit( m_position, new_mapping.size() != m_mapping.size() ) ; 
                }
        };

        typedef boost::shared_ptr<DataModelFilter> DataModelFilter_SP_t;

        class Column
        {
                int                 m_width ;
                int                 m_column ;
                std::string         m_title ;
                Pango::Alignment    m_alignment ;
                
            public:

                Column (std::string const& title)
                : m_width( 0 )
                , m_column( 0 )
                , m_title( title )
                , m_alignment( Pango::ALIGN_LEFT )
                {
                }

                ~Column ()
                {
                }

                void
                set_width (int width)
                {
                    if( m_width != width )
                    {
                        m_width = width; 
                    }
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
                render_header(
                      Cairo::RefPtr<Cairo::Context>&    cairo
                    , Gtk::Widget&                      widget
                    , int                               xpos
                    , int                               ypos
                    , int                               rowheight
                    , int                               column
                    , const ThemeColor&                 color
                )
                {
                    using boost::get;

                    cairo->rectangle(
                          xpos + 5
                        , ypos + 6
                        , m_width
                        , rowheight
                    ) ;

                    cairo->clip() ;

                    cairo->move_to(
                          xpos + 5
                        , ypos + 6
                    ) ;

                    cairo->set_operator(Cairo::OPERATOR_OVER);

                    cairo->set_source_rgba(
                          color.r
                        , color.g
                        , color.b
                        , color.a   
                    ) ; 

                    Glib::RefPtr<Pango::Layout> layout = widget.create_pango_layout(""); 

                    layout->set_markup(
                          (boost::format("<b>%s</b>") % Glib::Markup::escape_text(m_title).c_str()).str()
                    ) ;

                    layout->set_ellipsize(
                          Pango::ELLIPSIZE_END
                    ) ;

                    layout->set_width(
                          (m_width-10)*PANGO_SCALE
                    ) ;

                    layout->set_alignment(
                          m_alignment
                    ) ;

                    pango_cairo_show_layout(
                          cairo->cobj()
                        , layout->gobj()
                    ) ;

                    cairo->reset_clip() ;
                }

                void
                render(
                      Cairo::RefPtr<Cairo::Context>&    cairo
                    , Gtk::Widget&                      widget
                    , const Row_t&                      datarow
                    , int                               row
                    , int                               xpos
                    , int                               ypos
                    , int                               rowheight
                    , const ThemeColor&                 color
                )
                {
                    using boost::get;

                    cairo->set_operator(Cairo::OPERATOR_OVER);

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
                        , ypos + 2
                    ) ;

                    std::string str;
                    switch( m_column )
                    {
                        case 0:
                            str = get<0>(datarow);
                            break;
                        case 1:
                            str = get<1>(datarow);
                            break;
                        case 2:
                            str = get<2>(datarow);
                            break;
                        case 5:
                            str = boost::lexical_cast<std::string>(get<5>(datarow)) ;
                            break;
                    }

                    Glib::RefPtr<Pango::Layout> layout; 

                    layout = widget.create_pango_layout( str );

                    layout->set_ellipsize(
                          Pango::ELLIPSIZE_END
                    ) ;

                    layout->set_width(
                          (m_width - 12) * PANGO_SCALE
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

        typedef boost::shared_ptr<Column>            Column_SP_t;
        typedef std::vector<Column_SP_t>                 Columns;

        typedef sigc::signal<void, MPX::Track, bool>    SignalTrackActivated ;
        typedef sigc::signal<void>                      SignalVAdjChanged ;
        typedef sigc::signal<void>                      SignalFindAccepted ;
        typedef sigc::signal<void, const std::string&>  SignalFindPropagate ;

        class Class
        : public Gtk::DrawingArea
//        , public sigx::glib_auto_dispatchable()
        {
            public:

                DataModelFilter_SP_t          m_model ;
//                DataModelInserter           * m_inserter ;

            private:

                int                                 m_row_height ;
                int                                 m_row_start ;
                int                                 m_visible_height ;

                Columns                             m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                boost::optional<boost::tuple<Model_t::const_iterator, std::size_t> >  m_selection ;

                IdV                                 m_dnd_idv ;
                bool                                m_dnd ;
                gint64                              m_clicked_row ;
                bool                                m_clicked ;
                bool                                m_highlight ;

                boost::optional<gint64>             m_active_track ;
                boost::optional<gint64>             m_hover_track ;
                boost::optional<gint64>             m_terminal_track ;

                Glib::RefPtr<Gdk::Pixbuf>           m_pb_play_l ;
                Glib::RefPtr<Gdk::Pixbuf>           m_pb_hover_l ;
                Glib::RefPtr<Gdk::Pixbuf>           m_pb_terminal ;

                std::set<int>                       m_collapsed ;
                std::set<int>                       m_fixed ;
                gint64                              m_fixed_total_width ;
        
                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;
                Gtk::HBox                         * m_SearchHBox ;
                Gtk::Button                       * m_SearchButton ;

                sigc::connection                    m_search_changed_conn ; 
                bool                                m_search_active ;

                SignalTrackActivated                m_SIGNAL_track_activated ;
                SignalVAdjChanged                   m_SIGNAL_vadj_changed ;
                SignalFindAccepted                  m_SIGNAL_find_accepted ;
                SignalFindPropagate                 m_SIGNAL_find_propagate ;

                Interval<std::size_t>               m_Model_I ;

                void
                initialize_metrics ()
                {
                    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (gobj()));

                    PangoFontMetrics *metrics = pango_context_get_metrics (context,
                                                                            GTK_WIDGET (gobj())->style->font_desc, 
                                                                            pango_context_get_language (context));

                    m_row_height = (pango_font_metrics_get_ascent (metrics)/PANGO_SCALE) + 
                                   (pango_font_metrics_get_descent (metrics)/PANGO_SCALE) + 5 ;

                    const int visible_area_pad = 2 ;

                    m_row_start = m_row_height + visible_area_pad ;
                }

                void
                on_vadj_value_changed ()
                {
                    if( m_model->size() )
                    {
                        m_model->set_current_row( get_upper_row() ) ;        
                        queue_draw ();
                        m_SIGNAL_vadj_changed.emit() ;
                    }
                }

            protected:

                virtual bool
                on_focus_in_event (GdkEventFocus* G_GNUC_UNUSED)
                {
                    if( !m_selection && m_model->size() )
                    {
                        std::size_t row = get_upper_row();
                        m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                    }

                    queue_draw() ;

                    return false ;
                }

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

                            case GDK_Return:
                            case GDK_KP_Enter:
                            case GDK_ISO_Enter:
                            case GDK_3270_Enter:
                                cancel_search() ;
                                goto continue_matching ;
        
                            default: ;
                        }

                        GdkEvent *new_event = gdk_event_copy( (GdkEvent*)(event) ) ;
                        g_object_unref( ((GdkEventKey*)new_event)->window ) ;
                        ((GdkEventKey *) new_event)->window = GDK_WINDOW(g_object_ref(G_OBJECT(GTK_WIDGET(m_SearchWindow->gobj())->window))) ;
                        gtk_widget_event(GTK_WIDGET(m_SearchEntry->gobj()), new_event) ;
                        gdk_event_free(new_event) ;

                        return true ;
                    }

                    continue_matching:

                    int step = 0 ; 
                    int origin = m_selection ? boost::get<1>(m_selection.get()) : 0 ;

                    switch( event->keyval )
                    {
                        case GDK_Delete:
                        {
                            if( m_selection )
                            {
                                std::size_t p = origin ;
                                m_selection.reset() ;
                                m_model->erase( p ) ;
                            }
                            return true ;
                        }

                        case GDK_Return:
                        case GDK_KP_Enter:
                        case GDK_ISO_Enter:
                        case GDK_3270_Enter:
                        {
                            if( m_search_active )
                            {
                                cancel_search() ;
                            }

                            if( m_selection )
                            {
                                using boost::get;

                                MPX::Track track = get<4>(*(get<0>(m_selection.get()))) ;
                                m_SIGNAL_track_activated.emit( track, !(event->state & GDK_CONTROL_MASK) ) ;
                            }

                            return true;
                        }

                        case GDK_Up:
                        case GDK_KP_Up:
                        case GDK_Page_Up:
                        {
                            if( event->keyval == GDK_Page_Up )
                            {
                                step = get_page_size() ; 
                            }
                            else
                            {
                                step = 1 ;
                            }

                            if( event->state & GDK_SHIFT_MASK )
                            {
                                if( m_Model_I.in( origin - step ))
                                {
                                    m_model->swap( origin, origin-step ) ;
                                    m_selection = boost::make_tuple(m_model->m_mapping[origin-step], origin-step) ;
                                }
                        
                                return true ;
                            }

                            if( !m_selection || !get_row_is_visible( origin ))
                            {
                                select_row( get_upper_row() ) ;
                            }
                            else
                            {
                                std::size_t row = ((origin-step) < 0 ) ? 0 : (origin-step) ;

                                if( row < get_upper_row() ) 
                                {
                                    m_prop_vadj.get_value()->set_value( std::max<int>( int(get_upper_row()) - step, row )) ; 
                                }
    
                                select_row( row ) ;
                            }

                            return true;
                        }

                        case GDK_Down:
                        case GDK_KP_Down:
                        case GDK_Page_Down:
                        {
                            if( event->keyval == GDK_Page_Down )
                            {
                                step = get_page_size() ; 
                            }
                            else
                            {
                                step = 1 ;
                            }

                            if( event->state & GDK_SHIFT_MASK )
                            {
                                if( m_Model_I.in( origin + step ))
                                {
                                    m_model->swap( origin, origin+step ) ;
                                    m_selection = boost::make_tuple(m_model->m_mapping[origin+step], origin+step) ;
                                }
                        
                                return true ;
                            }

                            if( !m_selection || !get_row_is_visible( origin ))
                            {
                                select_row( get_upper_row() ) ;
                            }
                            else
                            {
                                std::size_t row = std::min<std::size_t>( origin+step, m_model->size()-1 ) ;

                                if( row >= get_lower_row() ) 
                                {
                                    m_prop_vadj.get_value()->set_value( std::min<std::size_t>(get_upper_row()+step, row )) ; 
                                }

                                select_row( row ) ;
                            }

                            return true;
                        }

                        default:

                            if( !m_search_active )
                            {
                                int x, y, x_root, y_root ;

                                dynamic_cast<Gtk::Window*>(get_toplevel())->get_position( x_root, y_root ) ;

                                x = x_root + get_allocation().get_x() ;
                                y = y_root + get_allocation().get_y() + get_allocation().get_height() ;

                                m_SearchWindow->set_size_request( m_columns[0]->get_width(), - 1 ) ;
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
                        grab_focus() ;

                        int x = event->x ;

                        if( event->y < (m_row_height+4))
                        {
                            int p = 32 ;

                            for( std::size_t n = 0; n < m_columns.size() ; ++n )
                            {
                                int w = m_columns[n]->get_width() ;

                                if( (x >= p) && (x <= p + w) && !m_fixed.count(n) )
                                {
                                    column_set_collapsed( n, !m_collapsed.count( n ) ) ;
                                    break ;
                                }

                                p += w ;
                            }
                            return true;
                        }

                        Limiter<int64_t> row ( 
                              Limiter<int64_t>::ABS_ABS
                            , 0
                            , m_model->size() - 1
                            , get_upper_row() + (event->y-m_row_start) / m_row_height
                        ) ;

                        if( x >= 32 )
                        {
                            m_selection     = boost::make_tuple( m_model->m_mapping[row], row ) ;
                            m_clicked_row   = row ;
                            m_clicked       = true ;
                            queue_draw() ;
                        }
                        else
                        if( x >= 16 )
                        {
                            /* reserved for future use for clicking on the hover 'play' icon */
                        }
                        else
                        {
                            gint64 id = get<3>( m_model->row( row )) ;

                            if( m_terminal_track && id == m_terminal_track.get() )
                            {
                                m_terminal_track.reset() ;
                            }
                            else
                            {
                                m_terminal_track = id ;
                            }
                
                            queue_draw() ;
                        }
                    }
                    else
                    if( event->type == GDK_2BUTTON_PRESS )
                    {
                        Limiter<int64_t> row ( 
                              Limiter<int64_t>::ABS_ABS
                            , 0
                            , m_model->m_mapping.size()
                            , get_upper_row() + (event->y-m_row_start) / m_row_height
                        ) ;

                        Interval<std::size_t> i (
                              Interval<std::size_t>::IN_EX
                            , 0
                            , m_model->size()
                        ) ;

                        if( i.in( row )) 
                        {
                            MPX::Track track = get<4>(m_model->row(row)) ;
                            m_SIGNAL_track_activated.emit( track, true ) ;
                        }
                    }
                
                    return true;
                }

                bool
                on_button_release_event (GdkEventButton * event)
                {
                    m_clicked = false ;
                    return true ;
                }

                bool
                on_leave_notify_event(
                    GdkEventCrossing* G_GNUC_UNUSED
                )
                {
                    m_hover_track.reset() ;
                    queue_draw () ;

                    return true ;
                }

                bool
                on_motion_notify_event(
                    GdkEventMotion* event
                )
                {
                    int x_orig, y_orig;
                    GdkModifierType state;

                    if (event->is_hint)
                    {
                        gdk_window_get_pointer( event->window, &x_orig, &y_orig, &state ) ;
                    }
                    else
                    {
                        x_orig = int( event->x ) ;
                        y_orig = int( event->y ) ;
                        state  = GdkModifierType( event->state ) ;
                    }

                    gint64 row = get_upper_row() + ( y_orig - m_row_start ) / m_row_height ;

                    if( !m_clicked )
                    {
                            if( m_Model_I.in( row ) && y_orig >= m_row_start ) 
                            {
                                m_hover_track = row ;
                                queue_draw_area( 0, m_row_start, 32, get_allocation().get_height() - m_row_start ) ;
                            }
                            else
                            {
                                m_hover_track.reset() ;
                                queue_draw_area( 0, m_row_start, 32, get_allocation().get_height() - m_row_start ) ;
                            }
                    }
                    else
                    {
                            if( row != m_clicked_row )
                            {
                                if( m_Model_I.in( row )) 
                                {
                                        m_model->swap( row, m_clicked_row ) ;
                                        m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                                        m_clicked_row = row ;
                                }
                            }
                    }

                    return true ;
                }

                bool
                on_configure_event(
                    GdkEventConfigure* event
                )        
                {
                    const double column_width_collapsed = 40. ;

                    m_visible_height = event->height - m_row_start ;

                    if( m_visible_height && m_row_height && m_prop_vadj.get_value() )
                    {
                        m_prop_vadj.get_value()->set_upper( m_model->size() ) ; 
                        m_prop_vadj.get_value()->set_page_size( get_page_size() ) ;
                    }

                    int width = event->width - 32 ;

                    double column_width_calculated = (double(width) - double(m_fixed_total_width) - double(column_width_collapsed*double(m_collapsed.size()))) / (m_columns.size() - m_collapsed.size() - m_fixed.size()) ;

                    for( std::size_t n = 0; n < m_columns.size(); ++n )
                    {
                        if( !m_fixed.count( n ) )
                        {
                            m_columns[n]->set_width(m_collapsed.count( n ) ? column_width_collapsed : column_width_calculated ) ; 
                        }
                    }

                    return false;
                }

                inline bool
                compare_id_to_optional(
                      const Row_t&                      row
                    , const boost::optional<gint64>&    id
                )
                {
                    if( id && id.get() == boost::get<3>( row ))
                        return true ;

                    return false ;
                }

                template <typename T>
                inline bool
                compare_val_to_optional(
                      const T&                          val
                    , const boost::optional<T>&         cmp
                )
                {
                    if( cmp && cmp.get() == val ) 
                        return true ;

                    return false ;
                }


                bool
                on_expose_event (GdkEventExpose *event)
                {
                    const Gtk::Allocation& a = get_allocation();

                    boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

                    const ThemeColor& c_text        = theme->get_color( THEME_COLOR_TEXT ) ;
                    const ThemeColor& c_text_sel    = theme->get_color( THEME_COLOR_TEXT_SELECTED ) ;
                    const ThemeColor& c_rules_hint  = theme->get_color( THEME_COLOR_BASE_ALTERNATE ) ;

                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context(); 

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;

                    std::size_t row = get_upper_row() ;

                    int col     = 0 ;
                    int cnt     = get_page_size() ; 

                    int xpos    = 0 ;
                    int ypos    = m_row_start ;

                    const int inner_pad = 1 ;

                    if( event->area.y <= m_row_start )
                    {
                            xpos = 32 ;

                            for( Columns::iterator i = m_columns.begin(); i != m_columns.end(); ++i, ++col )
                            {
                                (*i)->render_header(
                                    cairo
                                  , *this
                                  , xpos
                                  , 0
                                  , m_row_start
                                  , col
                                  , c_text
                                ) ;

                                xpos += (*i)->get_width() ; 
                            }
                    }

                    cairo->set_operator(Cairo::OPERATOR_OVER);

                    //// ROWS

                    if( event->area.width > 32 )
                    {
                            while( m_model->is_set() && cnt && m_Model_I.in( row ) ) 
                            {
                                if( !(row % 2) ) 
                                {
                                    GdkRectangle r ;

                                    r.x       = 32 + inner_pad ;
                                    r.y       = ypos + inner_pad ;
                                    r.width   = a.get_width() - 2 * inner_pad - 32 ;
                                    r.height  = m_row_height - 2 * inner_pad ;

                                    RoundedRectangle(
                                          cairo
                                        , r.x
                                        , r.y
                                        , r.width
                                        , r.height
                                        , rounding
                                    ) ;

                                    cairo->set_source_rgba(
                                          c_rules_hint.r 
                                        , c_rules_hint.g
                                        , c_rules_hint.b 
                                        , c_rules_hint.a
                                    ) ;

                                    cairo->fill() ;
                                }

                                ypos += m_row_height ;
                                row  ++ ;
                                cnt  -- ;
                            }

                            boost::optional<std::size_t> row_select ;

                            if( m_selection )
                            {
                                Interval<std::size_t> i (
                                      Interval<std::size_t>::IN_IN
                                    , get_upper_row()
                                    , get_upper_row() + get_page_size()
                                ) ;

                                std::size_t row = boost::get<1>(m_selection.get()) ; 
                                row_select = row ;

                                if( i.in( row ) )
                                {
                                        GdkRectangle r ;

                                        r.x         = 32 + inner_pad ; 
                                        r.y         = inner_pad + (row - get_upper_row()) * m_row_height + m_row_start ;
                                        r.width     = a.get_width() - 2*inner_pad - 32 ; 
                                        r.height    = m_row_height  - 2*inner_pad ;

                                        theme->draw_selection_rectangle(
                                              cairo
                                            , r
                                            , has_focus()
                                        ) ;
                                    }
                            }

                            row     = get_upper_row() ;

                            col     = 0 ;
                            cnt     = get_page_size() ; 

                            ypos    = m_row_start ;

                            while( m_model->is_set() && cnt && m_Model_I.in( row ) ) 
                            {
                                xpos = 32 ;

                                for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
                                {
                                    (*i)->render(
                                          cairo
                                        , *this
                                        , m_model->row(row)
                                        , row
                                        , xpos
                                        , ypos
                                        , m_row_height
                                        , (row_select && (row == row_select.get())) ? c_text_sel : c_text
                                    ) ;

                                    xpos += (*i)->get_width() ; 
                                }

                                ypos += m_row_height ;
                                row  ++ ;
                                cnt  -- ;
                            }

                            if( true /*m_render_dashes*/)
                            {
                                std::valarray<double> dashes ( 3 ) ;
                                dashes[0] = 0. ;
                                dashes[1] = 3. ;
                                dashes[2] = 0. ;

                                xpos = 32 ;

                                cairo->save() ;

                                const ThemeColor& c_treelines = theme->get_color( THEME_COLOR_TREELINES ) ;

                                cairo->set_line_width(
                                      .3
                                ) ;
                                cairo->set_dash(
                                      dashes
                                    , 0
                                ) ;

                                Columns::iterator i2 = m_columns.end() ;
                                --i2 ;

                                for( Columns::const_iterator i = m_columns.begin(); i != i2; ++i )
                                {
                                    xpos += (*i)->get_width() ; 

                                    cairo->set_source_rgba(
                                          c_treelines.r
                                        , c_treelines.g
                                        , c_treelines.b
                                        , c_treelines.a
                                    ) ;

                                    cairo->move_to(
                                          xpos
                                        , 0 
                                    ) ; 

                                    cairo->line_to(
                                          xpos
                                        , a.get_height()
                                    ) ;

                                    cairo->stroke() ;
                                }

                                cairo->restore(); 
                            }
                    }
    
                    //// ICONS

                    const int icon_lateral = 16 ;

                    ypos    = m_row_start ;
                    cnt     = get_page_size() ; 
                    row     = get_upper_row() ;

                    while( m_model->is_set() && cnt && m_Model_I.in( row ) )
                    {
                        const Row_t& r_data = m_model->row( row ) ;

                        enum Skip
                        {
                              SKIP_SKIP_NONE
                            , SKIP_SKIP_PLAY        = 1 << 0
                            , SKIP_SKIP_TERMINAL    = 1 << 1
                        } ;

                        enum Icon
                        {
                              ICON_PLAY
                            , ICON_TERMINAL
                        } ;

                        int skip = SKIP_SKIP_NONE ;

                        const int icon_y = ypos + (m_row_height - icon_lateral) / 2 ;

                        const int icon_x[2] = { 16, 0 } ;

                        if( compare_id_to_optional( r_data, m_active_track )) 
                        {
                            Gdk::Cairo::set_source_pixbuf(
                                  cairo
                                , m_pb_play_l
                                , icon_x[ICON_PLAY]
                                , icon_y 
                            ) ;
                            cairo->rectangle(
                                  icon_x[ICON_PLAY]
                                , icon_y 
                                , icon_lateral
                                , icon_lateral
                            ) ;
                            cairo->fill () ;

                            skip &= SKIP_SKIP_PLAY ;
                        }

                        if( compare_id_to_optional( r_data, m_terminal_track )) 
                        {    
                            Gdk::Cairo::set_source_pixbuf(
                                  cairo
                                , m_pb_terminal
                                , icon_x[ICON_TERMINAL]
                                , icon_y 
                            ) ;
                            cairo->rectangle(
                                  icon_x[ICON_TERMINAL] 
                                , icon_y 
                                , icon_lateral
                                , icon_lateral
                            ) ;
                            cairo->fill () ;

                            skip &= SKIP_SKIP_TERMINAL ; 
                        }

                        if( compare_val_to_optional<gint64>( row, m_hover_track )) 
                        {
                            if( !( skip & SKIP_SKIP_PLAY))
                            {
                                Gdk::Cairo::set_source_pixbuf(
                                      cairo
                                    , m_pb_hover_l
                                    , icon_x[ICON_PLAY]
                                    , icon_y 
                                ) ;
                                cairo->rectangle(
                                      icon_x[ICON_PLAY] 
                                    , icon_y 
                                    , icon_lateral
                                    , icon_lateral
                                ) ;
                                cairo->fill () ;
                            }

/*
                            if( !( skip & SKIP_SKIP_TERMINAL))
                            {
                                Gdk::Cairo::set_source_pixbuf(
                                      cairo
                                    , m_pb_terminal
                                    , icon_x[ICON_TERMINAL]
                                    , icon_y 
                                ) ;
                                cairo->rectangle(
                                      icon_x[ICON_TERMINAL] 
                                    , icon_y 
                                    , icon_lateral
                                    , icon_lateral
                                ) ;
                                cairo->clip () ;
                                cairo->paint_with_alpha( 0.5 ) ;
                                cairo->reset_clip() ;
                            }
*/
                        }

                        ypos += m_row_height ;
                        row  ++ ;
                        cnt  -- ;
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
                        m_Model_I = Interval<std::size_t> (
                                 Interval<std::size_t>::IN_EX
                                , 0
                                , m_model->size()
                        ) ;

                        if( m_prop_vadj.get_value() && m_visible_height && m_row_height )
                        {
                            m_prop_vadj.get_value()->set_upper( m_model->size() ) ; 
                            m_prop_vadj.get_value()->set_page_size( get_page_size() ) ;

                            scroll_to_row( position ) ;
                        }

                        m_selection.reset() ;
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

                bool
                query_tooltip(
                      int                                   tooltip_x
                    , int                                   tooltip_y
                    , bool                                  keypress
                    , const Glib::RefPtr<Gtk::Tooltip>&     tooltip
                )
                {
                    std::size_t row = (double( tooltip_y ) - m_row_start) / double(m_row_height) ;

                    MPX::Track track = boost::get<4>( m_model->row(row) ) ;

                    boost::shared_ptr<Covers> covers = services->get<Covers>("mpx-service-covers") ;
                    Glib::RefPtr<Gdk::Pixbuf> cover ;

                    const std::string& mbid = boost::get<std::string>( track[ATTRIBUTE_MB_ALBUM_ID].get() ) ;

                    Gtk::Image * image = Gtk::manage( new Gtk::Image ) ;

                    if( covers->fetch(
                          mbid
                        , cover
                    ))
                    {   
                        image->set( cover ) ;
                        tooltip->set_custom( *image ) ;
                        return true ;
                    }

                    return false ;
                }

            public:

                inline void
                set_terminal_id(
                    gint64    id
                )
                {
                    m_terminal_track = id ; 
                    queue_draw() ;
                }

                inline boost::optional<gint64>
                get_terminal_id(
                )
                {
                    return m_terminal_track ; 
                }

                inline void
                clear_terminal_id(
                )
                {
                    m_terminal_track.reset() ;
                    queue_draw() ;
                }
    
                inline std::size_t
                get_page_size(
                )
                {
                    if( m_visible_height && m_row_height )
                        return m_visible_height / m_row_height ; 
                    else
                        return 0 ;
                }

                inline std::size_t
                get_upper_row(
                )
                {
                    return m_prop_vadj.get_value()->get_value() ;
                }

                inline std::size_t
                get_lower_row(
                )
                {
                    return m_prop_vadj.get_value()->get_value() + get_page_size() ;
                }

                inline bool
                get_row_is_visible(
                      std::size_t   row
                )
                {
                    std::size_t up = get_upper_row() ;

                    Interval<std::size_t> i (
                          Interval<std::size_t>::IN_IN
                        , up 
                        , up + get_page_size()
                    ) ;
            
                    return i.in( row ) ;
                }

                void
                set_highlight(bool highlight)
                {
                    m_highlight = highlight;
                    queue_draw ();
                }

                void
                set_model(DataModelFilter_SP_t model)
                {
                    if( m_model )
                    {
                        boost::optional<gint64> active_track = m_model->m_active_track ;
                        m_model = model;
                        m_model->m_active_track = active_track ;
                        m_model->scan_active() ;
                    }
                    else
                    {
                        m_model = model;
                    }

                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &Class::on_model_changed
                    ));
                }

                void
                append_column (Column_SP_t column)
                {
                    m_columns.push_back(column) ;
                }

                SignalTrackActivated&
                signal_track_activated()
                {
                    return m_SIGNAL_track_activated ;
                }

                SignalVAdjChanged&
                signal_vadj_changed()
                {
                    return m_SIGNAL_vadj_changed ;
                }

                SignalFindAccepted&
                signal_find_accepted()
                {
                    return m_SIGNAL_find_accepted ;
                }

                SignalFindPropagate&
                signal_find_propagate()
                {
                    return m_SIGNAL_find_propagate ;
                }

                void
                clear_active_track()
                {
                    m_active_track.reset() ;
                    m_model->clear_active_track() ;
                }

                void
                set_active_track(gint64 id)
                {
                    m_active_track = id ;
                    m_model->set_active_track( id ) ;
                    queue_draw () ;
                }

                boost::optional<gint64>
                get_local_active_track ()
                {
                    return m_model->m_local_active_track ;
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
                    }
                    else
                    {
                        m_collapsed.erase( column ) ;
                        queue_resize () ;
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
                    }
                    else
                    {
                        m_fixed.erase( column ) ;
                        m_fixed_total_width -= m_columns[column]->get_width() ; 
                        queue_resize () ;
                    }
                }

                void
                scroll_to_id(
                      gint64 id
                )
                {
                    for( DataModelFilter::RowRowMapping_t::iterator i = m_model->m_mapping.begin() ; i != m_model->m_mapping.end(); ++i )
                    {
                        const Row_t& row = **i ;
                        if( boost::get<3>(row) == id )
                        {
                            Limiter<std::size_t> d ( 
                                  Limiter<std::size_t>::ABS_ABS
                                , 0
                                , m_model->m_mapping.size() - get_page_size()
                                , std::distance( m_model->m_mapping.begin(), i )
                            ) ;

                            m_prop_vadj.get_value()->set_value( d ) ; 
                            break ;
                        }
                    } 
                }

                void
                scroll_to_row(
                      std::size_t row
                )
                {
                    if( m_visible_height && m_row_height )
                    {
                        Limiter<std::size_t> d ( 
                              Limiter<std::size_t>::ABS_ABS
                            , 0
                            , m_model->m_mapping.size() - get_page_size()
                            , row 
                        ) ;

                        if( m_model->m_mapping.size() < get_page_size()) 
                            m_prop_vadj.get_value()->set_value( 0 ) ; 
                        else
                        if( row > (m_model->m_mapping.size() - get_page_size()) )
                            m_prop_vadj.get_value()->set_value( m_model->m_mapping.size() - get_page_size() ) ; 
                        else
                            m_prop_vadj.get_value()->set_value( d ) ; 
                    }
                }

                void
                select_row(
                      std::size_t row
                )
                {
                    Interval<std::size_t> i (
                          Interval<std::size_t>::IN_EX
                        , 0
                        , m_model->size()
                    ) ;

                    if( i.in( row ))
                    {
                        m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                        queue_draw() ;
                    }
                }

                void
                clear_selection(
                )
                {
                    m_selection.reset() ;
                    queue_draw() ;
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

                    DataModelFilter::RowRowMapping_t::iterator i = m_model->m_mapping.begin(); 

                    if( m_selection )
                    {
                        std::advance( i, get<1>(m_selection.get()) ) ;
                        ++i ;
                    }

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        Glib::ustring match = Glib::ustring(get<0>(**i)).casefold() ;

                        if( match.length() && match.substr( 0, text.length()) == text.substr( 0, text.length()) )
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

                    DataModelFilter::RowRowMapping_t::iterator i = m_model->m_mapping.begin(); 

                    if( m_selection )
                    {
                        std::advance( i, get<1>(m_selection.get()) ) ;
                        --i ; 
                    }

                    for( ; i >= m_model->m_mapping.begin(); --i )
                    {
                        Glib::ustring match = Glib::ustring(get<0>(**i)).casefold() ;

                        if( match.length() && match.substr( 0, text.length()) == text.substr( 0, text.length()) )
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

                    DataModelFilter::RowRowMapping_t::iterator i = m_model->m_mapping.begin(); 
                
                    if( m_selection )
                    {
                        std::advance( i, get<1>(m_selection.get()) ) ;
                    }

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        Glib::ustring match = Glib::ustring(get<0>(**i)).casefold() ;

                        if( match.length() && match.substr( 0, text.length()) == text.substr( 0, text.length()) )
                        {
                            std::size_t d = std::distance( m_model->m_mapping.begin(), i ) ; 
                            scroll_to_row( d ) ; 
                            select_row( d ) ;
                            return ;
                        }
                    }

                    clear_selection() ;
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
                on_search_button_clicked(
                )
                {
                    std::string text = m_SearchEntry->get_text() ;
                    cancel_search() ;

                    m_SIGNAL_find_propagate.emit( text ) ;
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

                        : ObjectBase( "YoukiClassTracks" )
//                        , sigx::glib_auto_dispatchable()
//                        , m_inserter( 0 )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_dnd( false )
                        , m_clicked_row( 0 ) 
                        , m_clicked( false )
                        , m_highlight( false )
                        , m_fixed_total_width( 0 )
                        , m_search_active( false )

                {
                    boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
                    const ThemeColor& c = theme->get_color( THEME_COLOR_BASE ) ;
                    Gdk::Color cgdk ;
                    cgdk.set_rgb_p( c.r, c.g, c.b ) ; 
                    modify_bg( Gtk::STATE_NORMAL, cgdk ) ;
                    modify_base( Gtk::STATE_NORMAL, cgdk ) ;

                    m_pb_play_l  = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "row-play.png" )) ;
                    m_pb_hover_l = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "row-hover.png" )) ;
                    m_pb_terminal = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "icons" G_DIR_SEPARATOR_S "hicolor" G_DIR_SEPARATOR_S "16x16" G_DIR_SEPARATOR_S "stock" G_DIR_SEPARATOR_S "deadend.png" )) ;

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

                    /*
                    signal_query_tooltip().connect(
                        sigc::mem_fun(
                              *this
                            , &Class::query_tooltip
                    )) ;

                    set_has_tooltip( true ) ;
                    */

                    m_SearchEntry = Gtk::manage( new Gtk::Entry ) ;
                    gtk_widget_realize( GTK_WIDGET(m_SearchEntry->gobj()) ) ;
                    m_SearchEntry->show() ;

                    m_SearchHBox = Gtk::manage( new Gtk::HBox ) ;
                    m_SearchButton = Gtk::manage( new Gtk::Button ) ;
                    m_SearchButton->signal_clicked().connect(
                        sigc::mem_fun(
                              *this
                            , &Class::on_search_button_clicked
                    )) ;

                    Gtk::Image * img = Gtk::manage( new Gtk::Image ) ;
                    img->set( Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU ) ;
                    img->show() ;
    
                    m_SearchButton->set_image( *img ) ;

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

                    signal_key_press_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &Class::key_press_event
                    ), true ) ;

                    m_SearchHBox->pack_start( *m_SearchEntry, true, true, 0 ) ;
                    m_SearchHBox->pack_start( *m_SearchButton, true, true, 0 ) ;

                    m_SearchWindow->add( *m_SearchHBox ) ;
                    m_SearchHBox->show_all() ;

                    property_can_focus() = true ;
                }

                virtual ~Class ()
                {
                }
        };
}
}
}

#endif // _YOUKI_TRACK_LIST_HH
