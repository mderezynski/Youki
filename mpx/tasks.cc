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
#include "tasks.hh"

namespace MPX
{
    TaskKernel::TaskKernel ()
    : mNextTID (0)
    {
    }

    TaskKernel::~TaskKernel ()
    {
        Glib::Mutex::Lock L (mTaskListLock);

        TaskListIter taskListIter = mTaskList.begin();
        for( ; taskListIter != mTaskList.end(); )
        {
          TaskControlDataP control( taskListIter->second );
          control->mTaskEndLock.lock();
          control->mTaskEndFunc(true); // aborted 
          control->mTaskEndLock.unlock();
          mTaskList.erase( taskListIter ); 
          taskListIter = mTaskList.begin();
        }
    }

    TID
    TaskKernel::newTask (const std::string& name,
                const sigc::slot<void>& task_start,
                const sigc::slot<bool>& task_run,
                const sigc::slot<void, bool>& task_end)
    { 
        TaskControlDataP control (new TaskControlData);
        control->mTaskInitFunc = task_start; 
        control->mTaskRunFunc = task_run;
        control->mTaskEndFunc = task_end;

        Glib::Mutex::Lock L (mTaskListLock);

        TID tid = ++mNextTID;
        mTaskList.insert(std::make_pair(tid, control));

        control->mTaskInitFunc();

        if( mTaskList.size() == 1 )
        {
            mTaskListIter = mTaskList.begin();
            Glib::signal_idle().connect( sigc::mem_fun( *this, &TaskKernel::tasksRun ) );
        }

        return tid;
    }

    void
    TaskKernel::stopTask (TID tid) 
    {
        Glib::Mutex::Lock L (mTaskListLock);

        TaskListIter taskListIter = mTaskList.find( tid );

        g_return_if_fail( taskListIter != mTaskList.end() );

        TaskControlDataP control( taskListIter->second );
        control->mTaskEndLock.lock();
        control->mTaskEndFunc(true); // aborted 
        control->mTaskEndLock.unlock();
        mTaskList.erase( mTaskListIter );
        mTaskListIter = mTaskList.begin();
    }

    bool
    TaskKernel::tasksRun ()
    {
        Glib::Mutex::Lock L (mTaskListLock);

        TaskControlDataP control( mTaskListIter->second );
        bool run = control->mTaskRunFunc();
        if( !run )
        {
            control->mTaskEndLock.lock();
            control->mTaskEndFunc(false); // !aborted
            control->mTaskEndLock.unlock();
            mTaskList.erase( mTaskListIter );
            mTaskListIter = mTaskList.begin();
        }

        ++mTaskListIter;
        if(mTaskListIter == mTaskList.end())
        {
            mTaskListIter = mTaskList.begin(); // cycle
        }

        return !mTaskList.empty();
    }

}; // namespace MPX
