#ifndef _YOUKI_NTREE_HH
#define _YOUKI_NTREE_HH

#include <boost/shared_ptr.hpp>

namespace MPX
{
    template <typename T>
    struct NTree
    {
        struct Node ;
        typedef boost::shared_ptr<Node> Node_SP_t ;
    
        struct Node
        { 
            private:

                    friend class NTree ;

                    Node ()
                    {
                    }

            public:

                    const Node_SP_t         Parent ;
                    std::vector<Node_SP_t>  Children ;
                    T                       Data ;

                    Node(
                          const Node_SP_t   parent
                        , const T&          data
                    )
                    : Parent( parent )
                    , Data( data )
                    {
                    }

                    void
                    append(
                          Node_SP_t         node
                        , const Node_SP_t   parent
                        , const T&          data
                    )
                    {
                        Children.push_back( Node_SP_t( new Node( parent, data ))) ;
                    }
        } ;

        Node_SP_t Root ;

        NTree()
        {
            Root = Node_SP_t( new Node ) ;
        }

        void
        append(
              Node_SP_t         node
            , const Node_SP_t   parent
            , const T&          data
        )
        {
            node->append(
                  node
                , parent
                , data
            ) ;
        }
    } ;
}

#endif // _YOUKI_NTREE_HH
