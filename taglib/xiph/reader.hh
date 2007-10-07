//
// (C) 2007 M.Derezynski
//

#ifndef MPX_XIPH_METADATA_READER_HH
#define MPX_XIPH_METADATA_READER_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/vorbisproperties.h>
#include <taglib/xiphcomment.h>

#include "mpx/types.hh"

namespace MPX
{
    void  metadata_get_xiph (TagLib::Ogg::XiphComment*, Track&);
}

#endif // ! MPX_XIPH_METADATA_READER_HH
