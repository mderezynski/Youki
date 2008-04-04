// (c) 2007 M. Derezynski

#include <glib/gmacros.h>
#include <glib/gmessages.h>
#include <string>
#include "mpx/playlistparser-xspf.hh"
#include "xmlcpp/xspf.hxx"
#include "xmltoc++.hh"

namespace MPX
{
	PlaylistParserXSPF::PlaylistParserXSPF ()
	{
	}

	PlaylistParserXSPF::~PlaylistParserXSPF ()
	{
	}

	bool
	PlaylistParserXSPF::read (std::string const& uri, Util::FileList& list)
	{
		try{
			MPX::XmlInstance<xspf::playlist> xspf (uri.c_str());
			if(xspf.xml().trackList().size())
			{
				xspf::trackList::track_sequence tracklist = (*xspf.xml().trackList().begin()).track();
				for(xspf::trackList::track_sequence::iterator i = tracklist.begin(); i != tracklist.end(); ++i)
				{
					// Extract only the URI for now
					std::string location = *(i->location().begin());
					list.push_back(location);
				}
			}
		} catch (...) {
			return false;
		}
		return true;
	}

	bool
	PlaylistParserXSPF::write (std::string const& uri, Util::FileList const& list)
	{
		return false;
	}
}
