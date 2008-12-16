//  BMPx - The Dumb Music Player
//  Copyright (C) 2005-2007 BMPx development team.
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
//  The BMPx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#ifndef MPX_PLAY_FWD_HH
#define MPX_PLAY_FWD_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glibmm.h>
#include <cstring>
#include "mpx/mpx-services.hh"

namespace MPX
{
    class Play
    : public Glib::Object
    , public Service::Base
    {
      public:

        void  set_custom_httpheader (char const*);
    };
}

#endif // MPX_PLAY_HH
