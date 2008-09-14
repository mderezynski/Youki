#include "model.hh"
#include <iostream>
#include <glib/gmessages.h>

#define NOOP
#define DEBUG NOOP

ViewModel::ViewModel ()
: m_rowheight_parent(0)
, m_rowheight_child (0)
{
}

ViewModel::~ViewModel ()
{
}

void
ViewModel::set_metrics( int rowheight_parent, double rowheight_child )
{
    m_rowheight_parent = rowheight_parent;
    m_rowheight_child = rowheight_child;
}

void
ViewModel::append_row (const Row_p row)
{
    m_rows.push_back(row);
}

void
ViewModel::scroll_to (int position)
{
    m_position = position;
}

const Rows_t&
ViewModel::get_rows ()
{
    return m_rows;
}

int
ViewModel::find_totalheight ()
{
    int height = 0;

    for( int n = 0; n < m_rows.size(); ++ n )
    {
        Row_p const& row = m_rows[n];
        height += m_rowheight_parent + m_rowheight_child * row->ChildData.size();
    }
    DEBUG(" Total Height: %d", height );
    return height;
}

PositionData_t
ViewModel::find_row_at_current_position ()
{
    PositionData_t pd; 
    int position = 0;

    pd.Parent_Index = 0; 
    
    while( position < m_position && pd.Parent_Index < m_rows.size())
    {
        Row_p const& row = m_rows[pd.Parent_Index]; 
        position += m_rowheight_parent + m_rowheight_child * row->ChildData.size();
        pd.Parent_Index++;
    }

    DEBUG(" Initial Parent_Index is: %d, Position: %d, m_position: %d", pd.Parent_Index, position, m_position );

    // Now we have a position which is guaranteed to be below the current top row
    // Traverse backwards to find the first actual row

    while( position > m_position )
    {
        DEBUG (" Position: %d, m_position: %d", position, m_position );
        if( !pd.Child_Index )
        {
            DEBUG (G_STRLOC);
            if( position <= m_position )
            {
                DEBUG (G_STRLOC);
                goto exit_func;           
            }

            DEBUG (G_STRLOC);
            pd.Parent_Index--;
            pd.Child_Index = m_rows[pd.Parent_Index]->ChildData.size() - 1;
            position -= m_rowheight_child;
        }
        else
        {
            DEBUG (G_STRLOC);
            if( pd.Child_Index.get()-1 < 0 )
            {
                DEBUG (G_STRLOC);
                pd.Child_Index.reset();
                position -= m_rowheight_parent;
            }
            else
            {
                DEBUG (G_STRLOC);
                pd.Child_Index.get() --;
                position -= m_rowheight_child;
            }
        }
    }

    exit_func:

    DEBUG(" Done calculating position " );
    DEBUG("\n\n\n"); 
    pd.Offset = m_position - position;
    DEBUG(" Child_Index: %d, Parent_Index: %d, Offset: %f ", pd.Child_Index ? pd.Child_Index.get() : -1, pd.Parent_Index, pd.Offset );
    return pd; 
}
