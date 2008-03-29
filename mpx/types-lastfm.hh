//  BMPx - The Dumb Music Player
//  Copyright (C) 2005-2007 BMPx development team.
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
//  The BMPx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#ifndef BMP_LAST_FM_TYPES_HH
#define BMP_LAST_FM_TYPES_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include "bmp/types/types-basic.hh"
#include "bmp/types/types-library.hh"
#include "bmp/types/types-ui.hh"

#include <ctime>
#include <vector>
#include <queue>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

using namespace Glib;

namespace Bmp
{
  namespace LastFM
  {
    typedef boost::variant <ustring, UStrV> xmlRpcVariant;
    typedef std::vector <xmlRpcVariant> xmlRpcVariantV;

    //// Queue
    struct TrackQueueItem
    {
      ustring   mbid;
      time_t    date;
      guint64  length;
      guint64  nr;
      ustring   artist;
      ustring   track;
      ustring   album;
      ustring   source;
      ustring   rating;

      TrackQueueItem () : date (0), length (0), nr (0) {}

      TrackQueueItem (Bmp::TrackMetadata const& x)
      {
        mbid      = x.mb_track_id ? x.mb_track_id.get() : std::string();
        artist    = x.artist ? x.artist.get() : std::string();
        track     = x.title ? x.title.get() : std::string();
        album     = x.album ? x.album.get() : std::string();
        length    = x.duration ? x.duration.get () : 0;
        nr        = x.tracknumber ? x.tracknumber.get () : 0;
        date      = time (NULL);
        source    = "P"; 
        rating    = "";
      }

      friend inline bool operator< (TrackQueueItem const& a, TrackQueueItem const& b)
      {
        return a.date < b.date;
      }
    };
    typedef std::deque <TrackQueueItem> LQM;

    // User
    struct User
    { 
      ustring	username;
      ustring	url;
      ustring	image;
      double match;
    };
    typedef std::vector <User> UserV;

    // Tag
    struct Tag
    {
      ustring name;
      ustring url;
      guint64 count;
    };
    typedef std::vector <Tag> TagV;
    typedef std::pair   <Tag, double> TagRankP;
    typedef std::vector <TagRankP> RankedTagV;

    bool operator< (TagRankP const& a, TagRankP const& b);
    bool operator< (Tag const& a, Tag const& b);

    // LastFMArtist
    struct LastFMArtist
    {
      ustring name;
      ustring mbid;
      ustring url;
      ustring thumbnail;
      ustring image;
      guint64 count;
      guint64 rank;
      bool    streamable;

      LastFMArtist () : streamable(false) {}
    };
    typedef std::vector <LastFMArtist> LastFMArtistV;

    enum TagGlobalType
    {
      TAGS_GLOBAL_ARTIST,
      TAGS_GLOBAL_TRACK, 
    };

    enum TagUserType
    {
      TAGS_USER_TOPTAGS,
      TAGS_USER_ARTIST,
      TAGS_USER_ALBUM,
      TAGS_USER_TRACK, 
    };

    enum UserType
    {
      UT_N,
      UT_F, 
    };

    enum ArtistType
    {
      AT_USER_TOP,
      AT_USER_SYSTEMREC,
      AT_TAG_TOP,
      AT_SIMILAR
    };

  } // end namespace LastFM
} // end namespace Bmp

#endif // BMP_LAST_FM_TYPES_HH

