// (c) 2007 M. Derezynski

#ifndef MPX_I_METADATA_READER_HH
#define MPX_I_METADATA_READER_HH

#include <string>
#include "mpx/types.hh"

namespace MPX
{
    class MetadataReader
    {
        MetadataReader () {}
        virtual ~MetadataReader () {}

        bool
        get (std::string const& uri, Track & track) = 0;

        bool
        set (std::string const& uri, Track & track) = 0;
    };
}

#endif // MPX_I_METADATA_READER_HH
