#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glibmm.h>
#include <taglib-gio/fileref.h>
#include <taglib-gio/taglib-gio.h>
#include <taglib-gio/fileref.h>
#include <taglib-gio/tfile.h>
#include <taglib-gio/tag.h>
#include <taglib-gio/oggfile.h>
#include <taglib-gio/vorbisfile.h>
#include <taglib-gio/vorbisproperties.h>
#include <taglib-gio/xiphcomment.h>

#include "common/common.hh"
#include "xiph/reader.hh"
#include "mpx/types.hh"

using namespace MPX;
using namespace TagLib;
using namespace Glib;
using boost::get;

extern "C" int  _plugin_has_accessors;
int  _plugin_has_accessors = 1;

extern "C" int  _plugin_version;
int _plugin_version = PLUGIN_VERSION;

extern "C" const char ** _mimetypes ()
{
  static const char * _types[] =
  {
    "application/ogg",
    NULL
  };
  return _types;
}

extern "C" bool _set (std::string const& filename, Track & track)
{
  Ogg::Vorbis::File opfile (filename.c_str());
  if (!metadata_check_file (&opfile))
    return false;

  Ogg::XiphComment * comment = opfile.tag();
  if (comment)
  {
    metadata_set_common (&opfile, track);

    if (track[ATTRIBUTE_ALBUM_ARTIST])
      comment->addField (String ("ALBUMARTIST",                         String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST].get()),           String::UTF8));

    if (track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME])
      comment->addField (String ("ALBUMARTISTSORT",                     String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME].get()), String::UTF8));

    if (track[ATTRIBUTE_MB_ALBUM_ARTIST_ID])
      comment->addField (String ("MUSICBRAINZ_ALBUMARTISTID",           String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_MB_ALBUM_ARTIST_ID].get()),        String::UTF8));

    if (track[ATTRIBUTE_MB_TRACK_ID])
      comment->addField (String ("MUSICBRAINZ_TRACKID",                 String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_MB_TRACK_ID].get()),               String::UTF8));

    if (track[ATTRIBUTE_MB_ALBUM_ID])
      comment->addField (String ("MUSICBRAINZ_ALBUMID",                 String::UTF8),
                         String(get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get()),                String::UTF8));

    if (track[ATTRIBUTE_MB_ARTIST_ID])
      comment->addField (String ("MUSICBRAINZ_ARTISTID",                String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get()),              String::UTF8));

    if (track[ATTRIBUTE_ARTIST_SORTNAME])
      comment->addField (String ("ARTISTSORT",                          String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_ARTIST_SORTNAME].get()),       String::UTF8));

    if (track[ATTRIBUTE_MB_RELEASE_DATE])
      comment->addField (String ("DATE", String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_MB_RELEASE_DATE].get()), String::UTF8));

    if (track[ATTRIBUTE_ASIN])
      comment->addField (String ("ASIN",            String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_ASIN].get()),  String::UTF8));

    if (track[ATTRIBUTE_MUSICIP_PUID])
      comment->addField (String ("MUSICIP_PUID",   String::UTF8),
                         String (get<std::string>(track[ATTRIBUTE_MUSICIP_PUID].get()), String::UTF8));


    opfile.save ();
    return true;
  }
  else
  {
    return false;
  }
}

extern "C" bool _get (std::string const& filename, Track & track)
{
  Ogg::Vorbis::File opfile (filename.c_str());
  if (!metadata_check_file (&opfile))
    return false;

  Ogg::XiphComment * comment = opfile.tag();
  if (comment)
  {
    metadata_get_xiph (comment, track);
    metadata_get_common (&opfile, track);
    return true;
  }

  return false;
}
