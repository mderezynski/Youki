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
#include "mpx/util-string.hh"
#include "mpx/util-file.hh"

namespace MPX
{
    struct ScanData
    {
        ScanData ();

        StrV           URIV;
        StrV::iterator Iter;

        std::string insert_path ;
        std::string insert_path_sql ;
    
        Util::FileList collection ;
        Util::FileList::iterator position ;

        std::string name ;

        gint64 added ;
        gint64 erroneous ;
        gint64 uptodate ;
        gint64 updated ;
    };

    enum ScanResult {
        SCAN_RESULT_OK,
        SCAN_RESULT_ERROR,
        SCAN_RESULT_UPDATE,
        SCAN_RESULT_UPTODATE
    };

    class Library;
	class LibraryScannerThread : public sigx::glib_threadable
	{
        public:

            typedef sigc::slot<void>                                       SlotScanStart_t ;
            typedef sigc::slot<void, gint64,gint64>                        SlotScanRun_t ;
            typedef sigc::slot<void, gint64,gint64,gint64,gint64,gint64>   SlotScanEnd_t ;
            typedef sigc::slot<void>                                       SlotReload_t ;

            typedef sigc::signal<void>                                     SignalScanStart_t ;
            typedef sigc::signal<void, gint64,gint64>                      SignalScanRun_t ;
            typedef sigc::signal<void, gint64,gint64,gint64,gint64,gint64> SignalScanEnd_t ;
            typedef sigc::signal<void>                                     SignalReload_t ;

            
            typedef sigx::signal_f<SignalScanStart_t>   signal_scan_start_x ;
            typedef sigx::signal_f<SignalScanRun_t>     signal_scan_run_x ; 
            typedef sigx::signal_f<SignalScanEnd_t>     signal_scan_end_x ;
            typedef sigx::signal_f<SignalReload_t>      signal_reload_x ;


            struct ScannerConnectable
            {
                ScannerConnectable(signal_scan_start_x & start_x,
                                   signal_scan_run_x & run_x,
                                   signal_scan_end_x & end_x,
                                   signal_reload_x & reload_x) :
                signal_scan_start(start_x),
                signal_scan_run(run_x),
                signal_scan_end(end_x),
                signal_reload(reload_x)
                {
                }

                signal_scan_start_x signal_scan_start ;
                signal_scan_run_x   signal_scan_run ; 
                signal_scan_end_x   signal_scan_end ;
                signal_reload_x     signal_reload ;
            };


        public:

            sigx::request_f<ScanData const&> scan ;
            sigx::request_f<> scan_stop ;

            signal_scan_start_x signal_scan_start ;
            signal_scan_run_x   signal_scan_run ; 
            signal_scan_end_x   signal_scan_end ;
            signal_reload_x     signal_reload ;

        public:	

            LibraryScannerThread (MPX::Library*) ;
            ~LibraryScannerThread () ;

            ScannerConnectable&
            connect ();

        protected:

            virtual void on_startup () ; 
            virtual void on_cleanup () ;

            void on_scan (ScanData const&) ;
            void on_scan_stop () ;

        private:

            MPX::Library            * m_Library ;
            struct ThreadData;
            Glib::Private<ThreadData> m_ThreadData ;
            ScannerConnectable      * m_Connectable ; 
            
	};
}

#endif
