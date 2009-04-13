
/*
 *	This file was automatically generated by dbusxx-xml2cpp; DO NOT EDIT!
 */

#ifndef YOUKI_MLIBMAN_DBUS_ACTUAL_HH 
#define YOUKI_MLIBMAN_DBUS_ACTUAL_HH 

#include <dbus-c++/dbus.h>
#include "mpx/mpx-services.hh"
#include "mpx-mlibman-dbus-proxy.hh"

namespace info {
namespace backtrace {
namespace Youki {

class MLibMan_proxy_actual
: public info::backtrace::Youki::MLibMan_proxy 
, public DBus::ObjectProxy
, public Service::Base
{
public:

    MLibMan_proxy_actual(
        DBus::Connection conn
    )
    : DBus::ObjectProxy( conn, "/info/backtrace/Youki/MLibMan", "info.backtrace.Youki.MLibMan" )
    , Service::Base("mpx-service-mlibman")
    {
    }

public:

    typedef sigc::signal<void>                                                                          Signal_t ;
    typedef sigc::signal<void, gint64, std::string, std::string, std::string, std::string, std::string> Signal_1int64_5string_t ;

protected:

    Signal_t                SignalScanStart ;
    Signal_t                SignalScanEnd ;
    Signal_1int64_5string_t SignalNewAlbum ;

public:

    Signal_t&
    signal_scan_start ()
    {
        return SignalScanStart ;
    }

    Signal_t&
    signal_scan_end ()
    {
        return SignalScanEnd ;
    }

    Signal_1int64_5string_t&
    signal_new_album ()
    {
        return SignalNewAlbum ;
    }

    /* signal handlers for this interface
     */
    virtual
    void ScanStart(
    )
    {
        SignalScanStart.emit() ;
    }

    virtual
    void ScanEnd(
    ) 
    {
        SignalScanEnd.emit() ;
    }

    virtual
    void NewAlbum(
          const int64_t&        id
        , const std::string&    s1
        , const std::string&    s2
        , const std::string&    s3
        , const std::string&    s4
        , const std::string&    s5
    ) 
    {
        SignalNewAlbum.emit( id, s1, s2, s3, s4, s5 ) ;
    }
};

} } } 
#endif
