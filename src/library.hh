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

#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-services.hh"
#include "mpx/util-file.hh"
#include "mpx/util-string.hh"

#ifdef HAVE_HAL
#include "mpx/i-youki-hal.hh"
#endif // HAVE_HAL

#include "mpx/i-youki-library.hh"

namespace MPX
{
    struct CollectionMeta
    {
        std::string Name;
        std::string Blurb;
        std::string Cover_URL;
        gint64      Id;
    };

    class MetadataReaderTagLib;
    class Library
    : public ILibrary
    , public Service::Base
    {
        public:

#include "mpx/exception.hh"

            EXCEPTION(FileQualificationError)

            Library(
            ) ;

            virtual ~Library () ;

            void
            switch_mode(
                  bool
            ) ;

            gint64
            get_flags(
            )
            {
                return m_Flags;
            }

            SQL::SQLDB*
            get_sql_db(
            )
            {
                return m_SQL;
            }

            void
            reload(
            ) ;

            void
            getSQL(
                SQL::RowV&,
                const std::string&
            ) const ;

			int64_t
			execSQL(
                const std::string&
            ) ;

            Track_sp 
            sqlToTrack(
                  SQL::Row&
                , bool /*all metadata?*/ = true
                , bool /*no_location?*/  = false
            ) ;

            Track_sp
            getTrackById(
                  gint64
            ) ;

            void
            recacheCovers() ;

            void
            getMetadata(
                  const std::string&
                , Track&
            ) ;
            void
            albumAddNewRating(
                  gint64
                , int
                , const std::string&
            ) ;
            void
            albumGetAllRatings(
                  gint64
                , SQL::RowV& v
            ) ;
            int
            albumGetMeanRatingValue(
                  gint64
            ) ;
            void
            albumDeleteRating(
                  gint64
                , gint64
            ) ;
            void
            albumTagged(
                  gint64
                , const std::string&
            ) ;

			void
			trackRated(
                  gint64
                , int
            ) ;
			void
			trackPlayed(
                  gint64
                , gint64
                , time_t
            ) ;
            void
            trackTagged(
                  gint64
                , const std::string&
            ) ;

            enum LovedHatedStatus
            {
                  TRACK_LOVED
                , TRACK_HATED
                , TRACK_INDIFFERENT
            } ;

            void
            trackLovedHated(
                  gint64
                , LovedHatedStatus 
            ) ;

            void
            trackSetLocation( Track_sp&, const std::string& );

            std::string
            trackGetLocation( const Track_sp& );

            SQL::RowV
            getTrackTags(
                gint64
            ) ;

            LovedHatedStatus
            getTrackLovedHated(
                gint64
            ) ;

            void
            markovUpdate(gint64 /* track a */, gint64 /* track b */) ;

            gint64 
            markovGetRandomProbableTrack(gint64 /* track a*/); 

            gint64
            collectionCreate(const std::string& /*name*/, const std::string& /*blurb*/) ;
    
            void
            collectionDelete(gint64 /*id*/) ;

            void
            collectionAppend(gint64 /*id*/, const IdV&) ;

            void
            collectionErase(gint64 /*id*/, const IdV&) ;

            void
            collectionGetMeta(gint64 /*id*/, CollectionMeta& /*collection_meta*/) ;

            void
            collectionGetAll(IdV& /*collections*/) ;

            void
            collectionGetTracks(gint64 id, IdV& /*collections*/) ;

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
                Track&,
                gint64 /*album_id*/,
                gint64 /*artist id*/>                           SignalTrackUpdated;

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

/*
            struct CollectionSignalsT
            {
                SignalNewCollection             New; 
                SignalCollectionDeleted         Deleted;
                SignalNewCollectionTrack        NewTrack;
                SignalCollectionTrackDeleted    TrackDeleted;
            };
*/

            struct SignalsT
            {
                SignalAlbumUpdated          AlbumUpdated ;
            };

            SignalsT Signals ;

            SignalAlbumUpdated&
            signal_album_updated()
            { return Signals.AlbumUpdated ; }
            
        public:

            enum Flags
            {
                  F_NONE            = 0
                , F_USING_HAL       = 1 << 0
            };

        private:

            SQL::SQLDB  * m_SQL ;

#ifdef HAVE_HAL
            IHAL        * m_HAL ;
#endif // HAVE_HAL

            gint64        m_Flags ;

        protected:

            bool
            recache_covers_handler(
                  SQL::RowV*
                , std::size_t*
            ) ; 

            gint64
            get_tag_id(
                std::string const&
            ) ;
    };
} // namespace MPX

#endif // !MPX_LIBRARY_CLASS_HH
