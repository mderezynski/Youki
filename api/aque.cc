#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include <vector>
#include <glib.h>
#include "mpx/util-string.hh"
#include "mpx/algorithm/aque.hh"

namespace MPX
{
namespace AQE
{
    Glib::ustring
    parse_advanced_query (Constraints_t& constraints, const std::string& text)
    {
        typedef std::vector< std::string > VectorType;

        VectorType non_attr_strings;

        VectorType v;
        boost::algorithm::split( v, text, boost::algorithm::is_any_of(" ") );

        if( v.empty() )
        {
            v.push_back( text );
        }

        for( VectorType::const_iterator i = v.begin(); i != v.end(); ++i )
        {
            std::string const& token = *i;

            struct MatchData
            {
                char const* op;
                MatchType_t type;
            };

            const MatchData data[] =
            {
                  {"=", MT_EQUAL}
                , {">", MT_GREATER_THAN}
                , {"<", MT_LESSER_THAN}
            };

            MatchType_t type = MT_UNDEFINED;

            VectorType v2;

            for( unsigned n = 0; n < G_N_ELEMENTS(data); ++n )
            {
                v2.clear();
                boost::algorithm::split( v2, token, boost::algorithm::is_any_of(data[n].op) );
                if( v2.size() == 2) 
                {
                    type = data[n].type; 
                    break;
                }
            }

            if( type != MT_UNDEFINED )
            {
                Constraint_t c;

                if( v2[0] == "musicip-puid" )
                {
                    c.TargetAttr = ATTRIBUTE_MUSICIP_PUID;
                    c.MatchType = type;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "album-mbid" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_ALBUM_ID;
                    c.MatchType = type;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "album-artist-mbid" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_ALBUM_ARTIST_ID;
                    c.MatchType = type;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "artist-mbid" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_ARTIST_ID;
                    c.MatchType = type;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "country" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_RELEASE_COUNTRY;
                    c.MatchType = type;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "type" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_RELEASE_TYPE;
                    c.MatchType = type;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "bitrate" )
                {
                    try{
                            c.TargetValue = gint64(boost::lexical_cast<int>(v2[1]));
                            c.TargetAttr = ATTRIBUTE_BITRATE;
                            c.MatchType = type;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
                else
                if( v2[0] == "year" )
                {
                    try{
                            c.TargetValue = gint64(boost::lexical_cast<int>(v2[1]));
                            c.TargetAttr = ATTRIBUTE_DATE;
                            c.MatchType = type;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
                else
                if( v2[0] == "quality" )
                {
                    try{
                            c.TargetValue = gint64(boost::lexical_cast<int>(v2[1]));
                            c.TargetAttr = ATTRIBUTE_QUALITY;
                            c.MatchType = type;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
                else
                if( v2[0] == "genre" )
                {
                    try{
                            c.TargetValue = boost::lexical_cast<std::string>(v2[1]);
                            c.TargetAttr = ATTRIBUTE_GENRE;
                            c.MatchType = MT_FUZZY_EQUAL;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
                else
                if( v2[0] == "artist" )
                {
                    try{
                            c.TargetValue = boost::lexical_cast<std::string>(v2[1]);
                            c.TargetAttr = ATTRIBUTE_ARTIST;
                            c.MatchType = MT_FUZZY_EQUAL;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
                else
                if( v2[0] == "album-artist" )
                {
                    try{
                            c.TargetValue = boost::lexical_cast<std::string>(v2[1]);
                            c.TargetAttr = ATTRIBUTE_ALBUM_ARTIST;
                            c.MatchType = MT_FUZZY_EQUAL;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
                else
                if( v2[0] == "album" )
                {
                    try{
                            c.TargetValue = boost::lexical_cast<std::string>(v2[1]);
                            c.TargetAttr = ATTRIBUTE_ALBUM;
                            c.MatchType = MT_FUZZY_EQUAL;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
            }
            else
            {
                non_attr_strings.push_back( token );
            }
        }

        return Glib::ustring(boost::algorithm::join( non_attr_strings, " ")).lowercase();
    }

    template <typename T>
    bool
    determine_match (const Constraint_t& c, const MPX::Track& track)
    {
        g_return_val_if_fail(track.has(c.TargetAttr), false);

        bool truthvalue = false;

        switch( c.MatchType )
        {
            case MT_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) == boost::get<T>(c.TargetValue.get());
                break;

            case MT_NOT_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) != boost::get<T>(c.TargetValue.get());
                break;

            case MT_GREATER_THAN:
                truthvalue = boost::get<T>(track[c.TargetAttr].get())  > boost::get<T>(c.TargetValue.get());
                break;

            case MT_LESSER_THAN:
                truthvalue = boost::get<T>(track[c.TargetAttr].get())  < boost::get<T>(c.TargetValue.get());
                break;

            case MT_GREATER_THAN_OR_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) >= boost::get<T>(c.TargetValue.get());
                break;

            case MT_LESSER_THAN_OR_EQUAL:
                truthvalue = boost::get<T>(track[c.TargetAttr].get()) <= boost::get<T>(c.TargetValue.get());
                break;

            case MT_FUZZY_EQUAL:
                break;
        }

        return truthvalue;
    }

    template <>
    bool
    determine_match<std::string>(const Constraint_t& c, const MPX::Track& track)
    {
        g_return_val_if_fail(track.has(c.TargetAttr), false);

        bool truthvalue = false;

        switch( c.MatchType )
        {
            case MT_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) == boost::get<std::string>(c.TargetValue.get());
                break;

            case MT_NOT_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) != boost::get<std::string>(c.TargetValue.get());
                break;

            case MT_GREATER_THAN:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get())  > boost::get<std::string>(c.TargetValue.get());
                break;

            case MT_LESSER_THAN:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get())  < boost::get<std::string>(c.TargetValue.get());
                break;

            case MT_GREATER_THAN_OR_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) >= boost::get<std::string>(c.TargetValue.get());
                break;

            case MT_LESSER_THAN_OR_EQUAL:
                truthvalue = boost::get<std::string>(track[c.TargetAttr].get()) <= boost::get<std::string>(c.TargetValue.get());
                break;

            case MT_FUZZY_EQUAL:
                truthvalue = Util::match_keys(boost::get<std::string>(track[c.TargetAttr].get()), boost::get<std::string>(c.TargetValue.get()));
                break;
        }

        return truthvalue;
    }


    bool
    match_track( const Constraints_t& c, const MPX::Track& track)
    {
        for( Constraints_t::const_iterator i = c.begin(); i != c.end(); ++i )
        {
            Constraint_t const& c = *i;

            if( !track.has(c.TargetAttr) )
            {
                return false;
            }
        
            bool truthvalue; 

            if( c.TargetAttr >= ATTRIBUTE_TRACK )
                truthvalue = determine_match<gint64>(c, track);
            else
                truthvalue = determine_match<std::string>(c, track);

            if( !truthvalue )
                return false;
        }

        return true;
    }
}
}
