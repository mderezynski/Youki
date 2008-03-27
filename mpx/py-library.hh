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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.
#ifndef MPX_PY__LIBRARY_CLASS_HH
#define MPX_PY__LIBRARY_CLASS_HH

#include "mpx/library.hh"
#include <Python.h>
#include <boost/python.hpp>
using namespace boost::python;

namespace MPX
{
	namespace Py
	{
		class PyLibrary
		{
				Library * m_Library;
	
			public:

				PyLibrary ();

				PyLibrary (Library & library);

		};
	}
}

BOOST_PYTHON_MODULE(mpxlibrary)
{
	class_<MPX::Py::PyLibrary>("MPXLibrary")
		;
}

#endif // !MPX_PY__LIBRARY_CLASS_HH
