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
#include <mcs/mcs.h>

namespace MPX
{
	struct PluginHolder
	{
		private:

			PyObject	*	m_PluginInstance;

			std::string		m_Name;
			std::string		m_Description;
			std::string		m_Authors;
			std::string		m_Copyright;
			int				m_IAge;
			std::string		m_Website;
			bool			m_Active;
            bool            m_HasGUI;
			gint64			m_Id;

		public:

			std::string const&
			get_name ()			const	{ return m_Name; }
	
			std::string	const&
			get_desc ()			const	{ return m_Description; }

			std::string const&
			get_authors ()		const	{ return m_Authors; }
	
			std::string const&
			get_copyright ()	const	{ return m_Copyright; }

			std::string const&
			get_website ()		const	{ return m_Website; }
	
			bool
			get_active ()		const	{ return m_Active; }

            bool
            get_has_gui ()      const   { return m_HasGUI; }

			gint64
			get_id ()			const	{ return m_Id; }

		friend class PluginManager;
        friend class PluginActivate;
	};

	typedef boost::shared_ptr<PluginHolder>	PluginHolderRefP;
	typedef std::map<gint64, PluginHolderRefP> PluginHoldMap; 
	typedef std::vector<std::string> Strings;

    class Traceback
	{
		std::string name;
		std::string traceback;

		public:
			Traceback(const std::string& /*name*/, const std::string& /*traceback*/);
			~Traceback();

			std::string
			get_name() const;

			std::string
			get_traceback() const;
	};


	class Player;
    class PluginManager
    {
		public:
	
			PluginManager (MPX::Player* /*player*/);
			~PluginManager ();
	
			void
			append_search_path (std::string const& /*path*/);

			void
			load_plugins ();

            void
            activate_plugins ();

			PluginHoldMap const&
			get_map () const;
		

			bool	
			activate (gint64 /*id*/);
	
			bool
			deactivate (gint64 /*id*/);

            bool
            show (gint64 /*id*/);


			void
			push_traceback(gint64 id, const std::string& /*method*/);

			unsigned int
			get_traceback_count() const;

			Traceback
			get_last_traceback() const;

			Traceback
			pull_last_traceback();


		private:

			PluginHoldMap	      m_Map;	
			Strings		          m_Paths;
			gint64			      m_Id;
			Player		        * m_Player;
			Glib::Mutex		      m_StateChangeLock;
			std::list<Traceback>  m_TracebackList;
            Mcs::Mcs            * mcs_plugins;
    };
}
#endif
