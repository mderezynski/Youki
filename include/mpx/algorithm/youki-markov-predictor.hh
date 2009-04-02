#ifndef MPX_MARKOV_PREDICTOR
#define MPX_MARKOV_PREDICTOR

#include <glibmm.h>
#include <glib.h>
#include <boost/lexical_cast.hpp>
#include "mpx/algorithm/ntree.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-library.hh"

namespace MPX
{
    struct GlyphData
    {
        double      Intensity ;
        gunichar    Char ;
    } ;

    class MarkovTypingPredictor
    {
        private:

                NTree<GlyphData>      m_tree ;

                Library             * m_library ;

        public:

                MarkovTypingPredictor()
                {
                    m_library = services->get<Library>("mpx-service-library").get() ;

/*

                    bool exists = m_library->get_sql_db()->table_exists( "markov_predictor_node_chain" ) ;

                    m_library->execSQL( "CREATE TABLE IF NOT EXISTS markov_predictor_node_chain (id INTEGER PRIMARY KEY AUTOINCREMENT, parent INTEGER DEFAULT NULL)" ) ;
                    m_library->execSQL( "CREATE TABLE IF NOT EXISTS markov_predictor_node_data  (id INTEGER, char STRING, intensity INTEGER)" ) ;

                    if( !exists )
*/
                    {
                        SQL::RowV v ;
                        m_library->getSQL( v, "SELECT album_artist FROM album_artist ORDER BY album_artist" ) ; 

                        for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                        {
                            process_string(
                                boost::get<std::string>((*i)["album_artist"])
                            ) ;
                        }
                    }
/*
                    else
                    {
                        restore_node( m_tree.Root, 1 ) ;
                    }
*/
                }

                void
                restore_node(
                      const NTree<GlyphData>::Node_SP_t&    node 
                    , int64_t                               id_cur 
                )
                {
                    SQL::RowV v ;
                    m_library->getSQL(v, (
                        boost::format("SELECT * FROM markov_predictor_node_chain WHERE parent = '%lld'")
                            % id_cur
                    ).str()) ; // get all the node's children

                    if( v.empty() )
                        return ;

                    for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i ) 
                    {
                        int64_t id = boost::get<gint64>((*i)["id"]) ;
                        NTree<GlyphData>::Node_SP_t node_new = new NTree<GlyphData>::Node ;

                        SQL::RowV v2 ;
                        m_library->getSQL(v2, (
                            boost::format("SELECT char, intensity FROM markov_predictor_node_data WHERE id = '%lld'")
                                % id
                        ).str() ) ; 

                        if( !v2.empty() )
                        {
                            gunichar c ;    

                            if( v2[0]["char"].which() == 0 )
                            {
                                c = Glib::ustring(boost::lexical_cast<std::string>(boost::get<gint64>(v2[0]["char"])))[0] ;
                            }
                            else
                            {
                                c = Glib::ustring(boost::get<std::string>(v2[0]["char"]))[0] ;
                            }

                            node_new->Data.Char = c ; 
                            node_new->Data.Intensity = boost::get<gint64>(v2[0]["intensity"]) ;
                        }

                        node->append( node_new ) ;

                        g_message( "Restoring node with parent: %lld and node id: %lld", id_cur, id ) ;
                        restore_node( node_new, id ) ;
                    }
                }

                void
                store_node(
                      const NTree<GlyphData>::Node_SP_t&    node 
                    , int64_t                               parent 
                )
                {
                    for( NTree<GlyphData>::Children_t::iterator n = node->Children.begin(); n != node->Children.end(); ++n )
                    {
                        int64_t id = m_library->execSQL((
                            boost::format("INSERT INTO markov_predictor_node_chain (parent) VALUES ('%lld')")
                                % parent
                        ).str()) ; 

                        m_library->execSQL(
                            mprintf("INSERT INTO markov_predictor_node_data (id, char, intensity) VALUES ('%lld', '%q', '%d')"
                                , id
                                , Glib::ustring (1, (*n)->Data.Char).c_str()
                                , int((*n)->Data.Intensity)
                        )) ;

                        store_node( *n, id ) ; 
                    }
                }

                virtual ~MarkovTypingPredictor()
                {
/*
                    boost::shared_ptr<Library> lib = services->get<Library>("mpx-service-library") ;

                    m_library->execSQL( "DELETE FROM markov_predictor_node_chain" ) ;
                    m_library->execSQL( "DELETE FROM markov_predictor_node_data" ) ;

                    m_library->execSQL((boost::format("INSERT INTO markov_predictor_node_chain (id, parent) VALUES ('%lld', NULL)")
                            % int64_t(1) 
                    ).str()) ; 

                    store_node( m_tree.Root, 1 ) ;
*/
                }

                void
                process_string(     
                    const Glib::ustring& u
                )
                {
                    NTree<GlyphData>::Node_SP_t root = m_tree.Root ;
                    NTree<GlyphData>::Node_SP_t curr = root ;

                    for( Glib::ustring::const_iterator i = u.begin(); i != u.end(); ++i )
                    {
                        int found = 0 ;

                        for( NTree<GlyphData>::Children_t::iterator n = curr->Children.begin(); n != curr->Children.end(); ++n )
                        {       
                            GlyphData & data = (*n)->Data ;

                            if( g_unichar_tolower(data.Char) == g_unichar_tolower(*i) )
                            {
                                data.Intensity++ ; 
                                curr = *n ;
                                found = 1 ; 
                                break ;
                            }
                        }

                        if( !found )
                        {
                            GlyphData new_glyph_data ;
                            new_glyph_data.Intensity = 0 ;
                            new_glyph_data.Char = *i ; 
                            NTree<GlyphData>::Node_SP_t n =  new NTree<GlyphData>::Node( new_glyph_data ) ;
                            curr->append( n ) ; 
                            curr = n ;
                        }
                    }
                }
            
                Glib::ustring
                predict(     
                      const Glib::ustring&  u
                )
                {
                    Glib::ustring prediction ;

                    NTree<GlyphData>::Node_SP_t root = m_tree.Root ;
                    NTree<GlyphData>::Node_SP_t curr = root ;

                    for( Glib::ustring::const_iterator i = u.begin(); i != u.end(); ++i )
                    {
                        for( NTree<GlyphData>::Children_t::iterator n = curr->Children.begin(); n != curr->Children.end(); ++n )
                        {       
                            GlyphData & data = (*n)->Data ;

                            if( g_unichar_tolower(data.Char) == g_unichar_tolower(*i) )
                            {
                                curr = *n ;
                                goto continue_loop_in_case_of_found ;
                            }
                        }

                        // this string can not be predicted within the current tree, we reached the end of nodes, but not the end of text
                        return prediction ;

                        continue_loop_in_case_of_found: ;
                    }

                    while( curr && !curr->Children.empty() )
                    {
                        double Intensity_Max = 0 ;
                        NTree<GlyphData>::Node_SP_t Most_Intense_Node  = 0 ;

                        for( NTree<GlyphData>::Children_t::iterator n = curr->Children.begin(); n != curr->Children.end(); ++n )
                        {       
                            GlyphData & data = (*n)->Data ;

                            double Intensity = data.Intensity / curr->Children.size() ;
                           
                            if( Intensity > Intensity_Max ) 
                            {
                                Intensity_Max = Intensity ;
                                Most_Intense_Node = *n ;
                            }
                        }

                        if( Most_Intense_Node )
                        {
                            prediction += Most_Intense_Node->Data.Char ;
                            curr = Most_Intense_Node ;
                        }
                        else
                        if( !curr->Children.empty() )
                        {
                            curr = *(curr->Children.begin()) ;
                            prediction += curr->Data.Char ;
                        }
                    }
                }
    } ;
}

#endif // YOUKI_MARKOV_PREDICTOR
