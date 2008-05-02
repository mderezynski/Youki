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

#include <sigx/sigx.h>

#include "mpx/covers.hh"
#include "mpx/sql.hh"
#include "mpx/tasks.hh"
#include "mpx/types.hh"
#include "mpx/util-file.hh"
#include "mpx/util-string.hh"
#include "mpx/library-scanner-thread.hh"

namespace MPX
{
  class HAL;
  class MetadataReaderTagLib;
  class Library : public sigx::glib_auto_dispatchable
  {
        friend class LibraryScannerThread;

        public:

            void
            initScan (const StrV&, const std::string& = std::string());

        private:

#ifdef HAVE_HAL
            HAL                     * m_HAL;
#endif //HAVE_HAL 
            Covers                  * m_Covers;
            LibraryScannerThread    * m_ScannerThread;

        public:
#ifdef HAVE_HAL
            Library (Covers&, HAL&, bool use_hal) ;
#else //!HAVE_HAL
            Library (Covers&) ;
#endif //HAVE_HAL
            Library (const Library& other);
            ~Library () ;

            LibraryScannerThread::ScannerConnectable&
            scanner()
            {
                return m_ScannerThread->connect();
            }

            void
            getSQL(SQL::RowV&, const std::string&) ;

			void
			execSQL(const std::string&);

			void
			vacuum();

            void
            reload ();

            Track
            sqlToTrack (SQL::Row&);

            SQL::RowV
            getTrackTags (gint64);

            void
            getMetadata(const std::string&, Track&) ;

			void
			albumRated(gint64, int);
		
			void
			trackRated(gint64, int);
		
			void
			trackPlayed(gint64, time_t);

            void
            trackTagged(gint64, std::string const&);

        public:

            typedef sigc::signal<void, gint64 /*album id*/> SignalNewAlbum; 
            typedef sigc::signal<void, gint64 /*album id*/> SignalAlbumUpdated;
            typedef sigc::signal<void, gint64 /*artist id*/> SignalNewArtist;
            typedef sigc::signal<void, Track&, gint64/*albumid*/> SignalNewTrack;
			typedef sigc::signal<void, gint64 /*id*/> SignalTrackUpdated;
			typedef sigc::signal<void, gint64 /*id*/, gint64 /* tag id*/> SignalTrackTagged;
            typedef sigc::signal<void> SignalScanStart;
            typedef sigc::signal<void, gint64,gint64> SignalScanRun;
            typedef sigc::signal<void, gint64,gint64,gint64,gint64,gint64> SignalScanEnd;
            typedef sigc::signal<void> SignalReload;

            struct SignalsT
            {
                SignalNewAlbum      NewAlbum;
                SignalNewArtist     NewArtist;
                SignalNewTrack      NewTrack;
				SignalTrackUpdated	TrackUpdated;
                SignalTrackTagged   TrackTagged;
                SignalScanStart     ScanStart;  
                SignalScanRun       ScanRun;
                SignalScanEnd       ScanEnd;
				SignalAlbumUpdated	AlbumUpdated;
				SignalReload        Reload;
            };

            SignalsT Signals;

            SignalNewAlbum&
            signal_new_album()
            { return Signals.NewAlbum ; }
            
            SignalNewArtist&
            signal_new_artist()
            { return Signals.NewArtist ; }
            
            SignalNewTrack&
            signal_new_track()
            { return Signals.NewTrack ; }

            SignalTrackUpdated&
            signal_track_updated()
            { return Signals.TrackUpdated ; }

            SignalTrackTagged&
            signal_track_tagged()
            { return Signals.TrackTagged ; }

            SignalScanStart&
            signal_scan_start()
            { return Signals.ScanStart ; }
            
            SignalScanRun&
            signal_scan_run()
            { return Signals.ScanRun ; }
            
            SignalScanEnd&
            signal_scan_end()
            { return Signals.ScanEnd ; }

            SignalAlbumUpdated&
            signal_album_updated()
            { return Signals.AlbumUpdated ; }

            SignalReload&
            signal_reload()
            { return Signals.Reload ; }

        private:

            enum Flags
            {
                F_USING_HAL = (1<<0),
            };

            SQL::SQLDB              * m_SQL;
            MetadataReaderTagLib    * m_MetadataReaderTagLib ;
            gint64                    m_Flags;

        protected:

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

            gint64
            get_tag_id (std::string const&);

            void
            insert (Track&, const std::string& uri, const std::string& insert_path, ScanResult&);

			void
			set_mean_genre_for_album (gint64 id);

            void
            on_new_album (gint64);

            void
            on_new_artist (gint64);
    };

} // namespace MPX

#endif // !MPX_LIBRARY_CLASS_HH
