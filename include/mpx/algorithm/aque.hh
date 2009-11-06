#ifndef MPX_AQE_HH
#define MPX_AQE_HH

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"
#include <vector>
#include <glib.h>

namespace MPX
{
namespace AQE
{
    enum MatchType_t
    {
          MT_UNDEFINED
        , MT_EQUAL
        , MT_NOT_EQUAL
        , MT_EQUAL_BEGIN
        , MT_GREATER_THAN
        , MT_LESSER_THAN
        , MT_GREATER_THAN_OR_EQUAL
        , MT_LESSER_THAN_OR_EQUAL
        , MT_FUZZY_EQUAL
    };

    struct Constraint_t
    {
        int                     TargetAttr ;
        MPX::OVariant           TargetValue ;
        MatchType_t             MatchType ;
        bool                    InverseMatch ;

        Constraint_t ()
        : InverseMatch( false )
        {
        }
    };

    bool operator == (const Constraint_t& a, const Constraint_t& b )
    {
        return  (a.TargetAttr == b.TargetAttr) 
                    &&
                (a.TargetValue == b.TargetValue)
                    &&
                (a.MatchType == b.MatchType)
                    &&
                (a.InverseMatch == b.InverseMatch)
        ;
    }

    typedef std::vector<Constraint_t> Constraints_t;

    bool operator == (const Constraints_t& a, const Constraints_t& b)
    {
        if( a.size() != b.size() )
            return false ;

        for( Constraints_t::size_type n = 0 ; n < a.size() ; ++n )
        {
            const Constraint_t& c1 = a[n] ;
            const Constraint_t& c2 = b[n] ;

            if( c1 == c2 ) 
                continue ;
            else
                return false ;
        }

        return true ;
    }

    void
    parse_advanced_query(
          Constraints_t&        /*OUT: constraints*/
        , const std::string&    /*IN:  text*/
        , StrV&
    ) ;

    template <typename T>
    bool
    determine_match(
          const Constraint_t&   /*IN: constraints*/
        , const MPX::Track_sp&  /*IN: track*/
    ) ;

    template <>
    bool
    determine_match<std::string>(
          const Constraint_t&   /*IN: constraints*/
        , const MPX::Track_sp&  /*IN: track*/
    ) ;

    template <>
    bool
    determine_match<StrS>(
          const Constraint_t&   /*IN: constraints*/
        , const MPX::Track_sp&  /*IN: track*/
    ) ;

    bool
    match_track(
          const Constraints_t&
        , const MPX::Track_sp&
    ) ;
}
}

#endif
