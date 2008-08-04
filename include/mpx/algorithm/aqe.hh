#ifndef MPX_AQE_HH
#define MPX_AQE_HH

#include <boost/algorithm/string.hpp>
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include <vector>
#include <glib.h>

namespace MPX
{
namespace AQE
{
    enum MatchType_t
    {
        MT_EQUAL,
        MT_NOT_EQUAL,
        MT_GREATER_THAN,
        MT_LESSER_THAN,
        MT_GREATER_THAN_OR_EQUAL,
        MT_LESSER_THAN_OR_EQUAL,
    };

    struct Constraint_t
    {
        int                     TargetAttr;
        MPX::OVariant           TargetValue;
        MatchType_t             MatchType;
    };

    typedef std::vector<Constraint_t> Constraints_t;

    Glib::ustring
    parse_advanced_query (Constraints_t& constraints, const std::string& text)
    {
        typedef std::vector< std::string > VectorType;

        VectorType non_attr_strings;

        VectorType v;
        boost::algorithm::split( v, text, boost::algorithm::is_any_of(" ") );

        for( VectorType::const_iterator i = v.begin(); i != v.end(); ++i )
        {
            std::string const& token = *i;

            VectorType v2;
            boost::algorithm::split( v2, token, boost::algorithm::is_any_of(":") );

            if( v2.size() == 1) 
            {
                non_attr_strings.push_back(v2[0]);
            }
            else
            {
                Constraint_t c;

                if( v2[0] == "musicip-puid" )
                {
                    c.TargetAttr = ATTRIBUTE_MUSICIP_PUID;
                    c.MatchType = MT_EQUAL;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "album-mbid" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_ALBUM_ID;
                    c.MatchType = MT_EQUAL;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "album-artist-mbid" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_ALBUM_ARTIST_ID;
                    c.MatchType = MT_EQUAL;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "artist-mbid" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_ARTIST_ID;
                    c.MatchType = MT_EQUAL;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "country" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_RELEASE_COUNTRY;
                    c.MatchType = MT_EQUAL;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "type" )
                {
                    c.TargetAttr = ATTRIBUTE_MB_RELEASE_TYPE;
                    c.MatchType = MT_EQUAL;
                    c.TargetValue = v2[1];
                    constraints.push_back(c);
                }
                else
                if( v2[0] == "year" )
                {
                    try{
                            c.TargetValue = gint64(boost::lexical_cast<int>(v2[1]));
                            c.TargetAttr = ATTRIBUTE_DATE;
                            c.MatchType = MT_EQUAL;
                            constraints.push_back(c);
                    } catch( boost::bad_lexical_cast ) {
                    }
                }
            }
        }

        return Glib::ustring(boost::algorithm::join( non_attr_strings, " ")).lowercase();
    }


    template <typename T>
    bool
    determine_match (const Constraint_t& c, MPX::Track& track)
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

        }

        return truthvalue;
    }

    bool
    match_track( const Constraints_t& c, MPX::Track& track)
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

#endif
