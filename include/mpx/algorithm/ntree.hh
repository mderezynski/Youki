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
        typedef std::vector<Node_SP_t>  Children_t ;

        struct Node
        { 
            public:

            private:

                    friend class NTree ;

                    Node ()
                    {
                    }

            public:

                    Node_SP_t               Parent ;
                    Children_t              Children ;
                    T                       Data ;

                    Node(
                        const T&            data
                    )
                    : Data( data )
                    {
                    }

                    void
                    append(
                        Node_SP_t           node
                    )
                    {
                        node->Parent = Node_SP_t( this ) ;
                        Children.push_back( node ) ; 
                    }
        } ;

        Node_SP_t Root ;

        NTree()
        {
            Root = Node_SP_t( new Node ) ;
        }

        void
        append(
              Node_SP_t     node
            , Node_SP_t     parent
            , T&            data
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
