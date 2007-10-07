// (C) 2006 M. Derezynski

#define MPX_PLUGIN_BUILD 1

#include "resolver.h"
#include "sidfile.h"
#include "mpx/audio.hh"

TagLib::File *SIDFileTypeResolver::createFile (const char                        *filename,
                                               bool                               read_properties,
                                               TagLib::AudioProperties::ReadStyle properties_style) const 
{
    std::string type;
    
    if (!MPX::Audio::typefind (filename, type))
      return 0;

    if (type == "audio/x-sid")
    {
      TagLib::SID::File * p = new TagLib::SID::File(filename, read_properties, properties_style);

      if (p->isValid())
          return p;
      else
          delete p;
    }

    return 0;
}
