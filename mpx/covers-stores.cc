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

#include "mpx/mpx-covers.hh"
#include "mpx/mpx-covers-stores.hh"

#include "mpx/algorithm/ld.hh"

#include "mpx/mpx-uri.hh"

#include "mpx/util-string.hh"
#include "mpx/util-string.hh"

#include "mpx/xml/xml.hh"

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

    static boost::format amapi2_f ("http://webservices.amazon.com/onca/xml?Service=AWSECommerceService&SubscriptionId=1E90RVC80K4MVNTCHG02&Operation=ItemSearch&Artist=%s&Title=%s&SearchIndex=Music&ResponseGroup=Images&Version=2008-06-26");
}

using namespace Glib;

namespace MPX
{
    // --------------------------------------------------
    //
    // Remote store (still an abstract class, but provides a callback)
    //
    // --------------------------------------------------
    void
    RemoteStore::load_artwork(CoverFetchData* data)
    {
        if(!can_load_artwork(data))
        {
            Signals.NotFound.emit(data);
            return;
        }

        data->request = Soup::Request::create ( get_url(data) );
        data->request->request_callback().connect(sigc::bind(sigc::mem_fun(*this, &RemoteStore::save_image), data));
        data->request->run();
    }

    bool
    RemoteStore::can_load_artwork(CoverFetchData* data)
    {
        return true;
    }

    void
    RemoteStore::save_image(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* cb_data
    )
    {
        if (code == 200)
        {
            RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
            loader->write (reinterpret_cast<const guint8*>(data), size);
            loader->close ();

            RefPtr<Gdk::Pixbuf> cover = loader->get_pixbuf();

            if (cover->get_width() == 1 && cover->get_height() == 1)
            {
                g_message("%s: 1x1px, Trying Next Image", G_STRFUNC);
                goto next_image;
            }
            else
            { 
                Glib::Mutex::Lock L( covers.RequestKeeperLock );

                try{
                        cover->save( covers.get_thumb_path( cb_data->mbid ), "png" );
                        covers.cache_artwork( cb_data->mbid, cover );
                        g_message("%s: Got Image", G_STRFUNC);
                        covers.Signals.GotCover.emit( cb_data->mbid );
                } catch( Glib::FileError & cxe )
                {
                        g_message("File Error while trying to save cover image: %s", cxe.what().c_str());
                }

                delete cb_data;
            }
        }
        else
        {
            g_message("%s: Image Retrieval: FAILED", G_STRFUNC);

            next_image:

            ++(cb_data->n);

            if(cb_data->n < 6)
            {
                load_artwork(cb_data);
                return;
            }

            Signals.NotFound.emit(cb_data);
        }
    }
    
    // --------------------------------------------------
    //
    // Amapi cover art urls
    //
    // --------------------------------------------------
    void
    AmapiCovers::load_artwork(CoverFetchData* data)
    {
        data->request = Soup::Request::create ((amapi2_f % (URI::escape_string(data->artist)) % (URI::escape_string(data->album))).str());
        data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &AmapiCovers::reply_cb ), data ));
        data->request->run();
    }

    void
    AmapiCovers::reply_cb(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* cb_data
    )
    {
        if( code == 200 )
        {
			try
            {
                std::string album;

                album = xpath_get_text(
                    data,
                    size,
					"/amazon:ItemSearchResponse/amazon:Items//amazon:Item//amazon:ItemAttributes//amazon:Title",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                if( ld_distance<std::string>(album, cb_data->album) > 3 )
                {
                    Signals.NotFound.emit(cb_data);
                    return;
                }

			}
            catch (std::runtime_error & cxe)
			{ }

            std::string image_url;
            bool got_image = false;

			try
            {
                image_url = xpath_get_text(
                    data,
                    size,
					"//amazon:Items//amazon:Item//amazon:LargeImage//amazon:URL",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                got_image = true;

			} 
            catch (std::runtime_error & cxe)
			{
                g_message(cxe.what());
			}

            if(!got_image)
            {
                try
                {
                    image_url = xpath_get_text(
                        data,
                        size,
	    				"//amazon:Items//amazon:Item//amazon:MediumImage//amazon:URL",
                        "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                    ); 

                    got_image = true;
    			}
                catch (std::runtime_error & cxe)
			    {
                    g_message(cxe.what());
    			}
            }

            if( !got_image )
            {
                try
                {
                    image_url = xpath_get_text(
                        data,
                        size,
			    		"//amazon:Items//amazon:Item//amazon:SmallImage//amazon:URL",
                        "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                    ); 

                    got_image = true;
    			}
                catch (std::runtime_error & cxe)
	    		{
                    g_message(cxe.what());
    			}
            }

            if( got_image && image_url.length() )
            {
                g_message("Fetching Image from URL: %s", image_url.c_str());

    	        cb_data->request = Soup::Request::create(image_url);
	    	    cb_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &RemoteStore::save_image ), cb_data ));
		    	cb_data->request->run();

                return;
            }
            else
            {
                Signals.NotFound.emit(cb_data);
                return;
            }
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
                Signals.NotFound(cb_data);
				return;
			}

	        cb_data->request = Soup::Request::create(image_url);
		    cb_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &RemoteStore::save_image ), cb_data ));
			cb_data->request->run();
        }
        else
        {
            Signals.NotFound(cb_data);
        }
    }

    // --------------------------------------------------
    //
    // Amazon store
    //
    // --------------------------------------------------
    bool
    AmazonCovers::can_load_artwork(CoverFetchData* data)
    {
        return !data->asin.empty();
    }

    std::string
    AmazonCovers::get_url(CoverFetchData* data)
    {
        return (amazon_f[data->n] % data->asin).str();
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
                break;
            }
        }

        if(cover_art.empty())
        {
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
}
