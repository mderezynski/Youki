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

#include <taglib-gio/id3v2tag.h>
#include <taglib-gio/mpegfile.h>
#include <taglib-gio/id3v2framefactory.h>
#include <taglib-gio/textidentificationframe.h>
#include <taglib-gio/uniquefileidentifierframe.h>
#include <taglib-gio/attachedpictureframe.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "mpx/types.hh"

using namespace Glib;
using namespace TagLib;
using namespace std;

namespace
{
      using namespace MPX;

      TagLib::ID3v2::UserTextIdentificationFrame*
      find_utif (TagLib::ID3v2::Tag *tag, TagLib::String const& description)
      {
        TagLib::ID3v2::FrameList l = tag->frameList("TXXX");
        for(TagLib::ID3v2::FrameList::Iterator i = l.begin(); i != l.end(); ++i)
        {
          TagLib::ID3v2::UserTextIdentificationFrame * f (dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame *> (*i));
          if (f && f->description() == description)
            return f;
        }
        return 0;
      }
}

// we redeclare it locally here so we don't need to include BMP stuff

namespace MPX
{
    void
    metadata_get_id3v2 (TagLib::ID3v2::Tag * tag, Track & track)
    {
        using boost::algorithm::split;
        using boost::algorithm::find_nth;
        using boost::iterator_range;
        using TagLib::ID3v2::FrameList;

        FrameList const& list = tag->frameList();
        g_message("Listing Frames");
        for(FrameList::ConstIterator i = list.begin(); i != list.end(); ++i)
        {
            TagLib::ID3v2::Frame const* f = *i;
            TagLib::ID3v2::AttachedPictureFrame const* pf = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame const*>(f);
            if(pf)
                g_message(G_STRLOC ": Got Picture");
        }

        struct {
            int         datum;
            std::string id;
        } mb_metadata_id3v2[] = {
          { ATTRIBUTE_MB_ALBUM_ARTIST_ID,             "MusicBrainz Album Artist Id"   },
          { ATTRIBUTE_MB_ALBUM_ID,                    "MusicBrainz Album Id"          },
          { ATTRIBUTE_MB_ARTIST_ID,                   "MusicBrainz Artist Id"         },
          { ATTRIBUTE_ASIN,                           "ASIN"                          },
          { ATTRIBUTE_ALBUM_ARTIST_SORTNAME,          "ALBUMARTISTSORT"               },
          { ATTRIBUTE_MUSICIP_PUID,                   "MusicIP PUID"                  },
        };

        ID3v2::UserTextIdentificationFrame * frame = 0;
        for (unsigned int n = 0; n < G_N_ELEMENTS (mb_metadata_id3v2); ++n)
        {
            frame = find_utif (tag, String (mb_metadata_id3v2[n].id, String::UTF8));
            if(frame)
            {
                std::string s = frame->toString().toCString (true);
                iterator_range <std::string::iterator> match = find_nth (s, mb_metadata_id3v2[n].id + std::string(" "), 0);
                if(!match.empty())
                {
                    ustring substr (match.end(), s.end());
                    if(!substr.empty())
                    {
                        track[mb_metadata_id3v2[n].datum] = substr;
                    }
                }
            }
        }


        // MB UFID
        FrameList const& map = tag->frameListMap()["UFID"];
        if (!map.isEmpty())
        {
          for (FrameList::ConstIterator iter = map.begin(); iter != map.end(); ++iter)
          {
            ID3v2::UniqueFileIdentifierFrame *ufid = reinterpret_cast<ID3v2::UniqueFileIdentifierFrame*> (*iter);
            if (ufid->owner() == "http://musicbrainz.org")
            {
              ByteVector vec (ufid->identifier());
              vec.append ('\0');
              track[ATTRIBUTE_MB_TRACK_ID] = string (vec.data());
              break;
            }
          }
        }

        // TDRC 
        {
          FrameList const& map = tag->frameListMap()["TDRC"];
          if (!map.isEmpty())
          {
            track[ATTRIBUTE_MB_RELEASE_DATE] = string (map.front()->toString().toCString (true));
          }
        }

        // TPE2 (Album Artist) 
        {
          FrameList const& map = tag->frameListMap()["TPE2"];
          if (!map.isEmpty())
          {
            track[ATTRIBUTE_ALBUM_ARTIST] = string (map.front()->toString().toCString (true));
          }
        }


        // TSOP/XSOP (Artist Sort Name)
        {
          const char *id3v2frame = 0;

          if (!tag->frameListMap()["XSOP"].isEmpty())
            id3v2frame = "XSOP";
          else if (!tag->frameListMap()["TSOP"].isEmpty())
            id3v2frame = "TSOP";

          if (id3v2frame)
          {
            FrameList const& map = tag->frameListMap()[id3v2frame];
            if (!map.isEmpty())
            {
              track[ATTRIBUTE_ARTIST_SORTNAME] = string (map.front()->toString().toCString (true));
            }
          }
        }
    }

} // end namespace MPX
