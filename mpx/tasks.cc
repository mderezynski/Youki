//  MPX
//  Copyright (C) 2005-2007 MPX Project 
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#include "config.h"
#include <glibmm.h>
#include <string>

#include "mpx/tasks.hh"

namespace MPX
{
    TaskKernel::TaskKernel ()
    : mNextTID (0)
    {
    }

    TaskKernel::~TaskKernel ()
    {
        Glib::Mutex::Lock L (m_TaskListLock);

        TaskListIter taskListIter = m_TaskList.begin();
        for( ; taskListIter != m_TaskList.end(); )
        {
          TaskControlDataP control( taskListIter->second );
          control->m_TaskEndLock.lock();
          control->m_TaskEndFunc(true); // aborted 
          control->m_TaskEndLock.unlock();
          m_TaskList.erase( taskListIter ); 
          taskListIter = m_TaskList.begin();
        }
    }

    TID
    TaskKernel::newTask (const std::string& name,
                const sigc::slot<void>& task_start,
                const sigc::slot<bool>& task_run,
                const sigc::slot<void, bool>& task_end)
    { 
        TaskControlDataP control (new TaskControlData);
        control->m_TaskInitFunc = task_start; 
        control->m_TaskRunFunc = task_run;
        control->m_TaskEndFunc = task_end;

        Glib::Mutex::Lock L (m_TaskListLock);

        TID tid = ++mNextTID;
        m_TaskList.insert(std::make_pair(tid, control));

        control->m_TaskInitFunc();

        if( m_TaskList.size() == 1 )
        {
            m_TaskListIter = m_TaskList.begin();
            Glib::signal_idle().connect( sigc::mem_fun( *this, &TaskKernel::tasksRun ) );
        }

        return tid;
    }

    void
    TaskKernel::stopTask (TID tid) 
    {
        Glib::Mutex::Lock L (m_TaskListLock);

        TaskListIter taskListIter = m_TaskList.find( tid );

        g_return_if_fail( taskListIter != m_TaskList.end() );

        TaskControlDataP control( taskListIter->second );
        control->m_TaskEndLock.lock();
        control->m_TaskEndFunc(true); // aborted 
        control->m_TaskEndLock.unlock();
        m_TaskList.erase( m_TaskListIter );
        m_TaskListIter = m_TaskList.begin();
    }

    bool
    TaskKernel::tasksRun ()
    {
        Glib::Mutex::Lock L (m_TaskListLock);

        TaskControlDataP control( m_TaskListIter->second );
        bool run = control->m_TaskRunFunc();
        if( !run )
        {
            control->m_TaskEndLock.lock();
            control->m_TaskEndFunc(false); // !aborted
            control->m_TaskEndLock.unlock();
            m_TaskList.erase( m_TaskListIter );
            m_TaskListIter = m_TaskList.begin();
           
            if( m_TaskList.empty() ) 
                return false;
        }

        ++m_TaskListIter;
        if(m_TaskListIter == m_TaskList.end())
        {
            m_TaskListIter = m_TaskList.begin(); // cycle
        }

        return !m_TaskList.empty();
    }

}; // namespace MPX
