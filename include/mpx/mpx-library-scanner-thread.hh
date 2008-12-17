//
// library-scanner-thread
//
// Authors:
//     Milosz Derezynski <milosz@backtrace.info>
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
#ifndef MPX_LIBRARY_SCANNER_THREAD_HH
#define MPX_LIBRARY_SCANNER_THREAD_HH
#include <config.h>
#include <glibmm.h>
#include <sigx/sigx.h>
#include <sigx/signal_f.h>
#include <sigx/request_f.h>
#include <tr1/unordered_map> 
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"
#include "mpx/util-file.hh"

namespace MPX
{
    enum ScanResult
    {
        SCAN_RESULT_OK,
        SCAN_RESULT_UPDATE,
    };

    enum EntityType
    {
        ENTITY_TRACK,
        ENTITY_ALBUM,
        ENTITY_ARTIST,
        ENTITY_ALBUM_ARTIST
    };

    typedef std::pair<std::string, std::string> SSFileInfo;

    struct ScanSummary
    {
        gint64  FilesAdded;
        gint64  FilesErroneous;
        gint64  FilesUpToDate;
        gint64  FilesUpdated;
        gint64  FilesTotal;
        bool    DeepRescan;

        std::vector<SSFileInfo> FileListErroneous;
        std::vector<SSFileInfo> FileListUpdated;

        ScanSummary ()
        : FilesAdded(0)
        , FilesErroneous(0)
        , FilesUpToDate(0)
        , FilesUpdated(0)
        , FilesTotal(0)
        {
        }
    };

#include "mpx/exception.hh"

    EXCEPTION(ScanError)

    class HAL;
    class Library;
    class MetadataReaderTagLib;

	class LibraryScannerThread : public sigx::glib_threadable
	{
        public:

            typedef sigc::signal<void>                              SignalScanStart_t ;
            typedef sigc::signal<void, gint64,bool>                 SignalScanRun_t ;
            typedef sigc::signal<void>                              SignalScanEnd_t ;
            typedef sigc::signal<void, ScanSummary const&>          SignalScanSummary_t ;
            typedef sigc::signal<void, gint64>                      SignalNewAlbum_t ;
            typedef sigc::signal<void, gint64>                      SignalNewArtist_t ;
            typedef sigc::signal<void, Track&, gint64, gint64>      SignalNewTrack_t ;
            typedef sigc::signal<void, Track&, gint64, gint64>      SignalTrackUpdated_t ;
            typedef sigc::signal<void, gint64, EntityType>          SignalEntityDeleted_t ;
            typedef sigc::signal<void, gint64, EntityType>          SignalEntityUpdated_t ;
            typedef sigc::signal<void, const RequestQualifier&>     SignalCacheCover_t ;
            typedef sigc::signal<void>                              SignalReload_t ;
            typedef sigc::signal<void, const std::string&>          SignalMessage_t ;
            
            typedef sigx::signal_f<SignalScanStart_t>               signal_scan_start_x ;
            typedef sigx::signal_f<SignalScanRun_t>                 signal_scan_run_x ; 
            typedef sigx::signal_f<SignalScanEnd_t>                 signal_scan_end_x ;
            typedef sigx::signal_f<SignalScanSummary_t>             signal_scan_summary_x ;
            typedef sigx::signal_f<SignalNewAlbum_t>                signal_new_album_x ;
            typedef sigx::signal_f<SignalNewArtist_t>               signal_new_artist_x ;
            typedef sigx::signal_f<SignalNewTrack_t>                signal_new_track_x ;
            typedef sigx::signal_f<SignalTrackUpdated_t>            signal_track_updated_x ;
            typedef sigx::signal_f<SignalEntityDeleted_t>           signal_entity_deleted_x ;
            typedef sigx::signal_f<SignalEntityUpdated_t>           signal_entity_updated_x ;
            typedef sigx::signal_f<SignalCacheCover_t>              signal_cache_cover_x ;
            typedef sigx::signal_f<SignalReload_t>                  signal_reload_x ;
            typedef sigx::signal_f<SignalMessage_t>                 signal_message_x ;

        public:

            sigx::request_f<Util::FileList const&, bool>    scan ;
            sigx::request_f<Util::FileList const&>          scan_direct ;
            sigx::request_f<>                               scan_stop ;
            sigx::request_f<>                               vacuum ;
#ifdef HAVE_HAL
            sigx::request_f< const std::string&, const std::string& > vacuum_volume ;
#endif // HAVE_HAL
            sigx::request_f<>                               update_statistics ;

            signal_scan_start_x             signal_scan_start ;
            signal_scan_run_x               signal_scan_run ; 
            signal_scan_end_x               signal_scan_end ;
            signal_scan_summary_x           signal_scan_summary ;
            signal_new_album_x              signal_new_album ;
            signal_new_artist_x             signal_new_artist ;
            signal_new_track_x              signal_new_track ;
            signal_track_updated_x          signal_track_updated ;
            signal_entity_deleted_x         signal_entity_deleted ;
            signal_entity_updated_x         signal_entity_updated ;
            signal_cache_cover_x            signal_cache_cover ;
            signal_reload_x                 signal_reload ;
            signal_message_x                signal_message ;

            struct ScannerConnectable
            {
                ScannerConnectable(
                      signal_scan_start_x&            start_x
                    , signal_scan_run_x&              run_x
                    , signal_scan_end_x&              end_x
                    , signal_scan_summary_x&          summary_x
                    , signal_new_album_x&             album_x
                    , signal_new_artist_x&            artist_x
                    , signal_new_track_x&             track_x
                    , signal_track_updated_x&         track_updated_x
                    , signal_entity_deleted_x&        entity_deleted_x
                    , signal_entity_updated_x&        entity_updated_x
                    , signal_cache_cover_x&           cover_x
                    , signal_reload_x&                reload_x
                    , signal_message_x&               message_x 
                )
                : signal_scan_start(start_x)
                , signal_scan_run(run_x)
                , signal_scan_end(end_x)
                , signal_scan_summary(summary_x)
                , signal_new_album(album_x)
                , signal_new_artist(artist_x)
                , signal_new_track(track_x)
                , signal_track_updated(track_updated_x)
                , signal_entity_deleted(entity_deleted_x)
                , signal_entity_updated(entity_updated_x)
                , signal_cache_cover(cover_x)
                , signal_reload(reload_x)
                , signal_message(message_x)
                {
                }

                signal_scan_start_x         & signal_scan_start ;
                signal_scan_run_x           & signal_scan_run ; 
                signal_scan_end_x           & signal_scan_end ;
                signal_scan_summary_x       & signal_scan_summary ;
                signal_new_album_x          & signal_new_album ;
                signal_new_artist_x         & signal_new_artist ;
                signal_new_track_x          & signal_new_track ;
                signal_track_updated_x      & signal_track_updated ;
                signal_entity_deleted_x     & signal_entity_deleted ;
                signal_entity_updated_x     & signal_entity_updated ;
                signal_cache_cover_x        & signal_cache_cover ;
                signal_reload_x             & signal_reload ;
                signal_message_x            & signal_message ;
            };

            LibraryScannerThread(
                MPX::Library*,
                gint64
            ) ;

            ~LibraryScannerThread () ;

            ScannerConnectable&
            connect ();

        protected:

            virtual void on_startup () ; 
            virtual void on_cleanup () ;

// Requests /////////////

            void on_scan(
                const Util::FileList&,
                bool
            ) ;

            void on_scan_direct(
                const Util::FileList&
            ) ;

            void on_scan_stop() ;

            void on_vacuum ();

#ifdef HAVE_HAL
            void on_vacuum_volume(
                const std::string&,
                const std::string&
            );
#endif // HAVE_HAL

            void on_update_statistics ();

/////////////////////////

            void on_scan_list_deep(
                const Util::FileList&
            ); // on_scan delegate

            void on_scan_list_paths(
                const Util::FileList&
            ); // on_scan delegate

            void
            on_scan_list_paths_callback(
                const std::string&,
                const gint64&,
                const std::string&
            );

            gint64
            get_track_mtime(
                Track&
            ) const;

            gint64
            get_track_id(
                Track&
            ) const;

            /// INSERTION

            enum EntityIsNew
            {
                  ENTITY_IS_NEW
                , ENTITY_IS_NOT_NEW
                , ENTITY_IS_UNDEFINED
            };

            typedef std::pair<gint64, EntityIsNew>   EntityInfo;
 
            EntityInfo 
            get_track_artist_id(
                Track&,
                bool = false
            );

            EntityInfo 
            get_album_artist_id(
                Track&,
                bool = false
            );

            EntityInfo 
            get_album_id(
                Track&,
                gint64,
                bool = false
            );

            typedef boost::shared_ptr<Track>        Track_p;

            struct TrackInfo
            {
                EntityInfo      Artist;
                EntityInfo      Album;
                EntityInfo      AlbumArtist;

                gint64          TrackNumber;
                std::string     Title;
                std::string     Type;

                Track_p         Track;
                std::string     Insertion_SQL;
            };
 
            typedef boost::shared_ptr<TrackInfo>    TrackInfo_p;
            typedef std::vector<TrackInfo_p>        TrackInfo_p_Vector;

            typedef std::tr1::unordered_map<gint64,      TrackInfo_p_Vector>   Map_L4;
            typedef std::tr1::unordered_map<std::string, Map_L4>               Map_L3;
            typedef std::tr1::unordered_map<gint64,      Map_L3>               Map_L2;
            typedef std::tr1::unordered_map<gint64,      Map_L2>               Map_L1;

            //typedef std::map<gint64 , std::map<gint64, std::map<std::string, TrackInfo_p_Vector> > > InsertionTracks_t;

            Map_L1      m_InsertionTracks;

            void
            insert_file(
                const std::string& uri
              , const std::string& insert_path
            );

            void
            create_insertion_track(
                Track&             track
              , const std::string& uri
              , const std::string& insert_path
            );
    
            void
            process_insertion_list();

            TrackInfo_p
            prioritize(
                const TrackInfo_p_Vector& v
            );

            ScanResult
            insert(
                const TrackInfo_p&
            );

            void
            signal_new_entities(
                const TrackInfo_p_Vector&
            );

            void
            do_remove_dangling();

        private:

            ScannerConnectable                    * m_Connectable ;

            struct ThreadData;
            Glib::Private<ThreadData>               m_ThreadData ;

            MPX::Library                          & m_Library ;
            boost::shared_ptr<MPX::SQL::SQLDB>      m_SQL ;
            gint64                                  m_Flags ;

            ScanSummary                             m_ScanSummary ;
	};
}

#endif
