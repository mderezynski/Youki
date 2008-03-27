//  MPXx - The Dumb Music Player
//  Copyright (C) 2005-2007 MPXx development team.
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
//  The MPXx project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifndef MPX_MBXML_V2
#define MPX_MBXML_V2

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include "mbxml-types-v2.hh"
#include "mb-libxml2-sax-release.hh"
#include "mb-libxml2-sax-tracks.hh"

#include "bmp/types/types-library.hh"

namespace MPX
{
  namespace MusicBrainzXml
  {
    void
    mb_releases_query (Glib::ustring const& artist,
                       Glib::ustring const& album,
                       MusicBrainzReleaseV & releases);
 
    void
    mb_tracks_by_puid (Glib::ustring const& puid, MusicBrainzTracklistTrackV & tracks);

    void  
    mb_releases_by_id (Glib::ustring const& release_id, MusicBrainzReleaseV & releases);

    void
    mb_releases_by_disc_id (std::string const& disc_id, MusicBrainzReleaseV & releases);
  }
}

#endif //!MPX_MBXML_V2
