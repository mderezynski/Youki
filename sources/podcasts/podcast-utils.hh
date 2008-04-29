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

#ifndef MPX_PODCAST_UTILS_HH 
#define MPX_PODCAST_UTILS_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <vector>

#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <glibmm.h>
#include <glibmm/markup.h>

#include "podcast-v2-types.hh"

namespace MPX
{
  namespace PodcastV2
  {
    std::string
    cast_filename (Podcast & cast);

    std::string
    cast_image_filename (Podcast & cast);
   
    std::string
    cast_item_path (Podcast & cast);

    std::string
    cast_item_file (Podcast & cast, PodcastV2::Episode & item);

#include "mpx/exception.hh"

    EXCEPTION(PodcastInvalidError)
    EXCEPTION(EpisodeInvalidError)

    EXCEPTION(NetworkError)
    EXCEPTION(ParsingError)

    EXCEPTION(PodcastExistsError)
    EXCEPTION(PodcastNotLoadedError)

    EXCEPTION(InvalidPodcastListError)
    EXCEPTION(InvalidUriError)

  }
}

#endif //!MPX_PODCAST_UTILS
