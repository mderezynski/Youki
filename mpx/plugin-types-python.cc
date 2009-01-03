//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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

#include <config.h>
#include <glibmm.h>
#include <glibmm/i18n.h>

#define NO_IMPORT
#include <iostream>
#include <pygtk/pygtk.h>
#include <pygobject.h>

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#endif

#include <boost/python.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <Python.h>

using namespace Glib;
using namespace boost::python;

#include "plugin-types-python.hh"

namespace MPX
{
    bool
    PluginHolderPython::activate(
    ) 
    {
        bool success = false;

        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

        try{
            object instance = object((handle<>(borrowed(m_PluginInstance))));
            object callable = instance.attr("activate");
            success = boost::python::call<bool>(callable.ptr());
        } catch( error_already_set & cxe )
        {
            pyg_gil_state_release (state);
            throw MethodInvocationError(_("Could not activate plugin"));
        }

        pyg_gil_state_release (state);

        return success;
    }

    bool
    PluginHolderPython::deactivate(
    )
    {
        bool success = false;

        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

        try{
            object instance = object((handle<>(borrowed(m_PluginInstance))));
            object callable = instance.attr("deactivate");
            success = boost::python::call<bool>(callable.ptr());
        } catch( error_already_set )
        {
            pyg_gil_state_release (state);
            throw MethodInvocationError(_("Could not deactivate plugin"));
        }

        pyg_gil_state_release (state);

        return success;
    }

    Gtk::Widget*
    PluginHolderPython::get_gui(
    )
    {
        PyGObject * pygobj = 0;

        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

        try{
            object instance = object((handle<>(borrowed(m_PluginInstance))));
            object result = instance.attr("get_gui")();
            pygobj = (PyGObject*)(result.ptr());
        } catch( error_already_set )
        {
            pyg_gil_state_release (state);
            throw MethodInvocationError(_("Could not get gui"));
        }

        pyg_gil_state_release (state);

        if( pygobj )
        {
            return Glib::wrap(((GtkWidget*)(pygobj->obj)), false);
        }
        else
        {
            return 0;
        }
    }
}
