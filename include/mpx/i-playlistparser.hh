// (c) 2007 M. Derezynski

#ifndef MPX_I_PLAYLIST_PARSER_HH
#define MPX_I_PLAYLIST_PARSER_HH

#include <string>
#include "mpx/util-file.hh"

namespace MPX
{
    class PlaylistParser
    {
        public:

          PlaylistParser () {}
          virtual ~PlaylistParser () {}

          virtual bool
          read (std::string const&, Util::FileList&) = 0;

          virtual bool
          write (std::string const&, Util::FileList const&) = 0;
    };
}

#endif // MPX_I_PLAYLIST_PARSER_HH
