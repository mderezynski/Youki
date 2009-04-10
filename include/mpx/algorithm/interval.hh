#ifndef _MPX_ALGORITHM__INTERVAL__HH
#define _MPX_ALGORITHM__INTERVAL__HH

#include <cstdlib>

namespace MPX
{
    template <typename T>
    class Interval
    {
        public:

            enum Type
            {
                  IN_IN
                , EX_EX
                , IN_EX
                , EX_IN
            } ;

        protected:

            Type    t ;
            T       a ;
            T       b ;
        
        public:

            Interval(
                , Type      t_
                , const T&  a_
                , const T&  b_
            )
                : t( t_ )
                , a( a_ )
                , b( b_ )
            {}

            inline bool
            in_interval(
                  const T&  i_
            )
            {
                switch( t )
                {
                    case IN_IN:
                        return i >= a_ && i <= b_ ;
                    case EX_EX:
                        return i >  a_ && i <  b_ ;
                    case IN_EX:
                        return i >= a_ && i <  b_ ;
                    case EX_IN:
                        return i >  a_ && i <= b_ ;

                    default: std::abort() ;
                } ;
            }
    } ;
}
#endif // _MPX_ALGORITHM__INTERVAL__HH
