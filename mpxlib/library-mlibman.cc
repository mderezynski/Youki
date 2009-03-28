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

#include <boost/format.hpp>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <giomm.h>

#include <tr1/unordered_set>

#include "mpx/mpx-audio.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-uri.hh"
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

                {   "audio_quality",
                        VALUE_TYPE_INT      },
        };
}

namespace MPX
{

        Library_MLibMan::Library_MLibMan(
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

                RowV rows;
                m_SQL->get( rows, "SELECT flags FROM meta WHERE rowid = 1" ) ; 
                m_Flags |= get<gint64>(rows[0].find("flags")->second); 

                mcs->key_set<bool>("library","use-hal", bool(m_Flags & F_USING_HAL));

                m_ScannerThread = boost::shared_ptr<LibraryScannerThread_MLibMan>(
                        new LibraryScannerThread_MLibMan(
                            this,
                            m_Flags
                ));

                m_ScannerThread->run();

                m_ScannerThread->connect().signal_new_album().connect(
                    sigc::mem_fun(
                        *this,
                        &Library_MLibMan::on_new_album
                ));

                m_ScannerThread->connect().signal_new_artist().connect(
                    sigc::mem_fun(
                        *this,
                        &Library_MLibMan::on_new_artist
                ));

                m_ScannerThread->connect().signal_new_track().connect(
                    sigc::mem_fun(
                        *this,
                        &Library_MLibMan::on_new_track
                ));

                m_ScannerThread->connect().signal_entity_deleted().connect(
                    sigc::mem_fun(
                        *this,
                        &Library_MLibMan::on_entity_deleted
                ));

                m_ScannerThread->connect().signal_entity_updated().connect(
                    sigc::mem_fun(
                        *this,
                        &Library_MLibMan::on_entity_updated
                ));

                m_ScannerThread->connect().signal_track_updated().connect(
                    sigc::mem_fun(
                        *this,
                        &Library_MLibMan::on_track_updated
                ));

                m_ScannerThread->connect().signal_message().connect(
                    sigc::mem_fun(
                        *this, 
                        &Library_MLibMan::on_message
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
                }
#endif // HAVE_HAL

        void
                Library_MLibMan::on_new_album(
                    gint64 album_id
                )
                {
                        Signals.NewAlbum.emit( album_id );
                }

        void
                Library_MLibMan::on_new_artist(
                    gint64 artist_id
                )
                {
                        Signals.NewArtist.emit( artist_id );
                }

        void
                Library_MLibMan::on_new_track(
                      Track& track
                    , gint64 album_id
                    , gint64 artist_id
                )
                {
                        Signals.NewTrack.emit( track, album_id, artist_id );
                }
    
        void
                Library_MLibMan::on_entity_deleted(
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
                Library_MLibMan::on_entity_updated(
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
                Library_MLibMan::on_track_updated(
                        Track&      t
                      , gint64      id_album
                      , gint64      id_artst
                )
                {
                    Signals.TrackUpdated.emit( t, id_album, id_artst );
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
                Library_MLibMan::deletePath(
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
                Library_MLibMan::vacuum()
                {
                        m_ScannerThread->vacuum();
                }

#ifdef HAVE_HAL    
        void
		        Library_MLibMan::vacuumVolumeList(
                      const HAL::VolumeKey_v& v
                )
                {
                        m_ScannerThread->vacuum_volume_list( v ); 
                }
#endif // HAVE_HAL

        void
                Library_MLibMan::getMetadata (const std::string& uri, Track & track)
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
                Library_MLibMan::trackSetLocation(
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
                Library_MLibMan::trackGetLocation( const Track& track )
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
                Library_MLibMan::getSQL( RowV & rows, const std::string& sql)
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
                      SQL::Row & row
                    , bool all_metadata
                    , bool no_location
                )
                {
                        Track_sp track (new Track);

                        if( !no_location )
                        {
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

                                if (row.count("artist_j"))
                                        (*track.get())[ATTRIBUTE_MPX_ARTIST_ID] = get<gint64>(row["artist_j"]);

                                if (row.count("mpx_album_artist_id"))
                                        (*track.get())[ATTRIBUTE_MPX_ALBUM_ARTIST_ID] = get<gint64>(row["mpx_album_artist_id"]);

                                if (row.count("type"))
                                        (*track.get())[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);

                                if (row.count("bitrate"))
                                        (*track.get())[ATTRIBUTE_BITRATE] = get<gint64>(row["bitrate"]);

                                if (row.count("samplerate"))
                                        (*track.get())[ATTRIBUTE_SAMPLERATE] = get<gint64>(row["samplerate"]);

                                if (row.count("type"))
                                        (*track.get())[ATTRIBUTE_TYPE] = get<std::string>(row["type"]);

                                if (row.count("audio_quality"))
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
                Library_MLibMan::initScan (const Util::FileList & list)
                {
                        m_ScannerThread->scan(list);
                }

        void
                Library_MLibMan::initAdd (const Util::FileList & list)
                {
                        m_ScannerThread->add(list);
                }

        void
                Library_MLibMan::remove_dangling () 
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
