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
#ifndef MPX_SOURCE_MUSICLIB_PlaylistUGIN_HH
#define MPX_SOURCE_MUSICLIB_PlaylistUGIN_HH
#include "config.h"
#include <gtkmm.h>
#include <Python.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>
#include "mpx/library.hh"

namespace MPX
{
        namespace Source
        {
                class PlaybackSourceMusicLib;
        };

	struct PlaylistPluginHolder
	{
		private:

			PyObject	*	m_PluginInstance;
			std::string		m_Name;
			std::string		m_Description;
			std::string		m_Authors;
			std::string		m_Copyright;
			int				m_IAge;
			std::string		m_Website;
			gint64			m_Id;
			Glib::RefPtr<Gdk::Pixbuf> m_Icon;

		public:

			Glib::RefPtr<Gdk::Pixbuf> const&
			get_icon ()			const	{ return m_Icon; }

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
	
			gint64
			get_id ()			const	{ return m_Id; }

			PlaylistPluginHolder ()
			: m_Icon(Glib::RefPtr<Gdk::Pixbuf>(0))
			{
			}

		friend class PlaylistPluginManager;
	};

	typedef boost::shared_ptr<PlaylistPluginHolder> PlaylistPluginHolderRefP;
	typedef std::map<gint64, PlaylistPluginHolderRefP> PlaylistPluginHoldMap; 
	typedef std::vector<gint64> TrackIdV;

    class PlaylistPluginManager
    {
		public:
	
			PlaylistPluginManager (MPX::Library&, MPX::Source::PlaybackSourceMusicLib*); 
			~PlaylistPluginManager ();
	
			void
			append_search_path (std::string const& /*path*/);

			void
			load_plugins ();

			PlaylistPluginHoldMap const&
			get_map () const;
		
			void	
			run (gint64 /*id*/);
	
		private:

			PlaylistPluginHoldMap m_Map;	
			std::vector<std::string> m_Paths;
			gint64 m_Id;
			MPX::Library & m_Lib;
			MPX::Source::PlaybackSourceMusicLib * m_MusicLib; 
    };
}
#endif
