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

                NTree<GlyphData> m_tree ;

        public:

                MarkovTypingPredictor()
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
                    NTree<GlyphData>::Node_SP_t root = m_tree.Root ;
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

                    while( curr && !curr->Children.empty() )
                    {
                        double Intensity_Max = 0 ;
                        NTree<GlyphData>::Node_SP_t Most_Intense_Node ;

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
