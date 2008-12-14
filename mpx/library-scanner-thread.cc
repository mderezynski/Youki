#include "mpx/mpx-audio.hh"
#include "mpx/mpx-hal.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-library-scanner-thread.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/util-file.hh"
#include <queue>
#include <boost/ref.hpp>
#include <boost/format.hpp>
#include <giomm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <tr1/unordered_set>

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
    };
}

struct MPX::LibraryScannerThread::ThreadData
{
    LibraryScannerThread::SignalScanStart_t         ScanStart ;
    LibraryScannerThread::SignalScanRun_t           ScanRun ;
    LibraryScannerThread::SignalScanEnd_t           ScanEnd ;
    LibraryScannerThread::SignalScanSummary_t       ScanSummary ;
    LibraryScannerThread::SignalNewAlbum_t          NewAlbum ;
    LibraryScannerThread::SignalNewArtist_t         NewArtist ;
    LibraryScannerThread::SignalNewTrack_t          NewTrack ;
    LibraryScannerThread::SignalEntityDeleted_t     EntityDeleted ;
    LibraryScannerThread::SignalCacheCover_t        CacheCover ;
    LibraryScannerThread::SignalReload_t            Reload ;
    LibraryScannerThread::SignalMessage_t           Message ;

    int m_ScanStop;
};

MPX::LibraryScannerThread::LibraryScannerThread(
    MPX::Library*               obj_library
  , gint64                      flags
)
: sigx::glib_threadable()
, scan(sigc::mem_fun(*this, &LibraryScannerThread::on_scan))
, scan_stop(sigc::mem_fun(*this, &LibraryScannerThread::on_scan_stop))
, vacuum(sigc::mem_fun(*this, &LibraryScannerThread::on_vacuum))
#ifdef HAVE_HAL
, vacuum_volume(sigc::mem_fun(*this, &LibraryScannerThread::on_vacuum_volume))
#endif // HAVE_HAL
, signal_scan_start(*this, m_ThreadData, &ThreadData::ScanStart)
, signal_scan_run(*this, m_ThreadData, &ThreadData::ScanRun)
, signal_scan_end(*this, m_ThreadData, &ThreadData::ScanEnd)
, signal_scan_summary(*this, m_ThreadData, &ThreadData::ScanSummary)
, signal_new_album(*this, m_ThreadData, &ThreadData::NewAlbum)
, signal_new_artist(*this, m_ThreadData, &ThreadData::NewArtist)
, signal_new_track(*this, m_ThreadData, &ThreadData::NewTrack)
, signal_entity_deleted(*this, m_ThreadData, &ThreadData::EntityDeleted)
, signal_cache_cover(*this, m_ThreadData, &ThreadData::CacheCover)
, signal_reload(*this, m_ThreadData, &ThreadData::Reload)
, signal_message(*this, m_ThreadData, &ThreadData::Message)
, m_Library(boost::shared_ptr<Library>(obj_library))
, m_SQL(new SQL::SQLDB(*((m_Library->get_sql_db()))))
, m_HAL(services->get<HAL>("mpx-service-hal"))
, m_MetadataReaderTagLib(services->get<MetadataReaderTagLib>("mpx-service-taglib"))
, m_Flags(flags)
{
    m_Connectable =
        new ScannerConnectable(
                  signal_scan_start
                , signal_scan_run
                , signal_scan_end
                , signal_scan_summary
                , signal_new_album
                , signal_new_artist
                , signal_new_track
                , signal_entity_deleted
                , signal_cache_cover
                , signal_reload
                , signal_message
    ); 
}

MPX::LibraryScannerThread::~LibraryScannerThread ()
{
    delete m_Connectable;
}

MPX::LibraryScannerThread::ScannerConnectable&
MPX::LibraryScannerThread::connect ()
{
    return *m_Connectable;
} 

void
MPX::LibraryScannerThread::on_startup ()
{
    m_ThreadData.set(new ThreadData);
}

void
MPX::LibraryScannerThread::on_cleanup ()
{
}

void
MPX::LibraryScannerThread::on_scan (Util::FileList const& list, bool deep)
{
    if(list.empty())
    {
        return;
    }

    if( deep )
        on_scan_list_deep( list );
    else
        on_scan_list_paths( list ); 
}

void
MPX::LibraryScannerThread::on_scan_list_paths (Util::FileList const& list)
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_ScanStop, 0);

    pthreaddata->ScanStart.emit();
    m_SQL->exec_sql( "UPDATE album SET album_new = 0" );

    SQL::RowV rows;
    m_SQL->get(rows, "SELECT last_scan_date FROM meta");
    gint64 last_scan_date = boost::get<gint64>(rows[1]["last_scan_date"]);

    m_ScanSummary = ScanSummary();
    m_ScanSummary.DeepRescan = false;

    for(Util::FileList::const_iterator i = list.begin(); i != list.end(); ++i)
    {  
        std::string insert_path ;
        std::string insert_path_sql ;

        try{
            insert_path = *i; 
#ifdef HAVE_HAL
            try{
                if (m_Flags & Library::F_USING_HAL)
                {
                    HAL::Volume const& volume (m_HAL->get_volume_for_uri (*i));
                    insert_path_sql = Glib::filename_from_uri(*i).substr (volume.mount_point.length()) ;
                }
                else
#endif
                {
                    insert_path_sql = *i; 
                }
#ifdef HAVE_HAL
            }
          catch (HAL::Exception & cxe)
            {
              g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
              return;
            }
          catch (Glib::ConvertError & cxe)
            {
              g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
              return;
            }
#endif // HAVE_HAL
           
            try{ 
                Util::collect_dirs(
                    insert_path,
                    sigc::bind(
                        sigc::mem_fun(
                                *this,
                                &LibraryScannerThread::on_scan_list_paths_callback
                        ),
                        last_scan_date,
                        insert_path_sql
                ));
            } catch(...) {
            }
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
            return;
        }
    }

    pthreaddata->ScanSummary.emit( m_ScanSummary );
}

void
MPX::LibraryScannerThread::on_scan_list_paths_callback( std::string const& i3, gint64 const& last_scan_date, std::string const& insert_path_sql )
{
    ThreadData * pthreaddata = m_ThreadData.get();

    try{
        gint64 ctime = Util::get_file_ctime( i3 );
        if( ctime > last_scan_date )
        {
            Util::FileList collection2;

            try{ 
                Util::collect_audio_paths( i3, collection2 );
            } catch(...) {
            }

            for(Util::FileList::iterator i2 = collection2.begin(); i2 != collection2.end(); ++i2)
            {
                Track track;

                try{
                    m_Library->trackSetLocation( track, *i2 );

                    try{
                        time_t mtime1 = Util::get_file_mtime(*i2);
                        time_t mtime2 = get_track_mtime( track ) ;

                        if( mtime2 != 0 && mtime1 == mtime2 ) 
                        {
                            ++m_ScanSummary.FilesUpToDate;
                        }
                        else
                        {
                            if( !m_MetadataReaderTagLib->get( *i2, track ) )
                            {
                                ++m_ScanSummary.FilesErroneous;
                                m_ScanSummary.FileListErroneous.push_back( SSFileInfo( *i2, _("Could not acquire metadata using taglib-gio")));
                            }
                            else try{

                                ScanResult status = insert( track, *i2, insert_path_sql );

                                switch( status )
                                {
                                    case SCAN_RESULT_OK:
                                        ++m_ScanSummary.FilesAdded;
                                        break;

                                    case SCAN_RESULT_UPDATE:
                                        ++m_ScanSummary.FilesUpdated;
                                        m_ScanSummary.FileListUpdated.push_back( SSFileInfo( *i2, _("Updated")));
                                        break;
                                }
                            }
                            catch( ScanError & cxe )
                            {
                                ++m_ScanSummary.FilesErroneous;
                                m_ScanSummary.FileListErroneous.push_back( SSFileInfo( *i2, (boost::format (_("Error inserting file: %s")) % cxe.what()).str()));
                            }
                            catch( Glib::ConvertError & cxe )
                            {
                                g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
                            }
                        }
                    } catch( Glib::Error & cxe ) { 
                        g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
                    }
                }
                catch( Library::FileQualificationError & cxe )
                {
                    ++(m_ScanSummary.FilesErroneous);
                    m_ScanSummary.FileListErroneous.push_back( SSFileInfo( *i2, cxe.what()));
                }
            }
        }
        
        m_ScanSummary.FilesTotal++;

        if( m_ScanSummary.FilesTotal % 50 )
        {
            pthreaddata->ScanRun.emit( m_ScanSummary.FilesTotal, false );
        }
    } catch( Glib::Error & cxe ) {
        g_warning("%s: Error: %s", G_STRLOC, cxe.what().c_str()); 
    }
}

void
MPX::LibraryScannerThread::on_scan_list_deep (Util::FileList const& list)
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_ScanStop, 0);

    pthreaddata->ScanStart.emit();
    m_SQL->exec_sql( "UPDATE album SET album_new = 0" );

    m_ScanSummary = ScanSummary();
    m_ScanSummary.DeepRescan = true;

    Util::FileList collection;

    for(Util::FileList::const_iterator i = list.begin(); i != list.end(); ++i)
    {  
        std::string insert_path ;
        std::string insert_path_sql ;
        Util::FileList collection;

        try{
            insert_path = *i; 
#ifdef HAVE_HAL
            try{
                if (m_Flags & Library::F_USING_HAL)
                { 
                    HAL::Volume const& volume (m_HAL->get_volume_for_uri (*i));
                    insert_path_sql = Glib::filename_from_uri(*i).substr (volume.mount_point.length()) ;
                }
                else
#endif
                {
                    insert_path_sql = *i; 
                }
#ifdef HAVE_HAL
            }
          catch (HAL::Exception & cxe)
            {
              g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
              return;
            }
          catch (Glib::ConvertError & cxe)
            {
              g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
              return;
            }
#endif // HAVE_HAL
            collection.clear();

            try{
                Util::collect_audio_paths_recursive( insert_path, collection );
            } catch(...) {
            }

            m_ScanSummary.FilesTotal += collection.size();
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
            return;
        }

        for(Util::FileList::iterator i2 = collection.begin(); i2 != collection.end(); ++i2)
        {
            if( g_atomic_int_get(&pthreaddata->m_ScanStop) )
            {
                pthreaddata->ScanSummary.emit( m_ScanSummary );
                return;
            }

            Track track;

            try{
                m_Library->trackSetLocation( track, *i2 );

                try{
                    time_t mtime1 = Util::get_file_mtime(*i2);
                    time_t mtime2 = get_track_mtime( track ) ;

                    if( mtime2 != 0 && mtime1 == mtime2 ) 
                    {
                        ++m_ScanSummary.FilesUpToDate;
                    }
                    else
                    {
                        if( !m_MetadataReaderTagLib->get( *i2, track ) )
                        {
                            ++m_ScanSummary.FilesErroneous;
                            m_ScanSummary.FileListErroneous.push_back( SSFileInfo( *i2, _("Could not acquire metadata using taglib-gio")));
                        }
                        else try{

                            ScanResult status = insert( track, *i2, insert_path_sql );

                            switch( status )
                            {
                                case SCAN_RESULT_OK:
                                    ++m_ScanSummary.FilesAdded;
                                    break;

                                case SCAN_RESULT_UPDATE:
                                    ++m_ScanSummary.FilesUpdated;
                                    m_ScanSummary.FileListUpdated.push_back( SSFileInfo( *i2, _("Updated")));
                                    break;
                            }
                        }
                        catch( ScanError & cxe )
                        {
                            ++m_ScanSummary.FilesErroneous;
                            m_ScanSummary.FileListErroneous.push_back( SSFileInfo( *i2, (boost::format (_("Error inserting file: %s")) % cxe.what()).str()));
                        }
                        catch( Glib::ConvertError & cxe )
                        {
                            g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
                        }
                    }
                } catch( Glib::Error & cxe ) { 
                    g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
                }
            }
            catch( Library::FileQualificationError & cxe )
            {
                ++(m_ScanSummary.FilesErroneous);
                m_ScanSummary.FileListErroneous.push_back( SSFileInfo( *i2, cxe.what()));
            }

                        
            if (! (std::distance(collection.begin(), i2) % 50) )
            {
                pthreaddata->ScanRun.emit(std::distance(collection.begin(), i2), true ); 
            }
        }
    }

    pthreaddata->ScanSummary.emit( m_ScanSummary );
}


void
MPX::LibraryScannerThread::on_scan_stop ()
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_ScanStop, 1);
}

gint64
MPX::LibraryScannerThread::get_track_artist_id (Track &track, bool only_if_exists)
{
  RowV rows; 

  char const* select_artist_f ("SELECT id FROM artist WHERE %s %s AND %s %s AND %s %s;"); 
  m_SQL->get (rows, mprintf (select_artist_f,

         attrs[ATTRIBUTE_ARTIST].id,
        (track[ATTRIBUTE_ARTIST]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ARTIST].get()).c_str()).c_str()
            : "IS NULL"),

         attrs[ATTRIBUTE_MB_ARTIST_ID].id,
        (track[ATTRIBUTE_MB_ARTIST_ID]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()).c_str()).c_str()
            : "IS NULL"),

         attrs[ATTRIBUTE_ARTIST_SORTNAME].id,
        (track[ATTRIBUTE_ARTIST_SORTNAME]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()).c_str()).c_str()
            : "IS NULL")));

  gint64 artist_j = 0;

  if (rows.size ())
  {
    artist_j = get <gint64> (rows[0].find ("id")->second);
  }
  else
  if (!only_if_exists)
  {
    char const* set_artist_f ("INSERT INTO artist (%s, %s, %s) VALUES (%Q, %Q, %Q);");

    m_SQL->exec_sql (mprintf (set_artist_f,

         attrs[ATTRIBUTE_ARTIST].id,
         attrs[ATTRIBUTE_ARTIST_SORTNAME].id,
         attrs[ATTRIBUTE_MB_ARTIST_ID].id,

        (track[ATTRIBUTE_ARTIST]
            ? get<std::string>(track[ATTRIBUTE_ARTIST].get()).c_str()
            : NULL) ,

        (track[ATTRIBUTE_ARTIST_SORTNAME]
            ? get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()).c_str()
            : NULL) ,

        (track[ATTRIBUTE_MB_ARTIST_ID]
            ? get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()).c_str()
            : NULL))) ;

    artist_j = m_SQL->last_insert_rowid ();
  }
  return artist_j;
}

gint64
MPX::LibraryScannerThread::get_album_artist_id (Track& track, bool only_if_exists)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if (track[ATTRIBUTE_ALBUM_ARTIST] && track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]) 
    {
      track[ATTRIBUTE_IS_MB_ALBUM_ARTIST] = gint64(1); 
    }
    else
    {
      track[ATTRIBUTE_IS_MB_ALBUM_ARTIST] = gint64(0); 
      track[ATTRIBUTE_ALBUM_ARTIST] = track[ATTRIBUTE_ARTIST];
    }

    RowV rows;

    if( track[ATTRIBUTE_MB_ALBUM_ARTIST_ID] )   
    {
      char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s);"); 
      m_SQL->get (rows, (mprintf (select_artist_f, 

         attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
        (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
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
        (track[ATTRIBUTE_ALBUM_ARTIST]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,
        (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
            ? "= '1'"
            : "IS NULL"))

       )) ; 
    }

    gint64 artist_j = 0;

    if (!rows.empty())
    {
      artist_j = get <gint64> (rows[0].find ("id")->second);
    }
    else if (!only_if_exists)
    {
      char const* set_artist_f ("INSERT INTO album_artist (%s, %s, %s, %s) VALUES (%Q, %Q, %Q, %Q);");

      m_SQL->exec_sql (mprintf (set_artist_f,

        attrs[ATTRIBUTE_ALBUM_ARTIST].id,
        attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id,
        attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
        attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,

      (track[ATTRIBUTE_ALBUM_ARTIST]
          ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str() 
          : NULL) , 

      (track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME] 
          ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].get()).c_str()
          : NULL) , 

      (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
          ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()
          : NULL) ,

      (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
          ? "1"
          : NULL))) ;

      artist_j = m_SQL->last_insert_rowid (); //FIXME: not threadsafe, see get_tag_id

      pthreaddata->NewArtist.emit( artist_j ); 
    }
    return artist_j;
}

gint64
MPX::LibraryScannerThread::get_album_id (Track& track, gint64 album_artist_id, bool only_if_exists)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    gint64 album_j = 0;
    RowV rows;
    std::string sql;

    if( track[ATTRIBUTE_MB_ALBUM_ID] )
    {
      char const* select_album_f ("SELECT album, id, mb_album_id FROM album WHERE (%s %s) AND (%s %s) AND (%s %s) AND (%s = %lld);"); 

      sql = mprintf (select_album_f,

             attrs[ATTRIBUTE_MB_ALBUM_ID].id,
            (track[ATTRIBUTE_MB_ALBUM_ID]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_MB_RELEASE_DATE].id,
            (track[ATTRIBUTE_MB_RELEASE_DATE]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()).c_str()).c_str()
                : "IS NULL"), 

             attrs[ATTRIBUTE_MB_RELEASE_COUNTRY].id,
            (track[ATTRIBUTE_MB_RELEASE_COUNTRY]
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
            (track[ATTRIBUTE_ALBUM]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()).c_str()
                : "IS NULL"), 

            attrs[ATTRIBUTE_ASIN].id,
            (track[ATTRIBUTE_ASIN]
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
        album_j = get <gint64> (rows[0].find ("id")->second);

        if(rows[0].count("mb_album_id"))
            track[ATTRIBUTE_MB_ALBUM_ID] = get<std::string> (rows[0].find ("mb_album_id")->second);
    }
    else
    if(!only_if_exists)
    {
      bool custom_id = false;

      if(!track[ATTRIBUTE_MB_ALBUM_ID])
      {
        track[ATTRIBUTE_MB_ALBUM_ID] = "mpx-" + get<std::string>(track[ATTRIBUTE_ARTIST].get())
                                              + "-"
                                              + get<std::string>(track[ATTRIBUTE_ALBUM].get()); 
        custom_id = true;
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

          (track[ATTRIBUTE_ALBUM]
              ? get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()
              : NULL) , 

          (track[ATTRIBUTE_MB_ALBUM_ID]
              ? get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()).c_str()
              : NULL) , 

          (track[ATTRIBUTE_MB_RELEASE_DATE]
              ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()).c_str()
              : NULL) , 

          (track[ATTRIBUTE_MB_RELEASE_COUNTRY]
              ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_COUNTRY].get()).c_str()
              : NULL) , 

          (track[ATTRIBUTE_MB_RELEASE_TYPE]
              ? get<std::string>(track[ATTRIBUTE_MB_RELEASE_TYPE].get()).c_str()
              : NULL) , 

          (track[ATTRIBUTE_ASIN]
              ? get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()
              : NULL) , 

          album_artist_id,

          get<gint64>(track[ATTRIBUTE_MTIME].get())

      );

      m_SQL->exec_sql (sql);
      album_j = m_SQL->last_insert_rowid ();

      pthreaddata->NewAlbum.emit( album_j );

      RequestQualifier rq;
      rq.mbid       = get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get());
      rq.asin       = track[ATTRIBUTE_ASIN]
                            ? get<std::string>(track[ATTRIBUTE_ASIN].get())
                            : "";

      rq.uri        = get<std::string>(track[ATTRIBUTE_LOCATION].get());
      rq.artist     = track[ATTRIBUTE_ALBUM_ARTIST]
                            ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get())
                            : get<std::string>(track[ATTRIBUTE_ARTIST].get());

      rq.album      = get<std::string>(track[ATTRIBUTE_ALBUM].get());

      pthreaddata->CacheCover( rq );
    }
    return album_j;
}

gint64
MPX::LibraryScannerThread::get_track_mtime (Track& track) const
{
  RowV rows;
  if( m_Flags & Library::Library::F_USING_HAL )
  {
    static boost::format
      select_f ("SELECT %s FROM track WHERE %s='%s' AND %s='%s' AND %s='%s';");

    m_SQL->get (rows, (select_f  % attrs[ATTRIBUTE_MTIME].id
                                % attrs[ATTRIBUTE_HAL_VOLUME_UDI].id
                                % get<std::string>(track[ATTRIBUTE_HAL_VOLUME_UDI].get())
                                % attrs[ATTRIBUTE_HAL_DEVICE_UDI].id
                                % get<std::string>(track[ATTRIBUTE_HAL_DEVICE_UDI].get())
                                % attrs[ATTRIBUTE_VOLUME_RELATIVE_PATH].id
                                % mprintf ("%q", get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str())).str());
  }
  else
  {
    static boost::format
      select_f ("SELECT %s FROM track WHERE %s='%s';");

    m_SQL->get (rows, (select_f  % attrs[ATTRIBUTE_MTIME].id
                                % attrs[ATTRIBUTE_LOCATION].id
                                % mprintf ("%q", get<std::string>(track[ATTRIBUTE_LOCATION].get()).c_str())).str());
  }

  if (rows.size() && (rows[0].count (attrs[ATTRIBUTE_MTIME].id) != 0))
  {
    return boost::get<gint64>( rows[0].find (attrs[ATTRIBUTE_MTIME].id)->second ) ;
  }

  return 0;
}

gint64
MPX::LibraryScannerThread::get_track_id (Track& track) const
{
  RowV rows;
#ifdef HAVE_HAL
  if( m_Flags & Library::F_USING_HAL )
  {
    static boost::format
      select_f ("SELECT id FROM track WHERE %s='%s' AND %s='%s' AND %s='%s';");

    m_SQL->get (rows, (select_f % attrs[ATTRIBUTE_HAL_VOLUME_UDI].id 
                                % get<std::string>(track[ATTRIBUTE_HAL_VOLUME_UDI].get())
                                % attrs[ATTRIBUTE_HAL_DEVICE_UDI].id
                                % get<std::string>(track[ATTRIBUTE_HAL_DEVICE_UDI].get())
                                % attrs[ATTRIBUTE_VOLUME_RELATIVE_PATH].id
                                % mprintf ("%q", get<std::string>(track[ATTRIBUTE_VOLUME_RELATIVE_PATH].get()).c_str())).str());
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

ScanResult
MPX::LibraryScannerThread::insert (Track & track, const std::string& uri, const std::string& insert_path)
{
  static boost::format delete_track_f ("DELETE FROM track WHERE id='%lld';");

  if( uri.empty() )
  {
    throw ScanError(_("Empty URI/no URI given"));
  }

  ThreadData * pthreaddata = m_ThreadData.get();

  if( !(track[ATTRIBUTE_ALBUM] && track[ATTRIBUTE_ARTIST] && track[ATTRIBUTE_TITLE]) )
  {
    // check if the track already exists; if so, metadata has been removed
    gint64 id = get_track_id( track );
    if( id != 0 )
    {
        m_SQL->exec_sql ((delete_track_f % id).str()); 
        pthreaddata->EntityDeleted.emit( id , ENTITY_TRACK ); 
    }

    // FIXME/TODO: Still throw if deleted?
    throw ScanError(_("Insufficient Metadata (artist, album and title must be given)"));
  }

  if( (!track[ATTRIBUTE_DATE]) && (track[ATTRIBUTE_MB_RELEASE_DATE]))
  {
    std::string mb_date = get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get());
    int mb_date_int = 0;
    if (sscanf(mb_date.c_str(), "%04d", &mb_date_int) == 1)
    {
        track[ATTRIBUTE_DATE] = gint64(mb_date_int);
    }
  }

  track[ATTRIBUTE_INSERT_PATH] = insert_path;
  track[ATTRIBUTE_INSERT_DATE] = gint64(time(NULL));

  char const* track_set_f ("INSERT INTO track (%s) VALUES (%s);");

  /* Now let's prepare the SQL */
  std::string column_names;
  column_names.reserve (0x400);

  std::string column_values;
  column_values.reserve (0x400);

  try{
      gint64 artist_j = get_track_artist_id (track);
      gint64 album_j = get_album_id (track, get_album_artist_id (track));

      for (unsigned int n = 0; n < N_ATTRIBUTES_INT; ++n)
      {
          if(track[n])
          {
              switch (attrs[n].type)
              {
                case VALUE_TYPE_STRING:
                  column_values += mprintf ("'%q'", get <std::string> (track[n].get()).c_str());
                  break;

                case VALUE_TYPE_INT:
                  column_values += mprintf ("'%lld'", get <gint64> (track[n].get()));
                  break;

                case VALUE_TYPE_REAL:
                  column_values += mprintf ("'%f'", get <double> (track[n].get()));
                  break;
              }

              column_names += std::string (attrs[n].id) + ",";
              column_values += ",";
          }
      }

      column_names += "artist_j, album_j";
      column_values += mprintf ("'%lld'", artist_j) + "," + mprintf ("'%lld'", album_j); 
      m_SQL->exec_sql (mprintf (track_set_f, column_names.c_str(), column_values.c_str()));
      track[ATTRIBUTE_MPX_TRACK_ID] = m_SQL->last_insert_rowid ();
      pthreaddata->NewTrack.emit( track, album_j, artist_j ); 
  }
  catch (SqlConstraintError & cxe)
  {
      gint64 id = get_track_id (track);
      if( id != 0 )
      {
          m_SQL->exec_sql ((delete_track_f % id).str()); 
          m_SQL->exec_sql (mprintf (track_set_f, column_names.c_str(), column_values.c_str()));
          gint64 new_id = m_SQL->last_insert_rowid ();
          m_SQL->exec_sql (mprintf ("UPDATE track SET id = '%lld' WHERE id = '%lld';", id, new_id));
          return SCAN_RESULT_UPDATE ;
      }
      else
      {
          g_warning("%s: Got track ID 0 for internal track! Highly supicious! Please report!", G_STRLOC);
          throw ScanError(_("Got track ID 0 for internal track! Highly supicious! Please report!"));
      }
  }
  catch (SqlExceptionC & cxe)
  {
      g_message("%s: SQL Error: %s", G_STRFUNC, cxe.what());
      throw ScanError(cxe.what());
  }

  return SCAN_RESULT_OK ; 
}

void
MPX::LibraryScannerThread::remove_dangling () 
{
  ThreadData * pthreaddata = m_ThreadData.get();

  typedef std::tr1::unordered_set <gint64> IdSet;
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
                  m_SQL->exec_sql((delete_f % "artist" % (*i)).str());
                  pthreaddata->EntityDeleted( *i , ENTITY_ARTIST );
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
                  m_SQL->exec_sql((delete_f % "album" % (*i)).str());
                  pthreaddata->EntityDeleted( *i , ENTITY_ALBUM );
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
                  m_SQL->exec_sql((delete_f % "album_artist" % (*i)).str());
                  pthreaddata->EntityDeleted( *i , ENTITY_ALBUM_ARTIST );
  }
}

void
MPX::LibraryScannerThread::on_vacuum() 
{
  ThreadData * pthreaddata = m_ThreadData.get();

  pthreaddata->ScanStart.emit();

#ifdef HAVE_HAL
  typedef std::map<HAL::VolumeKey, std::string> VolMountPointMap;
#endif

  VolMountPointMap m;
  RowV rows;
  m_SQL->get (rows, "SELECT * FROM track");

  m_SQL->exec_sql("BEGIN");

  for( RowV::iterator i = rows.begin(); i != rows.end(); ++i )
  {
          std::string uri = get<std::string>(m_Library->sqlToTrack( *i, false )[ATTRIBUTE_LOCATION].get());

          if( !uri.empty() )
          {
                  pthreaddata->Message.emit((boost::format(_("Checking files for presence: %lld / %lld")) % std::distance(rows.begin(), i) % rows.size()).str());
                  try{
                          Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
                          if( !file->query_exists() )
                          {
                                  m_SQL->exec_sql((boost::format ("DELETE FROM track WHERE id = %lld") % get<gint64>((*i)["id"])).str()); 
                                  pthreaddata->EntityDeleted( get<gint64>((*i)["id"]) , ENTITY_TRACK );
                          }
                  } catch(Glib::Error) {
                          g_message(G_STRLOC ": Error while trying to test URI '%s' for presence", uri.c_str());
                  }
          }
  }

  remove_dangling ();

  m_SQL->exec_sql("COMMIT");

  pthreaddata->Message.emit(_("Vacuum process done."));
  pthreaddata->ScanEnd.emit();
}

#ifdef HAVE_HAL
void
MPX::LibraryScannerThread::on_vacuum_volume(
    const std::string& hal_device_udi,
    const std::string& hal_volume_udi
)
{
  ThreadData * pthreaddata = m_ThreadData.get();

  pthreaddata->ScanStart.emit();

  typedef std::map<HAL::VolumeKey, std::string> VolMountPointMap;

  VolMountPointMap m;
  RowV rows;
  m_SQL->get(
      rows,
      (boost::format ("SELECT * FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s'")
              % hal_device_udi
              % hal_volume_udi
      ).str());

  m_SQL->exec_sql("BEGIN");

  for( RowV::iterator i = rows.begin(); i != rows.end(); ++i )
  {
          std::string uri = get<std::string>(m_Library->sqlToTrack( *i, false )[ATTRIBUTE_LOCATION].get());

          if( !uri.empty() )
          {
              pthreaddata->Message((boost::format(_("Checking files for presence: %lld / %lld")) % std::distance(rows.begin(), i) % rows.size()).str());
              try{
                      Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
                      if( !file->query_exists() )
                      {
                              m_SQL->exec_sql((boost::format ("DELETE FROM track WHERE id = %lld") % get<gint64>((*i)["id"])).str()); 
                              pthreaddata->EntityDeleted( get<gint64>((*i)["id"]), ENTITY_TRACK );
                      }
              } catch(...) {
                      g_message(G_STRLOC ": Error while trying to test URI '%s' for presence", uri.c_str());
              }
        }
  }

  remove_dangling ();

  m_SQL->exec_sql("COMMIT");

  pthreaddata->Message((boost::format (_("Vacuum process done for [%s]:%s")) % hal_device_udi % hal_volume_udi).str());
  pthreaddata->ScanEnd.emit();
}
#endif // HAVE_HAL
