#include "config.h"

#include <queue>
#include <boost/ref.hpp>
#include <boost/format.hpp>
#include <giomm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>

#ifdef HAVE_TR1
#include <tr1/unordered_set>
#else
#include <set>
#endif

#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-uri.hh"

#include "mpx/util-file.hh"

#include "mpx/metadatareader-taglib.hh"

#include "mpx/xml/xmltoc++.hh"
#include "xmlcpp/mb-artist-basic-1.0.hxx"

#include "library-scanner-thread-mlibman.hh"
#include "library-mlibman.hh"

#ifdef HAVE_HAL
#include "mpx/i-youki-hal.hh"
#endif // HAVE_HAL

using boost::get;
using namespace MPX;
using namespace MPX::SQL;
using namespace Glib;

namespace
{
    struct AttrInfo
    {
        char const* id;
        ValueType   type;
    };

    AttrInfo attrs[] =
    {
            {   "location",
                    VALUE_TYPE_STRING   }, 

            {   "title",
                    VALUE_TYPE_STRING   }, 

            {   "genre",
                    VALUE_TYPE_STRING   }, 

            {   "comment",
                    VALUE_TYPE_STRING   }, 

            {   "musicip_puid",
                    VALUE_TYPE_STRING   }, 

            {   "hash",
                    VALUE_TYPE_STRING   }, 

            {   "mb_track_id",
                    VALUE_TYPE_STRING   }, 

            {   "artist",
                    VALUE_TYPE_STRING   }, 

            {   "artist_sortname",
                    VALUE_TYPE_STRING   }, 

            {   "mb_artist_id",
                    VALUE_TYPE_STRING   }, 

            {   "album",
                    VALUE_TYPE_STRING   }, 

            {   "mb_album_id",
                    VALUE_TYPE_STRING   }, 

            {   "mb_release_date",
                    VALUE_TYPE_STRING   }, 

            {   "mb_release_country",
                    VALUE_TYPE_STRING   }, 

            {   "mb_release_type",
                    VALUE_TYPE_STRING   }, 

            {   "amazon_asin",
                    VALUE_TYPE_STRING   }, 

            {   "album_artist",
                    VALUE_TYPE_STRING   }, 

            {   "album_artist_sortname",
                    VALUE_TYPE_STRING   }, 

            {   "mb_album_artist_id",
                    VALUE_TYPE_STRING   }, 

            {   "type",
                    VALUE_TYPE_STRING   }, 

            {   "hal_volume_udi",
                    VALUE_TYPE_STRING   }, 

            {   "hal_device_udi",
                    VALUE_TYPE_STRING   }, 

            {   "hal_vrp",
                    VALUE_TYPE_STRING   }, 

            {   "insert_path",
                    VALUE_TYPE_STRING   }, 

            {   "location_name",
                    VALUE_TYPE_STRING   }, 

            {   "track",
                    VALUE_TYPE_INT      },

            {   "time",
                    VALUE_TYPE_INT      },

            {   "rating",
                    VALUE_TYPE_INT      },

            {   "date",
                    VALUE_TYPE_INT      },

            {   "mtime",
                    VALUE_TYPE_INT      },

            {   "bitrate",
                    VALUE_TYPE_INT      },

            {   "samplerate",
                    VALUE_TYPE_INT      },

            {   "pcount",
                    VALUE_TYPE_INT      },

            {   "pdate",
                    VALUE_TYPE_INT      },

            {   "insert_date",
                    VALUE_TYPE_INT      },

            {   "is_mb_album_artist",
                    VALUE_TYPE_INT      },

            {   "active",
                    VALUE_TYPE_INT      },

            {   "audio_quality",
                    VALUE_TYPE_INT      },

            {   "device_id",
                    VALUE_TYPE_INT      },
    };

    const char delete_track_f[] = "DELETE FROM track WHERE id='%lld';";

    enum Quality
    {
          QUALITY_ABYSMAL
        , QUALITY_LOW
        , QUALITY_MED
        , QUALITY_HI
    };

    struct BitrateLookupTable
    {
        const char*     Type;
        int             Range0;
        int             Range1;
        int             Range2;
    };

    BitrateLookupTable table[]=
    {
          {"audio/x-flac",          320, 640, 896 }
        , {"audio/x-ape",           256, 512, 768 }
        , {"audio/x-vorbis+ogg",     32,  96, 192 }
        , {"audio/x-musepack",      128, 192, 256 }
        , {"audio/mp4",              24,  64, 128 }
        , {"audio/mpeg",             96, 192, 256 }
        , {"audio/x-ms-wma",         48,  96, 196 }
    };

    gint64
    get_audio_quality(
          const std::string& type
        , gint64             rate
    )
    {
        for( unsigned n = 0; n < G_N_ELEMENTS(table); ++n )
        {
            if( type == table[n].Type )
            {
                if( rate < table[n].Range0 )
                    return QUALITY_ABYSMAL;
                else if( rate < table[n].Range1 )
                    return QUALITY_LOW;
                else if( rate < table[n].Range2 )
                    return QUALITY_MED;
                else
                    return QUALITY_HI;
            }
        }

        return 0;
    }

    template <typename T>
    void
    append_value(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    { 
        throw;
    }

    template <>
    void
    append_value<std::string>(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    {
        sql += mprintf( "'%q'", get<std::string>(track[attr].get()).c_str()) ;
    }

    template <>
    void
    append_value<gint64>(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    {
        sql += mprintf( "'%lld'", get<gint64>(track[attr].get()) ) ;
    }

    template <>
    void
    append_value<double>(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    {
        sql += mprintf( "'%f'", get<double>(track[attr].get())) ;
    }



    template <typename T>
    void
    append_key_value_pair(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    { 
        throw;
    }

    template <>
    void
    append_key_value_pair<std::string>(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    {
        sql += mprintf( "%s = '%q'", attrs[attr].id, get<std::string>(track[attr].get()).c_str()) ;
    }

    template <>
    void
    append_key_value_pair<gint64>(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    {
        sql += mprintf( "%s = '%lld'", attrs[attr].id, get<gint64>(track[attr].get()) ) ;
    }

    template <>
    void
    append_key_value_pair<double>(
          std::string&      sql
        , const Track&      track
        , unsigned int      attr
    )
    {
        sql += mprintf( "%s = '%f'", attrs[attr].id, get<double>(track[attr].get())) ;
    }

}

struct MPX::LibraryScannerThread_MLibMan::ThreadData
{
    LibraryScannerThread_MLibMan::SignalScanStart_t         ScanStart ;
    LibraryScannerThread_MLibMan::SignalScanEnd_t           ScanEnd ;
    LibraryScannerThread_MLibMan::SignalScanSummary_t       ScanSummary ;
    LibraryScannerThread_MLibMan::SignalNewAlbum_t          NewAlbum ;
    LibraryScannerThread_MLibMan::SignalNewArtist_t         NewArtist ;
    LibraryScannerThread_MLibMan::SignalNewTrack_t          NewTrack ;
    LibraryScannerThread_MLibMan::SignalEntityDeleted_t     EntityDeleted ;
    LibraryScannerThread_MLibMan::SignalEntityUpdated_t     EntityUpdated ;
    LibraryScannerThread_MLibMan::SignalReload_t            Reload ;
    LibraryScannerThread_MLibMan::SignalMessage_t           Message ;

    int m_Cancel ;
};

MPX::LibraryScannerThread_MLibMan::LibraryScannerThread_MLibMan(
    MPX::Library_MLibMan*       obj_library
  , gint64                      flags
)
: sigx::glib_threadable()
, add(sigc::bind(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_add), false))
, scan(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_scan))
, scan_all(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_scan_all))
, scan_stop(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_scan_stop))
, vacuum(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_vacuum))
, set_priority_data(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_set_priority_data))
#ifdef HAVE_HAL
, vacuum_volume_list(sigc::bind(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_vacuum_volume_list), true))
#endif // HAVE_HAL
, update_statistics(sigc::mem_fun(*this, &LibraryScannerThread_MLibMan::on_update_statistics))
, signal_scan_start(*this, m_ThreadData, &ThreadData::ScanStart)
, signal_scan_end(*this, m_ThreadData, &ThreadData::ScanEnd)
, signal_scan_summary(*this, m_ThreadData, &ThreadData::ScanSummary)
, signal_new_album(*this, m_ThreadData, &ThreadData::NewAlbum)
, signal_new_artist(*this, m_ThreadData, &ThreadData::NewArtist)
, signal_new_track(*this, m_ThreadData, &ThreadData::NewTrack)
, signal_entity_deleted(*this, m_ThreadData, &ThreadData::EntityDeleted)
, signal_entity_updated(*this, m_ThreadData, &ThreadData::EntityUpdated)
, signal_reload(*this, m_ThreadData, &ThreadData::Reload)
, signal_message(*this, m_ThreadData, &ThreadData::Message)
, m_Library_MLibMan(*obj_library)
, m_SQL(new SQL::SQLDB(*((m_Library_MLibMan.get_sql_db()))))
#ifdef HAVE_HAL
, m_HAL( *(services->get<IHAL>("mpx-service-hal").get()) )
#endif // HAVE_HAL
, m_Flags(flags)
{
    m_Connectable =
        new ScannerConnectable(
                  signal_scan_start
                , signal_scan_end
                , signal_scan_summary
                , signal_new_album
                , signal_new_artist
                , signal_new_track
                , signal_entity_deleted
                , signal_entity_updated
                , signal_reload
                , signal_message
    ); 
}

MPX::LibraryScannerThread_MLibMan::~LibraryScannerThread_MLibMan ()
{
    delete m_Connectable;
}

MPX::LibraryScannerThread_MLibMan::ScannerConnectable&
MPX::LibraryScannerThread_MLibMan::connect ()
{
    return *m_Connectable;
} 

void
MPX::LibraryScannerThread_MLibMan::on_startup ()
{
    m_ThreadData.set(new ThreadData);
}

void
MPX::LibraryScannerThread_MLibMan::on_cleanup ()
{
}

bool
MPX::LibraryScannerThread_MLibMan::check_abort_scan ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if( g_atomic_int_get(&pthreaddata->m_Cancel) )
    {
        m_InsertionTracks.clear();
        m_AlbumIDs.clear();
        m_AlbumArtistIDs.clear();
        m_ProcessedAlbums.clear();

        pthreaddata->ScanSummary.emit( m_ScanSummary );
        pthreaddata->ScanEnd.emit();
        return true;
    }

    return false;
}

void
MPX::LibraryScannerThread_MLibMan::on_set_priority_data(
      const std::vector<std::string>&   types
    , bool                              prioritize_by_filetype
    , bool                              prioritize_by_bitrate
)
{
    m_MIME_Types = types; 
    m_PrioritizeByFileType = prioritize_by_filetype;
    m_PrioritizeByBitrate  = prioritize_by_bitrate;
}

void
MPX::LibraryScannerThread_MLibMan::cache_mtimes(
)
{
    m_MTIME_Map.clear();

    RowV v;
    m_SQL->get(
          v
        , "SELECT device_id, hal_vrp, mtime FROM track"
    );

    for( RowV::iterator i = v.begin(); i != v.end(); ++i )
    {
        m_MTIME_Map.insert(
            std::make_pair(
                  boost::make_tuple(
                      get<gint64>((*i)["device_id"])
                    , get<std::string>((*i)["hal_vrp"])
                  )
                , get<gint64>((*i)["mtime"])
        ));
    }
}

void
MPX::LibraryScannerThread_MLibMan::on_scan(
    const Util::FileList& list
)
{
    if(list.empty())
    {
        return;
    }

    on_scan_list_quick_stage_1( list ); 
    on_add( list, true ); 
}

void
MPX::LibraryScannerThread_MLibMan::on_scan_all(
)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    g_atomic_int_set(&pthreaddata->m_Cancel, 0);

    pthreaddata->ScanStart.emit();

    m_ScanSummary = ScanSummary();

    boost::shared_ptr<Library_MLibMan> library = services->get<Library_MLibMan>("mpx-service-library");

    RowV v;

#ifdef HAVE_HAL
    try{
        if (m_Flags & Library_MLibMan::F_USING_HAL)
        { 
            m_SQL->get(
                v,
                "SELECT id, device_id, hal_vrp, mtime, insert_path FROM track"
            );
        }
        else
#endif
        {
            m_SQL->get(
                v,
                "SELECT id, location, mtime, insert_path FROM track"
            );
        }
#ifdef HAVE_HAL
    }
    catch( IHAL::Exception& cxe )
    {
        g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
        return;
    }
    catch( Glib::ConvertError& cxe )
    {
        g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
        return;
    }
#endif // HAVE_HAL

    for( RowV::iterator i = v.begin(); i != v.end(); ++i )
    {
        Track_sp t = library->sqlToTrack( *i, false ); 

        std::string location = get<std::string>( (*(t.get()))[ATTRIBUTE_LOCATION].get() ) ; 

        gint64 mtime1 = Util::get_file_mtime( get<std::string>( (*(t.get()))[ATTRIBUTE_LOCATION].get() ) );

        if( mtime1 )
        {
            gint64 mtime2 = get<gint64>((*i)["mtime"]);

            if( mtime2 && mtime1 == mtime2 )
            {
                ++m_ScanSummary.FilesUpToDate;
            }
            else
            {
                (*(t.get()))[ATTRIBUTE_MTIME] = mtime1;
                insert_file_no_mtime_check( t, location, get<std::string>((*i)["insert_path"]), mtime2 && mtime1 != mtime2); 
            }
        }
        else
        {
            gint64 id = get<gint64>((*i)["id"]);
            pthreaddata->EntityDeleted.emit( id , ENTITY_TRACK ); 

            m_SQL->exec_sql(mprintf( delete_track_f, id));
        }


        if( check_abort_scan() )
            return;

        m_ScanSummary.FilesTotal ++ ;

        if( !(m_ScanSummary.FilesTotal % 100) )
        {
                pthreaddata->Message.emit(
                    (boost::format(_("Checking Tracks: %lld"))
                        % gint64(m_ScanSummary.FilesTotal)
                    ).str()
                );
        }
    }

    process_insertion_list ();
    do_remove_dangling ();
    update_albums ();

    pthreaddata->Message.emit(_("Rescan: Done"));
    pthreaddata->ScanSummary.emit( m_ScanSummary );
    pthreaddata->ScanEnd.emit();
}

void
MPX::LibraryScannerThread_MLibMan::on_add(
      const Util::FileList& list
    , bool                  check_mtimes
)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if( check_mtimes )
    {
        cache_mtimes();
    }
    else
    {
        pthreaddata->ScanStart.emit();
    }

    g_atomic_int_set(&pthreaddata->m_Cancel, 0);

    RowV rows;
    m_SQL->get(rows, "SELECT last_scan_date FROM meta WHERE rowid = 1");
    gint64 last_scan_date = boost::get<gint64>(rows[0]["last_scan_date"]);

    m_ScanSummary = ScanSummary();

    boost::shared_ptr<Library_MLibMan> library = services->get<Library_MLibMan>("mpx-service-library");

    Util::FileList collection;

    for(Util::FileList::const_iterator i = list.begin(); i != list.end(); ++i)
    {  
        std::string insert_path ;
        std::string insert_path_sql ;
        Util::FileList collection;

        // Collection from Filesystem

#ifdef HAVE_HAL
        try{
#endif // HAVE_HAL

            insert_path = Util::normalize_path( *i ) ;

#ifdef HAVE_HAL
            try{
                if (m_Flags & Library_MLibMan::F_USING_HAL)
                { 
                    const Volume& volume (m_HAL.get_volume_for_uri (*i));
                    insert_path_sql = Util::normalize_path(Glib::filename_from_uri(*i).substr (volume.mount_point.length())) ;
                }
                else
#endif // HAVE_HAL

                {
                    insert_path_sql = insert_path; 
                }

#ifdef HAVE_HAL
            }
            catch(
              IHAL::Exception&     cxe
            )
            {
                g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
                continue;
            }
            catch(
              Glib::ConvertError& cxe
            )
            {
                g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
                continue;
            }
#endif // HAVE_HAL

            collection.clear();

            try{
                Util::collect_audio_paths_recursive( insert_path, collection );
            } catch(...) {}

            m_ScanSummary.FilesTotal += collection.size();

#ifdef HAVE_HAL
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
            continue;
        }
#endif // HAVE_HAL

        // Collect + Process Tracks

        for(Util::FileList::iterator i2 = collection.begin(); i2 != collection.end(); ++i2)
        {
            if( check_abort_scan() )
                return;

            Track_sp t (new Track);
            library->trackSetLocation( (*(t.get())), *i2 );

            const std::string& location = get<std::string>( (*(t.get()))[ATTRIBUTE_LOCATION].get() );

            gint64 mtime1 = Util::get_file_mtime( location ); 
            gint64 mtime2 = get_track_mtime( (*(t.get())) );

            if( check_mtimes && mtime2 )
            {
                    if( mtime1 <= last_scan_date)
                    {
                        ++m_ScanSummary.FilesUpToDate;
                        continue;
                    }
                    else
                    if( mtime1 == mtime2 )
                    {
                        ++m_ScanSummary.FilesUpToDate;
                        continue;
                    }
            }

            (*(t.get()))[ATTRIBUTE_MTIME] = mtime1; 
            insert_file_no_mtime_check( t, *i2, insert_path_sql );
                        
            if( (! (std::distance(collection.begin(), i2) % 50)) )
            {
                pthreaddata->Message.emit(
                    (boost::format(_("Collecting Tracks: %lld of %lld"))
                        % gint64(std::distance(collection.begin(), i2))
                        % gint64(m_ScanSummary.FilesTotal)
                    ).str()
                );
            }
        }
    }

    process_insertion_list ();
    do_remove_dangling ();
    update_albums ();

    pthreaddata->Message.emit(_("Rescan: Done"));
    pthreaddata->ScanSummary.emit( m_ScanSummary );
    pthreaddata->ScanEnd.emit();
}

void
MPX::LibraryScannerThread_MLibMan::on_scan_list_quick_stage_1(
    const Util::FileList& list
)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    pthreaddata->ScanStart.emit();

    boost::shared_ptr<Library_MLibMan> library = services->get<Library_MLibMan>("mpx-service-library") ;

    for( Util::FileList::const_iterator i = list.begin(); i != list.end(); ++i )
    {  
        std::string insert_path ;
        std::string insert_path_sql ;

#ifdef HAVE_HAL
        try{
#endif

            RowV v;
            insert_path = Util::normalize_path( *i ) ;

#ifdef HAVE_HAL

            try{
                if (m_Flags & Library_MLibMan::F_USING_HAL)
                { 
                    const Volume& volume = m_HAL.get_volume_for_uri (*i) ;
                    gint64 device_id = m_HAL.get_id_for_volume( volume.volume_udi, volume.device_udi ) ;
                    insert_path_sql = Util::normalize_path(Glib::filename_from_uri(*i).substr(volume.mount_point.length())) ;

                    m_SQL->get(
                        v,
                        mprintf( 
                              "SELECT id, device_id, hal_vrp, mtime FROM track WHERE device_id ='%lld' AND insert_path ='%q'"
                            , device_id 
                            , insert_path_sql.c_str())
                    );
                }
                else
#endif
                {
                    insert_path_sql = insert_path; 

                    m_SQL->get(
                        v,
                        mprintf( 
                              "SELECT id, location, mtime FROM track WHERE insert_path ='%q'"
                            , insert_path_sql.c_str())
                    );
                }
#ifdef HAVE_HAL
            }
            catch( IHAL::Exception& cxe )
            {
                g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
                continue;
            }
            catch( Glib::ConvertError& cxe )
            {
                g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
                continue;
            }
#endif // HAVE_HAL

            for( RowV::iterator i = v.begin(); i != v.end(); ++i )
            {
                Track_sp t = library->sqlToTrack( *i, false ); 
                std::string location = get<std::string>(   (*(t.get()))[ATTRIBUTE_LOCATION].get() ) ; 

                Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(location);
                if( !file->query_exists() )
                {
                    gint64 id = get<gint64>((*i)["id"]);
                    pthreaddata->EntityDeleted.emit( id , ENTITY_TRACK ); 

                    m_SQL->exec_sql(mprintf( delete_track_f, id));
                }

                if( check_abort_scan() ) 
                    return;

                if( (! (std::distance(v.begin(), i) % 100)) )
                {
                    pthreaddata->Message.emit((boost::format(_("Checking Files for Presence: %lld / %lld")) % std::distance(v.begin(), i) % v.size()).str());
                }
            }

#ifdef HAVE_HAL
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
        }
#endif // HAVE_HAL

    }
}

void
MPX::LibraryScannerThread_MLibMan::on_scan_stop ()
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_Cancel, 1);
}

MPX::LibraryScannerThread_MLibMan::EntityInfo
MPX::LibraryScannerThread_MLibMan::get_track_artist_id (Track &track, bool only_if_exists)
{
    RowV rows; 

    EntityInfo info ( 0, MPX::LibraryScannerThread_MLibMan::ENTITY_IS_UNDEFINED );

    char const* select_artist_f ("SELECT id FROM artist WHERE %s %s AND %s %s AND %s %s;"); 
    m_SQL->get (rows, mprintf (select_artist_f,

           attrs[ATTRIBUTE_ARTIST].id,
          (track.has(ATTRIBUTE_ARTIST)
              ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ARTIST].get()).c_str()).c_str()
              : "IS NULL"),

           attrs[ATTRIBUTE_MB_ARTIST_ID].id,
          (track.has(ATTRIBUTE_MB_ARTIST_ID)
              ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()).c_str()).c_str()
              : "IS NULL"),

           attrs[ATTRIBUTE_ARTIST_SORTNAME].id,
          (track.has(ATTRIBUTE_ARTIST_SORTNAME)
              ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()).c_str()).c_str()
              : "IS NULL")));

    if (rows.size ())
    {
      info = EntityInfo( 
            get<gint64>(rows[0].find ("id")->second)
          , MPX::LibraryScannerThread_MLibMan::ENTITY_IS_NOT_NEW
      );
    }
    else
    if (!only_if_exists)
    {
      char const* set_artist_f ("INSERT INTO artist (%s, %s, %s) VALUES (%Q, %Q, %Q);");

       info = EntityInfo(

              m_SQL->exec_sql (mprintf (set_artist_f,

                 attrs[ATTRIBUTE_ARTIST].id,
                 attrs[ATTRIBUTE_ARTIST_SORTNAME].id,
                 attrs[ATTRIBUTE_MB_ARTIST_ID].id,

                (track.has(ATTRIBUTE_ARTIST)
                    ? get<std::string>(track[ATTRIBUTE_ARTIST].get()).c_str()
                    : NULL) ,

                (track.has(ATTRIBUTE_ARTIST_SORTNAME)
                    ? get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()).c_str()
                    : NULL) ,

                (track.has(ATTRIBUTE_MB_ARTIST_ID)
                    ? get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()).c_str()
                    : NULL)
                ))

            , MPX::LibraryScannerThread_MLibMan::ENTITY_IS_NEW
       );
    }

    return info;
}

MPX::LibraryScannerThread_MLibMan::EntityInfo
MPX::LibraryScannerThread_MLibMan::get_album_artist_id (Track& track, bool only_if_exists)
{
    EntityInfo info ( 0, MPX::LibraryScannerThread_MLibMan::ENTITY_IS_UNDEFINED );

    if( track.has(ATTRIBUTE_ALBUM_ARTIST) && track.has(ATTRIBUTE_MB_ALBUM_ARTIST_ID) )
    {
        track[ATTRIBUTE_IS_MB_ALBUM_ARTIST] = gint64(1); 
    }
    else
    {
        track[ATTRIBUTE_IS_MB_ALBUM_ARTIST] = gint64(0); 
        track[ATTRIBUTE_ALBUM_ARTIST] = track[ATTRIBUTE_ARTIST];
    }

    RowV rows;

    if( track.has(ATTRIBUTE_MB_ALBUM_ARTIST_ID) )
    {
      char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s);"); 
      m_SQL->get (rows, (mprintf (select_artist_f, 

         attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
        (track.has(ATTRIBUTE_MB_ALBUM_ARTIST_ID)
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,
        (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
            ? "= '1'"
            : "IS NULL"))

      ));
    }
    else // TODO: MB lookup to fix sortname, etc, for a given ID (and possibly even retag files on the fly?)
    {
      char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s);"); 
      m_SQL->get (rows, (mprintf (select_artist_f, 

         attrs[ATTRIBUTE_ALBUM_ARTIST].id,
        (track.has(ATTRIBUTE_ALBUM_ARTIST)
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,
        (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
            ? "= '1'"
            : "IS NULL"))

       )) ; 
    }

    if (!rows.empty())
    {
        info = EntityInfo(
              get<gint64>(rows[0].find ("id")->second)
            , MPX::LibraryScannerThread_MLibMan::ENTITY_IS_NOT_NEW
        );
    }
    else
    if( !only_if_exists )
    {
        if( !track.has(ATTRIBUTE_MB_ALBUM_ARTIST_ID) )
             track[ATTRIBUTE_MB_ALBUM_ARTIST_ID] = "mpx-" + get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()); 

        char const* set_artist_f ("INSERT INTO album_artist (%s, %s, %s, %s) VALUES (%Q, %Q, %Q, %Q);");

        info = EntityInfo(

            m_SQL->exec_sql (mprintf (set_artist_f,

              attrs[ATTRIBUTE_ALBUM_ARTIST].id,
              attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id,
              attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
              attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,

            (track.has(ATTRIBUTE_ALBUM_ARTIST)
                ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str() 
                : NULL) , 

            (track.has(ATTRIBUTE_ALBUM_ARTIST_SORTNAME)
                ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].get()).c_str()
                : NULL) , 

            (track.has(ATTRIBUTE_MB_ALBUM_ARTIST_ID)
                ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()
                : NULL) ,

            (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
                ? "1"
                : NULL)
            ))

            , MPX::LibraryScannerThread_MLibMan::ENTITY_IS_NEW
        );
    }

    return info;
}

MPX::LibraryScannerThread_MLibMan::EntityInfo
MPX::LibraryScannerThread_MLibMan::get_album_id (Track& track, gint64 album_artist_id, bool only_if_exists)
{
    RowV rows;

    std::string sql;

    EntityInfo info ( 0, MPX::LibraryScannerThread_MLibMan::ENTITY_IS_UNDEFINED );

    if( track.has(ATTRIBUTE_MB_ALBUM_ID) )
    {
      char const* select_album_f ("SELECT album, id, mb_album_id FROM album WHERE (%s = '%q') AND (%s %s) AND (%s %s) AND (%s = %lld);"); 

      sql = mprintf (select_album_f,

             attrs[ATTRIBUTE_MB_ALBUM_ID].id,
             get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()).c_str(),

             attrs[ATTRIBUTE_MB_RELEASE_DATE].id,
            (track.has(ATTRIBUTE_MB_RELEASE_DATE)
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id,
            (track.has(ATTRIBUTE_MB_RELEASE_COUNTRY)
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_RELEASE_COUNTRY].get()).c_str()).c_str()
                : "IS NULL"), 

            "album_artist_j",
            album_artist_id
      );
    }
    else
    {
      char const* select_album_f ("SELECT album, id, mb_album_id FROM album WHERE (%s %s) AND (%s %s) AND (%s = %lld);"); 

      sql = mprintf (select_album_f,

             attrs[ATTRIBUTE_ALBUM].id,
            (track.has(ATTRIBUTE_ALBUM)
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()).c_str()
                : "IS NULL"), 

            attrs[ATTRIBUTE_ASIN].id,
            (track.has(ATTRIBUTE_ASIN)
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()).c_str()
                : "IS NULL"), 

            "album_artist_j",
            album_artist_id
      );
    }

    OVariant mbid;
    m_SQL->get (rows, sql); 

    if(!rows.empty())
    {
        info = EntityInfo(
              get<gint64>(rows[0].find ("id")->second)
            , MPX::LibraryScannerThread_MLibMan::ENTITY_IS_NOT_NEW
        );

        if(rows[0].count("mb_album_id"))
        {
            track[ATTRIBUTE_MB_ALBUM_ID] = get<std::string>(rows[0].find ("mb_album_id")->second);
        }
    }
    else
    if(!only_if_exists)
    {
      if( !track.has(ATTRIBUTE_MB_ALBUM_ID) )
      {
        track[ATTRIBUTE_MB_ALBUM_ID] = "mpx-" + get<std::string>(track[ATTRIBUTE_ARTIST].get())
                                              + "-"
                                              + get<std::string>(track[ATTRIBUTE_ALBUM].get()); 
      }

      char const* set_album_f ("INSERT INTO album (%s, %s, %s, %s, %s, %s, %s, %s, album_new) VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %lld, %lld, 1);");

      std::string sql = mprintf (set_album_f,

          attrs[ATTRIBUTE_ALBUM].id,
          attrs[ATTRIBUTE_MB_ALBUM_ID].id,
          attrs[ATTRIBUTE_MB_RELEASE_DATE].id,
          attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id,
          attrs[ATTRIBUTE_MB_RELEASE_TYPE].id,
          attrs[ATTRIBUTE_ASIN].id,
          "album_artist_j",
          "album_insert_date",

          (track.has(ATTRIBUTE_ALBUM)
              ? get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()
              : NULL) , 

          (track.has(ATTRIBUTE_MB_ALBUM_ID)
              ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()).c_str()
              : NULL) , 

          (track.has(ATTRIBUTE_MB_RELEASE_DATE)
              ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()).c_str()
              : NULL) , 

          (track.has(ATTRIBUTE_MB_RELEASE_COUNTRY)
              ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_COUNTRY].get()).c_str()
              : NULL) , 

          (track.has(ATTRIBUTE_MB_RELEASE_TYPE)
              ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_TYPE].get()).c_str()
              : NULL) , 

          (track.has(ATTRIBUTE_ASIN)
              ? get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()
              : NULL) , 

          album_artist_id,

          gint64(time(NULL))

      );

      info = EntityInfo(
          m_SQL->exec_sql (sql)
        , MPX::LibraryScannerThread_MLibMan::ENTITY_IS_NEW
      );
    }

    return info;
}

gint64
MPX::LibraryScannerThread_MLibMan::get_track_mtime(
    Track& track
) const
{
    Duplet_MTIME_t::const_iterator i =
         m_MTIME_Map.find(
            boost::make_tuple(
                get<gint64>(track[ATTRIBUTE_MPX_DEVICE_ID].get())
              , get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get())
    ));

    if( i != m_MTIME_Map.end() )
    {
        return i->second;
    }

    return 0;
}

gint64
MPX::LibraryScannerThread_MLibMan::get_track_id (Track& track) const
{
  RowV rows;
#ifdef HAVE_HAL
  if( m_Flags & Library_MLibMan::F_USING_HAL )
  {
    static boost::format
      select_f ("SELECT id FROM track WHERE %s='%lld' AND %s='%s';");

    m_SQL->get (rows, (select_f % attrs[ATTRIBUTE_MPX_DEVICE_ID].id
                                % get<gint64>(track[ATTRIBUTE_MPX_DEVICE_ID].get())
                                % attrs[ATTRIBUTE_VOLUME_RELATIVE_PATH].id
                                % mprintf ("%q", Util::normalize_path(get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get())).c_str())).str());
  }
  else
#endif
  {
    static boost::format
      select_f ("SELECT id FROM track WHERE %s='%s';");

    m_SQL->get (rows, (select_f % attrs[ATTRIBUTE_LOCATION].id 
                                % mprintf ("%q", get<std::string>(track[ATTRIBUTE_LOCATION].get()).c_str())).str());
  }

  if (rows.size() && (rows[0].count("id") != 0))
  {
    return boost::get<gint64>( rows[0].find("id")->second ) ;
  }

  return 0;
}

void
MPX::LibraryScannerThread_MLibMan::add_erroneous_track(
      const std::string& uri
    , const std::string& info
)
{
    ++m_ScanSummary.FilesErroneous;

    URI u (uri);
    u.unescape();

    try{
          m_ScanSummary.FileListErroneous.push_back( SSFileInfo( filename_to_utf8((ustring(u))), info));
    } catch( Glib::ConvertError & cxe )
    {
          m_ScanSummary.FileListErroneous.push_back( SSFileInfo( _("(invalid UTF-8)"), (boost::format (_("Could not convert URI to UTF-8 for display: %s")) % cxe.what()).str()));
    }
}

void
MPX::LibraryScannerThread_MLibMan::insert_file_no_mtime_check(
      Track_sp           track
    , const std::string& uri
    , const std::string& insert_path
    , bool               update
)
{
    try{
        try{
            Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
            Glib::RefPtr<Gio::FileInfo> info = file->query_info("standard::content-type");
            (*(track.get()))[ATTRIBUTE_TYPE] = info->get_attribute_string("standard::content-type");
        } catch(Gio::Error & cxe) {
            add_erroneous_track( uri, _("An error ocurred trying to determine the file type"));
            return;
        }

        if( !services->get<MetadataReaderTagLib>("mpx-service-taglib")->get( uri, *(track.get()) ) )
        {
            add_erroneous_track( uri, _("Could not acquire metadata (using taglib-gio)"));

            quarantine_file( uri ) ;
        }
        else try{
            create_insertion_track( *(track.get()), uri, insert_path, update );
        }
        catch( ScanError & cxe )
        {
            add_erroneous_track( uri, (boost::format (_("Error inserting file: %s")) % cxe.what()).str());
        }
    }
    catch( Library_MLibMan::FileQualificationError & cxe )
    {
        add_erroneous_track( uri, (boost::format (_("Error inserting file: %s")) % cxe.what()).str());
    }
}

std::string
MPX::LibraryScannerThread_MLibMan::create_update_sql(
      const Track&  track
    , gint64        album_j
    , gint64        artist_j
)
{
    std::string sql = "UPDATE track SET ";

    for( unsigned int n = 0; n < ATTRIBUTE_MPX_TRACK_ID; ++n )
    {
        if( track.has( n ))
        {
            switch( attrs[n].type )
            {
                  case VALUE_TYPE_STRING:
                      append_key_value_pair<std::string>( sql, track, n );
                      break;

                  case VALUE_TYPE_INT:
                      append_key_value_pair<gint64>( sql, track, n );
                      break;

                  case VALUE_TYPE_REAL:
                      append_key_value_pair<double>( sql, track, n );
                      break;
            }

            sql += ","; 
        }
    }

    sql += mprintf("artist_j = '%lld', ", artist_j ); 
    sql += mprintf("album_j = '%lld'  ", album_j ); 

    return sql;
}

std::string
MPX::LibraryScannerThread_MLibMan::create_insertion_sql(
      const Track&  track
    , gint64        album_j
    , gint64        artist_j
)
{
    char const track_set_f[] = "INSERT INTO track (%s) VALUES (%s);";

    std::string sql;

    std::string column_n;
    std::string column_v;

    column_n.reserve( 0x400 );
    column_v.reserve( 0x400 );

    for( unsigned int n = 0; n < ATTRIBUTE_MPX_TRACK_ID; ++n )
    {
        if( track.has( n ))
        {
            switch( attrs[n].type )
            {
                  case VALUE_TYPE_STRING:
                      append_value<std::string>( column_v, track, n );
                      break;

                  case VALUE_TYPE_INT:
                      append_value<gint64>( column_v, track, n );
                      break;

                  case VALUE_TYPE_REAL:
                      append_value<double>( column_v, track, n );
                      break;
            }

            column_n += std::string( attrs[n].id ) + ",";
            column_v += ",";
        }
    }

    column_n += "artist_j, album_j";
    column_v += mprintf( "'%lld'", artist_j ) + "," + mprintf( "'%lld'", album_j ) ; 

    return mprintf( track_set_f, column_n.c_str(), column_v.c_str());
}

void
MPX::LibraryScannerThread_MLibMan::create_insertion_track(
      Track&             track
    , const std::string& uri
    , const std::string& insert_path
    , bool               update
)
{
    if( uri.empty() )
    {
      throw ScanError(_("Empty URI/no URI given"));
    }

    ThreadData * pthreaddata = m_ThreadData.get();

    if( !(track.has(ATTRIBUTE_ALBUM) && track.has(ATTRIBUTE_ARTIST) && track.has(ATTRIBUTE_TITLE)) )
    {
        gint64 id = get_track_id( track );

        if( id != 0 )
        {
            pthreaddata->EntityDeleted.emit( id , ENTITY_TRACK ); 
            m_SQL->exec_sql(mprintf( delete_track_f, id));
        }

        quarantine_file( uri ) ;

        throw ScanError(_("Insufficient Metadata (artist, album and title must be given)"));
    }

    track[ATTRIBUTE_INSERT_PATH] = Util::normalize_path( insert_path ) ;
    track[ATTRIBUTE_INSERT_DATE] = gint64(time(NULL));
    track[ATTRIBUTE_QUALITY]     = get_audio_quality( get<std::string>(track[ATTRIBUTE_TYPE].get()), get<gint64>(track[ATTRIBUTE_BITRATE].get()) );

    TrackInfo_p p (new TrackInfo);

    p->Artist         = get_track_artist_id( track ) ;
    p->AlbumArtist    = get_album_artist_id( track ) ;
    p->Album          = get_album_id( track, p->AlbumArtist.first );
    p->Title          = get<std::string>(track[ATTRIBUTE_TITLE].get());
    p->TrackNumber    = get<gint64>(track[ATTRIBUTE_TRACK].get());
    p->Type           = get<std::string>(track[ATTRIBUTE_TYPE].get());
    p->Update         = update;
    p->Track          = Track_sp( new Track( track ));

    if( p->Album.second == ENTITY_IS_NEW )
    {
        m_AlbumIDs.insert( p->Album.first );
    }

    if( p->AlbumArtist.second == ENTITY_IS_NEW )
    {
        m_AlbumArtistIDs.insert( p->AlbumArtist.first );
    }

    m_InsertionTracks[p->Album.first][p->Artist.first][p->Title][p->TrackNumber].push_back( p );
}

MPX::LibraryScannerThread_MLibMan::TrackInfo_p
MPX::LibraryScannerThread_MLibMan::prioritize(
    const TrackInfo_p_Vector& v
)
{
    if( v.size() == 1 )
    {
        return v[0];
    }

    TrackInfo_p_Vector v2;

    if( m_PrioritizeByFileType )
    {
            for( MIME_Types_t::const_iterator t = m_MIME_Types.begin(); t != m_MIME_Types.end(); ++t )
            {
                for( TrackInfo_p_Vector::const_iterator i = v.begin(); i != v.end(); ++i )
                {
                    if( (*i)->Type == (*t) )
                    {
                        v2.push_back( *i );
                    }
                }

                if( !v2.empty() )
                    break;
            }
    }
    else
    {
            v2 = v;
    }

    if( v2.size() == 1 || !m_PrioritizeByBitrate )
    {
        return v2[0];
    }
    else
    {
        TrackInfo_p p_highest_bitrate;
        gint64      bitrate = 0;

        for( TrackInfo_p_Vector::const_iterator i = v2.begin(); i != v2.end(); ++i )
        {
            gint64 c_bitrate = get<gint64>((*(*i)->Track.get())[ATTRIBUTE_BITRATE].get());

            if( c_bitrate > bitrate )
            {
                bitrate = c_bitrate;
                p_highest_bitrate = (*i);
            }
        }

        return p_highest_bitrate;
    }

    return v[0];
}

void
MPX::LibraryScannerThread_MLibMan::process_insertion_list()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    gint64 tracks = 0;

    for( Map_L1::const_iterator l1 = m_InsertionTracks.begin(); l1 != m_InsertionTracks.end(); ++l1 ) {

    for( Map_L2::const_iterator l2 = (l1->second).begin(); l2 != (l1->second).end(); ++l2 ) {
    for( Map_L3::const_iterator l3 = (l2->second).begin(); l3 != (l2->second).end(); ++l3 ) {
    for( Map_L4::const_iterator l4 = (l3->second).begin(); l4 != (l3->second).end(); ++l4 ) {


                    const TrackInfo_p_Vector& v = (*l4).second ; 

                    std::string location;
    
                    if( !v.empty() )
                    try{

                        TrackInfo_p p = prioritize( v );

                        location = get<std::string>((*(p->Track.get()))[ATTRIBUTE_LOCATION].get());

                        switch( insert( p, v ) ) 
                        {
                            case SCAN_RESULT_OK:
                                 pthreaddata->Message.emit(
                                   (boost::format(_("Inserting Tracks: %lld"))
                                        % ++tracks 
                                    ).str()
                                 );
                                ++m_ScanSummary.FilesAdded;
                                break;

                            case SCAN_RESULT_UPDATE:
                                try{
                                    ++m_ScanSummary.FilesUpdated;
                                      m_ScanSummary.FileListUpdated.push_back( SSFileInfo( filename_to_utf8(location), _("Updated")));
                                } catch (Glib::ConvertError & cxe )
                                {
                                      m_ScanSummary.FileListErroneous.push_back( SSFileInfo( _("(invalid UTF-8)"), _("Could not convert URI to UTF-8 for display")));
                                }
                                break;

                            case SCAN_RESULT_UPTODATE:
                                ++m_ScanSummary.FilesUpToDate;
                                break;
                        }
                    }
                    catch( ScanError & cxe )
                    {
                        add_erroneous_track( location, (boost::format (_("Error inserting file: %s")) % cxe.what()).str() );
                    }

    }
    }
    }
    }
       
    m_InsertionTracks.clear(); 
    m_AlbumIDs.clear();
    m_AlbumArtistIDs.clear();
}

void
MPX::LibraryScannerThread_MLibMan::signal_new_entities(
    const TrackInfo_p& p
)
{
    ThreadData * pthreaddata = m_ThreadData.get();
    
    gint64 album_id = p->Album.first; 
    gint64 artst_id = p->AlbumArtist.first;

    if( m_AlbumArtistIDs.find( artst_id ) != m_AlbumArtistIDs.end() )
    { 
        pthreaddata->NewArtist.emit( artst_id );
        m_AlbumArtistIDs.erase( artst_id );
    }

    if( m_AlbumIDs.find( album_id ) != m_AlbumIDs.end() )
    {
            m_AlbumIDs.erase( album_id );

            RequestQualifier rq;

            rq.mbid       = get<std::string>((*(p->Track.get()))[ATTRIBUTE_MB_ALBUM_ID].get());
            rq.asin       = (*(p->Track.get())).has(ATTRIBUTE_ASIN)
                                  ? get<std::string>((*(p->Track.get()))[ATTRIBUTE_ASIN].get())
                                  : "";

            rq.uri        = get<std::string>((*(p->Track.get()))[ATTRIBUTE_LOCATION].get());
            rq.artist     = (*(p->Track.get())).has(ATTRIBUTE_ALBUM_ARTIST)
                                  ? get<std::string>((*(p->Track.get()))[ATTRIBUTE_ALBUM_ARTIST].get())
                                  : get<std::string>((*(p->Track.get()))[ATTRIBUTE_ARTIST].get());

            rq.album      = get<std::string>((*(p->Track.get()))[ATTRIBUTE_ALBUM].get());

            pthreaddata->NewAlbum.emit( album_id, rq.mbid, rq.asin, rq.uri, rq.artist, rq.album );
    }
}

int
MPX::LibraryScannerThread_MLibMan::compare_types(
      const std::string& a
    , const std::string& b
)
{
    int val_a = std::numeric_limits<int>::max();
    int val_b = std::numeric_limits<int>::max();

    for( MIME_Types_t::iterator t = m_MIME_Types.begin(); t != m_MIME_Types.end(); ++t )
    {
        if( *t == a )
        {
            val_a = std::distance( m_MIME_Types.begin(), t );
            break;
        }
    }

    
    for( MIME_Types_t::iterator t = m_MIME_Types.begin(); t != m_MIME_Types.end(); ++t )
    {
        if( *t == b )
        {
            val_b = std::distance( m_MIME_Types.begin(), t );
            break;
        }
    }

    if( val_a < val_b )
        return 1;
    else if( val_a > val_b )
        return -1;

    return 0;
}

ScanResult
MPX::LibraryScannerThread_MLibMan::insert(
      const TrackInfo_p& p
    , const TrackInfo_p_Vector& v
)
{
  ThreadData * pthreaddata = m_ThreadData.get();

  Track & track = *(p->Track.get());

  m_ProcessedAlbums.insert( p->Album.first );

  try{

    if( p->Update )
    {
        gint64 id_old = 0;
        gint64 id_new = 0;
        RowV   rv;

        if( mcs->key_get<bool>("library","use-hal"))
        {
            m_SQL->get(
                rv,
                mprintf(
                      "SELECT id FROM track WHERE device_id = '%lld' AND hal_vrp = '%s'"
                    , get<gint64>(track[ATTRIBUTE_MPX_DEVICE_ID].get())
                    , get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str() 
            )); 
        }
        else
        {
            m_SQL->get(
                rv,
                mprintf(
                      "SELECT id FROM track WHERE location = '%s'"
                    , get<std::string>(track[ATTRIBUTE_LOCATION].get()).c_str() 
            )); 
        }

        id_old = get<gint64>(rv[0]["id"]);

        m_SQL->exec_sql(
            mprintf(
                  "DELETE FROM track WHERE id ='%lld"
                , id_old
        ));

        id_new = m_SQL->exec_sql( create_insertion_sql( track, p->Album.first, p->Artist.first )); 

        m_SQL->exec_sql(
            mprintf(
                  "UPDATE track SET id = '%lld' WHERE id ='%lld"
                , id_old
                , id_new
        ));
    }
    else
    {
        track[ATTRIBUTE_MPX_TRACK_ID] = m_SQL->exec_sql( create_insertion_sql( track, p->Album.first, p->Artist.first )); 
    } 
  } catch( SqlConstraintError & cxe )
  {
    RowV rv;
    m_SQL->get(
        rv,
        mprintf(
              "SELECT id, type, bitrate, location FROM track WHERE album_j = '%lld' AND artist_j = '%lld' AND track = '%lld' AND title = '%q'"
            , p->Album.first
            , p->Artist.first
            , p->TrackNumber
            , p->Title.c_str()
    )); 

    gint64 id = rv.empty() ? 0 : get<gint64>(rv[0]["id"]);
    if( id != 0 ) 
    {
        int types_compare = compare_types( p->Type, get<std::string>(rv[0]["type"]));

        if( (m_PrioritizeByFileType && types_compare > 0) || (m_PrioritizeByBitrate && (get<gint64>(track[ATTRIBUTE_BITRATE].get())) > (get<gint64>(rv[0]["bitrate"]))))
        try{
                track[ATTRIBUTE_MPX_TRACK_ID] = id; 
                m_SQL->exec_sql( create_update_sql( track, p->Album.first, p->Artist.first ) + (boost::format(" WHERE id = '%lld'") % id).str()  ); 
                signal_new_entities( p );
                pthreaddata->EntityUpdated( id, ENTITY_TRACK );

                return SCAN_RESULT_UPDATE ;

        } catch( SqlConstraintError & cxe )
        {
                g_message("Scan Error!: %s", cxe.what());
                throw ScanError((boost::format(_("Constraint error while trying to set new track data!: '%s'")) % cxe.what()).str());
        }

        return SCAN_RESULT_UPTODATE ;
    }
    else
    {
        throw ScanError(_("Got track ID 0 for internal track! Highly supicious! Please report!"));
    }
  }
  catch( SqlExceptionC & cxe )
  {
        g_message("Scan Error!: %s", cxe.what());
        throw ScanError((boost::format(_("SQL error while inserting/updating track: '%s'")) % cxe.what()).str());
  }

  signal_new_entities( p );

  pthreaddata->NewTrack.emit( get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) ) ;

  return SCAN_RESULT_OK ; 
}

void
MPX::LibraryScannerThread_MLibMan::do_remove_dangling () 
{
  ThreadData * pthreaddata = m_ThreadData.get();

#ifdef HAVE_TR1
  typedef std::tr1::unordered_set<gint64> IdSet;
#else
  typedef std::set<gint64> IdSet;
#endif // HAVE_TR1

  static boost::format delete_f ("DELETE FROM %s WHERE id = '%lld'");
  IdSet idset1;
  IdSet idset2;
  RowV rows;

  /// CLEAR DANGLING ARTISTS
  pthreaddata->Message.emit(_("Finding Lost Artists..."));

  idset1.clear();
  rows.clear();
  m_SQL->get(rows, "SELECT DISTINCT artist_j FROM track");
  for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset1.insert (get<gint64>(i->find ("artist_j")->second));

  idset2.clear();
  rows.clear();
  m_SQL->get(rows, "SELECT DISTINCT id FROM artist");
  for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset2.insert (get<gint64>(i->find ("id")->second));

  for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
  {
        if (idset1.find (*i) == idset1.end())
        {
            pthreaddata->EntityDeleted( *i , ENTITY_ARTIST );

            m_SQL->exec_sql((delete_f % "artist" % (*i)).str());
        }
  }


  /// CLEAR DANGLING ALBUMS
  pthreaddata->Message.emit(_("Finding Lost Albums..."));

  idset1.clear();
  rows.clear();
  m_SQL->get(rows, "SELECT DISTINCT album_j FROM track");
  for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset1.insert (get<gint64>(i->find ("album_j")->second));

  idset2.clear();
  rows.clear();
  m_SQL->get(rows, "SELECT DISTINCT id FROM album");
  for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset2.insert (get<gint64>(i->find ("id")->second));

  for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
  {
        if (idset1.find (*i) == idset1.end())
        {
            pthreaddata->EntityDeleted( *i , ENTITY_ALBUM );

            m_SQL->exec_sql((delete_f % "album" % (*i)).str());
        }
  }

  /// CLEAR DANGLING ALBUM ARTISTS
  pthreaddata->Message.emit(_("Finding Lost Album Artists..."));

  idset1.clear();
  rows.clear();
  m_SQL->get(rows, "SELECT DISTINCT album_artist_j FROM album");
  for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset1.insert (get<gint64>(i->find ("album_artist_j")->second));

  idset2.clear();
  rows.clear();
  m_SQL->get(rows, "SELECT DISTINCT id FROM album_artist");
  for (RowV::const_iterator i = rows.begin(); i != rows.end(); ++i)
          idset2.insert (get<gint64>(i->find ("id")->second));

  for (IdSet::const_iterator i = idset2.begin(); i != idset2.end(); ++i)
  {
        if (idset1.find (*i) == idset1.end())
        {
            pthreaddata->EntityDeleted( *i , ENTITY_ALBUM_ARTIST );

            m_SQL->exec_sql((delete_f % "album_artist" % (*i)).str());
        }
  }

  pthreaddata->Message.emit(_("Cleanup: Done"));
}

void
MPX::LibraryScannerThread_MLibMan::on_vacuum() 
{
  ThreadData * pthreaddata = m_ThreadData.get();

  pthreaddata->ScanStart.emit();

  RowV rows;
  m_SQL->get (rows, "SELECT id, device_id, hal_vrp, location FROM track"); // FIXME: We shouldn't need to do this here, it should be transparent (HAL vs. non HAL)

  for( RowV::iterator i = rows.begin(); i != rows.end(); ++i )
  {
          std::string uri = get<std::string>((*(m_Library_MLibMan.sqlToTrack( *i, false )))[ATTRIBUTE_LOCATION].get());

          if( !uri.empty() )
          {
              if( (!(std::distance(rows.begin(), i) % 50)) )
              {
                      pthreaddata->Message.emit((boost::format(_("Checking files for presence: %lld / %lld")) % std::distance(rows.begin(), i) % rows.size()).str());
              }

              try{
                      Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
                      if( !file->query_exists() )
                      {
                              pthreaddata->EntityDeleted( get<gint64>((*i)["id"]) , ENTITY_TRACK );

                              m_SQL->exec_sql((boost::format ("DELETE FROM track WHERE id = %lld") % get<gint64>((*i)["id"])).str()); 
                      }
              } catch(Glib::Error) {
                        g_message(G_STRLOC ": Error while trying to test URI '%s' for presence", uri.c_str());
              }
          }
  }

  do_remove_dangling ();

  pthreaddata->Message.emit(_("Vacuum process done."));
  pthreaddata->ScanEnd.emit();
}

#ifdef HAVE_HAL
void
MPX::LibraryScannerThread_MLibMan::on_vacuum_volume_list(
      const VolumeKey_v&    volumes
    , bool                    do_signal
)
{
  ThreadData * pthreaddata = m_ThreadData.get();
    
  if( do_signal )
      pthreaddata->ScanStart.emit();


  for( VolumeKey_v::const_iterator i = volumes.begin(); i != volumes.end(); ++i )
  {
          gint64 device_id = m_HAL.get_id_for_volume( (*i).first, (*i).second ) ;

          RowV rows;
          m_SQL->get(
              rows,
              (boost::format ("SELECT * FROM track WHERE device_id = '%lld'")
                      % device_id 
              ).str());

          for( RowV::iterator i = rows.begin(); i != rows.end(); ++i )
          {
                  std::string uri = get<std::string>((*(m_Library_MLibMan.sqlToTrack( *i, false )))[ATTRIBUTE_LOCATION].get());

                  if( !uri.empty() )
                  {
                      if( (!(std::distance(rows.begin(), i) % 50)) )
                      {
                              pthreaddata->Message.emit((boost::format(_("Checking files for presence: %lld / %lld")) % std::distance(rows.begin(), i) % rows.size()).str());
                      }

                      try{
                              Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
                              if( !file->query_exists() )
                              {
                                      pthreaddata->EntityDeleted( get<gint64>((*i)["id"]), ENTITY_TRACK );

                                      m_SQL->exec_sql((boost::format ("DELETE FROM track WHERE id = %lld") % get<gint64>((*i)["id"])).str()); 
                              }
                      } catch(Glib::Error) {
                                g_message(G_STRLOC ": Error while trying to test URI '%s' for presence", uri.c_str());
                      }
                  }
          }
  }

  do_remove_dangling ();

  pthreaddata->Message.emit(_("Vacuum Process: Done"));

  if( do_signal )
      pthreaddata->ScanEnd.emit();
}
#endif // HAVE_HAL

void
MPX::LibraryScannerThread_MLibMan::update_albums(
)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    for( IdSet_t::const_iterator i = m_ProcessedAlbums.begin(); i != m_ProcessedAlbums.end() ; ++i )
    {
        update_album( (*i) );

        if( !(std::distance(m_ProcessedAlbums.begin(), i) % 50) )
        {
            pthreaddata->Message.emit((boost::format(_("Additional Metadata Update: %lld of %lld")) % std::distance(m_ProcessedAlbums.begin(), i) % m_ProcessedAlbums.size() ).str());
        }
    }

    m_ProcessedAlbums.clear();
}

void
MPX::LibraryScannerThread_MLibMan::update_album(
      gint64 id
)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    RowV        rows;
    std::string genre;
    gint64      quality = 0;

    m_SQL->get(
        rows,
        (boost::format ("SELECT DISTINCT genre FROM track WHERE album_j = %lld AND genre IS NOT NULL AND genre != '' LIMIT 1") 
            % id 
        ).str()
    ); 
    if( !rows.empty() )
    {
        genre = get<std::string>(rows[0]["genre"]);
    }

    rows.clear();
    m_SQL->get(
        rows,
        (boost::format ("SELECT sum(audio_quality)/count(*) AS quality FROM track WHERE album_j = %lld AND audio_quality IS NOT NULL AND audio_quality != '0'") 
            % id 
        ).str()
    ); 
    if( !rows.empty() )
    {
        quality = get<gint64>(rows[0]["quality"]);
    }

    m_SQL->exec_sql(mprintf("UPDATE album SET album_genre = '%q', album_quality = '%lld' WHERE id = '%lld'", genre.c_str(), quality, id));

    pthreaddata->EntityUpdated( id, ENTITY_ALBUM );
}

void
MPX::LibraryScannerThread_MLibMan::quarantine_file(
      const std::string& uri
)
{
    if( mcs->key_get<bool>("library","quarantine-invalid") ) 
    {
        std::string new_dir_path ;
        std::string new_file_path ;

        Glib::RefPtr<Gio::File> orig = Gio::File::create_for_uri( uri ) ;

        try{
            Glib::RefPtr<Gio::File> orig_parent_dir = orig->get_parent() ;
            new_dir_path = Glib::build_filename( mcs->key_get<std::string>("mpx","music-quarantine-path"), orig_parent_dir->get_basename() ) ;
            Glib::RefPtr<Gio::File> new_dir = Gio::File::create_for_path( new_dir_path ) ;
            Glib::RefPtr<Gio::Cancellable> cancellable = Gio::Cancellable::create();
            new_dir->make_directory_with_parents(cancellable) ;
        } catch( Glib::Error & cxe ) {
        }

        try{
            new_file_path = Glib::build_filename( new_dir_path, orig->get_basename() ) ;
            Glib::RefPtr<Gio::File> target = Gio::File::create_for_path( new_file_path ) ;
            g_message("%s: Moving file to '%s'", G_STRLOC, new_file_path.c_str() ) ;
            orig->move( target ) ;
        } catch( Glib::Error & cxe )
        {
            g_message("%s: Error moving file into quarantine: %s", G_STRLOC, cxe.what().c_str() ) ;
        }
    }
}

void
MPX::LibraryScannerThread_MLibMan::on_update_statistics()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    pthreaddata->ScanStart.emit();

    SQL::RowV v;

    m_SQL->get(
          v
        , "SELECT DISTINCT id, mb_album_artist_id FROM album_artist WHERE is_mb_album_artist = 1"
    );

    for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
    {
        try{
            MPX::XmlInstance<mmd::metadata> mmd ((boost::format("http://www.uk.musicbrainz.org/ws/1/artist/%s?type=xml") % get<std::string>((*i)["mb_album_artist_id"])).str());

            std::string ls_begin, ls_end;

            if( mmd.xml().artist().life_span().present() )
            {
                if( mmd.xml().artist().life_span().get().begin().present() )
                    ls_begin = mmd.xml().artist().life_span().get().begin().get();

                if( mmd.xml().artist().life_span().get().end().present() )
                    ls_end = mmd.xml().artist().life_span().get().end().get();
            }

            gint64 id = get<gint64>((*i)["id"]);

            m_SQL->exec_sql(
                (boost::format("UPDATE album_artist SET life_span_begin = '%s', life_span_end = '%s' WHERE id = '%lld'") % ls_begin % ls_end % id).str()
            );

            pthreaddata->Message.emit((boost::format(_("Updating Artist Life Spans: %lld / %lld")) % std::distance(v.begin(), i) % v.size()).str());

            pthreaddata->EntityUpdated( id, ENTITY_ALBUM_ARTIST );

        } catch( ... ) {        
        }
    }

    pthreaddata->Message(_("Additional Metadata Update: Done"));
    pthreaddata->ScanEnd.emit();
}


