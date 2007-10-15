#ifndef MPX_URI_HH
#define MPX_URI_HH

#include <map>
#include <string>
#include <glibmm/ustring.h>

using namespace Glib;

namespace MPX
{
    class URI
    {
      public:

#include "exception.hh"
        EXCEPTION(ParseError)

        typedef std::pair <ustring, ustring >   QElement;
        typedef std::map  <ustring, QElement >	Query;
        typedef std::pair <ustring, QElement >	QueryPair;

        enum Protocol
        {
          PROTOCOL_UNKNOWN = -1,
          PROTOCOL_FILE,
          PROTOCOL_CDDA,
          PROTOCOL_HTTP,
          PROTOCOL_FTP,
          PROTOCOL_QUERY,
          PROTOCOL_TRACK,
          PROTOCOL_MMS, 
          PROTOCOL_MMSU,
          PROTOCOL_MMST,
          PROTOCOL_LASTFM,
          PROTOCOL_ITPC
        };

        URI ();
        URI (ustring const &uri, bool escape = false);
        URI (URI const& uri) { *this  = uri; }
        ~URI  () {};

        Protocol            get_protocol ();
        void                set_protocol (Protocol p);
        static std::string  get_protocol_scheme (Protocol p);
        static std::string  escape_string (std::string const& string);
        static std::string  unescape_string (std::string const& string);
        void                escape ();
        void                unescape ();
        std::string         fullpath () const;
        void                parse_query (Query & q);

        operator ustring () const;

        ustring	scheme;
        ustring	userinfo;
        ustring	hostname;
        ustring	path;
        ustring	query;
        ustring	fragment;
        int		  port;

      private:

        bool fragmentize (ustring const& uri);

    };
}

#endif //!MPX_URI_HH
