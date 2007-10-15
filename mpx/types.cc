//  MPX - The Dumb Music Player
//  Copyright (C) 2007 MPX Project 
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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.
#include "config.h"
#include "mpx/types.hh"
using namespace MPX::SQL;

namespace MPX
{
    Album::Album (Row const& row)
    {
      m_row = row;

      Row::const_iterator i, end = row.end();

      i = row.find ("album");
      if (i != end)
        album = boost::get <std::string> (i->second);

      i = row.find ("mb_album_id");
      if (i != end)
        mb_album_id = boost::get <std::string> (i->second);

      i = row.find ("mb_release_date");
      if (i != end)
        mb_release_date = boost::get <std::string> (i->second);

      i = row.find ("amazon_asin");
      if (i != end)
        asin = boost::get <std::string> (i->second);

      i = row.find ("album_artist_j");
      if (i != end)
        bmpx_album_artist_id = boost::get <gint64> (i->second);

      i = row.find ("id");
      if (i != end)
        bmpx_album_id = boost::get <gint64> (i->second);

      i = row.find ("album_artist");
      if (i != end)
        album_artist = boost::get <std::string> (i->second);

      i = row.find ("mb_album_artist_id");
      if (i != end)
        mb_album_artist_id = boost::get <std::string> (i->second);

      i = row.find ("mb_album_artist_sortname");
      if (i != end)
        mb_album_artist_sort_name = boost::get <std::string> (i->second);

      i = row.find ("is_mb_album_artist");
      if (i != end)
        is_mb_album_artist = true; 
      else
        is_mb_album_artist = false; 
    }

    Artist::Artist (MPX::SQL::Row const& row) 
    {
      m_row = row;

      Row::const_iterator i, end = row.end ();

      i = row.find ("artist");
      if (i != end)
        artist = boost::get <std::string> (i->second);
         
      i = row.find ("mb_artist_id");
      if (i != end)
        mb_artist_id = boost::get <std::string> (i->second);

      i = row.find ("artist_sortname");
      if (i != end)
        mb_artist_sort_name = boost::get <std::string> (i->second);

      i = row.find ("id");
      if (i != end)
        bmpx_artist_id = boost::get <gint64> (i->second);
    }

    AlbumArtist::AlbumArtist (MPX::SQL::Row const& row) 
    {
      m_row = row;

      Row::const_iterator i, end = row.end ();

      i = row.find ("album_artist");
      if (i != end)
        album_artist = boost::get <std::string> (i->second);

      i = row.find ("mb_album_artist_id");
      if (i != end)
        mb_album_artist_id = boost::get <std::string> (i->second);

      i = row.find ("mb_album_artist_sortname");
      if (i != end)
        mb_album_artist_sort_name = boost::get <std::string> (i->second);

      i = row.find ("is_mb_album_artist");
      if (i != end)
        is_mb_album_artist = true; 
      else
        is_mb_album_artist = false; 

      i = row.find ("id");
      if (i != end)
        bmpx_album_artist_id = boost::get <gint64> (i->second);
    }
} //namespace MPX 
