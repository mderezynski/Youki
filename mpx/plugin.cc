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
#include "config.h"
#include "plugin.hh"
#include "mpx/mpx-main.hh"
#include "mcs/mcs.h"

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

#include "mpx/mpx-public-mpx.hh"
#include "mpx/mpx-python.hh"

using namespace Glib;
using namespace boost::python;

namespace MPX
{
	Traceback::Traceback(const std::string& n, const std::string& m, const std::string& t)
	:	name (n)
	,	method (m)
	,	traceback (t)
	{
	}

	Traceback::~Traceback()
	{
	}

	std::string
	Traceback::get_name() const
	{
		return name;
	}

	std::string
	Traceback::get_method() const
	{
		return method;
	}

	std::string
	Traceback::get_traceback() const
	{
		return traceback;
	}

	PluginManager::PluginManager (MPX::Player *player)
    : Service::Base("mpx-service-plugins")
	, m_Id(1)
	, m_Player(player)
	{
        mcs->domain_register("pyplugs");

        std::string const user_path =
                build_filename(
                                build_filename(
                                        g_get_user_data_dir(),
                                        "mpx"),
                                "python-plugins"
                              );

        if(file_test(user_path, FILE_TEST_EXISTS))
        {
                append_search_path (user_path);
        }

        append_search_path
                (build_filename(
                                DATA_DIR,
                                "python-plugins"
                               ));
	}

	void
	PluginManager::append_search_path (std::string const& path)
	{	
		m_Paths.push_back(path);
	}

    void
    PluginManager::activate_plugins ()
    {
        for(PluginHoldMap::iterator i = m_Map.begin(); i != m_Map.end(); ++i)
        {
            PluginHolderRefP & item = i->second;
            try{
                bool active = mcs->key_get<bool>("pyplugs", item->get_name());

                g_message("Name: [%s], Active: %d", item->get_name().c_str(), int(active));

                if (active)
                {
                    try{    
                        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                        object instance = object((handle<>(borrowed(item->m_PluginInstance))));
                        instance.attr("activate")();
                        item->m_Active = true;
                        pyg_gil_state_release(state);
                        } catch( error_already_set )
                        {
                            PyErr_Print();
                        }
                }
             } catch(...)
             {
             }
        }
    }

	void
	PluginManager::load_plugins ()
	{
        PyObject * main_module = PyImport_AddModule ("__main__");
        if(main_module == NULL)
        {
            g_message("Couldn't get __main__");
            return;	
        }

		PyObject * sys_path = PySys_GetObject ((char*)"path");

        {
            std::string pyapi_path = build_filename(DATA_DIR, "pyapi");
	        PyObject *path = PyString_FromString ((char*)pyapi_path.c_str());
            PyList_Insert (sys_path, 0, path); 
            Py_DECREF(path);
        }

		for(Strings::const_iterator i = m_Paths.begin(); i != m_Paths.end(); ++i)
		{
			PyObject *path = PyString_FromString ((char*)i->c_str());
			if (PySequence_Contains(sys_path, path) == 0)
				PyList_Insert (sys_path, 0, path);
			Py_DECREF(path);
		}

        PyObject * main_locals_orig = PyModule_GetDict(main_module);
        PyObject * mpx_orig = PyImport_ImportModule("mpx"); 
        PyObject * mpx_dict = PyModule_GetDict(mpx_orig);
        PyTypeObject * PyMPXPlugin_Type = (PyTypeObject *) PyDict_GetItemString(mpx_dict, "Plugin"); 

		for(Strings::const_iterator p = m_Paths.begin(); p != m_Paths.end(); ++p)
		{
			if(!file_test(*p, FILE_TEST_EXISTS))
				continue;

			Glib::Dir dir (*p);
			std::vector<std::string> strv (dir.begin(), dir.end());
			dir.close();

			for(std::vector<std::string>::const_iterator i = strv.begin(); i != strv.end(); ++i)
			{
                    PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

                    PyObject * main_locals = PyDict_Copy(main_locals_orig);
					PyObject * module = PyImport_ImportModuleEx ((char*)i->c_str(), main_locals, main_locals, Py_None);
					if (!module) {
						g_message("%s: Couldn't load module '%s'", G_STRLOC, i->c_str());
						PyErr_Print ();
						continue;	
					}

					PyObject *locals = PyModule_GetDict (module);
					Py_ssize_t pos = 0;
					PyObject *key, *value;
					while (PyDict_Next (locals, &pos, &key, &value))
					{
						if(!PyType_Check(value))
							continue;

						if (PyObject_IsSubclass (value, (PyObject*) PyMPXPlugin_Type))
						{
                            try{
                                object instance = boost::python::call<object>(value, m_Id, boost::ref(m_Player), boost::ref(mcs));
                                if(!instance)
                                {
                                    PyErr_Print();
                                    continue;
                                }

                                PluginHolderRefP ptr = PluginHolderRefP(new PluginHolder);

                                ptr->m_PluginInstance = instance.ptr();
                                Py_INCREF(instance.ptr());

                                if(PyObject_HasAttrString(instance.ptr(), "__doc__"))
                                {
                                    const char* doc = PyString_AsString (PyObject_GetAttrString (instance.ptr(), "__doc__")); 
                                    ptr->m_Description = doc ? doc : "(No Description given)";
                                    if(PyErr_Occurred())
                                    {
                                        g_message("%s: Got no __doc__ from plugin", G_STRLOC);
                                        PyErr_Clear();
                                    }
                                }
                                else
                                {
                                    ptr->m_Description = "(No Description given)";
                                }

                                ptr->m_Id = m_Id++;

                                Glib::KeyFile keyfile;
                                std::string key_filename = build_filename(*p, build_filename(*i, (*i)+".mpx-plugin"));
                                if(file_test(key_filename, FILE_TEST_EXISTS))
                                {
                                    try{
                                        keyfile.load_from_file(build_filename(*p, build_filename(*i, (*i)+".mpx-plugin")));
                                        ptr->m_Name = keyfile.get_string("MPX Plugin", "Name"); 
                                        ptr->m_Authors = keyfile.get_string("MPX Plugin", "Authors"); 
                                        ptr->m_Copyright = keyfile.get_string("MPX Plugin", "Copyright"); 
                                        ptr->m_IAge = keyfile.get_integer("MPX Plugin", "IAge");
                                        ptr->m_Website = keyfile.get_string("MPX Plugin", "Website");
                                    } catch (Glib::KeyFileError) {
                                    }
                                }

                                if(ptr->m_Name.empty())
                                    ptr->m_Name = PyString_AsString (PyObject_GetAttrString (module, "__name__")); 

                                /* TODO: Query MCS for active state */

                                mcs->key_register("pyplugs", ptr->m_Name, false);

                                ptr->m_Active = false; 
                                ptr->m_HasGUI = PyObject_HasAttrString(instance.ptr(), "get_gui");
                                if(PyErr_Occurred())
                                {
                                    g_message("%s: No get_gui in plugin", G_STRLOC);
                                    PyErr_Clear();
                                }

                                ptr->m_CanActivate = PyObject_HasAttrString(instance.ptr(), "activate");
                                if(PyErr_Occurred())
                                {
                                    g_message("%s: No activate in plugin", G_STRLOC);
                                    PyErr_Clear();
                                }

                                m_Map.insert(std::make_pair(ptr->m_Id, ptr));

                                g_message("%s: >> Loaded: '%s'", G_STRLOC, ptr->m_Name.c_str());
                            } catch( error_already_set )
                            {
                                g_message("Plugin: %s", i->c_str());
                                PyErr_Print();
                            }
							break;
						}
					}
                    pyg_gil_state_release(state);
			}
		}
	}

    void
    PluginManager::shutdown ()
    {
        for(PluginHoldMap::iterator i = m_Map.begin(); i != m_Map.end(); ++i)
        {
                if(!i->second->m_Active)
                {
                    continue;
                }

                PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

                try{
                    object instance = object((handle<>(borrowed(i->second->m_PluginInstance))));
                    object callable = instance.attr("deactivate");
                    boost::python::call<bool>(callable.ptr());
                    i->second->m_Active = false;
                } catch( error_already_set ) 
                {
                }

                pyg_gil_state_release (state);
        }
    }

	PluginManager::~PluginManager ()
	{
	}

	PluginHoldMap const&
	PluginManager::get_map () const
	{
		return m_Map;
	}

	Gtk::Widget *
	PluginManager::get_gui(gint64 id)
	{
		PyGObject * pygobj;
		Glib::Mutex::Lock L (m_StateChangeLock);

		PluginHoldMap::iterator i = m_Map.find(id);
		g_return_val_if_fail(i != m_Map.end(), false);
        g_return_val_if_fail(m_Map.find(id)->second->get_has_gui(), false);

		PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

		try{
			object instance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			object result = instance.attr("get_gui")();
			pygobj = (PyGObject*)(result.ptr());
		} catch( error_already_set )
		{
			push_traceback (id, "get_gui");
			return NULL;
		}

        try{
            mcs->key_set<bool>("pyplugs", i->second->get_name(), i->second->m_Active);
        } catch(...) {
            g_message("%s: Failed saving plugin state for '%s'", G_STRLOC, i->second->get_name().c_str());
        }

		pyg_gil_state_release (state);

		return Glib::wrap(((GtkWidget*)(pygobj->obj)), false);
	}


	bool
	PluginManager::activate(gint64 id)
	{
		bool result = false;
		Glib::Mutex::Lock L (m_StateChangeLock);

		PluginHoldMap::iterator i = m_Map.find(id);
		g_return_val_if_fail(i != m_Map.end(), false);

		if(i->second->m_Active)
		{
			g_message("%s: Requested activate from plugin %lld, but is active.", G_STRLOC, id);	
			g_return_val_if_reached(false);
		}

		PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

		try{
			object instance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			object callable = instance.attr("activate");
			result = boost::python::call<bool>(callable.ptr());
			i->second->m_Active = true;
            signal_activated_.emit(id);
		} catch( error_already_set )
		{
			push_traceback (id, "activate");
		}

        try{
            mcs->key_set<bool>("pyplugs", i->second->get_name(), i->second->m_Active);
        } catch(...) {
            g_message("%s: Failed saving plugin state for '%s'", G_STRLOC, i->second->get_name().c_str());
        }

		pyg_gil_state_release (state);

		return result;
	}

    bool
	PluginManager::deactivate(gint64 id)
	{
		Glib::Mutex::Lock L (m_StateChangeLock);

		PluginHoldMap::iterator i = m_Map.find(id);
		g_return_val_if_fail(i != m_Map.end(), false);

        bool result = false;

		if(!i->second->m_Active)
		{
			g_message("%s: Requested deactivate from plugin %lld, but is deactivated.", G_STRLOC, id);	
			g_return_val_if_reached(false);
		}

		PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

		try{
			object instance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			object callable = instance.attr("deactivate");
			result = boost::python::call<bool>(callable.ptr());
			i->second->m_Active = false;
            signal_deactivated_.emit(id);
		} catch( error_already_set ) 
		{
			push_traceback (id, "deactivate");
		}

		pyg_gil_state_release (state);

        try{
            mcs->key_set<bool>("pyplugs", i->second->get_name(), i->second->m_Active);
        } catch(...) {
            g_message("%s: Failed saving plugin state for '%s'", G_STRLOC, i->second->get_name().c_str());
        }
        return result;
	}

    void
	PluginManager::show(gint64 id)
	{
        signal_plugin_show_gui_.emit(id);
	}

	void
	PluginManager::push_traceback(gint64 id, const std::string& method)
	{
        g_return_if_fail(m_Map.count(id) != 0);

        PyObject *pytype = NULL, *pyvalue = NULL, *pytraceback = NULL, *pystring = NULL;
        PyErr_Fetch (&pytype, &pyvalue, &pytraceback);
        PyErr_Clear();
        pystring = PyObject_Str(pyvalue);
        std::string traceback = PyString_AsString (pystring);

        std::string name = m_Map.find(id)->second->get_name();
        g_message("%s: Failed to call '%s' on plugin %lld:\nTraceback: %s", G_STRLOC, method.c_str(), id, traceback.c_str());
		m_TracebackList.push_front(Traceback(name, method, traceback));

        signal_traceback_.emit();

        Py_XDECREF (pytype);
        Py_XDECREF (pyvalue);
        Py_XDECREF (pytraceback);
        Py_XDECREF (pystring);
	}

	unsigned int
	PluginManager::get_traceback_count() const
	{
		return m_TracebackList.size();
	}

	Traceback
	PluginManager::get_last_traceback() const
	{
		return *(m_TracebackList.begin());
	}

	Traceback
	PluginManager::pull_last_traceback()
	{
		std::list<Traceback>::iterator i = m_TracebackList.begin();
		Traceback t = *i;
		m_TracebackList.pop_front();
		return t;
	}
}
