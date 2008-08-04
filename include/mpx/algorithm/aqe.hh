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
