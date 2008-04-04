// (c) 2007 M. Derezynski

#ifndef MPX_I_PLAYLIST_PARSER_XSPF_HH
#define MPX_I_PLAYLIST_PARSER_XSPF_HH

#include <string>
#include "mpx/i-playlistparser.hh"
#include "mpx/util-file.hh"

namespace MPX
{
    class PlaylistParserXSPF
		:	public PlaylistParser
    {
        public:

          PlaylistParserXSPF ();
          virtual ~PlaylistParserXSPF (); 

          virtual bool
          read (std::string const&, Util::FileList&);

          virtual bool
          write (std::string const&, Util::FileList const&);
    };
}

#endif // MPX_I_PLAYLIST_PARSER_HH
