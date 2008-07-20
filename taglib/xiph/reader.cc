//
// (C) 2007 DEREZYNSKI Milosz
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <taglib-gio/fileref.h>
#include <glibmm.h>

// Plugin-specific include
#include <taglib-gio/taglib-gio.h>
#include <taglib-gio/fileref.h>
#include <taglib-gio/tfile.h>
#include <taglib-gio/tag.h>

#include <taglib-gio/oggfile.h>
#include <taglib-gio/vorbisfile.h>
#include <taglib-gio/vorbisproperties.h>
#include <taglib-gio/xiphcomment.h>

#include "reader.hh"

using namespace TagLib;

namespace MPX
{
    void
    metadata_get_xiph (TagLib::Ogg::XiphComment * comment, Track & track)
    {
      struct {
          int   datum;
          char* id;
      } mb_metadata_xiph[] = {
        { ATTRIBUTE_ALBUM_ARTIST,           "ALBUMARTIST"               },
        { ATTRIBUTE_ALBUM_ARTIST_SORTNAME,  "ALBUMARTISTSORT"           },
        { ATTRIBUTE_MB_ALBUM_ARTIST_ID,     "MUSICBRAINZ_ALBUMARTISTID" },
        { ATTRIBUTE_MB_TRACK_ID,            "MUSICBRAINZ_TRACKID"       },
        { ATTRIBUTE_MB_ALBUM_ID,            "MUSICBRAINZ_ALBUMID"       },
        { ATTRIBUTE_MB_ARTIST_ID,           "MUSICBRAINZ_ARTISTID"      },
        { ATTRIBUTE_ARTIST_SORTNAME,        "ARTISTSORT"                },
        { ATTRIBUTE_MB_RELEASE_DATE,        "DATE"                      },
        { ATTRIBUTE_ASIN,                   "ASIN"                      },
        { ATTRIBUTE_MUSICIP_PUID,           "MUSICIP_PUID"              },
        { ATTRIBUTE_MB_RELEASE_COUNTRY,     "RELEASECOUNTRY"            },
        { ATTRIBUTE_MB_RELEASE_TYPE,        "MUSICBRAINZ_ALBUMTYPE"     },
      };

      Ogg::FieldListMap const& map (comment->fieldListMap());
      for (unsigned int n = 0; n < G_N_ELEMENTS (mb_metadata_xiph); ++n)
      {
        Ogg::FieldListMap::ConstIterator iter = map.find (mb_metadata_xiph[n].id);
        if (iter != map.end())
        {
          StringList const& list (iter->second);
          if (!list[0].isNull() && !list[0].isEmpty()) 
          {
            track[mb_metadata_xiph[n].datum] = list[0].to8Bit(true);
          }
        }
      }
    }
} // end namespace Bmp
