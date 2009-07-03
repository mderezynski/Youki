
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
, public ::MPX::Service::Base
{
public:

    MLibMan_proxy_actual(
        DBus::Connection conn
    )
    : DBus::ObjectProxy( conn, "/info/backtrace/Youki/MLibMan", "info.backtrace.Youki.MLibMan" )
    , ::MPX::Service::Base("mpx-service-mlibman")
    {
    }

public:

    typedef sigc::signal<void>                                                                          Signal_t ;
    typedef sigc::signal<void, gint64, std::string, std::string, std::string, std::string, std::string> Signal_1int64_5string_t ;
    typedef sigc::signal<void, const std::vector<int64_t>&>                                             Signal_1int64v ;
    typedef sigc::signal<void, int64_t>                                                                 Signal_1int64 ;

protected:

    Signal_t                SignalScanStart ;
    Signal_t                SignalScanEnd ;
    Signal_1int64_5string_t SignalNewAlbum ;
    Signal_1int64           SignalNewArtist ;
    Signal_1int64           SignalNewTrack ;
    Signal_1int64           SignalArtistDeleted ;
    Signal_1int64           SignalAlbumDeleted ;

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

    Signal_1int64
    signal_new_artist ()
    {
        return SignalNewArtist ;
    }

    Signal_1int64
    signal_artist_deleted ()
    {
        return SignalArtistDeleted ;
    }

    Signal_1int64
    signal_album_deleted ()
    {
        return SignalAlbumDeleted ;
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

    virtual
    void NewArtist(
          const int64_t&        id
    ) 
    {
        SignalNewArtist.emit( id ) ;
    }

    virtual
    void NewTrack(
          const int64_t&        id
    ) 
    {
        SignalNewTrack.emit( id ) ;
    }

    virtual
    void ArtistDeleted(
          const int64_t&        id
    ) 
    {
        SignalArtistDeleted.emit( id ) ;
    }

    virtual
    void AlbumDeleted(
          const int64_t&        id
    ) 
    {
        SignalAlbumDeleted.emit( id ) ;
    }

};

} } } 
#endif
