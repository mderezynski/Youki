//  MPX
//  Copyright (C) 2005-2007 MPX development.
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
#include <boost/format.hpp>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gio/gvfs.h>
#include "audio.hh"
#ifdef HAVE_HAL
#include "hal.hh"
#endif // HAVE_HAL
#include "library.hh"
#include "sql.hh"
#include "util-string.hh"
#include "uri.hh"
using namespace Glib;
using boost::get;

namespace
{
    using namespace MPX::SQL;

    struct AttrInfo
    {
        char const* id;
        ValueType   type;
    };

    AttrInfo attrs[] =
    {
        {   "location",
            VALUE_TYPE_STRING   }, 

        {   "title",
            VALUE_TYPE_STRING   }, 

        {   "genre",
            VALUE_TYPE_STRING   }, 

        {   "comment",
            VALUE_TYPE_STRING   }, 

        {   "musicip_puid",
            VALUE_TYPE_STRING   }, 

        {   "hash",
            VALUE_TYPE_STRING   }, 

        {   "mb_track_id",
            VALUE_TYPE_STRING   }, 

        {   "artist",
            VALUE_TYPE_STRING   }, 

        {   "artist_sortname",
            VALUE_TYPE_STRING   }, 

        {   "mb_artist_id",
            VALUE_TYPE_STRING   }, 

        {   "album",
            VALUE_TYPE_STRING   }, 

        {   "mb_album_id",
            VALUE_TYPE_STRING   }, 

        {   "mb_release_date",
            VALUE_TYPE_STRING   }, 

        {   "amazon_asin",
            VALUE_TYPE_STRING   }, 

        {   "album_artist",
            VALUE_TYPE_STRING   }, 

        {   "album_artist_sortname",
            VALUE_TYPE_STRING   }, 

        {   "mb_album_artist_id",
            VALUE_TYPE_STRING   }, 

        {   "type",
            VALUE_TYPE_STRING   }, 

        {   "hal_volume_udi",
            VALUE_TYPE_STRING   }, 

        {   "hal_device_udi",
            VALUE_TYPE_STRING   }, 

        {   "hal_vrp",
            VALUE_TYPE_STRING   }, 

        {   "insert_path",
            VALUE_TYPE_STRING   }, 

        {   "location_name",
            VALUE_TYPE_STRING   }, 

        {   "track",
            VALUE_TYPE_INT      },

        {   "time",
            VALUE_TYPE_INT      },

        {   "rating",
            VALUE_TYPE_INT      },

        {   "date",
            VALUE_TYPE_INT      },

        {   "mtime",
            VALUE_TYPE_INT      },

        {   "bitrate",
            VALUE_TYPE_INT      },

        {   "samplerate",
            VALUE_TYPE_INT      },

        {   "pcount",
            VALUE_TYPE_INT      },

        {   "pdate",
            VALUE_TYPE_INT      },

        {   "is_mb_album_artist",
            VALUE_TYPE_INT      },

        {   "active",
            VALUE_TYPE_INT      },
    };
}

namespace MPX
{
#ifdef HAVE_HAL
    Library::Library (HAL & hal, TaskKernel & kernel, bool use_hal)
    : m_HAL (hal)
    , m_TaskKernel (kernel)
#else
    Library::Library (TaskKernel & kernel)
    : m_TaskKernel (kernel)
#endif
    , m_Flags (0)
    {
        const int MLIB_VERSION_CUR = 1;
        const int MLIB_VERSION_REV = 0;
        const int MLIB_VERSION_AGE = 0;

        try{
          m_SQL = new SQL::SQLDB ((boost::format ("mpxdb-%d-%d-%d") % MLIB_VERSION_CUR 
                                     % MLIB_VERSION_REV
                                     % MLIB_VERSION_AGE).str(), build_filename(g_get_user_data_dir(),"mpx"), SQLDB_OPEN);
          }
        catch (DbInitError & cxe)
          {
            g_message("%s: Error Opening the DB", G_STRLOC);
            // FIXME: ?
          }

        if(!m_SQL->table_exists("meta"))
        {
            m_SQL->exec_sql ("CREATE TABLE meta (version STRING, flags INTEGER DEFAULT 0);");
#ifdef HAVE_HAL
            m_Flags |= (use_hal ? F_USING_HAL : 0); 
#else
            m_Flags = 0;
#endif
            m_SQL->exec_sql ((boost::format ("INSERT INTO meta (flags) VALUES(%lld);") % m_Flags).str());
        }
        else
        {
            RowV rows;
            m_SQL->get (rows, "SELECT flags FROM meta;"); 
            m_Flags |= get<gint64>(rows[0].find("flags")->second); 
        }

        static boost::format
          artist_table_f ("CREATE TABLE IF NOT EXISTS artist "
                            "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, '%s' TEXT, "
                            "UNIQUE ('%s', '%s', '%s'));");

        m_SQL->exec_sql ((artist_table_f  % attrs[ATTRIBUTE_ARTIST].id
                                              % attrs[ATTRIBUTE_MB_ARTIST_ID].id
                                              % attrs[ATTRIBUTE_ARTIST_SORTNAME].id
                                              % attrs[ATTRIBUTE_ARTIST].id
                                              % attrs[ATTRIBUTE_MB_ARTIST_ID].id
                                              % attrs[ATTRIBUTE_ARTIST_SORTNAME].id).str());

        static boost::format
          album_artist_table_f ("CREATE TABLE IF NOT EXISTS album_artist "
                                  "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, "
                                  "'%s' TEXT, '%s' INTEGER, UNIQUE ('%s', '%s', '%s', '%s'));");

        m_SQL->exec_sql ((album_artist_table_f  % attrs[ATTRIBUTE_ALBUM_ARTIST].id
                                                    % attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id
                                                    % attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id
                                                    % attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id
                                                    % attrs[ATTRIBUTE_ALBUM_ARTIST].id
                                                    % attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id
                                                    % attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id
                                                    % attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id).str());

        static boost::format
          album_table_f ("CREATE TABLE IF NOT EXISTS album "
                          "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, "
                          "'%s' TEXT, '%s' TEXT, '%s' INTEGER, UNIQUE "
                          "('%s', '%s', '%s', '%s', '%s'));");

        m_SQL->exec_sql ((album_table_f % attrs[ATTRIBUTE_ALBUM].id
                                          % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                          % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                          % attrs[ATTRIBUTE_ASIN].id
                                          % "album_artist_j"
                                          % attrs[ATTRIBUTE_ALBUM].id
                                          % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                          % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                          % attrs[ATTRIBUTE_ASIN].id
                                          % "album_artist_j").str());

        StrV columns;
        for (unsigned int n = 0; n < G_N_ELEMENTS(attrs); ++n)
        {
            std::string column (attrs[n].id);
            switch (attrs[n].type)
            {
              case VALUE_TYPE_STRING:
                column += (" TEXT DEFAULT NULL ");
                break;

              case VALUE_TYPE_INT:
                column += (" INTEGER DEFAULT NULL ");
                break;

              case VALUE_TYPE_REAL:
                column += (" REAL DEFAULT NULL ");
                break;
            }
            columns.push_back (column);
        }

        std::string column_names (Util::stdstrjoin (columns, ", "));

        static boost::format
          track_table_f ("CREATE TABLE IF NOT EXISTS track (id INTEGER PRIMARY KEY AUTOINCREMENT, %s, %s, %s, "
                         "UNIQUE (%s, %s, %s, %s));");

        m_SQL->exec_sql ((track_table_f
                                      % column_names
                                      % "artist_j INTEGER NOT NULL"   // track artist information 
                                      % "album_j INTEGER NOT NULL"    // album + album artist
                                      % attrs[ATTRIBUTE_HAL_VOLUME_UDI].id
                                      % attrs[ATTRIBUTE_HAL_DEVICE_UDI].id
                                      % attrs[ATTRIBUTE_VOLUME_RELATIVE_PATH].id
                                      % attrs[ATTRIBUTE_LOCATION].id).str());

        m_SQL->exec_sql ("CREATE VIEW IF NOT EXISTS track_view AS " 
                         "SELECT"
                         "  track.* "
                         ", album.id AS bmpx_album_id"
                         ", artist.id AS bmpx_artist_id "
                         ", album_artist.id AS bmpx_album_artist_id "
                         " FROM track "
                         "JOIN album ON album.id = track.album_j JOIN artist "
                         "ON artist.id = track.artist_j JOIN album_artist ON album.album_artist_j = album_artist.id;");

        static boost::format
          tag_table_f ("CREATE TABLE IF NOT EXISTS tag "
                       "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT NOT NULL)");

        static boost::format
          tag_datum_table_f ("CREATE TABLE IF NOT EXISTS %s "
                             "(id INTEGER PRIMARY KEY AUTOINCREMENT, tag_id INTEGER NOT NULL, fki INTEGER NOT NULL)");

        m_SQL->exec_sql ((tag_table_f  % "tag").str()); 
        m_SQL->exec_sql ((tag_datum_table_f  % "artist_tag").str()); 
        m_SQL->exec_sql ((tag_datum_table_f  % "title_tag").str()); 
    }

    Library::~Library ()
    {
    }

    void
    Library::getMetadata (const std::string& uri, Track & track)
    {
        mReaderTagLib.get(uri, track);
    }

    gint64
    Library::get_track_artist_id (Track &track, bool only_if_exists)
    {
      RowV rows; 

      char const* select_artist_f ("SELECT id FROM artist WHERE %s %s AND %s %s AND %s %s;"); 
      m_SQL->get (rows, mprintf (select_artist_f,

             attrs[ATTRIBUTE_ARTIST].id,
            (track[ATTRIBUTE_ARTIST]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ARTIST].get()).c_str()).c_str()
                : "IS NULL"),

             attrs[ATTRIBUTE_MB_ARTIST_ID].id,
            (track[ATTRIBUTE_MB_ARTIST_ID]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()).c_str()).c_str()
                : "IS NULL"),

             attrs[ATTRIBUTE_ARTIST_SORTNAME].id,
            (track[ATTRIBUTE_ARTIST_SORTNAME]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()).c_str()).c_str()
                : "IS NULL")));

      gint64 artist_j = 0;

      if (rows.size ())
      {
        artist_j = get <gint64> (rows[0].find ("id")->second);
      }
      else
      if (!only_if_exists)
      {
        char const* set_artist_f ("INSERT INTO artist (%s, %s, %s) VALUES (%Q, %Q, %Q);");

        m_SQL->exec_sql (mprintf (set_artist_f,

             attrs[ATTRIBUTE_ARTIST].id,
             attrs[ATTRIBUTE_ARTIST_SORTNAME].id,
             attrs[ATTRIBUTE_MB_ARTIST_ID].id,

            (track[ATTRIBUTE_ARTIST]
                ? get<std::string>(track[ATTRIBUTE_ARTIST].get()).c_str()
                : NULL) ,

            (track[ATTRIBUTE_ARTIST_SORTNAME]
                ? get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()).c_str()
                : NULL) ,

            (track[ATTRIBUTE_MB_ARTIST_ID]
                ? get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()).c_str()
                : NULL))) ;

        artist_j = m_SQL->last_insert_rowid ();
      }
      return artist_j;
    }

    gint64
    Library::get_album_artist_id (Track& track, bool only_if_exists)
    {
        if (track[ATTRIBUTE_ALBUM_ARTIST] && track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]) 
        {
          track[ATTRIBUTE_IS_MB_ALBUM_ARTIST] = gint64(1); 
        }
        else
        {
          track[ATTRIBUTE_IS_MB_ALBUM_ARTIST] = gint64(0); 
          track[ATTRIBUTE_ALBUM_ARTIST] = track[ATTRIBUTE_ARTIST];
        }

        RowV rows;

        if( track[ATTRIBUTE_MB_ALBUM_ARTIST_ID] )   
        {
          char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s) AND (%s %s) AND (%s %s);"); 
          m_SQL->get (rows, (mprintf (select_artist_f, 

             attrs[ATTRIBUTE_ALBUM_ARTIST].id,
            (track[ATTRIBUTE_ALBUM_ARTIST]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
            (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id,
            (track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,
            (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
                ? "= '1'"
                : "IS NULL")))
          );
        }
        else // TODO: MB lookup to fix sortname, etc, for a given ID (and possibly even retag files on the fly?)
        {
          char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s);"); 
          m_SQL->get (rows, (mprintf (select_artist_f, 

             attrs[ATTRIBUTE_ALBUM_ARTIST].id,
            (track[ATTRIBUTE_ALBUM_ARTIST]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
            (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()).c_str()
                : "IS NULL")))) ; 
        }

        gint64 artist_j = 0;

        if (!rows.empty())
        {
          artist_j = get <gint64> (rows[0].find ("id")->second);
        }
        else if (!only_if_exists)
        {
          char const* set_artist_f ("INSERT INTO album_artist (%s, %s, %s, %s) VALUES (%Q, %Q, %Q, %Q);");

          m_SQL->exec_sql (mprintf (set_artist_f,

            attrs[ATTRIBUTE_ALBUM_ARTIST].id,
            attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id,
            attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
            attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,

          (track[ATTRIBUTE_ALBUM_ARTIST]
              ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str() 
              : NULL) , 

          (track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME] 
              ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].get()).c_str()
              : NULL) , 

          (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
              ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()
              : NULL) ,

          (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
              ? "1"
              : NULL))) ;

          artist_j = m_SQL->last_insert_rowid ();

          Signals.NewArtist.emit(  (track[ATTRIBUTE_MB_ARTIST_ID] ? get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()) : std::string())
                                ,   artist_j );
        }
        return artist_j;
    }

    gint64
    Library::get_album_id (Track& track, gint64 album_artist_id, bool only_if_exists)
    {
        gint64 album_j = 0;
        RowV rows;
        std::string sql;

        if( track[ATTRIBUTE_MB_ALBUM_ID] )
        {
          char const* select_album_f ("SELECT album, id FROM album WHERE (%s %s) AND (%s %s) AND (%s %s) AND (%s %s) AND (%s = %lld);"); 

          sql = mprintf (select_album_f,

                 attrs[ATTRIBUTE_ALBUM].id,
                (track[ATTRIBUTE_ALBUM]
                    ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()).c_str()
                    : "IS NULL"), 

                 attrs[ATTRIBUTE_MB_ALBUM_ID].id,
                (track[ATTRIBUTE_MB_ALBUM_ID]
                    ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()).c_str()).c_str()
                    : "IS NULL"), 

                 attrs[ATTRIBUTE_MB_RELEASE_DATE].id,
                (track[ATTRIBUTE_MB_RELEASE_DATE]
                    ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()).c_str()).c_str()
                    : "IS NULL"), 

                attrs[ATTRIBUTE_ASIN].id,
                (track[ATTRIBUTE_ASIN]
                    ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()).c_str()
                    : "IS NULL"), 

                "album_artist_j",
                album_artist_id
          );
        }
        else
        {
          char const* select_album_f ("SELECT album, id FROM album WHERE (%s %s) AND (%s %s) AND (%s = %lld);"); 

          sql = mprintf (select_album_f,

                 attrs[ATTRIBUTE_ALBUM].id,
                (track[ATTRIBUTE_ALBUM]
                    ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()).c_str()
                    : "IS NULL"), 

                attrs[ATTRIBUTE_ASIN].id,
                (track[ATTRIBUTE_ASIN]
                    ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()).c_str()
                    : "IS NULL"), 

                "album_artist_j",
                album_artist_id
          );
        }

        m_SQL->get (rows, sql); 

        if (!rows.empty())
        {
            album_j = get <gint64> (rows[0].find ("id")->second);
        }
        else if (!only_if_exists)
        {
          char const* set_album_f ("INSERT INTO album (%s, %s, %s, %s, %s) VALUES (%Q, %Q, %Q, %Q, %lld);");

          std::string sql = mprintf (set_album_f,

              attrs[ATTRIBUTE_ALBUM].id,
              attrs[ATTRIBUTE_MB_ALBUM_ID].id,
              attrs[ATTRIBUTE_MB_RELEASE_DATE].id,
              attrs[ATTRIBUTE_ASIN].id,
              "album_artist_j",

              (track[ATTRIBUTE_ALBUM]
                  ? get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()
                  : NULL) , 

              (track[ATTRIBUTE_MB_ALBUM_ID]
                  ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()).c_str()
                  : NULL) , 

              (track[ATTRIBUTE_MB_RELEASE_DATE]
                  ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()).c_str()
                  : NULL) , 

              (track[ATTRIBUTE_ASIN]
                  ? get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()
                  : NULL) , 

              album_artist_id);

          m_SQL->exec_sql (sql);
          album_j = m_SQL->last_insert_rowid ();

          Signals.NewAlbum.emit(    (track[ATTRIBUTE_MB_ALBUM_ID] ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()) : std::string())
                                ,   (track[ATTRIBUTE_ASIN] ? get<std::string>(track[ATTRIBUTE_ASIN].get()) : std::string())
                                ,   album_j );
        }
        return album_j;
    }

    gint64
    Library::get_track_mtime (Track& track) const
    {
      RowV rows;
      if ((m_Flags & F_USING_HAL) == F_USING_HAL)
      {
        static boost::format
          select_f ("SELECT %s FROM track WHERE %s='%s' AND %s='%s' AND %s='%s';");

        m_SQL->get (rows, (select_f  % attrs[ATTRIBUTE_MTIME].id
                                    % attrs[ATTRIBUTE_HAL_VOLUME_UDI].id
                                    % get<std::string>(track[ATTRIBUTE_HAL_VOLUME_UDI].get())
                                    % attrs[ATTRIBUTE_HAL_DEVICE_UDI].id
                                    % get<std::string>(track[ATTRIBUTE_HAL_DEVICE_UDI].get())
                                    % attrs[ATTRIBUTE_VOLUME_RELATIVE_PATH].id
                                    % mprintf ("%q", get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str())).str());
      }
      else
      {
        static boost::format
          select_f ("SELECT %s FROM track WHERE %s='%s';");

        m_SQL->get (rows, (select_f  % attrs[ATTRIBUTE_MTIME].id
                                    % attrs[ATTRIBUTE_LOCATION].id
                                    % mprintf ("%q", get<std::string>(track[ATTRIBUTE_LOCATION].get()).c_str())).str());
      }

      if (rows.size() && (rows[0].count (attrs[ATTRIBUTE_MTIME].id) != 0))
      {
        return boost::get<gint64>( rows[0].find (attrs[ATTRIBUTE_MTIME].id)->second ) ;
      }

      return 0;
    }
 
    gint64
    Library::get_track_id (Track& track) const
    {
      RowV rows;
      if ((m_Flags & F_USING_HAL) == F_USING_HAL)
      {
        static boost::format
          select_f ("SELECT id FROM track WHERE %s='%s' AND %s='%s' AND %s='%s';");

        m_SQL->get (rows, (select_f % attrs[ATTRIBUTE_HAL_VOLUME_UDI].id 
                                    % get<std::string>(track[ATTRIBUTE_HAL_VOLUME_UDI].get())
                                    % attrs[ATTRIBUTE_HAL_DEVICE_UDI].id
                                    % get<std::string>(track[ATTRIBUTE_HAL_DEVICE_UDI].get())
                                    % attrs[ATTRIBUTE_VOLUME_RELATIVE_PATH].id
                                    % mprintf ("%q", get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str())).str());
      }
      else
      {
        static boost::format
          select_f ("SELECT id FROM track WHERE %s='%s';");

        m_SQL->get (rows, (select_f % attrs[ATTRIBUTE_LOCATION].id 
                                    % mprintf ("%q", get<std::string>(track[ATTRIBUTE_LOCATION].get()).c_str())).str());
      }

      if (rows.size() && (rows[0].count("id") != 0))
      {
        return boost::get<gint64>( rows[0].find("id")->second ) ;
      }

      return 0;
    }

    Library::ScanResult
    Library::insert (const std::string& uri, const std::string& insert_path, const std::string& name)
    {
      std::string type;        
   
      try{ 
          if (!Audio::typefind (uri, type))
            return SCAN_RESULT_ERROR ;
        }  
      catch (Glib::ConvertError & cxe)
        {
            return SCAN_RESULT_ERROR ;
        }

      Track track;

      track[ATTRIBUTE_TYPE] = type ;
      track[ATTRIBUTE_LOCATION] = uri ;
      track[ATTRIBUTE_LOCATION_NAME] = name; 

      if( !mReaderTagLib.get( uri, track ) )
        return SCAN_RESULT_ERROR ; // no play for us

      if( !(track[ATTRIBUTE_ALBUM] && track[ATTRIBUTE_ARTIST] && track[ATTRIBUTE_TITLE]) )
        return SCAN_RESULT_ERROR ;

      std::string insert_path_value ; 
      URI u (uri);

      if(u.get_protocol() == URI::PROTOCOL_FILE)
      {
          GFile * file = g_vfs_get_file_for_uri(g_vfs_get_default(), uri.c_str()); 
          GFileInfo * info = g_file_query_info(file,
                                            G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                            GFileQueryInfoFlags(0),
                                            NULL,
                                            NULL);

          track[ATTRIBUTE_MTIME] = gint64 (g_file_info_get_attribute_uint64(info,G_FILE_ATTRIBUTE_TIME_MODIFIED));

          g_object_unref(file);
          g_object_unref(info);

          time_t mtime = get_track_mtime (track);
          if ((mtime != 0) && (mtime == get<gint64>(track[ATTRIBUTE_MTIME].get()) ) )
            return SCAN_RESULT_UPTODATE ;

#ifdef HAVE_HAL
          try{
              if ((m_Flags & F_USING_HAL) == F_USING_HAL)
              {
                HAL::Volume const& volume (m_HAL.get_volume_for_uri (uri));

                track[ATTRIBUTE_HAL_VOLUME_UDI] =
                              volume.volume_udi ;

                track[ATTRIBUTE_HAL_DEVICE_UDI] =
                              volume.device_udi ;

                track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                              filename_from_uri (uri).substr (volume.mount_point.length()) ;

                insert_path_value = insert_path.substr (volume.mount_point.length()) ;
              }
              else
#endif
              {
                insert_path_value = insert_path;
              }
          }
#ifdef HAVE_HAL
        catch (HAL::Exception & cxe)
          {
            g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
            return SCAN_RESULT_ERROR ;
          }
        catch (Glib::ConvertError & cxe)
          {
            g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
            return SCAN_RESULT_ERROR ;
          }
#endif
      }
      else
      {
        insert_path_value = insert_path;
      }      

      track[ATTRIBUTE_INSERT_PATH] = insert_path_value;
      track[ATTRIBUTE_NEW_ITEM] = gint64(1); 

      char const* track_set_f ("INSERT INTO track (%s) VALUES (%s);");

      /* Now let's prepare the SQL */
      std::string column_names;
      column_names.reserve (0x400);

      std::string column_values;
      column_values.reserve (0x400);

      try{
          gint64 artist_j = get_track_artist_id (track);
          gint64 album_j = get_album_id (track, get_album_artist_id (track));

          for (unsigned int n = 0; n < N_ATTRIBUTES_INT; ++n)
          {
              if(track[n])
              {
                  switch (attrs[n].type)
                  {
                    case VALUE_TYPE_STRING:
                      column_values += mprintf ("'%q'", get <std::string> (track[n].get()).c_str());
                      break;

                    case VALUE_TYPE_INT:
                      column_values += mprintf ("'%lld'", get <gint64> (track[n].get()));
                      break;

                    case VALUE_TYPE_REAL:
                      column_values += mprintf ("'%f'", get <double> (track[n].get()));
                      break;
                  }

                  column_names += std::string (attrs[n].id) + ",";
                  column_values += ",";
              }
          }

          column_names += "artist_j, album_j";
          column_values += mprintf ("'%lld'", artist_j) + "," + mprintf ("'%lld'", album_j); 
          m_SQL->exec_sql (mprintf (track_set_f, column_names.c_str(), column_values.c_str()));
          track[ATTRIBUTE_MPX_TRACK_ID] = m_SQL->last_insert_rowid ();
          Signals.NewTrack.emit( track ); 
      }
      catch (SqlConstraintError & cxe)
      {
          gint64 id = get_track_id (track);
          if( id != 0 )
          {
              static boost::format delete_track_f ("DELETE FROM track WHERE id='%lld';");
              m_SQL->exec_sql ((delete_track_f % id).str()); 

              m_SQL->exec_sql (mprintf (track_set_f, column_names.c_str(), column_values.c_str()));
              gint64 new_id = m_SQL->last_insert_rowid ();
              m_SQL->exec_sql (mprintf ("UPDATE track SET id = '%lld' WHERE id = '%lld';", id, new_id));
              return SCAN_RESULT_UPDATE ;
          }
          else
          {
              g_warning("%s: Got track ID 0 for internal track! Highly supicious! Please report!", G_STRLOC);
              return SCAN_RESULT_ERROR ;
          }
      }
      catch (SqlExceptionC & cxe)
      {
          g_message("%s: SQL Error: %s", G_STRFUNC, cxe.what());
          return SCAN_RESULT_ERROR ;
      }

      return SCAN_RESULT_OK ; 
    }

    void
    Library::scanUri (const std::string& uri, const std::string& name)
    {
        ScanDataP p (new ScanData);
        
        try{
            p->insert_path = uri; 
            p->name = name;
            Util::collect_audio_paths( p->insert_path, p->collection );
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
            return;
        }

        p->position = p->collection.begin();
        m_ScanTID = m_TaskKernel.newTask( _("Library Rescan"),
            sigc::bind( sigc::mem_fun (*this, &Library::scanInit), p ),
            sigc::bind( sigc::mem_fun (*this, &Library::scanRun), p ),
            sigc::bind( sigc::mem_fun (*this, &Library::scanEnd), p ));
    }

    void
    Library::scanInit (ScanDataP p)
    {
        m_SQL->exec_sql("BEGIN;");
        Signals.ScanStart.emit();
    }

    bool
    Library::scanRun (ScanDataP p)
    {
        try{
            switch(insert( *(p->position) , p->insert_path, p->name ))
            {
                case SCAN_RESULT_UPTODATE:
                    ++(p->uptodate) ;
                    break;

                case SCAN_RESULT_OK:
                    ++(p->added) ;
                    break;

                case SCAN_RESULT_ERROR:
                    ++(p->erroneous) ;
                    break;

                case SCAN_RESULT_UPDATE:
                    ++(p->updated) ;
                    break;
            }
              
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
        }

        ++(p->position);
        Signals.ScanRun.emit(std::distance(p->collection.begin(), p->position), p->collection.size());

        return p->position != p->collection.end();
    }

    void
    Library::scanEnd (bool aborted, ScanDataP p)
    {
        if( aborted )
        {  
            m_SQL->exec_sql("ROLLBACK;");
        }
        else
            m_SQL->exec_sql("COMMIT;");

        Signals.ScanEnd.emit(p->added, p->uptodate, p->updated, p->erroneous, p->collection.size());
        p.reset();
    }
    
} // namespace MPX
