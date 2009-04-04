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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include "mpx/widgets/seekscale.hh"
#include "mpx/widgets/cairo-extensions.hh"

#include <gtkmm.h>

namespace MPX
{
    SeekScale::SeekScale ()
    {
    }

    SeekScale::~SeekScale()
    {
    }

    bool
    SeekScale::on_expose_event (GdkEventExpose *event)
    {
        Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context();
    
        Gdk::Rectangle const& allocation = get_allocation();

        RoundedRectangle( cr, allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height(), 4 );            
        cr->set_operator(Cairo::OPERATOR_ATOP);
        cr->set_source_rgba(1., 1., 1., .5);
        cr->fill();

        return true;
    }
} // namespace MPX
