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
#include "library-mlibman.hh"
#include "library-scanner-thread-mlibman.hh"

#include <boost/format.hpp>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <giomm.h>

#ifdef HAVE_TR1
#include <tr1/unordered_set>
#else
#include <set>
#endif

#include "mpx/mpx-sql.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/util-string.hh"

#include "mlibmanager.hh"

#undef PACKAGE
#define PACKAGE "youki"

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

                {   "label",
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

                {   "audio_quality",
                        VALUE_TYPE_INT      },

                {   "device_id",
                        VALUE_TYPE_INT      },

                {   "loved",
                        VALUE_TYPE_INT      },

                {   "hated",
                        VALUE_TYPE_INT      },
        };
}

namespace MPX
{
    /* no rv */
                Library_MLibMan::Library_MLibMan(
                )
                    : Service::Base("mpx-service-library")
                    , sigx::glib_auto_dispatchable()
                    , m_Flags(0)
                {
#ifdef HAVE_HAL
                        m_HAL = services->get<IHAL>("mpx-service-hal").get() ;
#endif // HAVE_HAL

                        create_and_init() ;

                        m_ScannerThread = boost::shared_ptr<LibraryScannerThread_MLibMan>(
                                new LibraryScannerThread_MLibMan(
                                      this
                                    , m_Flags
                        ));

                        m_ScannerThread->run();

                        m_ScannerThread->connect().signal_new_album().connect(
                            sigc::mem_fun(
                                 *this
                               , &Library_MLibMan::on_new_album
                        ));

                        m_ScannerThread->connect().signal_new_artist().connect(
                            sigc::mem_fun(
                                 *this
                               , &Library_MLibMan::on_new_artist
                        ));

                        m_ScannerThread->connect().signal_new_track().connect(
                            sigc::mem_fun(
                                 *this
                               , &Library_MLibMan::on_new_track
                        ));

                        m_ScannerThread->connect().signal_entity_deleted().connect(
                            sigc::mem_fun(
                                 *this
                               , &Library_MLibMan::on_entity_deleted
                        ));

                        m_ScannerThread->connect().signal_entity_updated().connect(
                            sigc::mem_fun(
                                 *this
                               , &Library_MLibMan::on_entity_updated
                        ));

                        m_ScannerThread->connect().signal_message().connect(
                            sigc::mem_fun(
                                 *this
                               , &Library_MLibMan::on_message
                        ));

                        /*
                        mcs->subscribe(
                              "Preferences-FileFormatPriorities"
                            , "Format6"
                            , sigc::mem_fun(
                                    *this,
                                   &Library_MLibMan::on_priority_settings_changed
                        ));

                        mcs->subscribe(
                              "Preferences-FileFormatPriorities"
                            , "prioritize-by-filetype"
                            , sigc::mem_fun(
                                    *this,
                                   &Library_MLibMan::on_priority_settings_changed
                        ));

                        mcs->subscribe(
                              "Preferences-FileFormatPriorities"
                            , "prioritize-by-bitrate"
                            , sigc::mem_fun(
                                    *this,
                                   &Library_MLibMan::on_priority_settings_changed
                        ));
                        */

                        library_scanner_thread_set_priorities();
                }

        void
                Library_MLibMan::create_and_init(
                )
                {
                        const int MLIB_VERSION_CUR = 2;
                        const int MLIB_VERSION_REV = 0;
                        const int MLIB_VERSION_AGE = 0;

                        try{
                                m_SQL = new SQL::SQLDB(
                                          (boost::format ("mpx-musicdata-%d-%d-%d")
                                              % MLIB_VERSION_CUR 
                                              % MLIB_VERSION_REV
                                              % MLIB_VERSION_AGE
                                          ).str()
                                        , build_filename(g_get_user_data_dir(),PACKAGE)
                                        , SQLDB_OPEN
                                );
                        }
                        catch( DbInitError& cxe )
                        {
                                g_message("%s: Error Opening the DB", G_STRLOC);
                                throw;
                        }

                        ///////////////////////////////////////////////////////////////
                        /// META TABLE 
                        ///////////////////////////////////////////////////////////////

                        if( !m_SQL->table_exists("meta") )
                        {
#ifdef HAVE_HAL
                                m_Flags |= ( mcs->key_get<bool>("library","use-hal") ? F_USING_HAL : 0) ; 
#endif // HAVE_HAL
                                m_SQL->exec_sql( "CREATE TABLE meta (version STRING, flags INTEGER DEFAULT 0, last_scan_date INTEGER DEFAULT 0)" ) ;
                                m_SQL->exec_sql( ( boost::format( "INSERT INTO meta (flags) VALUES (%lld)" ) % m_Flags ).str() ) ;
                        }
                        else
                        {
                                RowV v ;
                                m_SQL->get( v, "SELECT flags FROM meta WHERE rowid = 1" ) ; 
            
                                if( !v.empty() )
                                {
                                    m_Flags |= get<gint64>( v[0]["flags"]); 
                                }
                        }

                        ///////////////////////////////////////////////////////////////
                        /// ARTIST TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS artist (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' INTEGER, UNIQUE ('%s', '%s', '%s'))")
                                % attrs[ATTRIBUTE_ARTIST].id
                                % attrs[ATTRIBUTE_MB_ARTIST_ID].id
                                % attrs[ATTRIBUTE_ARTIST_SORTNAME].id
                                % attrs[ATTRIBUTE_COUNT].id
                                % attrs[ATTRIBUTE_ARTIST].id
                                % attrs[ATTRIBUTE_MB_ARTIST_ID].id
                                % attrs[ATTRIBUTE_ARTIST_SORTNAME].id
                        ).str());

                        ///////////////////////////////////////////////////////////////
                        /// ALBUM ARTIST TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS album_artist (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' INTEGER, UNIQUE ('%s', '%s', '%s', '%s'))")
                                % attrs[ATTRIBUTE_ALBUM_ARTIST].id
                                % attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id
                                % attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id
                                % "life_span_begin"
                                % "life_span_end"
                                % attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id
                                % attrs[ATTRIBUTE_ALBUM_ARTIST].id
                                % attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id
                                % attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id
                                % attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id
                        ).str());

                        ///////////////////////////////////////////////////////////////
                        /// ALBUM TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS album (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT DEFAULT NULL, '%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT NULL, '%s' FLOAT DEFAULT 0, UNIQUE ('%s', '%s', '%s', '%s', '%s', '%s', '%s'))")
                                % attrs[ATTRIBUTE_ALBUM].id
                                % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                % attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id
                                % attrs[ATTRIBUTE_MB_RELEASE_TYPE].id
                                % attrs[ATTRIBUTE_ASIN].id
                                % "album_label" 
                                % "album_genre" 
                                % "album_rating"
                                % "album_artist_j"
                                % "album_insert_date"
                                % "album_new"
                                % "album_quality"
                                % "album_playscore"
                                % attrs[ATTRIBUTE_ALBUM].id
                                % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                % attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id
                                % attrs[ATTRIBUTE_MB_RELEASE_TYPE].id
                                % attrs[ATTRIBUTE_ASIN].id
                                % "album_artist_j"
                        ).str());

                        ///////////////////////////////////////////////////////////////
                        /// TAG TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS tag (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, UNIQUE ('%s'))")
                                % "tag"
                                % "tag"
                        ).str()); 

                        ///////////////////////////////////////////////////////////////
                        /// TAGS TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS tags (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' INTEGER NOT NULL, '%s' INTEGER NOT NULL, '%s' INTEGER DEFAULT 1, UNIQUE ('%s', '%s'))")
                                % "tagid"
                                % "trackid"
                                % "amplitude" 
                                % "tagid"
                                % "trackid"
                        ).str()); 

                        ///////////////////////////////////////////////////////////////
                        /// ALBUM TAGS TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS tags_album (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' INTEGER NOT NULL, '%s' INTEGER NOT NULL, '%s' INTEGER DEFAULT 1, UNIQUE ('%s', '%s'))")
                                % "tagid"
                                % "albumid"
                                % "amplitude" 
                                % "tagid"
                                % "albumid"
                        ).str()); 

                        ///////////////////////////////////////////////////////////////
                        /// ALBUM RATING HISTORY TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS album_rating_history (id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT NOT NULL, '%s' INTEGER NOT NULL, '%s' TEXT DEFAULT NULL, '%s' INTEGER NOT NULL)")
                                % "mbid"
                                % "rating"
                                % "comment"
                                % "time"
                        ).str()); 

                        ///////////////////////////////////////////////////////////////
                        /// TRACK TABLE 
                        ///////////////////////////////////////////////////////////////

                        StrV columns;

                        for( unsigned n = 0; n < G_N_ELEMENTS(attrs); ++n )
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

                        m_SQL->exec_sql((
                            boost::format("CREATE TABLE IF NOT EXISTS track (id INTEGER PRIMARY KEY AUTOINCREMENT, %s, %s, %s, UNIQUE (%s, %s, %s, %s))")
                                % column_names
                                % "artist_j INTEGER NOT NULL"   // track artist information 
                                % "album_j INTEGER NOT NULL"    // album + album artist
                                % "album_j" 
                                % "artist_j" 
                                % "track" 
                                % "title"
                        ).str());

                        ///////////////////////////////////////////////////////////////
                        /// TRACK VIEW VIEW 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql(
                                "  CREATE VIEW IF NOT EXISTS track_view AS " 
                                "  SELECT"
                                "  track.* "
                                ", album.id AS mpx_album_id"
                                ", album.album_rating AS album_rating"
                                ", artist.id AS mpx_artist_id "
                                ", album_artist.id AS mpx_album_artist_id "
                                "  FROM track "
                                "  JOIN album ON album.id = track.album_j JOIN artist "
                                "  ON artist.id = track.artist_j JOIN album_artist ON album.album_artist_j = album_artist.id"
                        );

                        ///////////////////////////////////////////////////////////////
                        /// DEVICES TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql ("CREATE TABLE IF NOT EXISTS devices (id INTEGER PRIMARY KEY AUTOINCREMENT, device_udi TEXT, volume_udi TEXT)");

                        ///////////////////////////////////////////////////////////////
                        /// MARKOV TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql ("CREATE TABLE IF NOT EXISTS markov (id INTEGER PRIMARY KEY AUTOINCREMENT, count INTEGER DEFAULT 0, track_id_a INTEGER DEFAULT NULL, track_id_b INTEGER DEFAULT NULL)");

                        ///////////////////////////////////////////////////////////////
                        /// COLLECTIONS TABLE 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql ("CREATE TABLE IF NOT EXISTS collection (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, blurb TEXT NOT NULL, cover_url TEXT)");

                        ///////////////////////////////////////////////////////////////
                        /// ARTIST ALIASES 
                        ///////////////////////////////////////////////////////////////

                        m_SQL->exec_sql ("CREATE TABLE IF NOT EXISTS artist_aliases (id INTEGER PRIMARY KEY AUTOINCREMENT, mbid_alias TEXT NOT NULL, mbid_for TEXT NOT NULL, UNIQUE( mbid_alias, mbid_for))");
                }


        Library_MLibMan::~Library_MLibMan ()
                {
                    m_ScannerThread->finish ();
                }

        void
                Library_MLibMan::library_scanner_thread_set_priorities(
                )
                {
                    std::vector<std::string> strv;

                    for( int n = 0; n < 7; ++n )
                    {
                        std::string mime = mcs->key_get<std::string>("Preferences-FileFormatPriorities", (boost::format("Format%d")%n).str());
                        strv.push_back( mime ); 
                    }

                    m_ScannerThread->set_priority_data(
                          strv
                        , mcs->key_get<bool>("Preferences-FileFormatPriorities", "prioritize-by-filetype")
                        , mcs->key_get<bool>("Preferences-FileFormatPriorities", "prioritize-by-bitrate")
                    );
                }

        void
                Library_MLibMan::on_priority_settings_changed(
                    MCS_CB_DEFAULT_SIGNATURE
                )
                {
                    library_scanner_thread_set_priorities();
                }

#ifdef HAVE_HAL
        void
                Library_MLibMan::switch_mode(
                    bool use_hal 
                )
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

                        if( !use_hal )
                        {
                            // Update locations from HAL data

                            SQL::RowV v;
                            getSQL(
                                v,
                                "SELECT id, device_id, hal_vrp, insert_path FROM track"
                            );

                            for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                            {
                                Track_sp t = sqlToTrack( *i, false );

                                mm->push_message((boost::format(_("Updating Track Locations: %lld / %lld")) % gint64(std::distance(v.begin(), i)) % v.size()).str());

                                g_assert( t->has(ATTRIBUTE_LOCATION) );
                                g_assert( (*i).count("id") );

                                const gint64&      device_id   = get<gint64>((*i)["device_id"]) ;
                                const std::string& insert_path = Util::normalize_path(get<std::string>((*i)["insert_path"]));
                                const std::string& mount_point = m_HAL->get_mount_point_for_id( device_id ) ;

                                std::string insert_path_new = filename_to_uri( build_filename( Util::normalize_path(mount_point), insert_path ));
    
                                execSQL(
                                    mprintf(
                                          "UPDATE track SET location = '%q', insert_path = '%q', device_id = NULL, hal_vrp = NULL WHERE id ='%lld'"
                                        , get<std::string>((*(t.get()))[ATTRIBUTE_LOCATION].get()).c_str()
                                        , insert_path_new.c_str()
                                        , get<gint64>((*i)["id"])
                                ));

                            }

                            m_Flags&= ~F_USING_HAL;

                            mm->push_message(_("Updating Track Locations: Done")); 
                        }
                        else
                        {
                            // Update HAL data from locations 


                            m_Flags |= F_USING_HAL;

                            execSQL(
                                "DELETE FROM track WHERE location NOT LIKE 'file:%'"
                            ); // FIXME: Add warning to the GUI

                            SQL::RowV v;
                            getSQL(
                                v,
                                "SELECT id, location, insert_path FROM track"
                            );

                            for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                            {
                                Track t;

                                trackSetLocation( t, get<std::string>((*i)["location"]));

                                mm->push_message((boost::format(_("Updating Track HAL Data: %lld / %lld")) % gint64(std::distance(v.begin(), i)) % v.size()).str());

                                try{
                                        const std::string& insert_path = Util::normalize_path(get<std::string>((*i)["insert_path"]));
                                        const Volume& volume( m_HAL->get_volume_for_uri( insert_path )); 
                                        const std::string& insert_path_new = Util::normalize_path(filename_from_uri( Util::normalize_path(insert_path) ).substr( volume.mount_point.length() ));
                                               

                                        execSQL(
                                            mprintf(
                                                  "UPDATE track SET insert_path = '%q', device_id = '%lld', hal_vrp = '%q', location = NULL WHERE id ='%lld'"
                                                , insert_path_new.c_str()
                                                , m_HAL->get_id_for_volume( volume.volume_udi, volume.device_udi ) 
                                                , get<std::string>(t[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str()
                                                , get<gint64>((*i)["id"])
                                        ));
                                } catch( IHAL::NoVolumeForUriError& cxe )
                                {
                                        g_message("%s: Critical: Non-conversible!", G_STRFUNC);
                                }
                            }

                            mm->push_message(_("Updating Track HAL Data: Done")); 
                        }


                        m_SQL->exec_sql((boost::format ("UPDATE meta SET flags = '%lld' WHERE rowid = 1") % m_Flags).str());
                }
#endif // HAVE_HAL

        void
                Library_MLibMan::on_new_album(
                      gint64                album_id
                    , const std::string&    s1
                    , const std::string&    s2
                    , const std::string&    s3
                    , const std::string&    s4
                    , const std::string&    s5
                )
                {
                        Signals.NewAlbum.emit( album_id, s1, s2, s3, s4, s5 );
                }

        void
                Library_MLibMan::on_new_artist(
                      gint64 id
                )
                {
                        Signals.NewArtist.emit( id ) ;
                }

        void
                Library_MLibMan::on_new_track(
                      gint64 id
                )
                {
                        Signals.NewTrack.emit( id ) ; 
                }
    
        void
                Library_MLibMan::on_entity_deleted(
                      gint64      id
                    , EntityType  type
                )
                {
                    Signals.EntityDeleted.emit( id, int(type) ) ;
                }

        void
                Library_MLibMan::on_entity_updated(
                      gint64      id
                    , EntityType  type
                )
                {
                    Signals.EntityUpdated.emit( id, int(type) ) ;
                }

        void
                Library_MLibMan::on_message(
                    const std::string& msg
                )
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");
                        mm->push_message( msg );
                }

        void
                Library_MLibMan::removeDupes()
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

                        execSQL("BEGIN IMMEDIATE");

                        RowV rows_tracks;
                        getSQL(
                            rows_tracks,
                            "SELECT id FROM (track NATURAL JOIN (SELECT artist, album, title, track FROM track GROUP BY artist, album, title, track HAVING count(title) > 1)) EXCEPT SELECT id FROM track GROUP BY artist, album, title, track HAVING count(title) > 1" 
                        );
    
#ifdef HAVE_HAL
                        typedef std::map<VolumeKey, std::string> VolMountPointMap;
                        VolMountPointMap m;
#endif

                        for( RowV::iterator i = rows_tracks.begin(); i != rows_tracks.end(); ++i )
                        {
                                Row& rt = *i;

                                RowV rows;
                                getSQL(
                                    rows,
#ifdef HAVE_HAL
                                    mprintf("SELECT device_id, hal_vrp FROM track WHERE id = '%lld'", get<gint64>(rt["id"])) 
#else
                                    mprintf("SELECT location FROM track WHERE id = '%lld'", get<gint64>(rt["id"])) 
#endif
                                );

                                if( rows.empty() )
                                {
                                    continue; //FIXME: Maybe it was vacuumed during this operation, so let's just continue. We really need proper high-level locking for the database
                                }

                                std::string uri = get<std::string>( (*sqlToTrack( rows[0], false ))[ATTRIBUTE_LOCATION].get() ) ;

                                if( !uri.empty() )
                                {
                                        mm->push_message((boost::format(_("Removing duplicates: %s")) % filename_from_uri(uri)).str());
                                        try{
/*
                                                Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
                                                if( file->remove() )
*/
                                                {
                                                        execSQL((boost::format ("DELETE FROM track WHERE id = %lld") % get<gint64>(rt["id"])).str()); 
                                                        Signals.EntityDeleted.emit( get<gint64>(rt["id"]), ENTITY_TRACK );
                                                }
                                        } catch(Glib::Error) {
                                                g_message(G_STRLOC ": Error while trying to remove track at URI '%s'", uri.c_str());
                                        }
                                } 
                                else
                                    g_message("URI empty!");
                        }

                        execSQL("COMMIT");

                        mm->push_message( _("Removing duplicates: Done") );
                }

#ifdef HAVE_HAL
        void
                Library_MLibMan::deletePath(
                    const std::string& hal_device_udi,
                    const std::string& hal_volume_udi,
                    const std::string& insert_path 
                )
                {
                    g_message(
                          "Deleting: path['%s'], device_udi['%s'], volume_udi['%s']"
                        , insert_path.c_str()
                        , hal_device_udi.c_str()
                        , hal_volume_udi.c_str()
                    ) ;

                    boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

                    SQL::RowV rows;

                    gint64 device_id = m_HAL->get_id_for_volume( hal_volume_udi, hal_device_udi ) ;

                    const std::string& sql = (
                        boost::format("SELECT id FROM track WHERE device_id = '%lld' AND insert_path = '%s'")
                        % device_id
                        % Util::normalize_path( insert_path )
                    ).str();
    
                    getSQL(
                          rows
                        , sql
                    );

                    for( RowV::iterator i = rows.begin(); i != rows.end(); ++i )
                    {
                        Row& r = *i;
                        mm->push_message((boost::format(_("Removing Track: %lld of %lld")) % gint64(std::distance(rows.begin(), i)) % gint64(rows.size())).str());
                        execSQL( mprintf("DELETE FROM track WHERE id = '%lld'", get<gint64>(r["id"]))); 
                        Signals.EntityDeleted.emit( get<gint64>(r["id"]), ENTITY_TRACK );
                    }

                    remove_dangling();
                    mm->push_message("Done.") ;
                }
#endif // HAVE_HAL

        void
                Library_MLibMan::vacuum()
                {
                        m_ScannerThread->vacuum();
                }

#ifdef HAVE_HAL    
        void
		        Library_MLibMan::vacuumVolumeList(
                      const VolumeKey_v& v
                )
                {
                        m_ScannerThread->vacuum_volume_list( v ); 
                }
#endif // HAVE_HAL

        void
                Library_MLibMan::getMetadata (const std::string& uri, Track& track)
                {
                        services->get<MetadataReaderTagLib>("mpx-service-taglib")->get(uri, track);

                        track[ATTRIBUTE_LOCATION] = uri; 
  
#ifdef HAVE_HAL
                        if( m_Flags& F_USING_HAL )
                        {
                                try{
                                        URI u (uri);
                                        if( u.get_protocol() == URI::PROTOCOL_FILE )
                                        {
                                                try{
                                                        const Volume& volume = m_HAL->get_volume_for_uri (uri) ;

                                                        track[ATTRIBUTE_MPX_DEVICE_ID] =
                                                                m_HAL->get_id_for_volume( volume.volume_udi, volume.device_udi ) ; 

                                                        track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                                                                filename_from_uri (uri).substr (volume.mount_point.length()) ;
                                                }
                                                catch( Exception& cxe )
                                                {
                                                        g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
                                                        throw FileQualificationError((boost::format("%s: %s") % uri % cxe.what()).str());
                                                }
                                                catch( Glib::ConvertError& cxe )
                                                {
                                                        g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
                                                        throw FileQualificationError((boost::format("%s: %s") % uri % cxe.what()).str());
                                                }
                                        }
                                } catch( URI::ParseError )
                                {
                                        throw FileQualificationError((boost::format("URI Parse Error: %s") % uri).str());
                                }
                        }
#endif // HAVE_HAL
                }

        void
                Library_MLibMan::trackSetLocation(
                    Track&              track,
                    const std::string&  uri
                )
                {
                        track[ATTRIBUTE_LOCATION] = uri;
#ifdef HAVE_HAL
                        if( m_Flags& F_USING_HAL )
                        {
                                try{
                                        URI u (uri);
                                        if( u.get_protocol() == URI::PROTOCOL_FILE )
                                        {
                                                try{
                                                        {
                                                                const Volume& volume = m_HAL->get_volume_for_uri( uri ) ;

                                                                track[ATTRIBUTE_MPX_DEVICE_ID] =
                                                                        m_HAL->get_id_for_volume( volume.volume_udi, volume.device_udi ) ; 

                                                                track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                                                                        filename_from_uri( uri ).substr( volume.mount_point.length() ) ;
                                                        }
                                                }
                                                catch( Exception& cxe )
                                                {
                                                        g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
                                                        throw FileQualificationError((boost::format("%s: %s") % uri % cxe.what()).str());
                                                }
                                                catch( Glib::ConvertError& cxe )
                                                {
                                                        g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
                                                        throw FileQualificationError((boost::format("%s: %s") % uri % cxe.what()).str());
                                                }
                                        }
                                        else
                                        {
                                            throw FileQualificationError((boost::format("Unable to handle non-file:/// URI using HAL: %s") % uri).str());
                                        }

                                } catch( URI::ParseError )
                                {
                                    throw FileQualificationError((boost::format("URI Parse Error: %s") % uri).str());
                                }
                        }
#endif // HAVE_HAL
                }

        std::string
                Library_MLibMan::trackGetLocation( const Track& track )
                {
#ifdef HAVE_HAL
                        if( m_Flags& F_USING_HAL )
                        {
                                try{
                                        const std::string& path        = get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()) ;
                                        const gint64&      id          = get<gint64>(track[ATTRIBUTE_MPX_DEVICE_ID].get()) ;
                                        const std::string& mount_point = m_HAL->get_mount_point_for_id( id ) ;

                                        return filename_to_uri( build_filename( Util::normalize_path(mount_point), path ) );

                                } catch( IHAL::NoMountPathForVolumeError& cxe )
                                {
                                        g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
                                        throw FileQualificationError((boost::format("No available mountpoint for Track %lld: %s") % get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) % cxe.what() ).str());
                                }
                        }
                        else
#endif // HAVE_HAL
                        {
                                return get<std::string>(track[ATTRIBUTE_LOCATION].get());
                        }
                }

        void
                Library_MLibMan::getSQL( RowV& rows, const std::string& sql)
                {
                        m_SQL->get (rows, sql); 
                }

        void
                Library_MLibMan::execSQL(const std::string& sql)
                {
                        m_SQL->exec_sql(sql);
                }

        Track_sp
                Library_MLibMan::sqlToTrack(
                      SQL::Row& row
                    , bool all_metadata
                    , bool no_location
                )
                {
                        Track_sp track (new Track);

                        if( !no_location )
                        {
#ifdef HAVE_HAL
                                if( m_Flags& F_USING_HAL )
                                {
                                    if( row.count("hal_vrp") )
                                            (*track.get())[ATTRIBUTE_VOLUME_RELATIVE_PATH] = get<std::string>(row["hal_vrp"]);

                                    if( row.count("device_id") )
                                            (*track.get())[ATTRIBUTE_MPX_DEVICE_ID] = get<gint64>(row["device_id"]);

                                    (*track.get())[ATTRIBUTE_LOCATION] = trackGetLocation( (*track.get()) ); 

                                    g_assert( (*track.get()).has(ATTRIBUTE_LOCATION) );
                                }
                                else
#endif
                                if( row.count("location") )
                                {
                                    (*track.get())[ATTRIBUTE_LOCATION] = get<std::string>(row["location"]);
                                }
                        }

                        if( row.count("id") )
                        {
                            (*track.get())[ATTRIBUTE_MPX_TRACK_ID] = get<gint64>(row["id"]);
                        }

                        if( all_metadata )
                        {
                                if( row.count("album_artist") )
                                        (*track.get())[ATTRIBUTE_ALBUM_ARTIST] = get<std::string>(row["album_artist"]);

                                if( row.count("artist") )
                                        (*track.get())[ATTRIBUTE_ARTIST] = get<std::string>(row["artist"]);

                                if( row.count("album") )
                                        (*track.get())[ATTRIBUTE_ALBUM] = get<std::string>(row["album"]);

                                if( row.count("track") )
                                        (*track.get())[ATTRIBUTE_TRACK] = gint64(get<gint64>(row["track"]));

                                if( row.count("title") )
                                        (*track.get())[ATTRIBUTE_TITLE] = get<std::string>(row["title"]);

                                if( row.count("time") )
                                        (*track.get())[ATTRIBUTE_TIME] = gint64(get<gint64>(row["time"]));

                                if( row.count("mb_artist_id") )
                                        (*track.get())[ATTRIBUTE_MB_ARTIST_ID] = get<std::string>(row["mb_artist_id"]);

                                if( row.count("mb_album_id") )
                                        (*track.get())[ATTRIBUTE_MB_ALBUM_ID] = get<std::string>(row["mb_album_id"]);

                                if( row.count("mb_track_id") )
                                        (*track.get())[ATTRIBUTE_MB_TRACK_ID] = get<std::string>(row["mb_track_id"]);

                                if( row.count("mb_album_artist_id") )
                                        (*track.get())[ATTRIBUTE_MB_ALBUM_ARTIST_ID] = get<std::string>(row["mb_album_artist_id"]);

                                if( row.count("mb_release_country") )
                                        (*track.get())[ATTRIBUTE_MB_RELEASE_COUNTRY] = get<std::string>(row["mb_release_country"]);

                                if( row.count("mb_release_type") )
                                        (*track.get())[ATTRIBUTE_MB_RELEASE_TYPE] = get<std::string>(row["mb_release_type"]);

                                if( row.count("musicip_puid") )
                                        (*track.get())[ATTRIBUTE_MUSICIP_PUID] = get<std::string>(row["musicip_puid"]);

                                if( row.count("date") )
                                        (*track.get())[ATTRIBUTE_DATE] = get<gint64>(row["date"]);

                                if( row.count("amazon_asin") )
                                        (*track.get())[ATTRIBUTE_ASIN] = get<std::string>(row["amazon_asin"]);

                                if( row.count("id") )
                                        (*track.get())[ATTRIBUTE_MPX_TRACK_ID] = get<gint64>(row["id"]);

                                if( row.count("album_j") )
                                        (*track.get())[ATTRIBUTE_MPX_ALBUM_ID] = get<gint64>(row["album_j"]);

                                if( row.count("artist_j") )
                                        (*track.get())[ATTRIBUTE_MPX_ARTIST_ID] = get<gint64>(row["artist_j"]);

                                if( row.count("mpx_album_artist_id") )
                                        (*track.get())[ATTRIBUTE_MPX_ALBUM_ARTIST_ID] = get<gint64>(row["mpx_album_artist_id"]);

                                if( row.count("type") )
                                        (*track.get())[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);

                                if( row.count("bitrate") )
                                        (*track.get())[ATTRIBUTE_BITRATE] = get<gint64>(row["bitrate"]);

                                if( row.count("samplerate") )
                                        (*track.get())[ATTRIBUTE_SAMPLERATE] = get<gint64>(row["samplerate"]);

                                if( row.count("type") )
                                        (*track.get())[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);

                                if( row.count("audio_quality") )
                                        (*track.get())[ATTRIBUTE_QUALITY] = get<gint64>(row["audio_quality"]);
                        }

                       // g_assert( (*track.get()).has(ATTRIBUTE_LOCATION) );

                        return track;
                }

        void
                Library_MLibMan::initScanAll ()
                { 
                        m_ScannerThread->scan_all();
                }

        void
                Library_MLibMan::initScan (const Util::FileList& list)
                {
                        m_ScannerThread->scan(list);
                }

        void
                Library_MLibMan::initAdd (const Util::FileList& list)
                {
                        m_ScannerThread->add(list);
                }

        void
                Library_MLibMan::remove_dangling () 
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

#ifdef HAVE_TR1
                        typedef std::tr1::unordered_set<gint64> IdSet;
#else
                        typedef std::set<gint64> IdSet;
#endif
                        static boost::format delete_f ("DELETE FROM %s WHERE id = '%lld'");
                        IdSet idset1;
                        IdSet idset2;
                        RowV rows;

                        /// CLEAR DANGLING ARTISTS
                        mm->push_message(_("Finding Lost Artists..."));

                        idset1.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT artist_j FROM track");
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                idset1.insert (get<gint64>(i->find ("artist_j")->second));

                        idset2.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT id FROM artist");
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                idset2.insert (get<gint64>(i->find ("id")->second));

                        for( IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i )
                        {
                                if( idset1.find (*i) == idset1.end() )
                                {
                                        m_SQL->exec_sql((delete_f % "artist" % (*i)).str());
                                        on_entity_deleted( *i , ENTITY_ARTIST );
                                }
                        }


                        /// CLEAR DANGLING ALBUMS
                        mm->push_message(_("Finding Lost Albums..."));

                        idset1.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT album_j FROM track");
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                idset1.insert (get<gint64>(i->find ("album_j")->second));

                        idset2.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT id FROM album");
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                idset2.insert (get<gint64>(i->find ("id")->second));

                        for( IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i )
                        {
                                if( idset1.find (*i) == idset1.end() )
                                {
                                        m_SQL->exec_sql((delete_f % "album" % (*i)).str());
                                        on_entity_deleted( *i , ENTITY_ALBUM );
                                }
                        }

                        /// CLEAR DANGLING ALBUM ARTISTS
                        mm->push_message(_("Finding Lost Album Artists..."));

                        idset1.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT album_artist_j FROM album");
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                idset1.insert (get<gint64>(i->find ("album_artist_j")->second));

                        idset2.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT id FROM album_artist");
                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                idset2.insert (get<gint64>(i->find ("id")->second));

                        for( IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i )
                        {
                                if( idset1.find (*i) == idset1.end() )
                                        m_SQL->exec_sql((delete_f % "album_artist" % (*i)).str());
                                        on_entity_deleted( *i , ENTITY_ALBUM_ARTIST );
                        }
                }
} // namespace MPX
