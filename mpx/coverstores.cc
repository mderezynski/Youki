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

#include <gtkmm.h>
#include <glibmm.h>
#include <giomm.h>

#include <string>

#include <boost/format.hpp>

#include "mpx/covers.hh"
#include "mpx/coverstores.hh"

namespace
{
    static boost::format amazon_f[] =
    {
        boost::format("http://images.amazon.com/images/P/%s.01.LZZZZZZZ.jpg"),
        boost::format("http://images-eu.amazon.com/images/P/%s.01.LZZZZZZZ.jpg"),
        boost::format("http://ec1.images-amazon.com/images/P/%s.01.LZZZZZZZ.jpg"),
        boost::format("http://images.amazon.com/images/P/%s.01.MZZZZZZZ.jpg"),
        boost::format("http://images-eu.amazon.com/images/P/%s.01.MZZZZZZZ.jpg"),
        boost::format("http://ec1.images-amazon.com/images/P/%s.01.MZZZZZZZ.jpg"),
    };
}

using namespace Glib;

namespace MPX
{
    // --------------------------------------------------
    //
    // Amazon store
    //
    // --------------------------------------------------
    void
    AmazonCovers::load_artwork(CoverFetchData* data)
    {
        if(data->asin.empty())
        {
            // No ASIN... don't even bother looking
            Signals.NotFound.emit(data);
        }

        std::string url = (amazon_f[data->n] % data->asin).str();
        data->request = Soup::Request::create (url);
        data->request->request_callback().connect(sigc::bind(sigc::mem_fun(*this, &AmazonCovers::reply_cb_amazn), data));
        data->request->run();
    }

    void
    AmazonCovers::reply_cb_amazn(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* amzn_data
    )
    {
        if (code == 200)
        {
            RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
            loader->write (reinterpret_cast<const guint8*>(amzn_data->request->get_data_raw()), amzn_data->request->get_data_size());
            loader->close ();

            RefPtr<Gdk::Pixbuf> cover = loader->get_pixbuf();

            if (cover->get_width() == 1 && cover->get_height() == 1)
            {
                goto next_image;
            }
            else
            { 
                Glib::Mutex::Lock L(covers.RequestKeeperLock);
                cover->save( covers.get_thumb_path( amzn_data->mbid ), "png" );
                covers.cache_artwork( amzn_data->mbid, cover );
                covers.Signals.GotCover.emit( amzn_data->mbid );
                delete amzn_data;
            }
        }
        else
        {
            next_image:
            Signals.NotFound.emit(amzn_data);
        }
    }


    void
    DummyStore::load_artwork(CoverFetchData* data)
    {
        Signals.NotFound.emit(data);
    }
}
