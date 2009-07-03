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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.
#ifndef MPX_LIBRARY_MLIBMAN_HH
#define MPX_LIBRARY_MLIBMAN_HH

#include <sigx/sigx.h>

#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-services.hh"

#include "library-scanner-thread-mlibman.hh"

#include "mpx/util-file.hh"
#include "mpx/util-string.hh"

#include "mpx/i-youki-hal.hh"


namespace MPX
{
    struct CollectionMeta
    {
        std::string Name;
        std::string Blurb;
        std::string Cover_URL;
        gint64      Id;
    };

    class IHAL;
    class MetadataReaderTagLib;
    class Library_MLibMan
    : public Service::Base
    , public sigx::glib_auto_dispatchable
    {
        friend class LibraryScannerThread_MLibMan;

        public:

#include "mpx/exception.hh"

            EXCEPTION(FileQualificationError)

            Library_MLibMan(
            ) ;

            virtual ~Library_MLibMan () ;

            void
            create_and_init () ;

            gint64
            get_flags ()
            {
                return m_Flags;
            }

            SQL::SQLDB*
            get_sql_db ()
            {
                return m_SQL;
            }

            boost::shared_ptr<LibraryScannerThread_MLibMan>
            scanner()
            {
                return m_ScannerThread;
            }

            void
            removeDupes() ;

            void
            vacuum() ;

#ifdef HAVE_HAL    
            void
            switch_mode(
                bool /* use hal */
            );

			void
			vacuumVolumeList(
                const VolumeKey_v&
            ) ;

            void
            deletePath(
                const std::string& /*hal_device_udi*/,
                const std::string& /*hal_volume_udi*/,
                const std::string& /*insert_path*/
            ) ;
#endif
            void
            initScanAll(
            ); 

            void
            initScan(
                const Util::FileList& /*list*/
            ); 

            void
            initAdd(
                const Util::FileList& /*list*/
            ); 

            void
            getSQL(
                SQL::RowV&,
                const std::string&
            ) ;

			void
			execSQL(
                const std::string&
            ) ;

            Track_sp 
            sqlToTrack(
                  SQL::Row&
                , bool /*all metadata?*/ = true
                , bool /*no_location?*/  = false
            ) ;

            void
            getMetadata(const std::string&, Track&) ;

#ifndef HAVE_HAL
            inline
#endif //HAVE_HAL
            void
            trackSetLocation( Track&, const std::string& );

#ifndef HAVE_HAL
            inline
#endif //HAVE_HAL
            std::string
            trackGetLocation( const Track& );

        public:

            typedef sigc::signal<void,
                  gint64
                , std::string
                , std::string
                , std::string
                , std::string
                , std::string>                                  SignalNewAlbum; 

            typedef sigc::signal<void,
                gint64 /*artist id*/>                           SignalNewArtist;

            typedef sigc::signal<void,
                gint64 /*track id*/>                            SignalNewTrack;

            typedef sigc::signal<void,
                gint64 /*album id*/>                            SignalAlbumDeleted; 

            typedef sigc::signal<void,
                gint64/*artistid*/>                             SignalAlbumArtistDeleted;

            typedef sigc::signal<void,
                gint64/*artistid*/>                             SignalArtistDeleted;

            typedef sigc::signal<void,
                gint64/*artistid*/>                             SignalTrackDeleted;

			typedef sigc::signal<void,
                gint64 /*track id*/>                            SignalTrackUpdated;

            typedef sigc::signal<void,
                gint64 /*album id*/>                            SignalAlbumUpdated;

            struct SignalsT
            {
                SignalNewAlbum                  NewAlbum;
                SignalNewArtist                 NewArtist;
                SignalNewTrack                  NewTrack;

				SignalAlbumUpdated	            AlbumUpdated;
				SignalTrackUpdated              TrackUpdated;

                SignalAlbumDeleted              AlbumDeleted;
                SignalTrackDeleted              TrackDeleted;
                SignalArtistDeleted             ArtistDeleted;
                SignalAlbumArtistDeleted        AlbumArtistDeleted;
            };

            SignalsT Signals;

            SignalNewAlbum&
            signal_new_album()
            { return Signals.NewAlbum ; }
            
            SignalAlbumDeleted&
            signal_album_deleted()
            { return Signals.AlbumDeleted ; }

            SignalNewArtist&
            signal_new_artist()
            { return Signals.NewArtist ; }
            
            SignalNewTrack&
            signal_new_track()
            { return Signals.NewTrack ; }

            SignalTrackDeleted&
            signal_track_deleted()
            { return Signals.TrackDeleted ; }

            SignalTrackUpdated&
            signal_track_updated()
            { return Signals.TrackUpdated ; }

            SignalAlbumUpdated&
            signal_album_updated()
            { return Signals.AlbumUpdated ; }

            SignalArtistDeleted&
            signal_artist_deleted()
            { return Signals.ArtistDeleted ; }

            SignalAlbumArtistDeleted&
            signal_album_artist_deleted()
            { return Signals.AlbumArtistDeleted ; }

        public:

            enum Flags
            {
                  F_NONE            = 0
                , F_USING_HAL       = 1 << 0,
            };

        private:

            SQL::SQLDB                                          * m_SQL ;
            IHAL                                                * m_HAL ;
            boost::shared_ptr<LibraryScannerThread_MLibMan>       m_ScannerThread ;
            gint64                                                m_Flags ;

        protected:

            void
            on_new_album(
                  gint64
                , const std::string&
                , const std::string&
                , const std::string&
                , const std::string&
                , const std::string&
            ) ;

            void
            on_new_artist(
                  gint64
            ) ;

            void
            on_new_track(
                  gint64
            ) ;

            void
            on_entity_deleted(
                  gint64
                , EntityType
            ) ;

            void
            on_entity_updated(
                  gint64
                , EntityType
            ) ;

            void
            on_message(
                const std::string&
            );
        
            void
            remove_dangling() ;

            void
            on_priority_settings_changed(
                MCS_CB_DEFAULT_SIGNATURE
            );

            void
            library_scanner_thread_set_priorities(
            );
    };
} // namespace MPX

#endif // !MPX_LIBRARY_CLASS_HH
