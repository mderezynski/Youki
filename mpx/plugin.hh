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
#ifndef MPX_PLUGIN_HH
#define MPX_PLUGIN_HH
#include "config.h"
#include <gtkmm.h>
#include <Python.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace MPX
{
	struct PluginHolder
	{
		private:
			PyObject	*	m_PluginInstance;
			std::string		m_Name;
			std::string		m_Description;
			bool			m_Active;

		public:

			std::string const&
			get_name ()		const	{ return m_Name; }
	
			std::string	const&
			get_desc ()		const	{ return m_Description; }
	
			bool
			get_active ()	const	{ return m_Active; }

		friend class PluginManager;
	};

	typedef boost::shared_ptr<PluginHolder>	PluginHolderRefP;
	typedef std::map<std::string, PluginHolderRefP>	PluginHoldMap; 
	typedef std::vector<std::string> Strings;
	
	class Player;
    class PluginManager
    {
		public:
	
			PluginManager ();
			~PluginManager ();
	
			void
			append_search_path (std::string const& /*path*/);

			void
			load_plugins (Player *player);

			PluginHoldMap const&
			get_map () const;
			

		private:

			PluginHoldMap	m_Map;	
			Strings			m_Paths;
    };
}
#endif
