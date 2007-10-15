//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
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

#ifndef MPX_UTIL_STRING_HH
#define MPX_UTIL_STRING_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <string>
#include <vector>
#include <glibmm/ustring.h>

namespace MPX
{
  typedef std::vector<std::string> StrV;

  namespace Util
  {
    bool str_has_prefix_nocase    (std::string const& str, std::string const& prefix);
    bool str_has_suffix_nocase    (std::string const& str, std::string const& suffix);
    bool str_has_suffixes_nocase  (std::string const& str, char const**       suffixes);
    bool str_has_suffixes_nocase  (std::string const& str, StrV const&        suffixes);

    bool
    match_keys (Glib::ustring const& haystack,
                Glib::ustring const& needle);

    std::string stdstrjoin (StrV const& strings, std::string const& delimiter);
    Glib::ustring utf8_string_normalize (Glib::ustring const& in);

  } // Util namespace
} // MPX namespace

#endif //!MPX_UTIL_STRING_HH
