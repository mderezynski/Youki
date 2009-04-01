#ifndef _YOUKI_NTREE_HH
#define _YOUKI_NTREE_HH

#include <boost/shared_ptr.hpp>

namespace MPX
{
    template <typename T>
    struct NTree
    {
        struct Node ;
        typedef Node*                   Node_SP_t ;
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
                        node->Parent = this ;
                        Children.push_back( node ) ; 
                    }
        } ;

        Node_SP_t Root ;

        NTree()
        {
            Root = new Node ; 
        }
    } ;
}

#endif // _YOUKI_NTREE_HH
