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
#ifndef MPX_TASKS_HH
#define MPX_TASKS_HH

#include "config.h"
#include <glibmm.h>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

namespace MPX
{
    typedef gint64 TID;

    struct TaskData
    {
        //
    };

    typedef boost::shared_ptr<TaskData> TaskDataP;

    typedef sigc::slot<void> TaskFunc ;
    typedef sigc::slot<bool> TaskRunFunc ;
    
    struct TaskControlData
    {
        Glib::Mutex       mTaskEndLock ;
        TaskFunc          mTaskInitFunc ;
        TaskFunc          mTaskEndFunc ;
        TaskRunFunc       mTaskRunFunc ;
    };

    typedef boost::shared_ptr<TaskControlData> TaskControlDataP;
    typedef std::map<TID, TaskControlDataP> TaskList; 
    typedef TaskList::iterator TaskListIter;

    class TaskKernel
    {
        public:

            TaskKernel () ;
            ~TaskKernel () ;

            TID
            newTask (const std::string& name /* informal */,
                        const sigc::slot<void>& task_start,
                        const sigc::slot<bool>& task_run,
                        const sigc::slot<void>& task_end);

            void
            stopTask (TID) ;

        private:

            TID             mNextTID ;
            TaskList        mTaskList ;
            TaskListIter    mTaskListIter ;
            Glib::Mutex     mTaskListLock ;

            bool
            tasksRun ();
    };

}; // namespace MPX

#endif // !MPX_TASKS_HH
