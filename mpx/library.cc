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
#include <giomm.h>

#include <tr1/unordered_set>

#include "mpx/mpx-audio.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL
#include "mpx/mpx-library.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/mpx-public-mpx.hh"

#include "mpx/algorithm/random.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/util-string.hh"

#include "mlibmanager.hh"

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

        Library::Library(
        )
        : Service::Base("mpx-service-library")
        , sigx::glib_auto_dispatchable()
        , m_Flags(0)
        {
                const int MLIB_VERSION_CUR = 1;
                const int MLIB_VERSION_REV = 0;
                const int MLIB_VERSION_AGE = 0;

                try{
                        m_SQL = new SQL::SQLDB(
                                (boost::format ("mpxdb-%d-%d-%d")
                                    % MLIB_VERSION_CUR 
                                    % MLIB_VERSION_REV
                                    % MLIB_VERSION_AGE
                                ).str(),
                                build_filename(g_get_user_data_dir(),PACKAGE),
                                SQLDB_OPEN
                        );
                }
                catch( DbInitError& cxe )
                {
                        g_message("%s: Error Opening the DB", G_STRLOC);
                        throw;
                }

                if(!m_SQL->table_exists("meta"))
                {
#ifdef HAVE_HAL
                        m_Flags |= ( mcs->key_get<bool>("library","use-hal") ? F_USING_HAL : 0) ; 
#endif // HAVE_HAL
                        m_SQL->exec_sql("CREATE TABLE meta (version STRING, flags INTEGER DEFAULT 0, last_scan_date INTEGER DEFAULT 0)");
                        m_SQL->exec_sql((boost::format ("INSERT INTO meta (flags) VALUES (%lld)") % m_Flags).str());
                }
                else
                {
                        RowV rows;
                        m_SQL->get( rows, "SELECT flags FROM meta WHERE rowid = 1" ) ; 
                        m_Flags |= get<gint64>(rows[0].find("flags")->second); 
                }

                mcs->key_set<bool>("library","use-hal", bool(m_Flags & F_USING_HAL));

                m_ScannerThread = boost::shared_ptr<LibraryScannerThread>(
                        new LibraryScannerThread(
                            this,
                            m_Flags
                ));

                m_ScannerThread->run();

                m_ScannerThread->connect().signal_new_album().connect(
                    sigc::mem_fun(
                        *this,
                        &Library::on_new_album
                ));

                m_ScannerThread->connect().signal_new_artist().connect(
                    sigc::mem_fun(
                        *this,
                        &Library::on_new_artist
                ));

                m_ScannerThread->connect().signal_new_track().connect(
                    sigc::mem_fun(
                        *this,
                        &Library::on_new_track
                ));

                m_ScannerThread->connect().signal_entity_deleted().connect(
                    sigc::mem_fun(
                        *this,
                        &Library::on_entity_deleted
                ));

                m_ScannerThread->connect().signal_entity_updated().connect(
                    sigc::mem_fun(
                        *this,
                        &Library::on_entity_updated
                ));

                m_ScannerThread->connect().signal_track_updated().connect(
                    sigc::mem_fun(
                        *this,
                        &Library::on_track_updated
                ));

                m_ScannerThread->connect().signal_message().connect(
                    sigc::mem_fun(
                        *this, 
                        &Library::on_message
                ));

                // FIXME: Deprecated
                m_ScannerThread->connect().signal_reload().connect(
                                sigc::mem_fun( *this, &Library::reload ));

                m_ScannerThread->connect().signal_cache_cover().connect(
                                sigc::bind(
                                    sigc::mem_fun(
                                        *(services->get<Covers>("mpx-service-covers").get()),
                                        &Covers::cache
                                )
                                , true
                )); 

                ///////////////////////////////////////////////////////////////
                /// ARTIST TABLE 
                ///////////////////////////////////////////////////////////////

                static boost::format
                        artist_table_f ("CREATE TABLE IF NOT EXISTS artist "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, '%s' TEXT, "
                                        "UNIQUE ('%s', '%s', '%s'))");

                m_SQL->exec_sql ((artist_table_f  % attrs[ATTRIBUTE_ARTIST].id
                                        % attrs[ATTRIBUTE_MB_ARTIST_ID].id
                                        % attrs[ATTRIBUTE_ARTIST_SORTNAME].id
                                        % attrs[ATTRIBUTE_ARTIST].id
                                        % attrs[ATTRIBUTE_MB_ARTIST_ID].id
                                        % attrs[ATTRIBUTE_ARTIST_SORTNAME].id).str());

                static boost::format
                        album_artist_table_f ("CREATE TABLE IF NOT EXISTS album_artist "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, "
                                        "'%s' TEXT, '%s' INTEGER, UNIQUE ('%s', '%s', '%s', '%s'))");

                m_SQL->exec_sql ((album_artist_table_f  % attrs[ATTRIBUTE_ALBUM_ARTIST].id
                                        % attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id
                                        % attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id
                                        % attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id
                                        % attrs[ATTRIBUTE_ALBUM_ARTIST].id
                                        % attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id
                                        % attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id
                                        % attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id).str());

                ///////////////////////////////////////////////////////////////
                /// ALBUM TABLE 
                ///////////////////////////////////////////////////////////////

                static boost::format
                        album_table_f ("CREATE TABLE IF NOT EXISTS album "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, '%s' TEXT, "
                                        "'%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT, '%s' TEXT DEFAULT NULL, '%s' INTEGER DEFAULT 0, "
                                        "'%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, '%s' INTEGER DEFAULT 0, '%s' FLOAT DEFAULT 0, UNIQUE "
                                        "('%s', '%s', '%s', '%s', '%s', '%s', '%s'))");

                m_SQL->exec_sql ((album_table_f
                                        % attrs[ATTRIBUTE_ALBUM].id
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
                                        % "album_playscore"
                                        % attrs[ATTRIBUTE_ALBUM].id
                                        % attrs[ATTRIBUTE_MB_ALBUM_ID].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_DATE].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id
                                        % attrs[ATTRIBUTE_MB_RELEASE_TYPE].id
                                        % attrs[ATTRIBUTE_ASIN].id
                                        % "album_artist_j").str());


                ///////////////////////////////////////////////////////////////
                /// TAG TABLE 
                ///////////////////////////////////////////////////////////////

                static boost::format
                        tag_table_f ("CREATE TABLE IF NOT EXISTS tag "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT, " 
                                        "UNIQUE ('%s'))");

                m_SQL->exec_sql ((tag_table_f  % "tag" % "tag").str()); 


                ///////////////////////////////////////////////////////////////
                /// TAGS TABLE 
                ///////////////////////////////////////////////////////////////

                static boost::format
                        tags_table_f ("CREATE TABLE IF NOT EXISTS tags "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' INTEGER NOT NULL, '%s' INTEGER NOT NULL, " 
                                        " '%s' INTEGER DEFAULT 1, "
                                        "UNIQUE ('%s', '%s'))");

                m_SQL->exec_sql ((tags_table_f 
                                    % "tagid"
                                    % "trackid"
                                    % "amplitude" 
                                    % "tagid"
                                    % "trackid"
                ).str()); 

                ///////////////////////////////////////////////////////////////
                /// ALBUM TAGS TABLE 
                ///////////////////////////////////////////////////////////////

                static boost::format
                        album_tags_table_f ("CREATE TABLE IF NOT EXISTS tags_album "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' INTEGER NOT NULL, '%s' INTEGER NOT NULL, " 
                                        " '%s' INTEGER DEFAULT 1, "
                                        "UNIQUE ('%s', '%s'))");

                m_SQL->exec_sql ((album_tags_table_f 
                                    % "tagid"
                                    % "albumid"
                                    % "amplitude" 
                                    % "tagid"
                                    % "albumid"
                ).str()); 

                ///////////////////////////////////////////////////////////////
                /// ALBUM RATING HISTORY TABLE 
                ///////////////////////////////////////////////////////////////

                static boost::format
                        album_rating_history_f ("CREATE TABLE IF NOT EXISTS album_rating_history "
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, '%s' TEXT NOT NULL, '%s' INTEGER NOT NULL, '%s' TEXT DEFAULT NULL, '%s' INTEGER NOT NULL)"); 

                m_SQL->exec_sql(
                                (album_rating_history_f
                                 % "mbid"
                                 % "rating"
                                 % "comment"
                                 % "time"
                                ).str()); 


                ///////////////////////////////////////////////////////////////
                /// TRACK TABLE 
                ///////////////////////////////////////////////////////////////

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
                                        "UNIQUE (%s, %s, %s, %s))");

                m_SQL->exec_sql ((track_table_f
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

                m_SQL->exec_sql ("CREATE VIEW IF NOT EXISTS track_view AS " 
                                "SELECT"
                                "  track.* "
                                ", album.id AS mpx_album_id"
                                ", album.album_rating AS album_rating"
                                ", artist.id AS mpx_artist_id "
                                ", album_artist.id AS mpx_album_artist_id "
                                " FROM track "
                                "JOIN album ON album.id = track.album_j JOIN artist "
                                "ON artist.id = track.artist_j JOIN album_artist ON album.album_artist_j = album_artist.id");

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

                mcs->subscribe(
                      "Preferences-FileFormatPriorities"
                    , "Format6"
                    , sigc::mem_fun(
                            *this,
                            &Library::on_priority_settings_changed
                ));

                mcs->subscribe(
                      "Preferences-FileFormatPriorities"
                    , "prioritize-by-filetype"
                    , sigc::mem_fun(
                            *this,
                            &Library::on_priority_settings_changed
                ));

                mcs->subscribe(
                      "Preferences-FileFormatPriorities"
                    , "prioritize-by-bitrate"
                    , sigc::mem_fun(
                            *this,
                            &Library::on_priority_settings_changed
                ));

                library_scanner_thread_set_priorities();
        }

        Library::~Library ()
        {
            m_ScannerThread->finish();
            m_ScannerThread.reset();
        }

        void
                Library::library_scanner_thread_set_priorities(
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
                Library::on_priority_settings_changed(
                    MCS_CB_DEFAULT_SIGNATURE
                )
                {
                    library_scanner_thread_set_priorities();
                }

#ifdef HAVE_HAL
        void
                Library::switch_mode(
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
                                "SELECT id, hal_device_udi, hal_volume_udi, hal_vrp, insert_path FROM track"
                            );

                            for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                            {
                                Track_sp t = sqlToTrack( *i, false );

                                mm->push_message((boost::format(_("Updating Track Locations: %lld / %lld")) % gint64(std::distance(v.begin(), i)) % v.size()).str());

                                g_assert( t->has(ATTRIBUTE_LOCATION) );
                                g_assert( (*i).count("id") );

                                const std::string& volume_udi  = get<std::string>((*i)["hal_volume_udi"]) ;
                                const std::string& device_udi  = get<std::string>((*i)["hal_device_udi"]) ;
                                const std::string& insert_path = Util::normalize_path(get<std::string>((*i)["insert_path"]));
                                const std::string& mount_point = services->get<HAL>("mpx-service-hal")->get_mount_point_for_volume(volume_udi, device_udi) ;

                                std::string insert_path_new = filename_to_uri( build_filename( Util::normalize_path(mount_point), insert_path ));
    
                                execSQL(
                                    mprintf(
                                          "UPDATE track SET location = '%q', insert_path = '%q', hal_device_udi = NULL, hal_volume_udi = NULL, hal_vrp = NULL WHERE id ='%lld'"
                                        , get<std::string>((*(t.get()))[ATTRIBUTE_LOCATION].get()).c_str()
                                        , insert_path_new.c_str()
                                        , get<gint64>((*i)["id"])
                                ));

                            }

                            m_Flags &= ~F_USING_HAL;

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
                                        const HAL::Volume& volume( services->get<HAL>("mpx-service-hal")->get_volume_for_uri( insert_path )); 
                                        const std::string& insert_path_new = 
                                                Util::normalize_path(filename_from_uri( Util::normalize_path(insert_path) ).substr( volume.mount_point.length() ));

                                        execSQL(
                                            mprintf(
                                                  "UPDATE track SET insert_path = '%q', hal_device_udi = '%q', hal_volume_udi = '%q', hal_vrp = '%q', location = NULL WHERE id ='%lld'"
                                                , insert_path_new.c_str()
                                                , get<std::string>(t[ATTRIBUTE_HAL_DEVICE_UDI].get()).c_str()
                                                , get<std::string>(t[ATTRIBUTE_HAL_VOLUME_UDI].get()).c_str()
                                                , get<std::string>(t[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str()
                                                , get<gint64>((*i)["id"])
                                        ));
                                } catch( HAL::NoVolumeForUriError & cxe )
                                {
                                        g_message("%s: Critical: Non-conversible!", G_STRFUNC);
                                }
                            }

                            mm->push_message(_("Updating Track HAL Data: Done")); 
                        }


                        m_SQL->exec_sql((boost::format ("UPDATE meta SET flags = '%lld' WHERE rowid = 1") % m_Flags).str());

                        reload();
                }
#endif // HAVE_HAL

        bool
                Library::recache_covers_handler (SQL::RowV *v, int* position)
                {
                        Row & r = (*v)[*position]; 

                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");
                        mm->push_message((boost::format(_("Refreshing album covers: %lld / %lld")) % *position % (*v).size()).str());

                        Track_sp t = sqlToTrack( r, false );

                        std::string location = get<std::string>((*(t.get()))[ATTRIBUTE_LOCATION].get()) ;
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

                        RequestQualifier rq;
                        rq.mbid   = mb_album_id;
                        rq.asin   = amazon_asin;
                        rq.uri    = location;
                        rq.artist = album_artist;
                        rq.album  = album;

                        services->get<Covers>("mpx-service-covers")->cache(
                              rq
                            , true
                        );

                        (*position)++;

                        if((*position) == (*v).size())
                        {
                                mm->push_message("");

                                delete v;
                                delete position;
                                return false;
                        }

                        return true;
                }

        void
                Library::recacheCovers()
                {
                        services->get<Covers>("mpx-service-covers")->purge();
                        reload ();

                        RowV * v = new RowV;
                        getSQL(*v, "SELECT DISTINCT album_j, location, hal_volume_udi, hal_device_udi, hal_vrp, album.mb_album_id, album.amazon_asin, album_artist.album_artist, album.album "
                                        "FROM track JOIN album ON album_j = album.id JOIN album_artist ON album.album_artist_j = album_artist.id GROUP BY album_j");
                        int * position = new int;
                        *position = 0;

                        signal_timeout().connect( sigc::bind( sigc::mem_fun( *this, &Library::recache_covers_handler ), v, position), 500);
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
                        Signals.NewAlbum.emit( album_id );
                }

        void
                Library::on_new_artist(
                    gint64 artist_id
                )
                {
                        Signals.NewArtist.emit( artist_id );
                }

        void
                Library::on_new_track(
                      Track& track
                    , gint64 album_id
                    , gint64 artist_id
                )
                {
                        Signals.NewTrack.emit( track, album_id, artist_id );
                }
    
        void
                Library::on_entity_deleted(
                      gint64      id
                    , EntityType  type
                )
                {
                    switch( type )
                    {
                        case ENTITY_TRACK:
                            Signals.TrackDeleted.emit( id );
                            break;

                        case ENTITY_ALBUM:
                            Signals.AlbumDeleted.emit( id );
                            break;

                        case ENTITY_ARTIST:
                            Signals.ArtistDeleted.emit( id );
                            break;

                        case ENTITY_ALBUM_ARTIST:
                            Signals.AlbumArtistDeleted.emit( id );
                            break;

                    }
                }

        void
                Library::on_entity_updated(
                      gint64      id
                    , EntityType  type
                )
                {
                    switch( type )
                    {
                        case ENTITY_ALBUM:
                            Signals.AlbumUpdated.emit( id );
                            break;

                        case ENTITY_TRACK:
                            g_warning("%s: Got ENTITY_TRACK in on_entity_updated!", G_STRLOC);
                            break;

                        case ENTITY_ARTIST:
                        case ENTITY_ALBUM_ARTIST:
                            break;

                    }
                }

        void
                Library::on_track_updated(
                        Track&      t
                      , gint64      id_album
                      , gint64      id_artst
                )
                {
                    Signals.TrackUpdated.emit( t, id_album, id_artst );
                }


        void
                Library::on_message(
                    const std::string& msg
                )
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");
                        mm->push_message( msg );
                }

        void
                Library::removeDupes()
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

                        execSQL("BEGIN");

                        RowV rows_tracks;
                        getSQL(
                            rows_tracks,
                            "SELECT id FROM (track NATURAL JOIN (SELECT artist, album, title, track FROM track GROUP BY artist, album, title, track HAVING count(title) > 1)) EXCEPT SELECT id FROM track GROUP BY artist, album, title, track HAVING count(title) > 1" 
                        );
    
#ifdef HAVE_HAL
                        typedef std::map<HAL::VolumeKey, std::string> VolMountPointMap;
#endif
                        VolMountPointMap m;

                        for( RowV::iterator i = rows_tracks.begin(); i != rows_tracks.end(); ++i )
                        {
                                Row & rt = *i;

                                RowV rows;
                                getSQL(
                                    rows,
#ifdef HAVE_HAL
                                    mprintf("SELECT hal_volume_udi, hal_device_udi, hal_vrp FROM track WHERE id = '%lld'", get<gint64>(rt["id"])) 
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
                                                        Signals.TrackDeleted.emit( get<gint64>(rt["id"]) );
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
                Library::deletePath(
                    const std::string& hal_device_udi,
                    const std::string& hal_volume_udi,
                    const std::string& insert_path 
                )
                {
                    boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

                    SQL::RowV rows;

                    const std::string& sql = (
                        boost::format("SELECT id FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s' AND insert_path = '%s'")
                        % hal_device_udi
                        % hal_volume_udi
                        % Util::normalize_path( insert_path )
                    ).str();
    
                    getSQL(
                          rows
                        , sql
                    );

                    for( RowV::iterator i = rows.begin(); i != rows.end(); ++i )
                    {
                        Row & r = *i;
                        mm->push_message((boost::format(_("Deleting Track: %lld of %lld")) % gint64(std::distance(rows.begin(), i)) % gint64(rows.size())).str());
                        execSQL( mprintf("DELETE FROM track WHERE id = '%lld'", get<gint64>(r["id"]))); 
                        Signals.TrackDeleted.emit( get<gint64>(r["id"]) );
                    }

                    remove_dangling();
                }
#endif // HAVE_HAL

        void
                Library::vacuum()
                {
                        m_ScannerThread->vacuum();
                }

#ifdef HAVE_HAL    
        void
		        Library::vacuumVolume(
                      const std::string& device_udi
                    , const std::string& volume_udi
                )
                {
                        m_ScannerThread->vacuum_volume( device_udi, volume_udi );
                }
#endif // HAVE_HAL

        RowV
                Library::getTrackTags (gint64 id)
                {
                        RowV rows ;

                        char const get_f[] = "SELECT tagid, amplitude FROM tags WHERE trackid = %lld" ;

                        getSQL(rows, mprintf(get_f, id)) ; 

                        return rows ;
                }

        void
                Library::getMetadata (const std::string& uri, Track & track)
                {
                        services->get<MetadataReaderTagLib>("mpx-service-taglib")->get(uri, track);

                        track[ATTRIBUTE_LOCATION] = uri; 
  
#ifdef HAVE_HAL
                        if( m_Flags & F_USING_HAL )
                        {
                                try{
                                        URI u (uri);
                                        if( u.get_protocol() == URI::PROTOCOL_FILE )
                                        {
                                                try{
                                                        HAL::Volume const& volume (services->get<HAL>("mpx-service-hal")->get_volume_for_uri (uri));

                                                        track[ATTRIBUTE_HAL_VOLUME_UDI] =
                                                                volume.volume_udi ;

                                                        track[ATTRIBUTE_HAL_DEVICE_UDI] =
                                                                volume.device_udi ;

                                                        track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                                                                filename_from_uri (uri).substr (volume.mount_point.length()) ;
                                                }
                                                catch( HAL::Exception& cxe )
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

#ifndef HAVE_HAL
        inline
#endif //HAVE_HAL
        void
                Library::trackSetLocation(
                    Track&              track,
                    const std::string&  uri
                )
                {
                        track[ATTRIBUTE_LOCATION] = uri;
#ifdef HAVE_HAL
                        if( m_Flags & F_USING_HAL )
                        {
                                try{
                                        URI u (uri);
                                        if( u.get_protocol() == URI::PROTOCOL_FILE )
                                        {
                                                try{
                                                        {
                                                                HAL::Volume const& volume ( services->get<HAL>("mpx-service-hal")->get_volume_for_uri( uri ));

                                                                track[ATTRIBUTE_HAL_VOLUME_UDI] =
                                                                        volume.volume_udi ;

                                                                track[ATTRIBUTE_HAL_DEVICE_UDI] =
                                                                        volume.device_udi ;

                                                                track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                                                                        filename_from_uri( uri ).substr( volume.mount_point.length() ) ;
                                                        }
                                                }
                                                catch (HAL::Exception & cxe)
                                                {
                                                        g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
                                                        throw FileQualificationError((boost::format("%s: %s") % uri % cxe.what()).str());
                                                }
                                                catch (Glib::ConvertError & cxe)
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
                Library::trackGetLocation( const Track& track )
                {
#ifdef HAVE_HAL
                        if( m_Flags & F_USING_HAL )
                        {
                                try{
                                        const std::string& volume_udi  = get<std::string>(track[ATTRIBUTE_HAL_VOLUME_UDI].get()) ;
                                        const std::string& device_udi  = get<std::string>(track[ATTRIBUTE_HAL_DEVICE_UDI].get()) ;
                                        const std::string& vrp         = get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()) ;
                                        const std::string& mount_point = services->get<HAL>("mpx-service-hal")->get_mount_point_for_volume(volume_udi, device_udi) ;

                                        return filename_to_uri( build_filename( Util::normalize_path(mount_point), vrp) );

                                } catch (HAL::NoMountPathForVolumeError & cxe)
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
                Library::albumTagged(gint64 id, std::string const& tag)
                {
                        gint64 tag_id = get_tag_id( tag );

                        char const insert_f[] = "INSERT INTO tags_album (tagid, albumid) VALUES (%lld, %lld)";
                        char const update_f[] = "UPDATE tags SET amplitude = amplitude + 1 WHERE albumid = %lld AND tagid = %lld";
                        try{
                                execSQL(mprintf(insert_f, id, tag_id)); 
                        } catch( SqlConstraintError & cxe )
                        {
                                execSQL(mprintf(update_f, id, tag_id));
                        }
                }

        void	
                Library::trackRated(gint64 id, int rating)
                {
                        execSQL((boost::format ("UPDATE track SET rating = '%d' WHERE id = %lld") % rating % id).str());

                        RowV v;
                        getSQL(
                              v
                            , (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % id).str()
                        );

                        Track_sp t = sqlToTrack( v[0] );

                        gint64 id_album = get<gint64>(v[0]["album_j"]);
                        gint64 id_artst = get<gint64>(v[0]["album_artist_j"]);

                        Signals.TrackUpdated.emit( *(t.get()), id_album, id_artst );
                }

        void	
                Library::trackPlayed(gint64 id, gint64 album_id, time_t time_)
                {
                        execSQL((boost::format ("UPDATE track SET pdate = '%lld' WHERE id = %lld") % gint64(time_) % id).str());
                        execSQL((boost::format ("UPDATE track SET pcount = ifnull(pcount,0) + 1 WHERE Id =%lld") % id).str());

                        RowV v;

                        getSQL(v, (boost::format ("SELECT SUM(pcount) AS count FROM track_view WHERE album_j = '%lld'") % album_id).str());
                        gint64 count1 = get<gint64>(v[0]["count"]);
                        v.clear();

                        getSQL(v, (boost::format ("SELECT count(id) AS count FROM track_view WHERE album_j = '%lld'") % album_id).str());
                        gint64 count2 = get<gint64>(v[0]["count"]);

                        double score = double(count1) / double(count2);

                        execSQL((boost::format ("UPDATE album SET album_playscore = '%f' WHERE album.id = '%lld'") % score % album_id).str());

                        v.clear(); 
                        getSQL(
                              v
                            , (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % id).str()
                        );

                        Track_sp t = sqlToTrack( v[0] );

                        gint64 id_album = get<gint64>(v[0]["album_j"]);
                        gint64 id_artst = get<gint64>(v[0]["album_artist_j"]);

                        Signals.TrackUpdated.emit( (*t.get()), id_album, id_artst );
                        Signals.AlbumUpdated.emit( album_id ) ;
                }

        void
                Library::trackTagged(gint64 id, std::string const& tag)
                {
                        gint64 tag_id = get_tag_id( tag );

                        char const insert_f[] = "INSERT INTO tags (tagid, trackid) VALUES (%lld, %lld)";
                        char const update_f[] = "UPDATE tags SET amplitude = amplitude + 1 WHERE trackid = %lld AND tagid = %lld";
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
                                getSQL (rows, (boost::format("SELECT * FROM markov WHERE track_id_a = '%lld' AND count > 0") % a).str());
                                if( !rows.empty() )
                                {
                                        for( RowV::const_iterator i = rows.begin(); i != rows.end(); ++i )
                                        {
                                                gint64 count = get <gint64> ((*i).find ("count")->second);
                                                ratios.push_back(double(count) / double(rows.size()));
                                        }
                                        int row = MPX::rand(ratios);
                                        g_assert( row < rows.size() );
                                        gint64 id = get <gint64> (rows[row].find ("track_id_b")->second);
                                        g_message("Determined Markov-track-chain-ID [%lld], for track [%lld]", id, a);
                                        return id;
                                }
                                g_message("No rows based on Markov chain for track [%lld]", a);
                        } catch( ... ) {
                                g_message("Exception in %s", G_STRFUNC);
                        }

                        return 0;
                }

        Track_sp
                Library::sqlToTrack(
                      SQL::Row & row
                    , bool all_metadata
                )
                {
                        Track_sp track (new Track);

#ifdef HAVE_HAL
                        if( m_Flags & F_USING_HAL )
                        {
                            if (row.count("hal_volume_udi"))
                                    (*track.get())[ATTRIBUTE_HAL_VOLUME_UDI] = get<std::string>(row["hal_volume_udi"]);

                            if (row.count("hal_device_udi"))
                                    (*track.get())[ATTRIBUTE_HAL_DEVICE_UDI] = get<std::string>(row["hal_device_udi"]);

                            if (row.count("hal_vrp"))
                                    (*track.get())[ATTRIBUTE_VOLUME_RELATIVE_PATH] = get<std::string>(row["hal_vrp"]);

                            (*track.get())[ATTRIBUTE_LOCATION] = trackGetLocation( (*track.get()) ); 

                            g_assert( (*track.get()).has(ATTRIBUTE_LOCATION) );
                        }
                        else
#endif
                        if( row.count("location") )
                        {
                            (*track.get())[ATTRIBUTE_LOCATION] = get<std::string>(row["location"]);
                        }

                        if( row.count("id") )
                        {
                            (*track.get())[ATTRIBUTE_MPX_TRACK_ID] = get<gint64>(row["id"]);
                        }

                        if( all_metadata )
                        {
                                if (row.count("album_artist"))
                                        (*track.get())[ATTRIBUTE_ALBUM_ARTIST] = get<std::string>(row["album_artist"]);

                                if (row.count("artist"))
                                        (*track.get())[ATTRIBUTE_ARTIST] = get<std::string>(row["artist"]);

                                if (row.count("album"))
                                        (*track.get())[ATTRIBUTE_ALBUM] = get<std::string>(row["album"]);

                                if (row.count("track"))
                                        (*track.get())[ATTRIBUTE_TRACK] = gint64(get<gint64>(row["track"]));

                                if (row.count("title"))
                                        (*track.get())[ATTRIBUTE_TITLE] = get<std::string>(row["title"]);

                                if (row.count("time"))
                                        (*track.get())[ATTRIBUTE_TIME] = gint64(get<gint64>(row["time"]));

                                if (row.count("mb_artist_id"))
                                        (*track.get())[ATTRIBUTE_MB_ARTIST_ID] = get<std::string>(row["mb_artist_id"]);

                                if (row.count("mb_album_id"))
                                        (*track.get())[ATTRIBUTE_MB_ALBUM_ID] = get<std::string>(row["mb_album_id"]);

                                if (row.count("mb_track_id"))
                                        (*track.get())[ATTRIBUTE_MB_TRACK_ID] = get<std::string>(row["mb_track_id"]);

                                if (row.count("mb_album_artist_id"))
                                        (*track.get())[ATTRIBUTE_MB_ALBUM_ARTIST_ID] = get<std::string>(row["mb_album_artist_id"]);

                                if (row.count("mb_release_country"))
                                        (*track.get())[ATTRIBUTE_MB_RELEASE_COUNTRY] = get<std::string>(row["mb_release_country"]);

                                if (row.count("mb_release_type"))
                                        (*track.get())[ATTRIBUTE_MB_RELEASE_TYPE] = get<std::string>(row["mb_release_type"]);

                                if (row.count("musicip_puid"))
                                        (*track.get())[ATTRIBUTE_MUSICIP_PUID] = get<std::string>(row["musicip_puid"]);

                                if (row.count("date"))
                                        (*track.get())[ATTRIBUTE_DATE] = get<gint64>(row["date"]);

                                if (row.count("amazon_asin"))
                                        (*track.get())[ATTRIBUTE_ASIN] = get<std::string>(row["amazon_asin"]);

                                if (row.count("id"))
                                        (*track.get())[ATTRIBUTE_MPX_TRACK_ID] = get<gint64>(row["id"]);

                                if (row.count("album_j"))
                                        (*track.get())[ATTRIBUTE_MPX_ALBUM_ID] = get<gint64>(row["album_j"]);

                                if (row.count("type"))
                                        (*track.get())[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);

                                if (row.count("bitrate"))
                                        (*track.get())[ATTRIBUTE_BITRATE] = get<gint64>(row["bitrate"]);

                                if (row.count("samplerate"))
                                        (*track.get())[ATTRIBUTE_SAMPLERATE] = get<gint64>(row["samplerate"]);

                                if (row.count("type"))
                                        (*track.get())[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);
                        }

                        g_assert( (*track.get()).has(ATTRIBUTE_LOCATION) );

                        return track;
                }


        gint64
                Library::collectionCreate(
                    const std::string& name,
                    const std::string& blurb
                )
                {

                    static std::string 
                            collection_table_create_f ("CREATE TABLE collection_%lld (track_id INTEGER NOT NULL)");

                    static std::string 
                            collection_create_f ("INSERT INTO collection (name, blurb) VALUES ('%q', '%q')");

                    execSQL("BEGIN");

                    execSQL(mprintf(collection_create_f.c_str(),
                        name.c_str(),
                        blurb.c_str()
                    ));

                    gint64 id = m_SQL->last_insert_rowid(); // FIXME: NOT MULTIPLE CONNECTION SAFE

                    execSQL(mprintf(collection_table_create_f.c_str(),
                            id
                    ));

                    execSQL("COMMIT");

                    Signals.Collection.New.emit( id );
                   
                    return id; 
                }

        void
                Library::collectionDelete(
                    gint64 id
                )
                {

                    static std::string
                            collection_table_delete_f ("DROP TABLE collection_%lld");    
                        
                    static std::string
                            collection_delete_f ("DELETE FROM collection WHERE id = '%lld'");

                    execSQL("BEGIN");

                    execSQL(mprintf(collection_delete_f.c_str(),
                            id
                    ));

                    execSQL(mprintf(collection_table_delete_f.c_str(),
                            id
                    ));

                    execSQL("COMMIT");

                    Signals.Collection.Deleted.emit( id );
                }

        void
                Library::collectionAppend(
                    gint64      id,
                    const IdV&  tracks
                )
                {
                    static std::string
                            collection_append_f ("INSERT INTO collection_%lld (track_id) VALUES ('%lld')");

                    execSQL("BEGIN");

                    for( IdV::const_iterator i = tracks.begin(); i != tracks.end(); ++i )
                    {
                            execSQL(mprintf(collection_append_f.c_str(),
                                    id,
                                    *i
                            ));
                            // XXX: We can afford to emit the signal here even though we are in a transaction, because the tracks already exist in the library
                            Signals.Collection.NewTrack.emit( id, *i );
                    }

                    execSQL("COMMIT");
                } 

        void
                Library::collectionErase(
                    gint64      id,
                    const IdV&  tracks
                )
                {
                    static std::string
                            collection_erase_f ("DELETE FROM collection_%lld WHERE track_id ='%lld'");

                    for( IdV::const_iterator i = tracks.begin(); i != tracks.end(); ++i )
                    {
                            execSQL(mprintf(collection_erase_f.c_str(),
                                    id,
                                    *i
                            ));

                            Signals.Collection.TrackDeleted.emit( id, *i );
                    }
                }

        void
                Library::collectionGetMeta(
                    gint64          id,
                    Collection&     collection
                )
                {
                    static std::string
                            collection_acquire_meta ("SELECT * FROM collection WHERE id ='%lld'");

                    static std::string
                            collection_acquire_tracks ("SELECT track_id FROM collection_%lld");

                    SQL::RowV v;

                    getSQL(
                        v,
                        mprintf(
                            collection_acquire_meta.c_str(),
                            id
                        )
                    );

                    collection = Collection();

                    collection.Id = get<gint64>(v[0]["id"]);

                    if( v[0].count("name") )
                        collection.Name         = get<std::string>(v[0]["name"]);

                    if( v[0].count("blurb") )
                        collection.Blurb        = get<std::string>(v[0]["blurb"]);

                    if( v[0].count("cover_url") )
                        collection.Cover_URL    = get<std::string>(v[0]["cover_url"]);
                }

        void
                Library::collectionGetAll(
                    IdV&   collections
                )
                {
                    static std::string
                           collections_get_f ("SELECT id FROM collection");

                    SQL::RowV v;

                    getSQL(
                        v,
                        collections_get_f.c_str()
                    );
                         
                    for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )   
                    {
                        collections.push_back(get<gint64>((*i)["id"]));
                    }
                }

        void
                Library::collectionGetTracks(
                    gint64          id,
                    IdV&            tracks
                )
                {
                    static std::string
                            collection_acquire_tracks ("SELECT track_id FROM collection_%lld");

                    SQL::RowV v;

                    getSQL(
                        v,
                        mprintf(
                            collection_acquire_tracks.c_str(),
                            id
                        )
                    );

                    for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                    {
                        tracks.push_back( get<gint64>((*i)["track_id"]));
                    }
                }

        gint64
                Library::get_tag_id (std::string const& tag)
                {
                        gint64 tag_id = 0;
                        RowV rows;
                        std::string sql;

                        get_tag_id:

                        char const* select_tag_f ("SELECT id FROM tag WHERE tag = '%q'"); 
                        sql = mprintf(select_tag_f, tag.c_str());
                        m_SQL->get (rows, sql); 

                        if(!rows.empty())
                        {
                                tag_id = get <gint64> (rows[0].find ("id")->second);
                        }
                        else
                        {
                                char const* set_tag_f ("INSERT INTO tag (tag) VALUES ('%q')");
                                sql = mprintf(set_tag_f, tag.c_str());
                                m_SQL->exec_sql (sql);
                                goto get_tag_id; // threadsafe, as opposed to getting last insert rowid
                        }
                        g_assert(tag_id != 0);
                        return tag_id;
                }

        void
                Library::initScanAll ()
                { 
                        m_ScannerThread->scan_all();
                }

        void
                Library::initScan (const Util::FileList & list, bool deep)
                {
                        m_ScannerThread->scan(list, deep);
                }

        void
                Library::initAdd (const Util::FileList & list)
                {
                        m_ScannerThread->add(list);
                }

        void
                Library::remove_dangling () 
                {
                        boost::shared_ptr<MPX::MLibManager> mm = services->get<MLibManager>("mpx-service-mlibman");

                        typedef std::tr1::unordered_set <gint64> IdSet;
                        static boost::format delete_f ("DELETE FROM %s WHERE id = '%lld'");
                        IdSet idset1;
                        IdSet idset2;
                        RowV rows;

                        /// CLEAR DANGLING ARTISTS
                        mm->push_message(_("Finding Lost Artists..."));

                        idset1.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT artist_j FROM track");
                        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
                                idset1.insert (get<gint64>(i->find ("artist_j")->second));

                        idset2.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT id FROM artist");
                        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
                                idset2.insert (get<gint64>(i->find ("id")->second));

                        for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
                        {
                                if (idset1.find (*i) == idset1.end())
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
                        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
                                idset1.insert (get<gint64>(i->find ("album_j")->second));

                        idset2.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT id FROM album");
                        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
                                idset2.insert (get<gint64>(i->find ("id")->second));

                        for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
                        {
                                if (idset1.find (*i) == idset1.end())
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
                        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
                                idset1.insert (get<gint64>(i->find ("album_artist_j")->second));

                        idset2.clear();
                        rows.clear();
                        m_SQL->get(rows, "SELECT DISTINCT id FROM album_artist");
                        for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
                                idset2.insert (get<gint64>(i->find ("id")->second));

                        for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
                        {
                                if (idset1.find (*i) == idset1.end())
                                        m_SQL->exec_sql((delete_f % "album_artist" % (*i)).str());
                                        on_entity_deleted( *i , ENTITY_ALBUM_ARTIST );
                        }
                }
} // namespace MPX
