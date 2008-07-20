#include "mpx/audio.hh"
#include "mpx/hal.hh"
#include "mpx/library-scanner-thread.hh"
#include "mpx/library.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/sql.hh"
#include "mpx/types.hh"
#include <queue>
#include <boost/ref.hpp>
#include <boost/format.hpp>
#include <gio/gio.h>

using boost::get;
using namespace MPX;
using namespace MPX::SQL;

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
    LibraryScannerThread::SignalReload_t            Reload ;
    LibraryScannerThread::SignalNewAlbum_t          NewAlbum ;
    LibraryScannerThread::SignalNewArtist_t         NewArtist ;
    LibraryScannerThread::SignalCacheCover_t        CacheCover ;
    LibraryScannerThread::SignalTrack_t             NewTrack ;

    int m_ScanStop;
};

MPX::LibraryScannerThread::LibraryScannerThread (MPX::SQL::SQLDB* obj_sql, MPX::MetadataReaderTagLib & obj_tagreader, MPX::HAL & obj_hal, gint64 flags)
: sigx::glib_threadable()
, scan(sigc::mem_fun(*this, &LibraryScannerThread::on_scan))
, scan_stop(sigc::mem_fun(*this, &LibraryScannerThread::on_scan_stop))
, signal_scan_start(*this, m_ThreadData, &ThreadData::ScanStart)
, signal_scan_run(*this, m_ThreadData, &ThreadData::ScanRun)
, signal_scan_end(*this, m_ThreadData, &ThreadData::ScanEnd)
, signal_reload(*this, m_ThreadData, &ThreadData::Reload)
, signal_new_album(*this, m_ThreadData, &ThreadData::NewAlbum)
, signal_new_artist(*this, m_ThreadData, &ThreadData::NewArtist)
, signal_cache_cover(*this, m_ThreadData, &ThreadData::CacheCover)
, signal_track(*this, m_ThreadData, &ThreadData::NewTrack)
, m_SQL(obj_sql)
, m_MetadataReaderTagLib(obj_tagreader)
, m_HAL(obj_hal)
, m_Flags(flags)
{
    m_Connectable = new ScannerConnectable(
            signal_scan_start,
            signal_scan_run,
            signal_scan_end,
            signal_reload,
            signal_new_album,
            signal_new_artist,
            signal_cache_cover,
            signal_track
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
MPX::LibraryScannerThread::on_scan (Util::FileList const& list)
{
    if(list.empty())
    {
        return;
    }

    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_ScanStop, 0);

    pthreaddata->ScanStart.emit();
    m_SQL->exec_sql( "UPDATE album SET album_new = 0" );

    gint64 added = 0, erroneous = 0, uptodate = 0, updated = 0, total = 0;

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
                    HAL::Volume const& volume (m_HAL.get_volume_for_uri (*i));
                    insert_path_sql = filename_from_uri(*i).substr (volume.mount_point.length()) ;
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
            Util::collect_audio_paths( insert_path, collection );
            total += collection.size();
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
            return;
        }

        if(collection.empty())
        {
            pthreaddata->ScanEnd.emit(added, uptodate, updated, erroneous, collection.size());
            return;
        }

        for(Util::FileList::iterator i = collection.begin(); i != collection.end(); ++i)
        {
            if( g_atomic_int_get(&pthreaddata->m_ScanStop) )
            {
                pthreaddata->ScanEnd.emit(added, uptodate, updated, erroneous, collection.size());
                pthreaddata->Reload.emit();
                return;
            }

            Track track;
            std::string type;

            track[ATTRIBUTE_LOCATION] = *i ;

            try{

                URI u (*i, true);

                if(u.get_protocol() == URI::PROTOCOL_FILE)
                {

                    bool err_occured = false;

#ifdef HAVE_HAL
                    try{

                        if (m_Flags & Library::F_USING_HAL)
                        {
                          HAL::Volume const& volume (m_HAL.get_volume_for_uri (*i));

                          track[ATTRIBUTE_HAL_VOLUME_UDI] =
                                        volume.volume_udi ;

                          track[ATTRIBUTE_HAL_DEVICE_UDI] =
                                        volume.device_udi ;

                          track[ATTRIBUTE_VOLUME_RELATIVE_PATH] =
                                        filename_from_uri (*i).substr (volume.mount_point.length()) ;
                        }

                    }
                  catch (HAL::Exception & cxe)
                    {
                      g_warning( "%s: %s", G_STRLOC, cxe.what() ); 
                      ++(erroneous);
                      err_occured  = true;
                    }
                  catch (Glib::ConvertError & cxe)
                    {
                      g_warning( "%s: %s", G_STRLOC, cxe.what().c_str() ); 
                      ++(erroneous);
                      err_occured  = true;
                    }
#endif

                    if( !err_occured )
                    {
                            try{ 
                                if (!Audio::typefind(*i, type))  
                                {
                                  ++(erroneous) ;
                                  continue;
                                }
                              }  
                            catch (Glib::ConvertError & cxe)
                              {
                                  ++(erroneous) ;
                                  continue;
                              }   

                            track[ATTRIBUTE_TYPE] = type ;

                            GFile * file = g_vfs_get_file_for_uri(g_vfs_get_default(), (*i).c_str()); 
                            GFileInfo * info = g_file_query_info(file,
                                                              G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                                              GFileQueryInfoFlags(0),
                                                              NULL,
                                                              NULL);

                            track[ATTRIBUTE_MTIME] = gint64 (g_file_info_get_attribute_uint64(info,G_FILE_ATTRIBUTE_TIME_MODIFIED));

                            g_object_unref(file);
                            g_object_unref(info);

                            time_t mtime = get_track_mtime (track);
                            if (mtime != 0 && mtime == get<gint64>(track[ATTRIBUTE_MTIME].get()))
                            {
                                ++(uptodate);
                            }
                            else
                            {
                                if( !m_MetadataReaderTagLib.get( *i, track ) )
                                {
                                   ++(erroneous) ;
                                }
                                else try{
                                    ScanResult status = insert( track, *i, insert_path_sql );

                                    switch(status)
                                    {
                                        case SCAN_RESULT_OK:
                                            ++(added) ;
                                            break;

                                        case SCAN_RESULT_ERROR:
                                            ++(erroneous) ;
                                            break;

                                        case SCAN_RESULT_UPDATE:
                                            ++(updated) ;
                                            break;
                                    }
                                }
                                catch( Glib::ConvertError & cxe )
                                {
                                    g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
                                }
                            }
                    }
                }
            } catch(URI::ParseError)
            {
            }


            pthreaddata->ScanRun.emit(std::distance(collection.begin(), i), collection.size());
        }
    }

    pthreaddata->ScanEnd.emit(added, uptodate, updated, erroneous, total);
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
      char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s) AND (%s %s) AND (%s %s);"); 
      m_SQL->get (rows, (mprintf (select_artist_f, 

         attrs[ATTRIBUTE_ALBUM_ARTIST].id,
        (track[ATTRIBUTE_ALBUM_ARTIST]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
        (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].id,
        (track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_IS_MB_ALBUM_ARTIST].id,
        (get<gint64>(track[ATTRIBUTE_IS_MB_ALBUM_ARTIST].get())
            ? "= '1'"
            : "IS NULL")))
      );
    }
    else // TODO: MB lookup to fix sortname, etc, for a given ID (and possibly even retag files on the fly?)
    {
      char const* select_artist_f ("SELECT id FROM album_artist WHERE (%s %s) AND (%s %s);"); 
      m_SQL->get (rows, (mprintf (select_artist_f, 

         attrs[ATTRIBUTE_ALBUM_ARTIST].id,
        (track[ATTRIBUTE_ALBUM_ARTIST]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()).c_str()).c_str()
            : "IS NULL"), 

         attrs[ATTRIBUTE_MB_ALBUM_ARTIST_ID].id,
        (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID]
            ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()).c_str()).c_str()
            : "IS NULL")))) ; 
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
      char const* select_album_f ("SELECT album, id, mb_album_id FROM album WHERE (%s %s) AND (%s %s) AND (%s %s) AND (%s %s) AND (%s %s) AND (%s %s) AND (%s = %lld);"); 

      sql = mprintf (select_album_f,

             attrs[ATTRIBUTE_ALBUM].id,
            (track[ATTRIBUTE_ALBUM]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ALBUM].get()).c_str()).c_str()
                : "IS NULL"), 

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

             attrs[ATTRIBUTE_MB_RELEASE_TYPE].id,
            (track[ATTRIBUTE_MB_RELEASE_TYPE]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_MB_RELEASE_TYPE].get()).c_str()).c_str()
                : "IS NULL"), 

            attrs[ATTRIBUTE_ASIN].id,
            (track[ATTRIBUTE_ASIN]
                ? mprintf (" = '%q'", get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()).c_str()
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

      pthreaddata->CacheCover(

                       get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get())

                     , track[ATTRIBUTE_ASIN]
                            ? get<std::string>(track[ATTRIBUTE_ASIN].get())
                            : ""

                     , get<std::string>(track[ATTRIBUTE_LOCATION].get())

                     , track[ATTRIBUTE_ALBUM_ARTIST]
                            ? get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get())
                            : get<std::string>(track[ATTRIBUTE_ARTIST].get())

                     , get<std::string>(track[ATTRIBUTE_ALBUM].get()) 
      );

    }
    return album_j;
}

gint64
MPX::LibraryScannerThread::get_track_mtime (Track& track) const
{
  RowV rows;
  if (m_Flags & Library::Library::F_USING_HAL)
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
  if ((m_Flags & Library::F_USING_HAL) == Library::F_USING_HAL)
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
  g_return_val_if_fail(!uri.empty(), SCAN_RESULT_ERROR);

  ThreadData * pthreaddata = m_ThreadData.get();

  if( !(track[ATTRIBUTE_ALBUM] && track[ATTRIBUTE_ARTIST] && track[ATTRIBUTE_TITLE]) )
  {
    return SCAN_RESULT_ERROR ;
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
          static boost::format delete_track_f ("DELETE FROM track WHERE id='%lld';");
          m_SQL->exec_sql ((delete_track_f % id).str()); 
          m_SQL->exec_sql (mprintf (track_set_f, column_names.c_str(), column_values.c_str()));
          gint64 new_id = m_SQL->last_insert_rowid ();
          m_SQL->exec_sql (mprintf ("UPDATE track SET id = '%lld' WHERE id = '%lld';", id, new_id));
          return SCAN_RESULT_UPDATE ;
      }
      else
      {
          g_warning("%s: Got track ID 0 for internal track! Highly supicious! Please report!", G_STRLOC);
          return SCAN_RESULT_ERROR ;
      }
  }
  catch (SqlExceptionC & cxe)
  {
      g_message("%s: SQL Error: %s", G_STRFUNC, cxe.what());
      return SCAN_RESULT_ERROR ;
  }

  return SCAN_RESULT_OK ; 
}



