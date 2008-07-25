// (c) 2007 M. Derezynski

#include <glib/gmacros.h>
#include <glib/gmessages.h>
#include <string>
#include "mpx/playlistparser-pls.hh"
#include "xmlcpp/xspf.hxx"
#include "mpx/xmltoc++.hh"

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "mpx/util-string.hh"
#include "mpx/uri.hh"

using namespace Glib;
using namespace MPX;
using namespace std;

namespace
{
  typedef map < string, string > StringMap;

  void
  parse_to_map (StringMap& map, string const& buffer)
  {
    using boost::algorithm::split;
    using boost::algorithm::split_regex;
    using boost::algorithm::is_any_of;

    vector<string> lines;
    split_regex (lines, buffer, boost::regex ("\\\r?\\\n"));

    for (unsigned int n = 0; n < lines.size(); ++n)
    {
      char **line = g_strsplit (lines[n].c_str(), "=", 2);
      if (line[0] && line[1] && strlen(line[0]) && strlen(line[1]))
      {
        ustring key (line[0]);
        map[std::string (key.lowercase())] = line[1];
      }
      g_strfreev (line);
    }
  }
}


namespace MPX
{
namespace PlaylistParser
{
	PLS::PLS ()
	{
	}

	PLS::~PLS ()
	{
	}

	bool
	PLS::read (std::string const& uri, Track_v & v)
	{
		try{
            StringMap map;
            parse_to_map (map, file_get_contents(filename_from_uri(uri)));

            int n = boost::lexical_cast<int>(map.find("numberofentries")->second);

            for (int a = 1; a <= n ; ++a)
            {
              MPX::Track t;
              t[ATTRIBUTE_LOCATION] = map[(boost::format("file%d") % n).str()]; 
              t[ATTRIBUTE_TITLE] = map[(boost::format("title%d") % n).str()]; 
              t[ATTRIBUTE_TIME] = gint64(boost::lexical_cast<int>(  map[(boost::format("length%d") % n).str()]));
              v.push_back(t);
            }
		} catch (...) {
			return false;
		}
		return true;
	}

	bool
	PLS::write (std::string const& uri, Track_v const& list)
	{
		return false;
	}
}
}
