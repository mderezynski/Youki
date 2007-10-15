// (c) 2007 Milosz Derezynski
// See COPYING file for licensing information

#define MPX_PLUGIN_BUILD 1

#include "resolver.h"
#include "asffile.h"
#include "mpx/audio.hh"

TagLib::File *ASFFileTypeResolver::createFile (const char                        *filename,
                                               bool                               read_properties,
                                               TagLib::AudioProperties::ReadStyle properties_style) const 
{
    std::string type;

    if (!MPX::Audio::typefind (filename, type))
      return 0;

    if (type == "video/x-ms-asf")
    {
      TagLib::ASF::File * p = new TagLib::ASF::File (filename, read_properties, properties_style);

      if (p->isValid())
          return p;
      else
          delete p;
    }

    return 0;
}
