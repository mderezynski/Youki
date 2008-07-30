//
// (C) 2007 M.Derezynski
//

#ifndef MPX_XIPH_METADATA_READER_HH
#define MPX_XIPH_METADATA_READER_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <taglib-gio/oggfile.h>
#include <taglib-gio/vorbisfile.h>
#include <taglib-gio/vorbisproperties.h>
#include <taglib-gio/xiphcomment.h>

#include "mpx/mpx-types.hh"

namespace MPX
{
    void  metadata_get_xiph (TagLib::Ogg::XiphComment*, Track&);
}

#endif // ! MPX_XIPH_METADATA_READER_HH
