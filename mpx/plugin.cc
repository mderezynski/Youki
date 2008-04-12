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
#include "mpx/main.hh"
#include "mcs/mcs.h"

#define NO_IMPORT
#include <iostream>
#include <pygtk/pygtk.h>
#include <pygobject.h>
#include "mpx-py.hh"

#ifndef Py_ssize_t
#define Py_ssize_t int
#endif

namespace MPX
{
	Traceback::Traceback(const std::string& aname, const std::string& atraceback)
	:	name (aname)
	,	traceback (atraceback)
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
	Traceback::get_traceback() const
	{
		return traceback;
	}

	PluginManager::PluginManager (MPX::Player *player)
	: m_Id(1)
	, m_Player(player)
	{
		try {
			Py_Initialize();
			init_pygobject();
			init_pygtk();
			pyg_enable_threads ();
			mpx_py_init ();	
		} catch( error_already_set ) {
			g_warning("%s; Python Error:", G_STRFUNC);
		    PyErr_Print();
		}
	}

	void
	PluginManager::append_search_path (std::string const& path)
	{	
		m_Paths.push_back(path);
	}

	void
	PluginManager::load_plugins ()
	{
		PyObject *sys_path = PySys_GetObject ("path");
		for(Strings::const_iterator i = m_Paths.begin(); i != m_Paths.end(); ++i)
		{
			PyObject *path = PyString_FromString ((char*)i->c_str());
			if (PySequence_Contains(sys_path, path) == 0)
				PyList_Insert (sys_path, 0, path);
			Py_DECREF(path);
		}

		for(Strings::const_iterator p = m_Paths.begin(); p != m_Paths.end(); ++p)
		{
			if(!file_test(*p, FILE_TEST_EXISTS))
				continue;

			Glib::Dir dir (*p);
			std::vector<std::string> strv (dir.begin(), dir.end());
			dir.close();

			for(std::vector<std::string>::const_iterator i = strv.begin(); i != strv.end(); ++i)
			{
					PyObject * main_module = PyImport_AddModule ("__main__");
					if(main_module == NULL)
					{
						g_message("Couldn't get __main__");
						return;	
					}

					PyObject * mpx = PyImport_ImportModule("mpx");
					PyObject * mpx_dict = PyModule_GetDict(mpx);
					PyTypeObject *PyMPXPlugin_Type = (PyTypeObject *) PyDict_GetItemString(mpx_dict, "Plugin"); 

					PyObject * main_locals = PyModule_GetDict(main_module);
					PyObject * fromlist = PyTuple_New(0);
					PyObject * module = PyImport_ImportModuleEx ((char*)i->c_str(), main_locals, main_locals, fromlist);
					Py_DECREF (fromlist);
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
							PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

							PyObject * instance = PyObject_CallObject(value, NULL);
							if(instance == NULL)
							{
								PyErr_Print();
								continue;
							}

							PluginHolderRefP ptr = PluginHolderRefP(new PluginHolder);
							ptr->m_PluginInstance = instance;
							const char* doc = PyString_AsString (PyObject_GetAttrString (module, "__doc__")); 
							ptr->m_Description = doc ? doc : "(No Description given)";
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

							bool active = false;

							/* TODO: Query MCS for active state */

							if (active)
							{
								try {
									object ccinstance = object((handle<>(borrowed(instance))));
									ccinstance.attr("activate")(boost::ref(m_Player)); // TODO
									active = true;
								} catch( error_already_set )
								{
								}
							}

							ptr->m_Active = active; 
							m_Map.insert(std::make_pair(ptr->m_Id, ptr));
							pyg_gil_state_release(state);

							g_message("%s: >> Loaded: '%s'", G_STRLOC, ptr->m_Name.c_str());
							break;
						}
					}
			}
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
			object ccinstance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			object callable = ccinstance.attr("activate");
			result = boost::python::call<bool>(callable.ptr(), boost::ref(*m_Player), boost::ref(*mcs));
			i->second->m_Active = true;
		} catch( error_already_set )
		{
			std::string name (i->second->get_name());

			PyObject *pytype = NULL, *pyvalue = NULL, *pytraceback = NULL, *pystring = NULL;
			PyErr_Fetch (&pytype, &pyvalue, &pytraceback);
			PyErr_Clear();
			pystring = PyObject_Str(pyvalue);

			std::string traceback = PyString_AsString (pystring);

			push_traceback (name, traceback);

			g_message("%s: Failed to activate plugin %lld:\nTraceback: %s", G_STRLOC, id, traceback.c_str());

			Py_XDECREF (pytype);
			Py_XDECREF (pyvalue);
			Py_XDECREF (pytraceback);
			Py_XDECREF (pystring);

		}

		pyg_gil_state_release (state);

		return result;
	}

	void
	PluginManager::deactivate(gint64 id)
	{
		Glib::Mutex::Lock L (m_StateChangeLock);

		PluginHoldMap::iterator i = m_Map.find(id);
		g_return_if_fail(i != m_Map.end());

		if(!i->second->m_Active)
		{
			g_message("%s: Requested deactivate from plugin %lld, but is deactivated.", G_STRLOC, id);	
			g_return_if_reached();
		}

		PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

		try{
			object ccinstance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			ccinstance.attr("deactivate")();
			i->second->m_Active = false;
		} catch( error_already_set ) 
		{
			PyErr_Print ();
		}

		pyg_gil_state_release (state);
	}

	void
	PluginManager::push_traceback(const std::string& name, const std::string& traceback)
	{
		m_TracebackList.push_front(Traceback(name, traceback));
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
