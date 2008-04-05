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

#include "mpx/amazon.hh"
#include "mpx/sql.hh"
#include "mpx/tasks.hh"
#include "mpx/types.hh"
#include "mpx/util-file.hh"

namespace MPX
{
  class HAL;
  class MetadataReaderTagLib;
  class Library
  {
            enum ScanResult
            {
                SCAN_RESULT_OK,
                SCAN_RESULT_ERROR,
                SCAN_RESULT_UPDATE,
                SCAN_RESULT_UPTODATE
            };

            struct ScanData
            {
                Util::FileList collection ;
                Util::FileList::iterator position ;
                std::string insert_path ;
                std::string name ;
                gint64 added ;
                gint64 erroneous ;
                gint64 uptodate ;
                gint64 updated ;

                ScanData ()
                : added(0)
                , erroneous(0)
                , uptodate(0)
                , updated(0)
                {}
            };

            typedef boost::shared_ptr<ScanData> ScanDataP;

            void
            scanInit (ScanDataP);
            bool
            scanRun (ScanDataP);
            void
            scanEnd (bool, ScanDataP);

            TID m_ScanTID;
            
#ifdef HAVE_HAL
            HAL * m_HAL;
#endif
            TaskKernel * m_TaskKernel;
            Amazon::Covers * m_Covers;

        public:
#ifdef HAVE_HAL
            Library (Amazon::Covers&, HAL&, TaskKernel&, bool use_hal) ;
#else
            Library (Amazon::Covers&, TaskKernel&) ;
#endif
            ~Library () ;



            void
            getMetadata(const std::string& uri, Track & track) ;

            void
            getSQL(SQL::RowV & rows, const std::string& sql) ;

			void
			execSQL(const std::string& sql);

            void
            scanUri(const std::string& uri, const std::string& name = std::string());

			void
			vacuum();


			void
			albumRated(gint64 id, int rating);

		
			void
			trackRated(gint64 id, int rating);
		
			void
			trackPlayed(gint64 id, time_t time_);





            typedef sigc::signal<void, const std::string& /*mbid*/,
				const std::string& /*asin*/, gint64/*albumid*/, gint64 /*album artist id*/> SignalNewAlbum;
            typedef sigc::signal<void, gint64/*albumid*/> SignalAlbumUpdated;
            typedef sigc::signal<void, const std::string& /*mbid*/, gint64/*artistid*/> SignalNewArtist;
            typedef sigc::signal<void, Track&, gint64/*albumid*/> SignalNewTrack;
			typedef sigc::signal<void, gint64 /*id*/> SignalTrackUpdated;

            typedef sigc::signal<void> SignalScanStart;
            typedef sigc::signal<void, gint64,gint64> SignalScanRun;
            typedef sigc::signal<void, gint64,gint64,gint64,gint64,gint64> SignalScanEnd;
            typedef sigc::signal<void> SignalVacuumized;

            struct SignalsT
            {
                SignalNewAlbum      NewAlbum;
                SignalNewArtist     NewArtist;
                SignalNewTrack      NewTrack;
				SignalTrackUpdated	TrackUpdated;
                SignalScanStart     ScanStart;  
                SignalScanRun       ScanRun;
                SignalScanEnd       ScanEnd;
				SignalAlbumUpdated	AlbumUpdated;
				SignalVacuumized	Vacuumized;
            };

            SignalsT Signals;

            SignalVacuumized&
            signal_vacuumized()
            { return Signals.Vacuumized ; }

            SignalAlbumUpdated&
            signal_album_updated()
            { return Signals.AlbumUpdated ; }

            SignalNewAlbum&
            signal_new_album()
            { return Signals.NewAlbum ; }
            
            SignalNewArtist&
            signal_new_artist()
            { return Signals.NewArtist ; }
            
            SignalTrackUpdated&
            signal_track_updated()
            { return Signals.TrackUpdated ; }

            SignalNewTrack&
            signal_new_track()
            { return Signals.NewTrack ; }

            SignalScanStart&
            signal_scan_start()
            { return Signals.ScanStart ; }
            
            SignalScanRun&
            signal_scan_run()
            { return Signals.ScanRun ; }
            
            SignalScanEnd&
            signal_scan_end()
            { return Signals.ScanEnd ; }

        private:

            SQL::SQLDB * m_SQL;
            MetadataReaderTagLib * mReaderTagLib ; // TODO: Modularize out 

            enum Flags
            {
                F_USING_HAL     = (1<<0),
            };

            gint64 m_Flags;

            const std::string column_names;

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

			void
			mean_genre_for_album (gint64 id);

            ScanResult
            insert (const std::string& uri, const std::string& insert_path, const std::string& name = std::string());
    };

} // namespace MPX

#endif // !MPX_LIBRARY_CLASS_HH
