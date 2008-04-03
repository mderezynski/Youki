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
#include "mcs/base.h"

#define NO_IMPORT
#include <pygtk/pygtk.h>
#include <pygobject.h>
#include "mpxpy.hh"

#ifndef Py_ssize_t
#define Py_ssize_t int
#endif

namespace MPX
{
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

		PyObject *mpx = PyImport_ImportModule("mpx");
		PyObject *mpx_dict = PyModule_GetDict(mpx);
		PyTypeObject *PyMPXPlugin_Type = (PyTypeObject *) PyDict_GetItemString(mpx_dict, "Plugin"); 

		PyObject * main_module = PyImport_AddModule ("__main__");
		if(main_module == NULL)
		{
			g_message("Couldn't get __main__");
			return;	
		}

		for(Strings::const_iterator p = m_Paths.begin(); p != m_Paths.end(); ++p)
		{
			Glib::Dir dir (*p);
			std::vector<std::string> strv (dir.begin(), dir.end());
			dir.close();

			for(std::vector<std::string>::const_iterator i = strv.begin(); i != strv.end(); ++i)
			{
					PyObject * main_locals = PyModule_GetDict(main_module);
					PyObject * fromlist = PyTuple_New(0);
					PyObject * module = PyImport_ImportModuleEx ((char*)i->c_str(), main_locals, main_locals, fromlist);
					Py_DECREF (fromlist);
					if (!module) {
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

							Glib::KeyFile keyfile;
							keyfile.load_from_file(build_filename(*p, build_filename(*i, (*i)+".mpx-plugin")));

							PluginHolderRefP p = PluginHolderRefP(new PluginHolder);
							p->m_PluginInstance = instance;
							p->m_Name = keyfile.get_string("MPX Plugin", "Name"); 
							const char* doc = PyString_AsString (PyObject_GetAttrString (module, "__doc__")); 
							p->m_Description = doc ? doc : "(No Description given)";
							p->m_Authors = keyfile.get_string("MPX Plugin", "Authors"); 
							p->m_Copyright = keyfile.get_string("MPX Plugin", "Copyright"); 
							p->m_IAge = keyfile.get_integer("MPX Plugin", "IAge");
							p->m_Website = keyfile.get_string("MPX Plugin", "Website");
							p->m_Id = m_Id++;

							if(p->m_Name.empty())
								p->m_Name = PyString_AsString (PyObject_GetAttrString (module, "__name__")); 

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

							p->m_Active = active; 
							m_Map.insert(std::make_pair(p->m_Id, p));
							pyg_gil_state_release(state);

							g_message("%s: >> Loaded: '%s'", G_STRLOC, p->m_Name.c_str());
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

	void
	PluginManager::activate(gint64 id)
	{
		PluginHoldMap::iterator i = m_Map.find(id);
		g_return_if_fail(i != m_Map.end());

		if(i->second->m_Active)
		{
			g_message("%s: Requested deactivate from plugin %lld, but is active.", G_STRLOC, id);	
			g_return_if_reached();
		}
		try{
			object ccinstance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			ccinstance.attr("activate")(boost::ref(m_Player));
			i->second->m_Active = true;
		} catch( error_already_set ) 
		{
		}
	}

	void
	PluginManager::deactivate(gint64 id)
	{
		PluginHoldMap::iterator i = m_Map.find(id);
		g_return_if_fail(i != m_Map.end());

		if(!i->second->m_Active)
		{
			g_message("%s: Requested deactivate from plugin %lld, but is deactivated.", G_STRLOC, id);	
			g_return_if_reached();
		}
		try{
			object ccinstance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			ccinstance.attr("deactivate")();
			i->second->m_Active = false;
		} catch( error_already_set ) 
		{
		}
	}
}
