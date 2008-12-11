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
    time_t
    parseRFC822Date(
        const char * date
    ) ;

    std::string
    hex_string(
        void const* data,
        std::size_t len
    ) ;

    std::string
    md5_hex_string(
        void const* data,
        std::size_t len
    ) ;

    bool
    str_has_prefix_nocase(
        const std::string& str,
        const std::string& prefix
    ) ;

    bool
    str_has_suffix_nocase(
        const std::string& str,
        const std::string& suffix
    ) ;

    bool
    str_has_suffixes_nocase(
        const std::string& str,
        char const**       suffixes
    ) ;

    bool
    str_has_suffixes_nocase(
        const std::string& str,
        const StrV&        suffixes
    ) ;

    bool
    match_keys(
        const Glib::ustring& haystack,
        const Glib::ustring& needle
    ) ;

    std::string
    stdstrjoin(
        const StrV&,
        const std::string&
    ) ;

	std::string
    sanitize_lastfm(
        const std::string&
    ) ;

    Glib::ustring
    utf8_string_normalize(
        const Glib::ustring&
    ) ;

    std::string
    gprintf(
        const char *format,
        ...
    ) ;

    std::string
    text_match_highlight(
            const std::string& text,
            const std::string& matches,
            const std::string& color
    ) ;
  } // Util namespace
} // MPX namespace

#endif //!MPX_UTIL_STRING_HH
