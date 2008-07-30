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
#include <glibmm.h>
#include <sigx/sigx.h>
#include <sigx/signal_f.h>
#include <sigx/request_f.h>
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"
#include "mpx/util-file.hh"

namespace MPX
{
   enum ScanResult {
        SCAN_RESULT_OK,
        SCAN_RESULT_ERROR,
        SCAN_RESULT_UPDATE,
        SCAN_RESULT_UPTODATE
    };
    
    class HAL;
    class MetadataReaderTagLib;
	class LibraryScannerThread : public sigx::glib_threadable
	{
        public:

            typedef sigc::signal<void>                                     SignalScanStart_t ;
            typedef sigc::signal<void, gint64,gint64>                      SignalScanRun_t ;
            typedef sigc::signal<void, gint64,gint64,gint64,gint64,gint64> SignalScanEnd_t ;
            typedef sigc::signal<void>                                     SignalReload_t ;
            typedef sigc::signal<void, gint64>                             SignalNewAlbum_t ;
            typedef sigc::signal<void, gint64>                             SignalNewArtist_t ;

            typedef sigc::signal<void,
                std::string const&,
                std::string const&,
                std::string const&,
                std::string const&,
                std::string const&>                                         SignalCacheCover_t ;

            typedef sigc::signal<void, Track&, gint64, gint64>              SignalTrack_t ;
        

            
            typedef sigx::signal_f<SignalScanStart_t>           signal_scan_start_x ;
            typedef sigx::signal_f<SignalScanRun_t>             signal_scan_run_x ; 
            typedef sigx::signal_f<SignalScanEnd_t>             signal_scan_end_x ;
            typedef sigx::signal_f<SignalReload_t>              signal_reload_x ;
            typedef sigx::signal_f<SignalNewAlbum_t>            signal_new_album_x ;
            typedef sigx::signal_f<SignalNewArtist_t>           signal_new_artist_x ;
            typedef sigx::signal_f<SignalCacheCover_t>          signal_cache_cover_x ;
            typedef sigx::signal_f<SignalTrack_t>               signal_track_x ;


            struct ScannerConnectable
            {
                ScannerConnectable(signal_scan_start_x & start_x,
                                   signal_scan_run_x & run_x,
                                   signal_scan_end_x & end_x,
                                   signal_reload_x & reload_x,
                                   signal_new_album_x & album_x,
                                   signal_new_artist_x & artist_x,
                                   signal_cache_cover_x & cover_x,
                                   signal_track_x & track_x) :
                signal_scan_start(start_x),
                signal_scan_run(run_x),
                signal_scan_end(end_x),
                signal_reload(reload_x),
                signal_new_album(album_x),
                signal_new_artist(artist_x),
                signal_cache_cover(cover_x),
                signal_track(track_x)
                {
                }

                signal_scan_start_x         & signal_scan_start ;
                signal_scan_run_x           & signal_scan_run ; 
                signal_scan_end_x           & signal_scan_end ;
                signal_reload_x             & signal_reload ;
                signal_new_album_x          & signal_new_album ;
                signal_new_artist_x         & signal_new_artist ;
                signal_cache_cover_x        & signal_cache_cover ;
                signal_track_x              & signal_track ;
            };


        public:

            sigx::request_f<Util::FileList const&> scan ;
            sigx::request_f<> scan_stop ;

            signal_scan_start_x         signal_scan_start ;
            signal_scan_run_x           signal_scan_run ; 
            signal_scan_end_x           signal_scan_end ;
            signal_reload_x             signal_reload ;
            signal_new_album_x          signal_new_album ;
            signal_new_artist_x         signal_new_artist ;
            signal_cache_cover_x        signal_cache_cover ;
            signal_track_x              signal_track ;

        public:	

            LibraryScannerThread (MPX::SQL::SQLDB*,MPX::MetadataReaderTagLib&,MPX::HAL&,gint64) ;
            ~LibraryScannerThread () ;

            ScannerConnectable&
            connect ();

        protected:

            virtual void on_startup () ; 
            virtual void on_cleanup () ;

            void on_scan (Util::FileList const&) ;
            void on_scan_stop () ;

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

            ScanResult
            insert (Track&, const std::string& uri, const std::string& insert_path);

        private:

            struct ThreadData;

            MPX::SQL::SQLDB             * m_SQL ;
            MetadataReaderTagLib        & m_MetadataReaderTagLib;
            MPX::HAL                    & m_HAL ;
            ScannerConnectable          * m_Connectable ; 
            Glib::Private<ThreadData>     m_ThreadData ;
            gint64                        m_Flags;
            
	};
}

#endif
