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
#ifndef MPX_LIBRARY_CLASS_HH
#define MPX_LIBRARY_CLASS_HH

#include "metadatareader-taglib.hh"
#include "mpx/types.hh"
#include "sql.hh"

namespace MPX
{
  class HAL;
  class Library
  {
            HAL * m_HAL;

        public:
            Library (HAL *) ;
            ~Library () ;

            void
            get_metadata (std::string const& uri, Track & track) ;

            bool
            insert (const std::string& uri, const std::string& insert_path);

        private:

            SQL::SQLDB * m_SQL;
            MetadataReaderTagLib mReaderTagLib ; 

            gint64
            get_track_artist_id (Track& row, bool only_if_exists = false);

            gint64
            get_album_artist_id (Track& row, bool only_if_exists = false);

            gint64
            get_album_id (Track& row, gint64 album_artist_id, bool only_if_exists = false);

            gint64
            get_track_mtime (Track& track) const;

            gint64
            get_track_id (Track& track) const;
    };

} // namespace MPX

#endif // !MPX_LIBRARY_CLASS_HH
