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
#include <gio/gio.h>

#include "mpx/mpx-audio.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL
#include "mpx/mpx-library.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/algorithm/random.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/util-string.hh"

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

        {   "mb_release_country",
            VALUE_TYPE_STRING   }, 

        {   "mb_release_type",
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

        {   "insert_date",
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
    Library::Library(
        HAL                     & obj_hal,
        Covers                  & obj_covers,
        MetadataReaderTagLib    & obj_reader,
        bool                      use_hal
    )
    : sigx::glib_auto_dispatchable()
    , m_HAL(obj_hal)
#else
    Library::Library(
        Covers                  & obj_covers
        MetadataReaderTagLib    & obj_reader,
    )
    : sigx::glib_auto_dispatchable()
#endif
    , m_Covers(obj_covers)
    , m_MetadataReaderTagLib(obj_reader)
    , m_Flags (0)
    {
        const int MLIB_VERSION_CUR = 1;
        const int MLIB_VERSION_REV = 0;
        const int MLIB_VERSION_AGE = 0;

        try{
          m_SQL = new SQL::SQLDB ((boost::format ("mpxdb-%d-%d-%d")
                                   % MLIB_VERSION_CUR 
                                   % MLIB_VERSION_REV
                                   % MLIB_VERSION_AGE).str(), build_filename(g_get_user_data_dir(),"mpx"), SQLDB_OPEN);
          }
        catch (DbInitError & cxe)
          {
            g_message("%s: Error Opening the DB", G_STRLOC);
          }

       if(!m_SQL->table_exists("meta"))
        {
            m_Flags = 0;

#ifdef HAVE_HAL
            m_Flags |= (use_hal ? F_USING_HAL : 0); 
#endif // HAVE_HAL

            m_SQL->exec_sql("CREATE TABLE meta (version STRING, flags INTEGER DEFAULT 0);");
            m_SQL->exec_sql((boost::format ("INSERT INTO meta (flags) VALUES(%lld);") % m_Flags).str());
        }
        else
        {
            RowV rows;
            m_SQL->get (rows, "SELECT flags FROM meta;"); 
            m_Flags |= get<gint64>(rows[0].find("flags")->second); 
        }

        m_ScannerThread = (new LibraryScannerThread(new SQL::SQLDB(*m_SQL), m_MetadataReaderTagLib, m_HAL, m_Flags));
        m_ScannerThread->run();

        m_ScannerThread->connect().signal_new_album().connect(
            sigc::mem_fun( *this, &Library::on_new_album ));
        m_ScannerThread->connect().signal_new_artist().connect(
            sigc::mem_fun( *this, &Library::on_new_artist ));
        m_ScannerThread->connect().signal_cache_cover().connect(
            sigc::bind( sigc::mem_fun( m_Covers, &Covers::cache ), true ));
        m_ScannerThread->connect().signal_reload().connect(
            sigc::mem_fun( *this, &Library::reload ));
 
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
                          "'%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT DEFAULT NULL, '%s' INTEGER DEFAULT 0, "
                          "'%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, UNIQUE "
                          "('%s', '%s', '%s', '%s', '%s', '%s', '%s'));");

        m_SQL->exec_sql ((album_table_f % attrs[ATTRIBUTE_ALBUM].id
                                        % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_TYPE].id
                                        % attrs[ATTRIBUTE_ASIN].id
										% "album_genre" 
										% "album_rating"
                                        % "album_artist_j"
                                        % "album_insert_date"
                                        % "album_new"
                                        % attrs[ATTRIBUTE_ALBUM].id
                                        % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_TYPE].id
                                        % attrs[ATTRIBUTE_ASIN].id
                                        % "album_artist_j").str());

        static boost::format
          tag_table_f ("CREATE TABLE IF NOT EXISTS tag "
                            "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, " 
                            "UNIQUE ('%s'));");

        m_SQL->exec_sql ((tag_table_f  % "tag" % "tag").str()); 


        static boost::format
          tags_table_f ("CREATE TABLE IF NOT EXISTS tags "
                            "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' INTEGER NOT NULL, '%s' INTEGER NOT NULL, " 
                            " '%s' INTEGER DEFAULT 1, "
                            "UNIQUE ('%s', '%s'));");

        m_SQL->exec_sql ((tags_table_f  % "tagid" % "trackid" % "amplitude" 
                                        % "tagid" % "trackid").str()); 


        static boost::format
          album_rating_history_f ("CREATE TABLE IF NOT EXISTS album_rating_history "
                            "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT NOT NULL, '%s' INTEGER NOT NULL, '%s' TEXT DEFAULT NULL, '%s' INTEGER NOT NULL);"); 
                    
        m_SQL->exec_sql(
            (album_rating_history_f
                % "mbid"
                % "rating"
                % "comment"
                % "time"
        ).str()); 


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
                         ", album.id AS mpx_album_id"
						 ", album.album_rating AS album_rating"
                         ", artist.id AS mpx_artist_id "
                         ", album_artist.id AS mpx_album_artist_id "
                         " FROM track "
                         "JOIN album ON album.id = track.album_j JOIN artist "
                         "ON artist.id = track.artist_j JOIN album_artist ON album.album_artist_j = album_artist.id;");

        m_SQL->exec_sql ("CREATE TABLE IF NOT EXISTS devices (id INTEGER PRIMARY KEY AUTOINCREMENT, device_udi TEXT, volume_udi TEXT);");
        m_SQL->exec_sql ("CREATE TABLE IF NOT EXISTS markov (id INTEGER PRIMARY KEY AUTOINCREMENT, count INTEGER DEFAULT 0, track_id_a INTEGER DEFAULT NULL, track_id_b INTEGER DEFAULT NULL);");
    }

#ifdef HAVE_HAL
    Library::Library(
        Library const& other
    )
    : sigx::glib_auto_dispatchable()
    , m_HAL(other.m_HAL)
#else
    Library::Library(
        Library const& other
    )
    : sigx::glib_auto_dispatchable()
#endif
    , m_Covers(other.m_Covers)
    , m_MetadataReaderTagLib(other.m_MetadataReaderTagLib)
    , m_Flags(0)
    {
        const int MLIB_VERSION_CUR = 1;
        const int MLIB_VERSION_REV = 0;
        const int MLIB_VERSION_AGE = 0;

        try{
          m_SQL = new SQL::SQLDB (*other.m_SQL);
        }
        catch (DbInitError & cxe)
        {
            g_message("%s: Error Opening the DB", G_STRLOC);
        }
    }


    Library::~Library ()
    {
    }

    bool
    Library::recache_covers_handler (SQL::RowV *v, int* position)
    {
        Row & r = (*v)[*position]; 

        std::string location;
#ifdef HAVE_HAL
        if (m_Flags & F_USING_HAL)
        {
            try{
                std::string volume_udi  = get<std::string>(r["hal_volume_udi"]); 
                std::string device_udi  = get<std::string>(r["hal_device_udi"]); 
                std::string hal_vrp     = get<std::string>(r["hal_vrp"]);
                std::string mount_point = m_HAL.get_mount_point_for_volume (volume_udi, device_udi);
                location = filename_to_uri(build_filename(mount_point, hal_vrp));
            } catch (...) {
            }
        }
        else
#endif // HAVE_HAL
        {
            location = get<std::string>(r["location"]);
        }

        std::string mb_album_id;
        std::string amazon_asin;
        std::string album_artist;
        std::string album;

        if( r.count("album.mb_album_id"))
            mb_album_id = get<std::string>(r["album.mb_album_id"]); 

        if( r.count("album.amazon_asin") ) 
            amazon_asin = get<std::string>(r["album.amazon_asin"]);

        if( r.count("album_artist.album_artist") )
            album_artist = get<std::string>(r["album_artist.album_artist"]);

        if( r.count("album.album") )
            album = get<std::string>(r["album.album"]);

        m_Covers.cache(
                mb_album_id,
                amazon_asin,
                location,
                album_artist,
                album,
                true
        );

        (*position)++;

        if((*position) == (*v).size())
        {
            delete v;
            delete position;
            return false;
        }

        return true;
    }

    void
    Library::recacheCovers()
    {
        m_Covers.purge();
        reload ();

        RowV * v = new RowV;
        getSQL(*v, "SELECT DISTINCT album_j, location, hal_volume_udi, hal_device_udi, hal_vrp, album.mb_album_id, album.amazon_asin, album_artist.album_artist, album.album "
                  "FROM track JOIN album ON album_j = album.id JOIN album_artist ON album.album_artist_j = album_artist.id GROUP BY album_j");
        int * position = new int;
        *position = 0;

        signal_timeout().connect( sigc::bind( sigc::mem_fun( *this, &Library::recache_covers_handler ), v, position), 300);
    }

    void
    Library::reload ()
    {
        Signals.Reload.emit();
    }

    void
    Library::on_new_album(
        gint64 album_id
    )
    {
        Signals.NewAlbum.emit(album_id);
    }

    void
    Library::on_new_artist(
        gint64 artist_id
    )
    {
        Signals.NewArtist.emit(artist_id);
    }

    void
    Library::on_new_track(
        Track& track,
        gint64 album_id,
        gint64 artist_id
    )
    {
        Signals.NewTrack.emit(track, album_id, artist_id);
    }

	void
	Library::vacuum ()
	{
		typedef std::map<HAL::VolumeKey, std::string> VolMountPointMap;

		VolMountPointMap m;
		RowV rows;
		getSQL (rows, "SELECT * FROM track");

		for (RowV::iterator i = rows.begin(); i != rows.end(); ++i)
		{
		  Row & r = *i;

		  std::string uri;

#ifdef HAVE_HAL
          if (m_Flags & F_USING_HAL)
          {
              std::string volume_udi = get<std::string>(r["hal_volume_udi"]); 
              std::string device_udi = get<std::string>(r["hal_device_udi"]); 
              std::string hal_vrp	 = get<std::string>(r["hal_vrp"]);

              // FIXME: More intelliget scanning by prefetching grouped data from the DB

              HAL::VolumeKey key (volume_udi, device_udi);
              VolMountPointMap::const_iterator i = m.find(key);

              if(i == m.end())
              {
                  try{		
                      std::string mount_point = m_HAL.get_mount_point_for_volume (volume_udi, device_udi);
                      m[key] = mount_point;
                      uri = filename_to_uri(build_filename(mount_point, hal_vrp));
                  } catch (...) {
                      continue; // Not mounted, we're not checking
                  }
              }
              else
              {
                  uri = filename_to_uri(build_filename(i->second, hal_vrp));
              }
          } 
          else
#else
          {
		      uri = get<std::string>(r["location"]);
          }
#endif
		  if (!file_test (filename_from_uri (uri), FILE_TEST_EXISTS))
		  {
			  execSQL((boost::format ("DELETE FROM track WHERE id = %lld") % get<gint64>(r["id"])).str()); 
		  }
		}

		typedef std::set <gint64> IdSet;
		static boost::format delete_f ("DELETE FROM %s WHERE id = '%lld';");

		IdSet idset1;
		IdSet idset2;

		idset1.clear();
		rows.clear();
		getSQL(rows, "SELECT DISTINCT artist_j FROM track;");
		for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
		  idset1.insert (get<gint64>(i->find ("artist_j")->second));

		idset2.clear();
		rows.clear();
		getSQL(rows, "SELECT DISTINCT id FROM artist;");
		for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
		  idset2.insert (get<gint64>(i->find ("id")->second));

		// If the artist is unused in the track table, delete it
		for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
		{
		  if (idset1.find (*i) == idset1.end())
			execSQL((delete_f % "artist" % (*i)).str());
		}


        idset1.clear();
        rows.clear();
		getSQL(rows, "SELECT DISTINCT album_j FROM track;");
        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset1.insert (get<gint64>(i->find ("album_j")->second));

        idset2.clear();
        rows.clear();
        getSQL(rows, "SELECT DISTINCT id FROM album;");
        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset2.insert (get<gint64>(i->find ("id")->second));

        for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
        {
          if (idset1.find (*i) == idset1.end())
			execSQL((delete_f % "album" % (*i)).str());
        }


        idset1.clear();
        rows.clear();
		getSQL(rows, "SELECT DISTINCT album_artist_j FROM album;");
        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset1.insert (get<gint64>(i->find ("album_artist_j")->second));

        idset2.clear();
        rows.clear();
		getSQL(rows, "SELECT DISTINCT id FROM album_artist;");
        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset2.insert (get<gint64>(i->find ("id")->second));

        for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
        {
          if (idset1.find (*i) == idset1.end())
			execSQL((delete_f % "album_artist" % (*i)).str());
        }
        
        reload ();
	}

    RowV
    Library::getTrackTags (gint64 id)
    {
        RowV rows ;

        char const get_f[] = "SELECT tagid, amplitude FROM tags WHERE trackid = %lld;" ;

        getSQL(rows, mprintf(get_f, id)) ; 

        return rows ;
    }

    void
    Library::getMetadata (const std::string& uri, Track & track)
    {
        m_MetadataReaderTagLib.get(uri, track);

        std::string type;
        if(Audio::typefind(uri, type))
        {
            track[ATTRIBUTE_TYPE] = type; 
        }

#ifdef HAVE_HAL
        try{
          URI u (uri);
          if(u.get_protocol() == URI::PROTOCOL_FILE)
          {
              try{
                  if (m_Flags & F_USING_HAL)
                  {
                    HAL::Volume const& volume (m_HAL.get_volume_for_uri (uri));

                    track[ATTRIBUTE_HAL_VOLUME_UDI] =
                                  volume.volume_udi ;

                    track[ATTRIBUTE_HAL_DEVICE_UDI] =
                                  volume.device_udi ;

                    track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                                  filename_from_uri (uri).substr (volume.mount_point.length()) ;

                    std::string mount_point = m_HAL.get_mount_point_for_volume(volume.volume_udi, volume.device_udi);
                    std::string path = build_filename(mount_point, get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()));

                    track[ATTRIBUTE_LOCATION] = std::string(filename_to_uri(path));
                  }
              }
            catch (HAL::Exception & cxe)
              {
                g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
              }
            catch (Glib::ConvertError & cxe)
              {
                g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
              }
          }
        } catch (URI::ParseError)
        {
        }
#endif // HAVE_HAL
    }

    void
    Library::getSQL( RowV & rows, const std::string& sql)
    {
		m_SQL->get (rows, sql); 
    }

	void
	Library::execSQL(const std::string& sql)
	{
		m_SQL->exec_sql(sql);
	}

    void
    Library::albumAddNewRating(gint64 id, int rating, std::string const& comment)
    {
        RowV v;
        getSQL(v, (boost::format ("SELECT mb_album_id FROM album WHERE id = '%lld'") % id).str());
        if( v.empty ())
        {
            throw std::runtime_error("Invalid ID specified");
        }

        std::string mbid = get<std::string>(v[0]["mb_album_id"]);

        execSQL(
                mprintf(" INSERT INTO album_rating_history (mbid, rating, comment, time) VALUES ('%q', '%d', '%q', '%lld') ",
                    mbid.c_str(),
                    rating,
                    comment.c_str(),
                    gint64(time(NULL))
        ));

		Signals.AlbumUpdated.emit(id);
    }
    
    void
    Library::albumGetAllRatings(gint64 id, RowV & v)
    {
        RowV v2;
        getSQL(v2, (boost::format ("SELECT mb_album_id FROM album WHERE id = '%lld'") % id).str());
        if( v2.empty ())
        {
            throw std::runtime_error("Invalid ID specified");
        }

        std::string mbid = get<std::string>(v2[0]["mb_album_id"]);
        getSQL(v, mprintf("SELECT * FROM album_rating_history WHERE mbid = '%q'",mbid.c_str()));
    }

    int
    Library::albumGetMeanRatingValue(gint64 id)
    {
        RowV v;

        getSQL(v, (boost::format ("SELECT mb_album_id FROM album WHERE id = '%lld'") % id).str());
        if( v.empty ())
        {
            throw std::runtime_error("Invalid ID specified");
        }

        std::string mbid = get<std::string>(v[0]["mb_album_id"]);

        v.clear();
        getSQL(v, mprintf("SELECT rating FROM album_rating_history WHERE mbid = '%q'",mbid.c_str()));

        if( !v.empty() )
        {
                double rating = 0;

                for(RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                    rating += double(get<gint64>((*i)["rating"]));
                }

                rating /= double(v.size());

                return rating;
        }

        return 0;
    }

    void
    Library::albumDeleteRating(gint64 rating_id, gint64 album_id)
    {
		execSQL((boost::format ("DELETE FROM album_rating_history WHERE id = %lld") % rating_id).str());
		Signals.AlbumUpdated.emit(album_id);
    }

	void	
	Library::trackRated(gint64 id, int rating)
	{
		execSQL((boost::format ("UPDATE track SET rating = '%d' WHERE id = %lld") % rating % id).str());
		Signals.TrackUpdated.emit(id);
	}

	void	
	Library::trackPlayed(gint64 id, time_t time_)
	{
		execSQL((boost::format ("UPDATE track SET pdate = '%lld' WHERE id = %lld") % gint64(time_) % id).str());
		execSQL((boost::format ("UPDATE track SET pcount = ifnull(pcount,0) + 1 WHERE Id =%lld") % id).str());
		Signals.TrackUpdated.emit(id);
	}

    void
    Library::trackTagged(gint64 id, std::string const& tag)
    {
        gint64 tag_id = get_tag_id( tag );

        char const insert_f[] = "INSERT INTO tags (tagid, trackid) VALUES (%lld, %lld);";
        char const update_f[] = "UPDATE tags SET amplitude = amplitude + 1 WHERE trackid = %lld AND tagid = %lld;";
        try{
           execSQL(mprintf(insert_f, id, tag_id)); 
        } catch( SqlConstraintError & cxe )
        {
           execSQL(mprintf(update_f, id, tag_id));
        }
    }

    void
    Library::markovUpdate(gint64 a, gint64 b)
    {
		RowV rows;
		getSQL (rows, (boost::format("SELECT id FROM markov WHERE track_id_a = %lld AND track_id_b = %lld") % a % b).str());
        if( rows.empty ())
        {
            execSQL((boost::format("INSERT INTO markov (count, track_id_a, track_id_b) VALUES (1, %lld, %lld)") % a % b).str());
        }
        else
        {
            gint64 id = get <gint64> (rows[0].find ("id")->second);
            execSQL((boost::format("UPDATE markov SET count = count + 1 WHERE id = %lld") % id).str());
        }
    }

    gint64
    Library::markovGetRandomProbableTrack(gint64 a)
    {
        try{
                std::vector<double> ratios;
                RowV rows;
                getSQL (rows, (boost::format("SELECT * FROM markov WHERE track_id_a = %lld AND count > 0") % a).str());
                if( !rows.empty() )
                {
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                        {
                            gint64 count = get <gint64> ((*i).find ("count")->second);
                            ratios.push_back(double(count) / double(rows.size()));
                        }
                        int row = MPX::rand(ratios);
                        gint64 id = get <gint64> (rows[row].find ("id")->second);
                        return id;
                }
        } catch( ... ) {
        }

        return -1;
    }

    Track
    Library::sqlToTrack (SQL::Row & row)
    {
        Track track;

        if (row.count("album_artist"))
          track[ATTRIBUTE_ALBUM_ARTIST] = get<std::string>(row["album_artist"]);

        if (row.count("artist"))
          track[ATTRIBUTE_ARTIST] = get<std::string>(row["artist"]);

        if (row.count("album"))
          track[ATTRIBUTE_ALBUM] = get<std::string>(row["album"]);

        if (row.count("track"))
          track[ATTRIBUTE_TRACK] = gint64(get<gint64>(row["track"]));

        if (row.count("title"))
          track[ATTRIBUTE_TITLE] = get<std::string>(row["title"]);

        if (row.count("time"))
          track[ATTRIBUTE_TIME] = gint64(get<gint64>(row["time"]));

        if (row.count("mb_artist_id"))
          track[ATTRIBUTE_MB_ARTIST_ID] = get<std::string>(row["mb_artist_id"]);

        if (row.count("mb_album_id"))
          track[ATTRIBUTE_MB_ALBUM_ID] = get<std::string>(row["mb_album_id"]);

        if (row.count("mb_track_id"))
          track[ATTRIBUTE_MB_TRACK_ID] = get<std::string>(row["mb_track_id"]);

        if (row.count("mb_album_artist_id"))
          track[ATTRIBUTE_MB_ALBUM_ARTIST_ID] = get<std::string>(row["mb_album_artist_id"]);

        if (row.count("amazon_asin"))
          track[ATTRIBUTE_ASIN] = get<std::string>(row["amazon_asin"]);

        if (row.count("id"))
          track[ATTRIBUTE_MPX_TRACK_ID] = get<gint64>(row["id"]);

        if (row.count("album_j"))
          track[ATTRIBUTE_MPX_ALBUM_ID] = get<gint64>(row["album_j"]);

        if (row.count("hal_volume_udi"))
          track[ATTRIBUTE_HAL_VOLUME_UDI] = get<std::string>(row["hal_volume_udi"]);

        if (row.count("hal_device_udi"))
          track[ATTRIBUTE_HAL_DEVICE_UDI] = get<std::string>(row["hal_device_udi"]);

        if (row.count("hal_vrp"))
          track[ATTRIBUTE_VOLUME_RELATIVE_PATH] = get<std::string>(row["hal_vrp"]);

        if (row.count("type"))
          track[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);

#ifdef HAVE_HAL
        if (m_Flags & F_USING_HAL)
        {
          try{
              std::string volume_udi = get<std::string>(track[ATTRIBUTE_HAL_VOLUME_UDI].get());
              std::string device_udi = get<std::string>(track[ATTRIBUTE_HAL_DEVICE_UDI].get());
              std::string vrp = get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get());
              std::string mount_point = m_HAL.get_mount_point_for_volume(volume_udi, device_udi);
              std::string path = build_filename(mount_point, vrp);
              track[ATTRIBUTE_LOCATION] = std::string(filename_to_uri(path));
          } catch( boost::bad_get )
          {
          } catch( HAL::Exception )
          {
          }
        }
        else
#endif
        if (row.count("location"))
          track[ATTRIBUTE_LOCATION] = get<std::string>(row["location"]);

        if (row.count("bitrate"))
          track[ATTRIBUTE_BITRATE] = get<gint64>(row["bitrate"]);

        if (row.count("samplerate"))
          track[ATTRIBUTE_SAMPLERATE] = get<gint64>(row["samplerate"]);

        return track;
    }

    gint64
    Library::get_tag_id (std::string const& tag)
    {
        gint64 tag_id = 0;
        RowV rows;
        std::string sql;

        get_tag_id:

        char const* select_tag_f ("SELECT id FROM tag WHERE tag = '%q';"); 
        sql = mprintf(select_tag_f, tag.c_str());
        m_SQL->get (rows, sql); 

        if(!rows.empty())
        {
            tag_id = get <gint64> (rows[0].find ("id")->second);
        }
        else
        {
          char const* set_tag_f ("INSERT INTO tag (tag) VALUES (%q);");

          sql = mprintf(set_tag_f, tag.c_str());
          m_SQL->exec_sql (sql);
          goto get_tag_id; // threadsafe, as opposed to getting last insert rowid
        }
        g_assert(tag_id != 0);
        return tag_id;
    }

	void
	Library::set_mean_genre_for_album (gint64 id)
	{
		static boost::format select_f ("SELECT DISTINCT genre FROM track WHERE album_j = %lld AND genre IS NOT NULL;"); 
		RowV rows;
		m_SQL->get (rows, (select_f % id).str()); 

		if(rows.empty())
        {
			return;
        }

		execSQL((boost::format ("UPDATE album SET album_genre = '%s' WHERE id = '%lld'") % get<std::string>(rows[0]["genre"]) % id).str());
	}

    void
    Library::initScan (const Util::FileList & list)
    {
        execSQL("UPDATE album SET album_new = 0;");
        m_ScannerThread->scan(list);
    }

} // namespace MPX
