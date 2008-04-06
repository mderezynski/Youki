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
//  The MPX project hereby grants permission for non-GPlaylist compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPlaylist license
//  MPX is covered by.
#include "config.h"
#include "mpx/main.hh"
#include "mcs/base.h"

#define NO_IMPORT
#include <pygtk/pygtk.h>
#include <pygobject.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
using namespace boost::python;

#include "plugin-manager.hh"

#ifndef Py_ssize_t
#define Py_ssize_t int
#endif

using namespace Glib;

namespace MPX
{
	struct PlaylistPlugin
	{
		void
		run (MPX::Library& G_GNUC_UNUSED, TrackIdV& G_GNUC_UNUSED)
		{
		}
	};
}

BOOST_PYTHON_MODULE(mpx_playlist)
{
	class_<MPX::PlaylistPlugin>("PlaylistPlugin")	
		.def("run", &MPX::PlaylistPlugin::run)
	;

	class_<MPX::TrackIdV>("TrackIdVector")
		.def(vector_indexing_suite<MPX::TrackIdV>());
	;
}


namespace MPX
{
	PlaylistPluginManager::PlaylistPluginManager ()
	: m_Id(1)
	{
		try {
			initmpx_playlist();
		} catch( error_already_set ) {
			g_warning("%s; Python Error:", G_STRFUNC);
		    PyErr_Print();
		}
	}

	void
	PlaylistPluginManager::append_search_path (std::string const& path)
	{	
		m_Paths.push_back(path);
	}

	void
	PlaylistPluginManager::load_plugins ()
	{
		PyObject *sys_path = PySys_GetObject ("path");
		for(std::vector<std::string>::const_iterator i = m_Paths.begin(); i != m_Paths.end(); ++i)
		{
			g_message("%s: Appending path: '%s'", G_STRLOC, i->c_str());
	
			if(!file_test(*i, FILE_TEST_EXISTS))
				continue;

			PyObject *path = PyString_FromString ((char*)i->c_str());
			if (PySequence_Contains(sys_path, path) == 0)
				PyList_Insert (sys_path, 0, path);
			Py_DECREF(path);
		}

		for(std::vector<std::string>::const_iterator p = m_Paths.begin(); p != m_Paths.end(); ++p)
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

					PyObject * mpx = PyImport_ImportModule("mpx_playlist");
					PyObject * mpx_dict = PyModule_GetDict(mpx);
					PyTypeObject *PyMPXPlaylistPlugin_Type = (PyTypeObject *) PyDict_GetItemString(mpx_dict, "PlaylistPlugin"); 
					g_return_if_fail(PyMPXPlaylistPlugin_Type);

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

						if (PyObject_IsSubclass (value, (PyObject*) PyMPXPlaylistPlugin_Type))
						{
							PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());

							PyObject * instance = PyObject_CallObject(value, NULL);
							if(instance == NULL)
							{
								PyErr_Print();
								continue;
							}

							PlaylistPluginHolderRefP ptr = PlaylistPluginHolderRefP(new PlaylistPluginHolder);
							ptr->m_PluginInstance = instance;
							const char* doc = PyString_AsString (PyObject_GetAttrString (module, "__doc__")); 
							ptr->m_Description = doc ? doc : "(No Description given)";
							ptr->m_Id = m_Id++;

							Glib::KeyFile keyfile;
							std::string key_filename = build_filename(*p, build_filename(*i, (*i)+".mpx-plugin"));
			
							if(file_test(key_filename, FILE_TEST_EXISTS))
							{
								try{
									keyfile.load_from_file(key_filename);
									ptr->m_Name = keyfile.get_string("MPX PlaylistPlugin", "Name"); 
									ptr->m_Authors = keyfile.get_string("MPX PlaylistPlugin", "Authors"); 
									ptr->m_Copyright = keyfile.get_string("MPX PlaylistPlugin", "Copyright"); 
									ptr->m_IAge = keyfile.get_integer("MPX PlaylistPlugin", "IAge");
									ptr->m_Website = keyfile.get_string("MPX PlaylistPlugin", "Website");
								} catch (Glib::KeyFileError) {
								}
							}

							if(ptr->m_Name.empty())
								ptr->m_Name = PyString_AsString (PyObject_GetAttrString (module, "__name__")); 

							m_Map.insert(std::make_pair(ptr->m_Id, ptr));
							pyg_gil_state_release(state);

							g_message("%s: >> Loaded: '%s'", G_STRLOC, ptr->m_Name.c_str());
							break;
						}
					}
			}
		}
	}

	PlaylistPluginManager::~PlaylistPluginManager ()
	{
	}

	PlaylistPluginHoldMap const&
	PlaylistPluginManager::get_map () const
	{
		return m_Map;
	}

	void
	PlaylistPluginManager::run(gint64 id, MPX::Library & lib, MPX::TrackIdV & v)
	{
		PlaylistPluginHoldMap::iterator i = m_Map.find(id);
		g_return_if_fail(i != m_Map.end());

		try{
			object ccinstance = object((handle<>(borrowed(i->second->m_PluginInstance))));
			ccinstance.attr("run")(boost::ref(lib), boost::ref(v));
		} catch( error_already_set ) 
		{
			PyErr_Print();
		}
	}
}
