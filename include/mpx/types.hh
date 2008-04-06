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

      enum MatchStyle
      {
        NOOP,
        FUZZY,
        EXACT,
        NOT_EQUAL,
        GREATER_THAN,
        LESSER_THAN,
        GREATER_THAN_OR_EQUAL,
        LESSER_THAN_OR_EQUAL,
        NOT_NULL,
        IS_NULL
      };

      enum Chain
      {
        CHAIN_AND,
        CHAIN_OR,
      };

      typedef std::vector < Chain > AttributeChain;

      struct Attribute
      {
        MatchStyle    m_match_style;
        Glib::ustring m_name;
        Variant       m_variant;

        Attribute (MatchStyle match_style, Glib::ustring const& name, Variant variant)
          : m_match_style (match_style),
            m_name        (name),
            m_variant     (variant)
            {}

        Attribute (Glib::ustring const& name, Variant variant)
          : m_match_style (NOOP),
            m_name        (name),
            m_variant     (variant)
            {}

        Attribute (MatchStyle match_style, Glib::ustring const& name)
          : m_match_style (match_style),
            m_name        (name)
          {}

        Attribute () {}
        ~Attribute () {}
      };

      /** A vector of @link Attribute@endlink
        *
        */
      typedef std::vector <Attribute>                   AttributeV;
      typedef std::vector <AttributeV>                  AttributeVV;
      typedef std::pair   <AttributeV, AttributeChain>  AttributeSeqP;
      typedef std::vector <AttributeSeqP>               AttributeSeq;

      /** Query class, contains information to structure an SQL query  
        *
        */

      class SQLDB;
      class Query
      {
        private:

          AttributeSeq    m_attributes;
          AttributeChain  m_seq_chain;
          std::string     m_prefix;
          std::string     m_suffix;
          std::string     m_columns;

          friend class SQLDB;

        public:

          Query () : m_columns ("*") {}

          Query (AttributeV const& attributes)
          : m_columns ("*")
          {
            m_attributes.push_back (AttributeSeqP (attributes, AttributeChain()));
            attr_join (0, CHAIN_AND);
          }

          Query (AttributeV const& attributes,
                 AttributeChain const& chain)
          : m_columns ("*")
          {
            m_attributes.push_back (AttributeSeqP (attributes, chain));
          }

          Query (AttributeV const& attributes,
                 Chain chain) 
          : m_columns ("*")
          {
            m_attributes.push_back (AttributeSeqP (attributes, AttributeChain()));
            attr_join (0, chain);
          }

          ///---end ctors

          void
          set_seq_chain (AttributeChain const& chain)
          {
            m_seq_chain = chain;
          }

          void
          seq_chain_append (Chain chain)
          {
            m_seq_chain.push_back (chain);
          }

          void
          add_attr_v (AttributeV const& attributes)
          {
            m_attributes.push_back (AttributeSeqP (attributes, AttributeChain()));
          }

          void
          add_attr_v (AttributeV const& attributes, Chain chain)
          {
            m_attributes.push_back (AttributeSeqP (attributes, AttributeChain()));
            AttributeSeq::size_type n (m_attributes.size());
            attr_join (n-1, chain);
          }

          void
          add_attr_v (AttributeV const& attributes, AttributeChain const& chain)
          {
            m_attributes.push_back (AttributeSeqP (attributes, chain)); 
          }

          void
          attr_join (AttributeSeq::size_type i, Chain c_op)
          {
            AttributeV::size_type z (m_attributes[i].first.size());
            AttributeChain & chain (m_attributes[i].second);
            chain.clear ();
            for (AttributeChain::size_type n = 0; n < z; ++n)
            {
              chain.push_back (c_op);
            }
          }

          void
          set_prefix (std::string const& prefix)
          {
            m_prefix = prefix;
          }

          void
          set_suffix (std::string const& suffix)
          {
            m_suffix = suffix;
          }

          void
          set_columns (std::string const& columns)
          {
            m_columns = columns;
          }
      };
    } // namespace SQL

    class Artist
    {
      public:

        Artist (SQL::Row const& row);
        Artist () {}
        virtual ~Artist () {}

        ostring   artist;
        ostring   mb_artist_id;
        ostring   mb_artist_sort_name;
        gint64    bmpx_artist_id;

        SQL::Row m_row;
    };

    class AlbumArtist
    {
      public:

        AlbumArtist (SQL::Row const& row);
        AlbumArtist () {}
        virtual ~AlbumArtist () {}

        ostring   album_artist;
        ostring   mb_album_artist_id;
        ostring   mb_album_artist_sort_name;

        gint64    is_mb_album_artist;
        gint64    bmpx_album_artist_id;

        SQL::Row m_row;
    };
    typedef std::vector <AlbumArtist> AlbumArtistV;

    class Album
    {
      public:

        Album (SQL::Row const& row);
        Album () {}
        virtual ~Album () {};

        ostring     album;
        ostring     mb_album_id;
        ostring     mb_release_date;
        ostring     asin;

        ostring     album_artist;
        ostring     mb_album_artist_id;
        ostring     mb_album_artist_sort_name;

        gint64      is_mb_album_artist;
        gint64      bmpx_album_id;    
        gint64      bmpx_album_artist_id;

        SQL::Row m_row;
    };
    typedef std::vector <Album> AlbumV;

} //namespace MPX 

#endif //!MPX_LIBRARY_TYPES_HH
