#include "mpx/audio.hh"
#include "mpx/library-scanner-thread.hh"
#include "mpx/library.hh"
#include "mpx/sql.hh"
#include "mpx/types.hh"
#include "metadatareader-taglib.hh"
#include <queue>
#include <boost/ref.hpp>
#include <boost/format.hpp>

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

MPX::ScanData::ScanData ()
: added(0)
, erroneous(0)
, uptodate(0)
, updated(0)
{
}

struct MPX::LibraryScannerThread::ThreadData
{
    LibraryScannerThread::SignalScanStart_t ScanStart ;
    LibraryScannerThread::SignalScanRun_t   ScanRun ;
    LibraryScannerThread::SignalScanEnd_t   ScanEnd ;
    LibraryScannerThread::SignalReload_t    Reload ;
    LibraryScannerThread::SignalTrack_t     Track ;
    LibraryScannerThread::SignalNewAlbum_t  NewAlbum ;
    LibraryScannerThread::SignalNewArtist_t NewArtist ;

    int m_ScanStop;
};

MPX::LibraryScannerThread::LibraryScannerThread (MPX::SQL::SQLDB* obj_sql, MPX::MetadataReaderTagLib* obj_tagreader,gint64 flags)
: sigx::glib_threadable()
, scan(sigc::mem_fun(*this, &LibraryScannerThread::on_scan))
, scan_stop(sigc::mem_fun(*this, &LibraryScannerThread::on_scan_stop))
, signal_scan_start(*this, m_ThreadData, &ThreadData::ScanStart)
, signal_scan_run(*this, m_ThreadData, &ThreadData::ScanRun)
, signal_scan_end(*this, m_ThreadData, &ThreadData::ScanEnd)
, signal_reload(*this, m_ThreadData, &ThreadData::Reload)
, signal_track(*this, m_ThreadData, &ThreadData::Track)
, signal_new_album(*this, m_ThreadData, &ThreadData::NewAlbum)
, signal_new_artist(*this, m_ThreadData, &ThreadData::NewArtist)
, m_SQL(obj_sql)
, m_MetadataReaderTagLib(obj_tagreader)
, m_Flags(flags)
{
    m_Connectable = new ScannerConnectable(
            signal_scan_start,
            signal_scan_run,
            signal_scan_end,
            signal_reload,
            signal_track,
            signal_new_album,
            signal_new_artist
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
MPX::LibraryScannerThread::on_scan (ScanData const& scan_data_)
{
    ScanData scan_data = scan_data_;
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_ScanStop, 0);

    pthreaddata->ScanStart.emit();

    for(Util::FileList::iterator i = scan_data.collection.begin(); i != scan_data.collection.end(); ++i)
    {
        Track track;
        std::string type;

        try{ 
            if (!Audio::typefind(*i, type))  
            {
              ++(scan_data.erroneous) ;
              g_message("%s: No type for URI '%s'", G_STRLOC, (*i).c_str());
              continue;
            }
          }  
        catch (Glib::ConvertError & cxe)
          {
              ++(scan_data.erroneous) ;
              g_message("%s: Convert error for '%s'", G_STRLOC, (*i).c_str());
              continue;
          }   

        track[ATTRIBUTE_TYPE] = type ;
        track[ATTRIBUTE_LOCATION] = *i ;
        track[ATTRIBUTE_LOCATION_NAME] = scan_data.name;

        if( !m_MetadataReaderTagLib->get( *i, track ) )
        {
           ++(scan_data.erroneous) ;
           g_message("%s: Couldn't read metadata off '%s'", G_STRLOC, (*i).c_str());
           continue;
        }

        try{
            ScanResult status;
            pthreaddata->Track.emit( track, *i , scan_data.insert_path_sql, boost::ref(status) );

            switch(status)
            {
                case SCAN_RESULT_UPTODATE:
                    ++(scan_data.uptodate) ;
                    break;

                case SCAN_RESULT_OK:
                    ++(scan_data.added) ;
                    break;

                case SCAN_RESULT_ERROR:
                    ++(scan_data.erroneous) ;
                    break;

                case SCAN_RESULT_UPDATE:
                    ++(scan_data.updated) ;
                    break;
            }
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
        }

        ++(scan_data.position);
        pthreaddata->ScanRun.emit(std::distance(scan_data.collection.begin(), i), scan_data.collection.size());
    }

    pthreaddata->ScanEnd.emit(scan_data.added, scan_data.uptodate, scan_data.updated, scan_data.erroneous, scan_data.collection.size());

    
}

void
MPX::LibraryScannerThread::on_scan_stop ()
{
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
      char const* select_album_f ("SELECT album, id, mb_album_id FROM album WHERE (%s %s) AND (%s %s) AND (%s %s) AND (%s %s) AND (%s = %lld);"); 

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

      char const* set_album_f ("INSERT INTO album (%s, %s, %s, %s, %s, %s) VALUES (%Q, %Q, %Q, %Q, %lld, %lld);");

      std::string sql = mprintf (set_album_f,

          attrs[ATTRIBUTE_ALBUM].id,
          attrs[ATTRIBUTE_MB_ALBUM_ID].id,
          attrs[ATTRIBUTE_MB_RELEASE_DATE].id,
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

          (track[ATTRIBUTE_ASIN]
              ? get<std::string>(track[ATTRIBUTE_ASIN].get()).c_str()
              : NULL) , 

          album_artist_id,
          get<gint64>(track[ATTRIBUTE_MTIME].get()));

      m_SQL->exec_sql (sql);
      album_j = m_SQL->last_insert_rowid ();
      pthreaddata->NewAlbum.emit( album_j ); 

#if 0
      if(!custom_id)
      {
          g_message("Fetching normal");
          m_Covers->cache( get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get())
                         , (track[ATTRIBUTE_ASIN]
                           ? get<std::string>(track[ATTRIBUTE_ASIN].get())
                           : std::string())
                         , get<std::string>(track[ATTRIBUTE_LOCATION].get())
                         , true );
      }
      else
      {
          g_message("Fetching custom");
          m_Covers->cache_inline( get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()),
                                 get<std::string>(track[ATTRIBUTE_LOCATION].get()) );
      }
#endif
    }
    return album_j;
}

gint64
MPX::LibraryScannerThread::get_track_mtime (Track& track) const
{
  RowV rows;
  if (m_Flags & Library::F_USING_HAL)
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

