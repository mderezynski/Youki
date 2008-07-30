// (C) 2006 M. Derezynski

#define MPX_PLUGIN_BUILD 1

#include <string>
#include "resolver.h"
#include "modfile.h"
#include "mpx/mpx-audio.hh"

TagLib::File * MODFileTypeResolver::createFile (const char                        *filename,
                                                bool                               read_properties,
                                                TagLib::AudioProperties::ReadStyle properties_style) const 
{
    std::string type;

    if (!MPX::Audio::typefind (filename, type))
      return 0;

    if (type == "audio/x-mod")
    {
      TagLib::MOD::File * p = new TagLib::MOD::File(filename, read_properties, properties_style);

      if (p->isValid())
          return p;
      else
          delete p;
    }

    return 0;
}
