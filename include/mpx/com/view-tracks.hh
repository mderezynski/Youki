#ifndef _YOUKI_TRACK_LIST_HH
#define _YOUKI_TRACK_LIST_HH

#include <gtkmm.h>
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
    const double rounding = 4. ; 
}

namespace MPX
{
        typedef boost::tuple<std::string, std::string, std::string, gint64, MPX::Track, gint64, gint64> Row7 ;
        typedef std::vector<Row7>                                                                       Model_t ;
        typedef boost::shared_ptr<Model_t>                                                              Model_SP_t ;
        typedef sigc::signal<void, gint64>                                                              Signal0 ;
        typedef std::map<gint64, Model_t::iterator>                                                     IdIterMap_t ;

        struct DataModelTracks : public sigc::trackable
        {
                Model_SP_t      m_realmodel;
                Signal0         m_changed;
                IdIterMap_t     m_iter_map;
                std::size_t     m_position ;

                DataModelTracks()
                : m_position( 0 )
                {
                    m_realmodel = Model_SP_t(new Model_t); 
                }

                DataModelTracks(Model_SP_t model)
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

                virtual Signal0&
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

                virtual Row7&
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
                append_track(SQL::Row & r, const MPX::Track& track)
                {
                    using boost::get ;

                    std::string artist, album, title ;
                    gint64 id = 0, num = 0, artist_id = 0 ;

                    if(r.count("id"))
                    { 
                        id = get<gint64>(r["id"]) ;
                    }
                    else
                        g_critical("%s: No id for track, extremely suspicious", G_STRLOC) ;

                    if(r.count("artist_sortname"))
                        artist = get<std::string>(r["artist_sortname"]) ;
                    else
                    if(r.count("artist"))
                        artist = get<std::string>(r["artist"]) ;

                    if(r.count("album"))
                        album = get<std::string>(r["album"]) ;
                    if(r.count("title"))
                        title = get<std::string>(r["title"]) ;

                    if(r.count("track"))
                    { 
                        num = get<gint64>(r["track"]) ;
                    }

                    if(r.count("mpx_album_artist_id"))
                    { 
                        artist_id = get<gint64>(r["mpx_album_artist_id"]) ;
                    }

                    Row7 row ( title, artist, album, id, track, num, artist_id ) ;
                    m_realmodel->push_back(row) ;

                    Model_t::iterator i = m_realmodel->end() ;
                    std::advance( i, -1 ) ;
                    m_iter_map.insert(std::make_pair(id, i)) ; 
                }

                virtual void
                append_track(MPX::Track & track)
                {
                    using boost::get ;

                    std::string artist, album, title ;
                    gint64 id = 0, num = 0, artist_id = 0 ;

                    if(track[ATTRIBUTE_MPX_TRACK_ID])
                        id = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) ; 
                    else
                        g_critical("Warning, no id given for track; this is totally wrong and should never happen.") ;


                    if(track[ATTRIBUTE_ARTIST_SORTNAME])
                        artist = get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()) ;
                    else
                    if(track[ATTRIBUTE_ARTIST])
                        artist = get<std::string>(track[ATTRIBUTE_ARTIST].get()) ;

                    if(track[ATTRIBUTE_ALBUM])
                        album = get<std::string>(track[ATTRIBUTE_ALBUM].get()) ;

                    if(track[ATTRIBUTE_TITLE])
                        title = get<std::string>(track[ATTRIBUTE_TITLE].get()) ;

                    if(track[ATTRIBUTE_TRACK])
                        num = get<gint64>(track[ATTRIBUTE_TRACK].get()) ;

                    if(track[ATTRIBUTE_MPX_ALBUM_ARTIST_ID])
                        artist_id = get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ARTIST_ID].get()) ;

                    Row7 row ( artist, album, title, id, track, num, artist_id ) ;
                    m_realmodel->push_back(row) ;

                    Model_t::iterator i = m_realmodel->end() ;
                    std::advance( i, -1 ) ;
                    m_iter_map.insert(std::make_pair(id, i)) ; 
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

        typedef boost::shared_ptr<DataModelTracks> DataModelTracks_SP_t;

        struct DataModelFilterTracks : public DataModelTracks
        {
                typedef std::vector<Model_t::iterator> RowRowMapping;

                RowRowMapping                           m_mapping ;
                std::string                             m_filter_full ;
                std::string                             m_filter_effective ;
                AQE::Constraints_t                      m_constraints ;
                AQE::Constraints_t                      m_constraints_synthetic ;
                bool                                    m_advanced ;
                boost::optional<gint64>                 m_active_track ;
                boost::optional<gint64>                 m_local_active_track ;
                boost::optional<std::set<gint64> >      m_constraint_albums ;
                boost::optional<std::set<gint64> >      m_constraint_artist ;

                typedef std::map<std::string, RowRowMapping>                MappingCache ; 
                typedef std::map<std::string, std::set<Model_t::iterator> > FragmentCache ;

                MappingCache    m_mapping_cache ;
                FragmentCache   m_fragment_cache ;

                DataModelFilterTracks(DataModelTracks_SP_t & model)
                : DataModelTracks(model->m_realmodel)
                , m_advanced(false)
                {
                    regen_mapping() ;
                }

                virtual ~DataModelFilterTracks()
                {
                }

                virtual void
                clear ()
                {
                    m_realmodel->clear () ;
                    m_mapping.clear() ;
                    m_iter_map.clear() ;
                    clear_active_track() ;
                    m_changed.emit( 0 ) ;
                } 

                void
                clear_active_track()
                {
                    m_local_active_track.reset() ;
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
                    m_constraints_synthetic.push_back( c ) ;    
                    regen_mapping() ;
                }

                void
                add_synthetic_constraint_quiet(
                    const AQE::Constraint_t& c
                )
                {
                    m_constraints_synthetic.push_back( c ) ;    
                }

                virtual void
                clear_synthetic_constraints(
                )
                {
                    m_constraints_synthetic.clear() ;
                    regen_mapping () ;
                }

                virtual void
                clear_synthetic_constraints_quiet(
                )
                {
                    m_constraints_synthetic.clear() ;
                    scan_active() ;
                }

                virtual std::size_t 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row7&
                row (std::size_t row)
                {
                    return *(m_mapping[row]);
                }

                void
                swap( std::size_t p1, std::size_t p2 )
                {
                    std::swap( m_mapping[p1], m_mapping[p2] ) ;
                    scan_active() ;
                    m_changed.emit( m_position ) ;
                }

                void
                erase( std::size_t p )
                {
                    RowRowMapping::iterator i = m_mapping.begin() ;
                    std::advance( i, p ) ;
                    m_mapping.erase( i ) ;
                    scan_active() ;
                    m_changed.emit( m_position ) ;
                }

                virtual void
                append_track(MPX::Track& track)
                {
                    DataModelTracks::append_track(track);
                    regen_mapping();
                }
                
                virtual void
                append_track(SQL::Row& r, const MPX::Track& track)
                {
                    DataModelTracks::append_track(r, track);
                    regen_mapping();
                }

                virtual void
                append_track_quiet(MPX::Track& track)
                {
                    DataModelTracks::append_track(track);
                }

                virtual void
                append_track_quiet(SQL::Row& r, const MPX::Track& track)
                {
                    DataModelTracks::append_track(r, track);
                }

                void
                erase_track(gint64 id)
                {
                    DataModelTracks::erase_track( id );
                    regen_mapping();
                }

                virtual void
                set_filter(
                    const std::string& text
                )
                { 
                    using boost::get ;

                    if(!m_filter_full.empty() && (text.substr(0, text.size()-1) == m_filter_full) && !m_advanced)
                    {
                        m_filter_full = text ; 
                        m_filter_effective = text ;

                        regen_mapping_iterative() ;
                    }
                    else
                    {
                        m_filter_full = text; 

                        if( m_advanced )
                        {
                            m_constraints.clear();
                            m_filter_effective = AQE::parse_advanced_query( m_constraints, text ) ;
                        }
                        else
                        {
                            m_filter_effective = m_filter_full;
                        }

                        regen_mapping();
                    }
                }

                virtual void
                set_advanced (bool advanced)
                {
                    m_advanced = advanced;
                    set_filter(m_filter_full);
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

                    for( RowRowMapping::iterator i = m_mapping.begin(); i != m_mapping.end(); ++i )
                    {
                        if( m_active_track && get<3>(*(*i)) == m_active_track.get() )
                        {
                            m_local_active_track = std::distance( m_mapping.begin(), i ) ; 
                            return ;
                        }
                    }

                    m_local_active_track.reset() ;
                }

                virtual void
                regen_mapping(
                )
                {
                    typedef std::set<Model_t::iterator>  RowSet_t ;
                    typedef std::vector<RowSet_t>        CacheVec_t ;

                    using boost::get;
                    using boost::algorithm::split;
                    using boost::algorithm::is_any_of;
                    using boost::algorithm::find_first;

                    RowRowMapping   new_mapping ;
                    std::string     text        = Glib::ustring( m_filter_effective ).lowercase().c_str() ;
                    gint64          id          = ( m_position < m_mapping.size()) ? get<3>(row( m_position )) : -1 ; 

                    m_position = 0 ;

                    if( text.empty() )
                    {
                        m_constraint_albums.reset() ;
                        m_constraint_artist.reset() ;

                        for( Model_t::iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i )
                        {
                            int truth = m_constraints.empty() && m_constraints_synthetic.empty() ; 

                            const MPX::Track& track = get<4>(*i);

                            if( !m_constraints.empty() )
                                truth |= AQE::match_track( m_constraints, track ) ;

                            if( !m_constraints_synthetic.empty() )
                                truth |= AQE::match_track( m_constraints_synthetic, track ) ;

                            if( truth )
                            {
                                new_mapping.push_back( i ) ;
                            }

                            if( id >= 0 && get<3>(*i) == id )
                            {
                                m_position = new_mapping.size()  - 1 ;
                            }
                        }
                    }
                    else
                    {
                        StrV m;
                        split( m, text, is_any_of(" ") );

                        CacheVec_t m_cachevec ; 

                        for( std::size_t n = 0 ; n < m.size(); ++n )
                        {
                            if( !m[n].length() )
                            {
                                continue ;
                            }

                            FragmentCache::iterator it = m_fragment_cache.find( m[n] ) ;
                            if( it != m_fragment_cache.end() )
                            {
                                m_cachevec.push_back( (*it).second ) ;
                                continue ;
                            }

                            m_cachevec.resize( m_cachevec.size() + 1 ) ; 
                            for( Model_t::iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i )
                            {
                                const Row7& row = *i;

                                std::vector<std::string> vec (3) ;
                                vec[0] = Glib::ustring(boost::get<0>(row)).lowercase().c_str() ;
                                vec[1] = Glib::ustring(boost::get<1>(row)).lowercase().c_str() ;
                                vec[2] = Glib::ustring(boost::get<2>(row)).lowercase().c_str() ;

                                if( Util::match_vec( m[n], vec) )
                                {
                                    m_cachevec[n].insert( i ) ; 
                                }
                            }

                            m_fragment_cache.insert( std::make_pair( m[n], m_cachevec[n] )) ;
                        }

                        RowSet_t output  ;
                        for( std::size_t n = 0 ; n < m_cachevec.size() ; ++n )
                        {
                            if( n == 0 ) 
                            {
                                output = m_cachevec[n] ; 
                            }
                            else
                            {
                                RowSet_t output_tmp ;
                                const RowSet_t& s = m_cachevec[n] ;

                                for( RowSet_t::const_iterator i = s.begin(); i != s.end(); ++i )
                                {
                                    if( output.count( *i ) )
                                    {
                                        output_tmp.insert( *i ) ;
                                    }
                                }

                                std::swap( output, output_tmp ) ;
                            }
                        }

                        m_constraint_albums = std::set<gint64>() ;
                        m_constraint_artist = std::set<gint64>() ;

                        for( RowSet_t::iterator i = output.begin() ; i != output.end(); ++i )
                        {
                            int truth = m_constraints.empty() && m_constraints_synthetic.empty() ; 

                            const MPX::Track& track = get<4>(*(*i));

                            if( !m_constraints.empty() )
                                truth |= AQE::match_track( m_constraints, track ) ;

                            if( !m_constraints_synthetic.empty() )
                                truth |= AQE::match_track( m_constraints_synthetic, track ) ;

                            if( truth )
                            {
                                new_mapping.push_back( *i ) ;
                            }

                            m_constraint_albums.get().insert( get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ID].get()) ) ;
                            m_constraint_artist.get().insert( get<6>(*(*i)) ) ;
                        }
                    }

                    if( new_mapping != m_mapping )
                    {
                        std::swap( new_mapping, m_mapping ) ;
                        scan_active () ;
                        m_changed.emit( m_position ) ;
                    }                
                }

                void
                regen_mapping_iterative(
                )
                {
                    typedef std::set<Model_t::iterator>  RowSet_t ;
                    typedef std::vector<RowSet_t>       CacheVec_t ;

                    using boost::get;
                    using boost::algorithm::split;
                    using boost::algorithm::is_any_of;
                    using boost::algorithm::find_first;

                    RowRowMapping   new_mapping ;
                    std::string     text        = Glib::ustring( m_filter_effective ).lowercase().c_str() ;
                    gint64          id          = ( m_position < m_mapping.size()) ? get<3>(row( m_position )) : -1 ; 

                    m_position  = 0 ;

                    if( text.empty() )
                    {
                        m_constraint_albums.reset() ;
                        m_constraint_artist.reset() ;

                        for( RowRowMapping::iterator i = m_mapping.begin(); i != m_mapping.end(); ++i )
                        {
                            int truth = m_constraints.empty() && m_constraints_synthetic.empty() ; 

                            const MPX::Track& track = get<4>(*(*i));

                            if( !m_constraints.empty() )
                                truth |= AQE::match_track( m_constraints, track ) ;

                            if( !m_constraints_synthetic.empty() )
                                truth |= AQE::match_track( m_constraints_synthetic, track ) ;

                            if( truth )
                            {
                                new_mapping.push_back( *i ) ;
                            }

                            if( id >= 0 && get<3>(**i) == id )
                            {
                                m_position = new_mapping.size()  - 1 ;
                            }
                        }
                    }
                    else
                    {
                        StrV m;
                        split( m, text, is_any_of(" ") );

                        CacheVec_t m_cachevec ; 

                        for( std::size_t n = 0 ; n < m.size(); ++n )
                        {
                            if( !m[n].length() )
                            {
                                continue ;
                            }

                            FragmentCache::iterator it = m_fragment_cache.find( m[n] ) ;
                            if( it != m_fragment_cache.end() )
                            {
                                m_cachevec.push_back( (*it).second ) ;
                                continue ;
                            }

                            m_cachevec.resize( m_cachevec.size() + 1 ) ; 
                            for( RowRowMapping::iterator i = m_mapping.begin(); i != m_mapping.end(); ++i )
                            {
                                const Row7& row = *(*i);

                                std::vector<std::string> vec (3) ;
                                vec[0] = Glib::ustring(boost::get<0>(row)).lowercase().c_str() ;
                                vec[1] = Glib::ustring(boost::get<1>(row)).lowercase().c_str() ;
                                vec[2] = Glib::ustring(boost::get<2>(row)).lowercase().c_str() ;

                                if( Util::match_vec( m[n], vec) )
                                {
                                    m_cachevec[n].insert( *i ) ; 
                                }
                            }

                            //m_fragment_cache.insert( std::make_pair( m[n], m_cachevec[n] )) ;
                        }

                        RowSet_t output  ;
                        for( std::size_t n = 0 ; n < m_cachevec.size() ; ++n )
                        {
                            if( n == 0 ) 
                            {
                                output = m_cachevec[n] ; 
                            }
                            else
                            {
                                RowSet_t output_tmp ;
                                const RowSet_t& s = m_cachevec[n] ;

                                for( RowSet_t::const_iterator i = s.begin(); i != s.end(); ++i )
                                {
                                    if( output.count( *i ) )
                                    {
                                        output_tmp.insert( *i ) ;
                                    }
                                }

                                std::swap( output, output_tmp ) ;
                            }
                        }

                        m_constraint_albums = std::set<gint64>() ;
                        m_constraint_artist = std::set<gint64>() ;

                        for( RowSet_t::iterator i = output.begin() ; i != output.end(); ++i )
                        {
                            int truth = m_constraints.empty() && m_constraints_synthetic.empty() ; 

                            const MPX::Track& track = get<4>(*(*i));

                            if( !m_constraints.empty() )
                                truth |= AQE::match_track( m_constraints, track ) ;

                            if( !m_constraints_synthetic.empty() )
                                truth |= AQE::match_track( m_constraints_synthetic, track ) ;

                            if( truth )
                            {
                                new_mapping.push_back( *i ) ;
                            }

                            m_constraint_albums.get().insert( get<gint64>(track[ATTRIBUTE_MPX_ALBUM_ID].get()) ) ;
                            m_constraint_artist.get().insert( get<6>(*(*i)) ) ;
                        }
                    }

                    if( new_mapping != m_mapping )
                    {
                        std::swap( new_mapping, m_mapping ) ;
                        scan_active () ;
                        m_changed.emit( m_position ) ;
                    }
                }
        };

        typedef boost::shared_ptr<DataModelFilterTracks> DataModelFilterTracks_SP_t;

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
                render_header(Cairo::RefPtr<Cairo::Context> cairo, Gtk::Widget& widget, int xpos, int ypos, int rowheight, int column)
                {
                    using boost::get;

                    int off = (column == 0) ? 16 : 0 ;

                    cairo->rectangle(
                          xpos + off + 5
                        , ypos + 6
                        , m_width
                        , rowheight
                    ) ;
                    cairo->clip() ;

                    cairo->move_to(
                          xpos + off + 5
                        , ypos + 6
                    ) ;
                    cairo->set_operator(Cairo::OPERATOR_ATOP);
                    cairo->set_source_rgba( 1., 1., 1., 1. ) ;

                    Glib::RefPtr<Pango::Layout> layout = widget.create_pango_layout(""); 

                    layout->set_markup(
                          (boost::format("<b>%s</b>") % Glib::Markup::escape_text(m_title).c_str()).str()
                    ) ;

                    layout->set_ellipsize(
                          Pango::ELLIPSIZE_END
                    ) ;

                    layout->set_width(
                          (m_width-off-10)*PANGO_SCALE
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
                    Cairo::RefPtr<Cairo::Context>   cairo,
                    Row7 const&                     datarow,
                    std::string const&              filter,
                    Gtk::Widget&                    widget,
                    int                             row,
                    int                             xpos,
                    int                             ypos,
                    int                             rowheight,
                    bool                            selected,
                    bool                            highlight = true
                )
                {
                    using boost::get;

                    cairo->set_operator(Cairo::OPERATOR_ATOP);
                    cairo->set_source_rgba( 1., 1., 1., 1. ) ;

                    int off = (m_column == 0) ? 16 : 0 ;

                    cairo->rectangle(
                          xpos + off
                        , ypos
                        , m_width - off
                        , rowheight
                    ) ;
                    cairo->clip();
                    cairo->move_to(
                          xpos + off + 6
                        , ypos + 4
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

                    if( highlight )
                    {
                        layout = widget.create_pango_layout("") ;
                        layout->set_markup( Util::text_match_highlight( str, filter, "#ff3030") ) ;
                    }
                    else
                    {
                        layout = widget.create_pango_layout( str );
                    }

                    layout->set_ellipsize(
                          Pango::ELLIPSIZE_END
                    ) ;

                    layout->set_width(
                          (m_width - off - 12) * PANGO_SCALE
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

        typedef boost::shared_ptr<Column> ColumnP;
        typedef std::vector<ColumnP> Columns;

        typedef sigc::signal<void, MPX::Track, bool> SignalTrackActivated;

        class ListViewTracks : public Gtk::DrawingArea
        {
            public:

                DataModelFilterTracks_SP_t                    m_model ;

            private:

                int                                 m_row_height ;
                int                                 m_row_start ;
                int                                 m_visible_height ;
                gint64                              m_previous_drawn_row ;

                Columns                             m_columns ;

                PropAdj                             m_prop_vadj ;
                PropAdj                             m_prop_hadj ;

                guint                               m_signal0 ; 

                boost::optional<boost::tuple<Model_t::iterator, std::size_t> >  m_selection ;

                IdV                                 m_dnd_idv ;
                bool                                m_dnd ;
                gint64                              m_clicked_row ;
                bool                                m_clicked ;
                bool                                m_highlight ;

                boost::optional<gint64>             m_active_track ;
                boost::optional<gint64>             m_hover_track ;

                Glib::RefPtr<Gdk::Pixbuf>           m_playing_pixbuf ;
                Glib::RefPtr<Gdk::Pixbuf>           m_hover_pixbuf ;

                std::set<int>                       m_collapsed ;
                std::set<int>                       m_fixed ;
                gint64                              m_fixed_total_width ;
        
                SignalTrackActivated                m_SIGNAL_track_activated;

                GtkWidget                         * m_treeview ;
                Gtk::Entry                        * m_SearchEntry ;
                Gtk::Window                       * m_SearchWindow ;

                sigc::connection                    m_search_changed_conn ; 

                bool                                m_search_active ;

                void
                initialize_metrics ()
                {
                    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (gobj()));

                    PangoFontMetrics *metrics = pango_context_get_metrics (context,
                                                                            GTK_WIDGET (gobj())->style->font_desc, 
                                                                            pango_context_get_language (context));

                    m_row_height = (pango_font_metrics_get_ascent (metrics)/PANGO_SCALE) + 
                                   (pango_font_metrics_get_descent (metrics)/PANGO_SCALE) + 8;

                    const int visible_area_pad = 2 ;

                    m_row_start = m_row_height + visible_area_pad ;
                }

                void
                on_vadj_value_changed ()
                {
                    if( m_model->size() )
                    {
                            std::size_t row = get_upper_row() ; 

                            m_model->set_current_row( row ) ;        

                            if( m_previous_drawn_row != row )
                            {
                                queue_draw ();
                            }
                    }
                }

            protected:

                virtual bool
                on_focus_in_event (GdkEventFocus* G_GNUC_UNUSED)
                {
                    if( !m_selection && m_model->m_mapping.size() )
                    {
                        std::size_t row = get_upper_row();
                        m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                    }

                    queue_draw() ;

                    return false ;
                }

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    int step; 

                    if( m_search_active )
                    {
                        switch( event->keyval )
                        {
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

                        return false ;
                    }

                    continue_matching:

                    switch( event->keyval )
                    {
                        case GDK_Delete:
                            if( m_selection )
                            {
                                std::size_t p = get<1>(m_selection.get()) ;
                                m_selection.reset() ;
                                m_model->erase( p ) ;
                            }
                            return true ;

                        case GDK_Return:
                        case GDK_KP_Enter:
                        case GDK_ISO_Enter:
                        case GDK_3270_Enter:
                            if( m_selection )
                            {
                                using boost::get;

                                MPX::Track track = get<4>(*(get<0>(m_selection.get()))) ;
                                m_SIGNAL_track_activated.emit(track, !(event->state & GDK_CONTROL_MASK)) ;
                                m_selection.reset() ;
                                queue_draw () ;
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
                                step = -1;
                            }

                            if( !m_selection )
                            {
                                mark_first_row_up:
                                std::size_t row = get_upper_row();
                                m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                            }
                            else
                            {
                                if( get_row_is_visible( boost::get<1>(m_selection.get()) ))
                                {
                                    std::size_t row = boost::get<1>(m_selection.get());
                                    row = std::max( 0, int(row) + int(step) ) ;

                                    m_selection = (boost::make_tuple(m_model->m_mapping[row], row));

                                    if( row < get_upper_row() ) 
                                    {
                                        double value = m_prop_vadj.get_value()->get_value();
                                        value += step*m_row_height;
                                        m_prop_vadj.get_value()->set_value( value );
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
                                step = (m_visible_height / m_row_height) ; 
                            }
                            else
                            {
                                step = 1;
                            }

                            if( !m_selection )
                            {
                                mark_first_row_down:

                                std::size_t row = get_upper_row();
                                m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                            }
                            else
                            {
                                if( get_row_is_visible( boost::get<1>(m_selection.get()) ))
                                {
                                    std::size_t row = boost::get<1>(m_selection.get());
                                    row = std::min( int(m_model->m_mapping.size()) - 1 , int(row) + int(step) ) ;

                                    m_selection = (boost::make_tuple(m_model->m_mapping[row], row));

                                    if( row >= (get_upper_row() + (m_visible_height/m_row_height)))
                                    {
                                        double value = m_prop_vadj.get_value()->get_value();
                                        value += step*m_row_height;
                                        m_prop_vadj.get_value()->set_value( value );
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
                                    int x, y, x_root, y_root ;

                                    dynamic_cast<Gtk::Window*>(get_toplevel())->get_position( x_root, y_root ) ;

                                    x = x_root + get_allocation().get_x() ;
                                    y = y_root + get_allocation().get_y() + get_allocation().get_height() ;

                                    m_SearchWindow->set_size_request( m_columns[0]->get_width(), -1 ) ;
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

                        if( event->y < (m_row_height+4))
                        {
                            int x = event->x ;
                            int p = 0 ;
                            for( std::size_t n = 0; n < m_columns.size() ; ++n )
                            {
                                int w = m_columns[n]->get_width() ;
                                if( (x >= p) && (x <= p + w) && !m_fixed.count(n))
                                {
                                    column_set_collapsed( n, !m_collapsed.count( n ) ) ;
                                    break ;
                                }
                                p += w ;
                            }
                            return true;
                        }
    
                        std::size_t row = get_upper_row() + ((int(event->y)-(m_row_start)) / m_row_height);

                        m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                        m_clicked_row = row ;
                        m_clicked = true ;
                        queue_draw() ;
                    }
                    else
                    if( event->type == GDK_2BUTTON_PRESS )
                    {
                        std::size_t row = get_upper_row() + ((event->y-m_row_start) / m_row_height);

                        if( row < m_model->size() )
                        {
                            MPX::Track track = get<4>(m_model->row(row)) ;
                            m_SIGNAL_track_activated.emit(track, true) ;
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
                            if( row < m_model->m_mapping.size() && (x_orig < m_columns[0]->get_width()) )
                            {
                                m_hover_track = row ;
                                queue_draw_area (0, m_row_start, 16, get_allocation().get_height() - m_row_start ) ;
                            }
                            else
                            {
                                m_hover_track.reset() ;
                                queue_draw_area (0, m_row_start, 16, get_allocation().get_height() - m_row_start ) ;
                            }
                    }
                    else
                    {
                            if( row != m_clicked_row )
                            {
                                if( row >= 0 && row < m_model->size() )
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
                    m_visible_height = event->height - m_row_start ;

                    m_prop_vadj.get_value()->set_upper( m_model->size() * m_row_height ) ;
                    m_prop_vadj.get_value()->set_page_size( (m_visible_height/m_row_height)*int(m_row_height) ) ;

                    const double column_width_collapsed = 40. ;

                    double n = m_columns.size() - m_collapsed.size() - m_fixed.size() ;
                    double column_width_calculated = (double(event->width) - double(m_fixed_total_width) - double(column_width_collapsed*double(m_collapsed.size()))) / n ; 

                    for( std::size_t n = 0; n < m_columns.size(); ++n )
                    {
                        if( !m_fixed.count( n ) )
                        {
                            m_columns[n]->set_width(m_collapsed.count( n ) ? column_width_collapsed : column_width_calculated ) ; 
                        }
                    }

                    return false;
                }

                bool
                on_expose_event (GdkEventExpose *event)
                {
                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context(); 
                    const Gtk::Allocation& alloc = get_allocation();

                    cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

                    std::size_t row = get_upper_row() ;
                    m_previous_drawn_row = row;

                    int ypos    = m_row_start ;
                    int xpos    = 0 ;
                    int col     = 0 ;
                    int cnt     = m_visible_height / m_row_height + 1 ; 

                    if( event->area.y <= m_row_start )
                    {
                            for( Columns::iterator i = m_columns.begin(); i != m_columns.end(); ++i, ++col )
                            {
                                (*i)->render_header(
                                    cairo
                                  , *this
                                  , xpos
                                  , 0
                                  , m_row_start
                                  , col
                                ) ;

                                xpos += (*i)->get_width() + 1 ;
                            }
                    }

                    cairo->set_operator(Cairo::OPERATOR_ATOP);
    
                    const int xpad = 16 ;

                    while( m_model->is_set() && cnt && (row < m_model->m_mapping.size()) ) 
                    {
                        const int inner_pad  = 1 ;

                        xpos = 0 ;

                        Model_t::iterator selected = m_model->m_mapping[row];

                        bool iter_is_selected = ( m_selection && boost::get<1>(m_selection.get()) == row ) ;

                        if( ! event->area.width <= 16 )
                        {
                                if( !(row % 2) ) 
                                {
                                    GdkRectangle r ;

                                    r.x       = xpad  + inner_pad ;
                                    r.y       = ypos  + inner_pad ;
                                    r.width   = alloc.get_width() - xpad - 2 * inner_pad ;
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
                                          0.2
                                        , 0.2
                                        , 0.2
                                        , 1.
                                    ) ;

                                    cairo->fill() ;
                                }

                                double factor = has_focus() ? 1. : 0.3 ;

                                if( iter_is_selected )
                                {
                                    Gdk::Color c = get_style()->get_base( Gtk::STATE_SELECTED ) ;

                                    GdkRectangle r ;

                                    r.x         = inner_pad + 16 ;
                                    r.y         = ypos + inner_pad ;
                                    r.width     = alloc.get_width() - 2*inner_pad - 16 ;  
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
                                        , rounding
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

                                for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
                                {
                                    (*i)->render(cairo, m_model->row(row), m_model->m_filter_effective, *this, row, xpos, ypos, m_row_height, iter_is_selected, m_highlight);
                                    xpos += (*i)->get_width() + 1;
                                }
                        }
                    
                        const int icon_lateral = 16 ;
                        const int icon_xorigin = 0 ;

                        if( m_active_track && boost::get<3>(m_model->row(row)) == m_active_track.get() )
                        {
                            const int icon_x = icon_xorigin ;
                            const int icon_y = ypos + (m_row_height - icon_lateral) / 2 ;

                            Gdk::Cairo::set_source_pixbuf(
                                  cairo
                                , m_playing_pixbuf
                                , icon_x
                                , icon_y 
                            ) ;

                            cairo->rectangle(
                                  icon_x
                                , icon_y 
                                , icon_lateral
                                , icon_lateral
                            ) ;

                            cairo->fill () ;
                        }
                        else
                        if( m_hover_track && row == m_hover_track.get() )
                        {
                            const int icon_x = icon_xorigin ;
                            const int icon_y = ypos + (m_row_height - icon_lateral) / 2 ;

                            Gdk::Cairo::set_source_pixbuf(
                                  cairo
                                , m_hover_pixbuf
                                , icon_x
                                , icon_y 
                            ) ;

                            cairo->rectangle(
                                  icon_x 
                                , icon_y 
                                , icon_lateral
                                , icon_lateral
                            ) ;

                            cairo->fill () ;
                        }

                        ypos += m_row_height;
                        row ++;
                        cnt --;
                    }

                    std::valarray<double> dashes ( 3 ) ;
                    dashes[0] = 0. ;
                    dashes[1] = 3. ;
                    dashes[2] = 0. ;

                    xpos = 0 ;

                    Columns::iterator i2 = m_columns.end() ;
                    --i2 ;

                    cairo->save() ;

                    for( Columns::const_iterator i = m_columns.begin(); i != i2; ++i )
                    {
                                xpos += (*i)->get_width() ; 

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
                                      xpos
                                    , 0 
                                ) ; 

                                cairo->line_to(
                                      xpos
                                    , alloc.get_height()
                                ) ;

                                cairo->stroke() ;
                    }

                    cairo->restore(); 

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
                on_model_changed( gint64 position )
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

                    m_selection.reset() ;
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

                            ListViewTracks & view = *(reinterpret_cast<ListViewTracks*>(data));

                            view.m_prop_vadj.get_value()->set_value(0.);
                            view.m_prop_vadj.get_value()->set_upper( view.m_model->size() * view.m_row_height ) ;
                            view.m_prop_vadj.get_value()->set_page_size( (view.m_visible_height/view.m_row_height)*int(view.m_row_height) ) ; 

                            view.m_prop_vadj.get_value()->signal_value_changed().connect(
                                sigc::mem_fun(
                                    view,
                                    &ListViewTracks::on_vadj_value_changed
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

                    MPX::Track track = boost::get<4>(m_model->row(row));

                    boost::shared_ptr<Covers> covers = services->get<Covers>("mpx-service-covers") ;
                    Glib::RefPtr<Gdk::Pixbuf> cover ;

                    const std::string& mbid = boost::get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()) ;

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
    
                std::size_t
                get_upper_row ()
                {
                    return double(m_prop_vadj.get_value()->get_value()) / double(m_row_height) ;
                }

                bool
                get_row_is_visible (std::size_t row)
                {
                    std::size_t row_upper = (m_prop_vadj.get_value()->get_value() / m_row_height); 
                    std::size_t row_lower = row_upper + m_visible_height/m_row_height;
                    return ( row >= row_upper && row <= row_lower);
                }

                void
                set_highlight(bool highlight)
                {
                    m_highlight = highlight;
                    queue_draw ();
                }

                void
                set_model(DataModelFilterTracks_SP_t model)
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
                            &ListViewTracks::on_model_changed
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
                    return m_SIGNAL_track_activated;
                }

                void
                set_advanced (bool advanced)
                {
                    m_model->set_advanced(advanced);
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

                void
                scroll_to_id(
                      gint64 id
                )
                {
                    for( DataModelFilterTracks::RowRowMapping::iterator i = m_model->m_mapping.begin() ; i != m_model->m_mapping.end(); ++i )
                    {
                        const Row7& row = **i ;
                        if( boost::get<3>(row) == id )
                        {
                            std::size_t d = std::distance( m_model->m_mapping.begin(), i ) ;
                            m_prop_vadj.get_value()->set_value( d * m_row_height );
                            break ;
                        }
                    } 
                }

        
                void
                select_row(
                      std::size_t row
                )
                {
                    m_selection = (boost::make_tuple(m_model->m_mapping[row], row));
                    queue_draw() ;
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

                    DataModelFilterTracks::RowRowMapping::iterator i = m_model->m_mapping.begin(); 
                    ++i ; // first row is "All" FIXME this sucks

                    for( ; i != m_model->m_mapping.end(); ++i )
                    {
                        const Row7& row = **i ;
                        Glib::ustring match = get<0>(row) ;

                        if( match.length() && match.substr( 0, std::min( text.length(), match.length())) == text.substr( 0, std::min( text.length(), match.length())) )   
                        {
                            int row = std::distance( m_model->m_mapping.begin(), i ) ; 
                            m_prop_vadj.get_value()->set_value( row * m_row_height ) ; 
                            select_row( row ) ;
                            break ;
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
                }

            public:

                ListViewTracks ()

                        : ObjectBase( "YoukiListViewTracksTracks" )
                        , m_previous_drawn_row( 0 )
                        , m_prop_vadj( *this, "vadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_prop_hadj( *this, "hadjustment", (Gtk::Adjustment*)( 0 ))
                        , m_dnd( false )
                        , m_clicked_row( 0 ) 
                        , m_clicked( false )
                        , m_highlight( false )
                        , m_fixed_total_width( 0 )
                        , m_search_active( false )

                {
                    Gdk::Color c ;
    
                    c.set_rgb_p( .10, .10, .10 ) ;

                    modify_bg( Gtk::STATE_NORMAL, c ) ;
                    modify_base( Gtk::STATE_NORMAL, c ) ;

                    m_playing_pixbuf = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "speaker.png" )) ;
                    m_hover_pixbuf = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "speaker_ghost.png" )) ;

                    m_treeview = gtk_tree_view_new();
                    gtk_widget_hide(GTK_WIDGET(m_treeview)) ;
                    gtk_widget_realize(GTK_WIDGET(m_treeview));

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

                    gtk_widget_realize(GTK_WIDGET(gobj()));
                    initialize_metrics();

                    /*
                    signal_query_tooltip().connect(
                        sigc::mem_fun(
                              *this
                            , &ListViewTracks::query_tooltip
                    )) ;

                    set_has_tooltip( true ) ;
                    */

                    m_SearchEntry = Gtk::manage( new Gtk::Entry ) ;
                    m_search_changed_conn = m_SearchEntry->signal_changed().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewTracks::on_search_entry_changed
                    )) ;
    
                    m_SearchWindow = new Gtk::Window( Gtk::WINDOW_POPUP ) ;
                    m_SearchWindow->set_decorated( false ) ;

                    m_SearchWindow->signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewTracks::on_search_window_focus_out
                    )) ;

                    signal_focus_out_event().connect(
                            sigc::mem_fun(
                                  *this
                                , &ListViewTracks::on_search_window_focus_out
                    )) ;

                    m_SearchWindow->add( *m_SearchEntry ) ;
                    m_SearchEntry->show() ;

                    property_can_focus() = true ;
                }

                virtual ~ListViewTracks ()
                {
                }
        };
}

#endif // _YOUKI_TRACK_LIST_HH
