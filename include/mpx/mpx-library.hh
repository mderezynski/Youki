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
#ifndef MPX_LIBRARY_CLASS_HH
#define MPX_LIBRARY_CLASS_HH

#include <sigx/sigx.h>

#include "mpx/mpx-covers.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-file.hh"
#include "mpx/util-string.hh"
#include "mpx/mpx-library-scanner-thread.hh"
#include "mpx/mpx-services.hh"

namespace MPX
{
    struct Collection
    {
        gint64          Id;
        std::string     Name;
        std::string     Blurb;
        std::string     Cover_URL;
    };

    typedef std::vector<Collection> CollectionV;

    class HAL;
    class MetadataReaderTagLib;
    class Library : public sigx::glib_auto_dispatchable, public Service::Base
    {
        friend class LibraryScannerThread;

        public:

            Library(
                Service::Manager&
#ifdef HAVE_HAL
                , bool
#endif //HAVE_HAL
            );

            Library (const Library& other);
            ~Library () ;

            SQL::SQLDB*
            get_sql_db ()
            {
                return m_SQL;
            }

            LibraryScannerThread::ScannerConnectable&
            scanner()
            {
                return m_ScannerThread->connect();
            }

            void
            vacuum();

#ifdef HAVE_HAL    
			void
			vacuum_volume(const std::string&, const std::string&);
#endif

            void
            reload();


            void
            initScan (const Util::FileList& list, bool deep = false); 


            void
            getSQL(SQL::RowV&, const std::string&) ;

			void
			execSQL(const std::string&);


            void
            recacheCovers();


            Track
            sqlToTrack (SQL::Row&);

            SQL::RowV
            getTrackTags (gint64);


            void
            getMetadata(const std::string&, Track&) ;


            void
            albumAddNewRating(gint64 id, int rating, std::string const& comment);

            void
            albumGetAllRatings(gint64 id, SQL::RowV & v);

            int
            albumGetMeanRatingValue(gint64 id);

            void
            albumDeleteRating(gint64 rating_id, gint64 album_idd);

            void
            albumTagged(gint64, std::string const&);


			void
			trackRated(gint64, int);
		
			void
			trackPlayed(gint64, gint64, time_t);

            void
            trackTagged(gint64, std::string const&);


            void
            markovUpdate(gint64 /* track a */, gint64 /* track b */);

            gint64 
            markovGetRandomProbableTrack(gint64 /* track a*/); 


            gint64
            collectionCreate(const std::string& /*name*/, const std::string& /*blurb*/);
    
            void
            collectionDelete(gint64 /*id*/);

            void
            collectionAppend(gint64 /*id*/, const IdV&);

            void
            collectionErase(gint64 /*id*/, const IdV&);

            void
            collectionGetMeta(gint64 /*id*/, Collection& /*collection*/);

            void
            collectionGetAll(IdV& /*collections*/);

            void
            collectionGetTracks(gint64 id, IdV& /*collections*/);


        public:

            typedef sigc::signal<void,
                gint64 /*album id*/>                            SignalNewAlbum; 

            typedef sigc::signal<void,
                gint64 /*artist id*/>                           SignalNewArtist;

            typedef sigc::signal<void,
                Track&,
                gint64 /*albumid*/,
                gint64/*artistid*/>                             SignalNewTrack;

            typedef sigc::signal<void,
                gint64 /*collection id*/>                       SignalNewCollection; 

            typedef sigc::signal<void,
                gint64 /*collection id*/,
                gint64 /*track id*/>                            SignalNewCollectionTrack;



            typedef sigc::signal<void,
                gint64 /*collection id*/>                       SignalCollectionDeleted; 

            typedef sigc::signal<void,
                gint64 /*album id*/>                            SignalAlbumDeleted; 

            typedef sigc::signal<void,
                gint64/*artistid*/>                             SignalAlbumArtistDeleted;

            typedef sigc::signal<void,
                gint64/*artistid*/>                             SignalArtistDeleted;

            typedef sigc::signal<void,
                gint64 /*collection id*/,
                gint64 /*track id*/>                            SignalCollectionTrackDeleted;

            typedef sigc::signal<void,
                gint64/*artistid*/>                             SignalTrackDeleted;


			typedef sigc::signal<void,
                gint64 /*id*/>                                  SignalTrackUpdated;

            typedef sigc::signal<void,
                gint64 /*album id*/>                            SignalAlbumUpdated;


			typedef sigc::signal<void,
                gint64 /*id*/,
                gint64 /*tagid*/>                               SignalTrackTagged;



            typedef sigc::signal<void>                          SignalScanStart;

            typedef sigc::signal<void,
                gint64,
                gint64>                                         SignalScanRun;

            typedef sigc::signal<void,
                gint64,
                gint64,
                gint64,
                gint64,
                gint64>                                         SignalScanEnd;

            typedef sigc::signal<void>                          SignalReload;

            struct CollectionSignalsT
            {
                SignalNewCollection             New; 
                SignalCollectionDeleted         Deleted;
                SignalNewCollectionTrack        NewTrack;
                SignalCollectionTrackDeleted    TrackDeleted;
            };

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

                SignalTrackTagged               TrackTagged;

                SignalScanStart                 ScanStart;  
                SignalScanRun                   ScanRun;
                SignalScanEnd                   ScanEnd;

				SignalReload                    Reload;

                CollectionSignalsT              Collection;
            };

            SignalsT Signals;

            SignalNewCollection&
            signal_collection_new()
            { return Signals.Collection.New ; }
            
            SignalCollectionDeleted&
            signal_collection_deleted()
            { return Signals.Collection.Deleted ; }

            SignalNewCollectionTrack&
            signal_collection_new_track()
            { return Signals.Collection.NewTrack ; }
            
            SignalCollectionTrackDeleted&
            signal_collection_track_deleted()
            { return Signals.Collection.TrackDeleted ; }

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

            SignalArtistDeleted&
            signal_artist_deleted()
            { return Signals.ArtistDeleted ; }

            SignalAlbumArtistDeleted&
            signal_album_artist_deleted()
            { return Signals.AlbumArtistDeleted ; }

        private:

            enum Flags
            {
                F_NONE      =       0,
                F_USING_HAL =       1 << 0,
            };

#ifdef HAVE_HAL
            HAL                     & m_HAL;
#endif //HAVE_HAL 
            Service::Manager        & m_Services;
            Covers                  & m_Covers;
            MetadataReaderTagLib    & m_MetadataReaderTagLib ;
            SQL::SQLDB              * m_SQL;
            LibraryScannerThread    * m_ScannerThread;

            gint64                    m_Flags;

        protected:

            gint64
            get_tag_id (std::string const&);

			void
			set_mean_genre_for_album (gint64 id);

            void
            on_new_album (gint64);

            void
            on_new_artist (gint64);

            void
            on_new_track (Track&,gint64,gint64);

            bool
            recache_covers_handler (SQL::RowV *, int*); 

            void
            remove_dangling();
    };
} // namespace MPX

#endif // !MPX_LIBRARY_CLASS_HH
