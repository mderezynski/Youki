//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glibmm.h>

#include <boost/algorithm/string.hpp>
#define BOOST_REGEX_MATCH_EXTRA 1
#include <boost/regex.hpp>

#include "md5.h"
#include "mpx/util-string.hh"

using namespace Glib;

namespace
{
  const struct {
      const char *exp;
      const char *fmt;
  } lastfm_regexes[] = {

     { "(\\[[^\\]]+\\])",
       "(?1)"},

     { "(\\[\\/[^\\]]+\\])",
       "(?1)"},
  };
}

namespace MPX
{
  namespace Util
  {
    std::string
    hex_string (void const* data,
                std::size_t len)
    {
      static const char hexchars[] = "0123456789abcdef";

      guint8 const* byte_array = static_cast<guint8 const*> (data);

      std::string s;
      s.reserve (len * 2);

      for(unsigned int i = 0; i < len; i++)
        {
          s.push_back (hexchars[byte_array[i] >> 4]);
          s.push_back (hexchars[byte_array[i] & 0x0f]);
        }

      return s;
    }

    std::string
    md5_hex_string (void const* data,
                    std::size_t len)
    {
        md5_state_t md5state;
        md5_byte_t  md5pword[16];

        md5_init (&md5state);
        md5_append (&md5state, static_cast<md5_byte_t const*> (data), static_cast<int> (len));
        md5_finish (&md5state, md5pword);

        return hex_string (md5pword, sizeof (md5pword));
    }

    bool
    str_has_prefix_nocase (std::string const& str,
                           std::string const& prefix)
    {
      if (str.empty () || prefix.empty ())
        return false;

      return (g_ascii_strncasecmp (str.c_str (), prefix.c_str (), prefix.length ()) == 0);
    }

    bool
    str_has_suffix_nocase (std::string const& str,
                           std::string const& suffix)
    {
      if (str.empty () || suffix.empty ())
        return false;

      return (g_ascii_strcasecmp (str.c_str () + str.length () - suffix.length (), suffix.c_str ()) == 0);
    }

    bool
    str_has_suffixes_nocase (std::string const& str,
                             char const**       suffixes)
    {
      if (str.empty () || !suffixes)
        return false;

      for (char const** suffix = suffixes; *suffix; ++suffix)
        {
          if (str_has_suffix_nocase (str, std::string (*suffix)))
            return true;
        }

      return false;
    }

    bool
    str_has_suffixes_nocase (std::string const& str,
                             StrV const&        strv)
    {
      if (str.empty () || strv.empty ())
        return false;

      for (StrV::const_iterator i = strv.begin () ; i != strv.end (); ++i)
        {
          if (str_has_suffix_nocase (str, *i))
            return true;
        }

      return false;
    }

    std::string
    stdstrjoin (StrV const& strings, std::string const& delimiter)
    {
      std::string result;
      StrV::const_iterator e = strings.end(); 
      --e;
      for (StrV::const_iterator i = strings.begin(); i != strings.end(); ++i) 
      {
        result += *i; 
      	if (i != e)
      	{
	        result += delimiter;
        }
      }
      return result;
    }

    Glib::ustring
    utf8_string_normalize (Glib::ustring const& in)
    {
      StrV uppercased;

      if (in.empty())
        return Glib::ustring();

      char ** splitted = g_strsplit_set (in.c_str(), " -", -1);
      int n = 0;

      while (splitted[n])
      {
        if (strlen (splitted[n]))
        {
          ustring o  = ustring (splitted[n]).normalize().lowercase();
          ustring ol = o.substr (1);
          ustring of = ustring (1, o[0]).uppercase();
          ustring compose = (of + ol);
          uppercased.push_back (compose);
        }
        ++n;
      }

      g_strfreev (splitted);
      ustring norm = stdstrjoin (uppercased, " ");
      return norm; 
    }

    bool
    match_keys (ustring const& _h,
                ustring const& _n)
    {
        using boost::algorithm::split;
        using boost::algorithm::is_any_of;
        using boost::algorithm::find_first;

        StrV m;

        std::string n (_n.lowercase());
        std::string h (_h.lowercase());

        split( m, n, is_any_of(" ") );

        for (StrV::const_iterator i = m.begin (); i != m.end (); ++i)
        {
			if (i->length() < 1)
				continue;

	        if (!find_first (h, (*i)))
				return false;
        }
        return true;
    }

    ustring
    sanitize_lastfm (ustring const& in)
    {
      std::string out = in;
      try {
        boost::regex e1;
        for (unsigned int n = 0; n < G_N_ELEMENTS(lastfm_regexes); ++n)
        {
          e1.assign (lastfm_regexes[n].exp);
          out = boost::regex_replace (out, e1, lastfm_regexes[n].fmt, boost::match_default | boost::format_all | boost::match_extra);
        }
       }
      catch (boost::regex_error & cxe)
      {
        g_warning ("%s: Error during Last.FM Markup sanitize: %s", G_STRLOC, cxe.what());
      }
      return out; 
    }

  }
}

