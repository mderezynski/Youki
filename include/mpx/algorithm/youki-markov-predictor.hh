#ifndef MPX_MARKOV_PREDICTOR
#define MPX_MARKOV_PREDICTOR

#include <glibmm.h>
#include <glib.h>
#include "mpx/algorithm/ntree.hh"

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

                boost::shared_ptr<NTree<GlyphData> > m_tree ;

        public:

                MarkovTypingPredictor()
                : m_tree( new NTree<GlyphData> )
                {
                }

                virtual ~MarkovTypingPredictor()
                {
                }

                void
                process_string(     
                    const Glib::ustring& u
                )
                {
                    NTree<GlyphData>::Node_SP_t root = m_tree->Root ;
                    NTree<GlyphData>::Node_SP_t curr = root ;

                    for( Glib::ustring::const_iterator i = u.begin(); i != u.end(); ++i )
                    {
                        int found = 0 ;

                        for( NTree<GlyphData>::Children_t::iterator n = curr->Children.begin(); n != curr->Children.end(); ++n )
                        {       
                            GlyphData & data = (*n)->Data ;

                            if( data.Char == *i )
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
                            NTree<GlyphData>::Node_SP_t n ( new NTree<GlyphData>::Node( new_glyph_data ) ) ;
                            curr->append( n ) ; 
                            curr = n ;
                        }
                    }
                }
            
                Glib::ustring
                predict(     
                      const Glib::ustring&  u
                    , std::size_t           max_predict_chars 
                )
                {
                    Glib::ustring prediction ;

                    NTree<GlyphData>::Node_SP_t root = m_tree->Root ;
                    NTree<GlyphData>::Node_SP_t curr = root ;

                    for( Glib::ustring::const_iterator i = u.begin(); i != u.end(); ++i )
                    {
                        for( NTree<GlyphData>::Children_t::iterator n = curr->Children.begin(); n != curr->Children.end(); ++n )
                        {       
                            GlyphData & data = (*n)->Data ;

                            if( data.Char == *i )
                            {
                                curr = *n ;
                                goto continue_loop_in_case_of_found ;
                            }
                        }

                        // this string can not be predicted within the current tree, we reached the end of nodes, but not the end of text
                        return prediction ;

                        continue_loop_in_case_of_found: ;
                    }

                    while( ! curr->Children.empty() )
                    {
                        double Intensity_Max = 0 ;
                        NTree<GlyphData>::Node_SP_t Most_Intense_Node ;

                        for( NTree<GlyphData>::Children_t::iterator n = curr->Children.begin(); n != curr->Children.end(); ++n )
                        {       
                            GlyphData & data = (*n)->Data ;
                           
                            if( data.Intensity > Intensity_Max ) 
                            {
                                Intensity_Max = data.Intensity ;
                                Most_Intense_Node = *n ;
                            }
                        }

                        if( Most_Intense_Node )
                        {
                            prediction += Most_Intense_Node->Data.Char ;
                        }
                        else
                        {
                            // uh-oh
                        }

                        curr = Most_Intense_Node ;
                    }
                }
    } ;
}

#endif // YOUKI_MARKOV_PREDICTOR
