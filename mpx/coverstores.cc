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
#include "mpx/xml.hh"

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

    static boost::format mbxml_f ("http://www.uk.musicbrainz.org/ws/1/release/%s?type=xml&inc=url-rels");
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
        data->request->request_callback().connect(sigc::bind(sigc::mem_fun(*this, &AmazonCovers::reply_amazn), data));
        data->request->run();
    }

    void
    AmazonCovers::reply_amazn(
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
                Glib::Mutex::Lock L( covers.RequestKeeperLock );
                cover->save( covers.get_thumb_path( amzn_data->mbid ), "png" );
                covers.cache_artwork( amzn_data->mbid, cover );
                covers.Signals.GotCover.emit( amzn_data->mbid );

                delete amzn_data;
            }
        }
        else
        {
            next_image:
            g_message("Amazon giving up");
            Signals.NotFound.emit(amzn_data);
        }
    }

    // --------------------------------------------------
    //
    // LocalFs (filesystem) covers
    //
    // --------------------------------------------------

    void
    LocalCovers::load_artwork(CoverFetchData* data)
    {
        Glib::RefPtr<Gio::File> directory = Gio::File::create_for_uri( data->uri )->get_parent( );
        Glib::RefPtr<Gio::FileEnumerator> files = directory->enumerate_children( );
    
        Glib::RefPtr<Gio::FileInfo> file;

        std::string cover_art;
        while ( (file = files->next_file()) != 0 )
        {
            if( file->get_content_type().find("image") != std::string::npos &&
                ( file->get_name().find("folder") ||
                  file->get_name().find("cover")  ||
                  file->get_name().find("front")     )
              )
            {
                cover_art = directory->get_child(file->get_name())->get_path();
                g_message("Found cover art %s", cover_art.c_str());
                break;
            }
        }

        if(cover_art.empty())
        {
            g_message("Local fs giving up");
            Signals.NotFound.emit(data);
        }
        else
        {
            RefPtr<Gdk::Pixbuf> cover = Gdk::Pixbuf::create_from_file( cover_art );
            cover->save( covers.get_thumb_path( data->mbid ), "png" );
            covers.cache_artwork( data->mbid, cover );
            covers.Signals.GotCover.emit( data->mbid );

            delete data;
        }
    }

    // --------------------------------------------------
    //
    // MusicBrainz cover art urls
    //
    // --------------------------------------------------
    void
    MusicBrainzCovers::load_artwork(CoverFetchData* data)
    {
        data->request = Soup::Request::create( (mbxml_f % data->mbid).str() );
        data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &MusicBrainzCovers::reply_cb ), data ));
        data->request->run();
    }

    void
    MusicBrainzCovers::reply_cb(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* cb_data
    )
    {
        if( code == 200 )
        {
			std::string image_url;

			try
            {
			    image_url = xpath_get_text(
                    data,
                    size,
					"/metadata/release/relation-list//relation[@type='CoverArtLink']/@target",
                    "mb=http://musicbrainz.org/ns/mmd-1.0#"
                ); 
			}
            catch (std::runtime_error & cxe)
			{
                g_message("MusicBrainz giving up");
                Signals.NotFound(cb_data);
				return;
			}

	        cb_data->request = Soup::Request::create(image_url);
		    cb_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &MusicBrainzCovers::save_image_cb ), cb_data ));
			cb_data->request->run();
        }
        else
        {
            g_message("MusicBrainz giving up");
            Signals.NotFound(cb_data);
        }
    }

    void
    MusicBrainzCovers::save_image_cb(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* cb_data
    )
    {
        if (code == 200)
        {
            RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
            loader->write (reinterpret_cast<const guint8*>(cb_data->request->get_data_raw()), cb_data->request->get_data_size());
            loader->close ();

            RefPtr<Gdk::Pixbuf> cover = loader->get_pixbuf();

            if (cover->get_width() == 1 && cover->get_height() == 1)
            {
                goto next_image;
            }
            else
            { 
                Glib::Mutex::Lock L( covers.RequestKeeperLock );
                cover->save( covers.get_thumb_path( cb_data->mbid ), "png" );
                covers.cache_artwork( cb_data->mbid, cover );
                covers.Signals.GotCover.emit( cb_data->mbid );

                delete cb_data;
            }
        }
        else
        {
            next_image:
            Signals.NotFound.emit(cb_data);
        }
    }
}
