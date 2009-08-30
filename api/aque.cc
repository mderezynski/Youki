#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include <vector>
#include <glib.h>
#include "mpx/util-string.hh"
#include "mpx/algorithm/aque.hh"

namespace
{
    using namespace MPX ;
    using namespace MPX::AQE ;

    void
    add_constraint(
          Constraints_t&        constraints
        , const std::string&    attribute
        , const std::string&    value
        , MatchType_t           type
        , bool                  inverse_match
    )
    {
        if( type != MT_UNDEFINED )
        {
            Constraint_t c ;
            c.MatchType = type ;
            c.InverseMatch = inverse_match ;

            if( attribute == "musicip-puid" )
            {
                c.TargetAttr = ATTRIBUTE_MUSICIP_PUID ;
                c.TargetValue = value ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "album-mbid" )
            {
                c.TargetAttr = ATTRIBUTE_MB_ALBUM_ID ;
                c.TargetValue = value ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "album-artist-mbid" )
            {
                c.TargetAttr = ATTRIBUTE_MB_ALBUM_ARTIST_ID ;
                c.TargetValue = value ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "artist-mbid" )
            {
                c.TargetAttr = ATTRIBUTE_MB_ARTIST_ID ;
                c.TargetValue = value ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "country" )
            {
                c.TargetAttr = ATTRIBUTE_MB_RELEASE_COUNTRY ;
                c.TargetValue = value ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "type" )
            {
                c.TargetAttr = ATTRIBUTE_MB_RELEASE_TYPE ;
                c.TargetValue = value ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "bitrate" )
            {
                try{
                        c.TargetValue = gint64(boost::lexical_cast<int>(value)) ;
                        c.TargetAttr = ATTRIBUTE_BITRATE ;
                        c.MatchType = type ;
                        constraints.push_back(c) ;
                } catch( boost::bad_lexical_cast ) {
                }
            }
            else
            if( attribute == "year" )
            {
                try{
                        c.TargetValue = gint64(boost::lexical_cast<int>(value)) ;
                        c.TargetAttr = ATTRIBUTE_DATE ;
                        c.MatchType = type ;
                        constraints.push_back(c) ;
                } catch( boost::bad_lexical_cast ) {
                }
            }
            else
            if( attribute == "quality" )
            {
                c.TargetValue = gint64(boost::lexical_cast<int>(value)) ;
                c.TargetAttr = ATTRIBUTE_QUALITY ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "genre" )
            {
                c.TargetValue = value ; 
                c.TargetAttr = ATTRIBUTE_GENRE ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "artist" )
            {
                c.TargetValue = value ; 
                c.TargetAttr = ATTRIBUTE_ARTIST ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "album-artist" )
            {
                c.TargetValue = value ;
                c.TargetAttr = ATTRIBUTE_ALBUM_ARTIST ;
                constraints.push_back(c) ;
            }
            else
            if( attribute == "album" )
            {
                c.TargetValue = value ; 
                c.TargetAttr = ATTRIBUTE_ALBUM ;
                constraints.push_back(c) ;
            }
            if( attribute == "title" )
            {
                c.TargetValue = value ; 
                c.TargetAttr = ATTRIBUTE_TITLE ;
                constraints.push_back(c) ;
            }
        }
    }
}

namespace MPX
{
namespace AQE
{
    Glib::ustring
    parse_advanced_query(
          Constraints_t&            constraints
        , const std::string&        text
    )
    {
        typedef std::vector< std::string > Vector_t ;

        Vector_t non_attr_strings;

        Glib::ustring tmp ;

        Glib::ustring attribute ;
        Glib::ustring value ;

        MatchType_t type ;
        bool quote_open = false ;
        bool inverse = false ;
        bool done_reading_pair  = false ;
        
        enum ReadType_t
        {
              READ_ATTR
            , READ_VAL
        } ;

        ReadType_t rt = READ_ATTR ;

        Glib::ustring text_utf8 ( text ) ;
        Glib::ustring::iterator i = text_utf8.begin() ;

        while( i != text_utf8.end() )
        {
            if( *i == '<' )
            {
                type = MT_LESSER_THAN ;
                rt = READ_VAL ;
            }
            else
            if( *i == '>' )
            {
                type = MT_GREATER_THAN ;
                rt = READ_VAL ;
            }
            else
            if( *i == '=' )
            {
                type = MT_EQUAL ;
                rt = READ_VAL ;
            }
            else
            if( *i == '~' )
            {
                type = MT_FUZZY_EQUAL ;
                rt = READ_VAL ;
            }
            else
            if( *i == '%' )
            {
                type = MT_EQUAL_BEGIN ;
                rt = READ_VAL ;
            }
            else
            if( *i == '!' )
            {
                if( rt == READ_ATTR && tmp.empty() )
                {
                    inverse = true ;
                }
                else
                {
                    tmp += *i ;
                }
            }
            else
            if( *i == '"' )
            {
                if( rt == READ_ATTR )
                {
                    // we interpret this as the start of the attribute and use MT_EQUAL
                    type = MT_EQUAL ;

                    attribute = tmp ;
                    tmp.clear() ;
                    rt = READ_VAL ;

                    quote_open = true ;
                }
                else
                if( !quote_open )
                {
                    quote_open = true ;
                }
                else
                {
                    quote_open = false ;

                    value = tmp ;
                    tmp.clear() ;

                    done_reading_pair = true ;
                }
            }
            else
            if( *i == ' ' ) 
            {
                if( quote_open )
                {
                    tmp += *i ;
                }
                else
                {
                    if( rt == READ_ATTR )
                    {
                        if( !tmp.empty() )
                        {
                            attribute = tmp ;
                        }

                        tmp.clear() ;
                    }
                    else
                    {
                        value = tmp ;
                        tmp.clear() ;

                        done_reading_pair = true ;
                    }
                }
            }
            else
            {
                if( tmp.empty() && !attribute.empty() && rt == READ_ATTR )
                {
                    non_attr_strings.push_back( attribute ) ;
                    attribute.clear() ;
                }

                tmp += *i ;
            }

            ++i ;

            if( i == text_utf8.end() && !done_reading_pair )
            {
                if( quote_open )
                {
                    tmp += *i ;

                    quote_open = false ;
                    done_reading_pair = true ;
                }
                else
                {
                    if( rt == READ_ATTR )
                    {
                        non_attr_strings.push_back( attribute ) ;
                        attribute.clear() ;
                    }
                    else
                    {
                        value = tmp ;
                        tmp.clear() ;

                        done_reading_pair = true ;
                    }
                }
            }

            if( done_reading_pair )
            {
                if( !attribute.empty() && !value.empty() )
                {
                    add_constraint(
                          constraints
                        , attribute
                        , value
                        , type
                        , inverse
                    ) ;

                    type = MT_UNDEFINED ;
                    attribute.clear() ;
                    value.clear() ;
                    tmp.clear() ;
                    done_reading_pair = false ;
                    inverse = false ;
                    rt = READ_ATTR ;
                }
            }
        }

        return Glib::ustring(boost::algorithm::join( non_attr_strings, " ")).lowercase() ;
    }

    template <typename T>
    bool
    determine_match(
          const Constraint_t&   c
        , const MPX::Track&     track
    )
    {
        g_return_val_if_fail(track.has(c.TargetAttr), false) ;

        bool truthvalue = false ;

        switch( c.MatchType )
        {
            case MT_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) == boost::get<T>(c.TargetValue.get()) ;
                break  ;

            case MT_NOT_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) != boost::get<T>(c.TargetValue.get()) ;
                break  ;

            case MT_GREATER_THAN:
                truthvalue = boost::get<T>(track[c.TargetAttr].get())  > boost::get<T>(c.TargetValue.get()) ;
                break  ;

            case MT_LESSER_THAN:
                truthvalue = boost::get<T>(track[c.TargetAttr].get())  < boost::get<T>(c.TargetValue.get()) ;
                break  ;

            case MT_GREATER_THAN_OR_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) >= boost::get<T>(c.TargetValue.get()) ;
                break  ;

            case MT_LESSER_THAN_OR_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) <= boost::get<T>(c.TargetValue.get()) ;
                break  ;

            default:
                truthvalue = false  ;
                break  ;
        }

        return (c.InverseMatch) ? !truthvalue : truthvalue ;
    }

    template <>
    bool
    determine_match<std::string>(
          const Constraint_t&   c
        , const MPX::Track&     track
    )
    {
        g_return_val_if_fail(track.has(c.TargetAttr), false) ;

        bool truthvalue = false ;

        switch( c.MatchType )
        {
            case MT_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) == boost::get<std::string>(c.TargetValue.get()) ;
                break  ;

            case MT_NOT_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) != boost::get<std::string>(c.TargetValue.get()) ;
                break  ;

            case MT_GREATER_THAN:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get())  > boost::get<std::string>(c.TargetValue.get()) ;
                break  ;

            case MT_LESSER_THAN:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get())  < boost::get<std::string>(c.TargetValue.get()) ;
                break  ;

            case MT_GREATER_THAN_OR_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) >= boost::get<std::string>(c.TargetValue.get()) ;
                break  ;

            case MT_LESSER_THAN_OR_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) <= boost::get<std::string>(c.TargetValue.get()) ;
                break  ;

            case MT_FUZZY_EQUAL:
                truthvalue = Util::match_keys(boost::get<std::string>(track[c.TargetAttr].get()), boost::get<std::string>(c.TargetValue.get())) ;
                break  ;

            case MT_EQUAL_BEGIN:

                {
                    Glib::ustring m = Glib::ustring(boost::get<std::string>(track[c.TargetAttr].get())).lowercase() ;
                    Glib::ustring v = Glib::ustring(boost::get<std::string>(c.TargetValue.get())).lowercase() ;

                    truthvalue = (!m.empty() && !v.empty()) && m.substr( 0, v.length()) == v ;
                }

                break  ;
        
            case MT_UNDEFINED:
            default:
                truthvalue = false  ;
                break  ;
        }

        return (c.InverseMatch) ? !truthvalue : truthvalue ;
    }


    bool
    match_track( const Constraints_t& c, const MPX::Track& track)
    {
        for( Constraints_t::const_iterator i = c.begin(); i != c.end(); ++i )
        {
            Constraint_t const& c = *i ;

            if( !track.has(c.TargetAttr) )
            {
                return false ;
            }
        
            bool truthvalue; 

            if( c.TargetAttr >= ATTRIBUTE_TRACK )
                truthvalue = determine_match<gint64>(c, track) ;
            else
                truthvalue = determine_match<std::string>(c, track) ;

            if( !truthvalue )
                return false ;
        }

        return true ;
    }
}
}
