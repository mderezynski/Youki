#ifndef MODEL_HH
#define MODEL_HH

#include <vector>
#include <boost/tuple/tuple.hpp>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <cairomm/cairomm.h>
#include "mpx/mpx-types.hh"
#include "mpx/mpx-sql.hh"

enum RowType
{
    AM_ROW_PARENT,
    AM_ROW_CHILD
};

typedef std::vector<MPX::Track>         ChildData_t;

struct Row_t
{
    Cairo::RefPtr<Cairo::ImageSurface>  Cover;
    MPX::SQL::Row                       AlbumData;
    ChildData_t                         ChildData;
};

typedef boost::shared_ptr<Row_t>    Row_p;
typedef std::vector<Row_p>          Rows_t;

struct PositionData_t
{
    int                         Parent_Index;
    boost::optional<int>        Child_Index;
    double                      Offset; 
};

class ViewModel
{
        public:

        public:

            ViewModel ();
            virtual ~ViewModel ();

            void
            set_metrics( int rowheight_parent, double rowheight_child );

            virtual void
            append_row (const Row_p);

            PositionData_t
            find_row_at_current_position ();

            int
            find_totalheight ();

            void
            scroll_to(int);

            const Rows_t&
            get_rows ();

        private:
    
            Rows_t    m_rows;
            int       m_position;

        public:

            int       m_rowheight_parent;
            int       m_rowheight_child;
             
};

#endif
