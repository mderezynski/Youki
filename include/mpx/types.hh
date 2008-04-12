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
#ifndef MPX_LIBRARY_TYPES_HH
#define MPX_LIBRARY_TYPES_HH
#include "config.h"
#include <glibmm.h>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#ifdef HAVE_TR1
#include<tr1/unordered_map>
#endif // HAVE_TR1
#include <string>
#include <vector>

namespace MPX
{
    enum AttributeIdString
    {
      ATTRIBUTE_LOCATION,
      ATTRIBUTE_TITLE,

      ATTRIBUTE_GENRE,
      ATTRIBUTE_COMMENT,

      ATTRIBUTE_MUSICIP_PUID,

      ATTRIBUTE_HASH,     
      ATTRIBUTE_MB_TRACK_ID, 

      ATTRIBUTE_ARTIST,
      ATTRIBUTE_ARTIST_SORTNAME,
      ATTRIBUTE_MB_ARTIST_ID,

      ATTRIBUTE_ALBUM,
      ATTRIBUTE_MB_ALBUM_ID,
      ATTRIBUTE_MB_RELEASE_DATE,
      ATTRIBUTE_ASIN,

      ATTRIBUTE_ALBUM_ARTIST,
      ATTRIBUTE_ALBUM_ARTIST_SORTNAME,
      ATTRIBUTE_MB_ALBUM_ARTIST_ID,

      // MIME type
      ATTRIBUTE_TYPE,

      // HAL
      ATTRIBUTE_HAL_VOLUME_UDI,
      ATTRIBUTE_HAL_DEVICE_UDI,
      ATTRIBUTE_VOLUME_RELATIVE_PATH,
        
      // SQL
      ATTRIBUTE_INSERT_PATH,
      ATTRIBUTE_LOCATION_NAME,

      N_ATTRIBUTES_STRING
    };

    enum AttributeIdInt
    {
      ATTRIBUTE_TRACK = N_ATTRIBUTES_STRING,
      ATTRIBUTE_TIME,
      ATTRIBUTE_RATING,
      ATTRIBUTE_DATE,
      ATTRIBUTE_MTIME,
      ATTRIBUTE_BITRATE,
      ATTRIBUTE_SAMPLERATE,
      ATTRIBUTE_COUNT,
      ATTRIBUTE_PLAYDATE,
      ATTRIBUTE_INSERT_DATE,  
      ATTRIBUTE_IS_MB_ALBUM_ARTIST,

      ATTRIBUTE_ACTIVE,
      ATTRIBUTE_MPX_TRACK_ID,
	  ATTRIBUTE_MPX_ALBUM_ID,

      N_ATTRIBUTES_INT
    };

    typedef boost::variant<gint64, gdouble, std::string> Variant;
    typedef boost::optional<Variant> OVariant;

    class Track
    {
          typedef std::vector<OVariant> DataT;
          
          DataT data;
    
        public:

          OVariant & operator[](DataT::size_type index)
          {
              return data[index];
          }

          Track () { data.resize( N_ATTRIBUTES_INT ); }
    };

    typedef boost::optional<std::string> ostring;

    namespace SQL
    {
      enum ValueType
      {
          VALUE_TYPE_INT,
          VALUE_TYPE_REAL,
          VALUE_TYPE_STRING  
      }; 

      typedef boost::variant< gint64, double, std::string > Variant;

  #ifdef HAVE_TR1
      typedef std::tr1::unordered_map < std::string, Variant > Row;
  #else //!HAVE_TR1
      typedef std::map < std::string, Variant > Row;
  #endif //HAVE_TR1
      typedef Row::value_type             VariantPair;

      typedef std::vector < Row >           RowV;
      typedef std::vector < std::string >   ColumnV;

    } // namespace SQL
} //namespace MPX 

#endif //!MPX_LIBRARY_TYPES_HH
