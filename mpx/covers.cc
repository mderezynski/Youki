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

#include <gtkmm.h>
#include <glibmm.h>
#include <giomm.h>
#include <glibmm/i18n.h>
#include <glib/gstdio.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/stat.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <taglib-gio.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <id3v2framefactory.h>
#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <attachedpictureframe.h>

#include "mpx/audio.hh"
#include "mpx/covers.hh"
#include "mpx/ld.hh"
#include "mpx/minisoup.hh"
#include "mpx/network.hh"
#include "mpx/uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/xml.hh"

using namespace Glib;
using namespace TagLib;

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
  static boost::format amapi1_f ("http://webservices.amazon.com/onca/xml?Service=AWSECommerceService&SubscriptionId=1E90RVC80K4MVNTCHG02&Operation=ItemLookup&ItemId=%s&ResponseGroup=Images&Version=2008-06-26");
  static boost::format amapi2_f ("http://webservices.amazon.com/onca/xml?Service=AWSECommerceService&SubscriptionId=1E90RVC80K4MVNTCHG02&Operation=ItemSearch&Artist=%s&Title=%s&SearchIndex=Music&ResponseGroup=Images&Version=2008-06-26");

  int
  pixel_size(
        MPX::CoverSize size
  )
  {
    int pixel_size = 0;
    switch (size)
    {
      case MPX::COVER_SIZE_ALBUM_LIST:
        pixel_size = 90;
        break;

      case MPX::COVER_SIZE_INFO_AREA:
        pixel_size = 48;
        break;

      case MPX::COVER_SIZE_DEFAULT:
        pixel_size = 256;
        break;

      default:
        pixel_size = 256;
    }
    return pixel_size;
  }
}

namespace MPX
{
    Covers::Covers ()
    : sigx::glib_auto_dispatchable()
    {
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), "mpx", "covers", NULL));
        g_mkdir(path.get(), 0700);
    }

    void
    Covers::site_fetch_and_save_cover_amazn(
        CoverFetchData * amzn_data
    )
    {
        RequestKeeper.insert(amzn_data->mbid);
        std::string url = ((amazon_f[amzn_data->n] % amzn_data->asin).str());
        amzn_data->request = Soup::Request::create ((amazon_f[amzn_data->n] % amzn_data->asin).str());
        amzn_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_amazn ), amzn_data ));
        amzn_data->request->run();
    }

    void
    Covers::site_fetch_and_save_cover_amapi(
        CoverFetchData * amzn_data
    )
    {
        RequestKeeper.insert(amzn_data->mbid);

        amzn_data->amapi = true;

        if( amzn_data->asin.length() )
        {
            amzn_data->request = Soup::Request::create ((amapi1_f % amzn_data->asin).str());
        }
        else
        {
            amzn_data->request = Soup::Request::create ((amapi2_f % (URI::escape_string(amzn_data->artist)) % (URI::escape_string(amzn_data->album))).str());
        }

        amzn_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_amapi ), amzn_data ));
        amzn_data->request->run();
    }

    void
    Covers::site_fetch_and_save_cover_mbxml(
        CoverFetchData * mbxml_data
    )
    {
        RequestKeeper.insert(mbxml_data->mbid);
        mbxml_data->request = Soup::Request::create ((mbxml_f % mbxml_data->mbid).str());
        mbxml_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_mbxml ), mbxml_data ));
        mbxml_data->request->run();
    }

    void
    Covers::reply_cb_mbxml(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* mbxml_data
    )
    {
        RequestKeeper.erase(mbxml_data->mbid);

        if( code == 200 )
        {
			std::string image_url;

			try{

				image_url = xpath_get_text(
                    data,
                    size,
					"/metadata/release/relation-list//relation[@type='CoverArtLink']/@target",
                    "mb=http://musicbrainz.org/ns/mmd-1.0#"
                ); 

			} catch (std::runtime_error & cxe)
			{
                if(!mbxml_data->amapi)
                {
                    site_fetch_and_save_cover_amapi(mbxml_data);
                }
				return;
			}

            RequestKeeper.insert(mbxml_data->mbid);
	        mbxml_data->request = Soup::Request::create(image_url);
		    mbxml_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_amazn ), mbxml_data ));
			mbxml_data->request->run();
        }
        else
        {
            if(mbxml_data->uri.length())
            {
                if( cache_inline( mbxml_data->mbid, mbxml_data->uri ) )
                {
                    delete mbxml_data;
                    return;
                }
            }

            if(!mbxml_data->amapi)
            {
                site_fetch_and_save_cover_amapi(mbxml_data);
            }
        }
    }

    void
    Covers::reply_cb_amapi(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* amzn_data
    )
    {
        RequestKeeper.erase(amzn_data->mbid);

        if( code == 200 )
        {
			try{

                std::string album;

                album = xpath_get_text(
                    data,
                    size,
					"/amazon:ItemSearchResponse/amazon:Items//amazon:Item//amazon:ItemAttributes//amazon:Title",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                if( ld_distance<std::string>(album, amzn_data->album) > 3 )
                {
                    if(amzn_data->uri.length())
                    {
                        cache_inline( amzn_data->mbid, amzn_data->uri );
                    }
                    delete amzn_data;
                    return;
                }

			} catch (std::runtime_error & cxe)
			{
			}

            std::string image_url;
            bool got_image = false;

			try{

                image_url = xpath_get_text(
                    data,
                    size,
					"//amazon:Items//amazon:Item//amazon:LargeImage//amazon:URL",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                got_image = true;

			} catch (std::runtime_error & cxe)
			{
                g_message(cxe.what());
			}

            if( !got_image) try{

                image_url = xpath_get_text(
                    data,
                    size,
					"//amazon:Items//amazon:Item//amazon:MediumImage//amazon:URL",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                got_image = true;

			} catch (std::runtime_error & cxe)
			{
                g_message(cxe.what());
			}

            if( !got_image) try{

                image_url = xpath_get_text(
                    data,
                    size,
					"//amazon:Items//amazon:Item//amazon:SmallImage//amazon:URL",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                got_image = true;

			} catch (std::runtime_error & cxe)
			{
                g_message(cxe.what());
			}

            if( got_image && image_url.length() )
            {
                g_message("Fetching Image from URL: %s", image_url.c_str());
                RequestKeeper.insert(amzn_data->mbid);
    	        amzn_data->request = Soup::Request::create(image_url);
	    	    amzn_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_amazn ), amzn_data ));
		    	amzn_data->request->run();
                return;
            }

			try{

				amzn_data->asin = xpath_get_text(
                    data,
                    size,
					"/amazon:ItemSearchResponse/amazon:Items//amazon:Item//amazon:ASIN",
                    "amazon=http://webservices.amazon.com/AWSECommerceService/2008-06-26"
                ); 

                amzn_data->n = 0;
                site_fetch_and_save_cover_amazn(amzn_data);

			} catch (std::runtime_error & cxe)
			{
                g_message("Runtime error during AMAPI XPath: %s", cxe.what());
                if(amzn_data->uri.length())
                {
                    cache_inline( amzn_data->mbid, amzn_data->uri );
                }
                delete amzn_data;
			}
        }
    }

    void
    Covers::reply_cb_amazn(
        char const*     data,
        guint           size,
        guint           code,
        CoverFetchData* amzn_data
    )
    {
        RequestKeeper.erase(amzn_data->mbid);

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
                Glib::Mutex::Lock L( RequestKeeperLock );
                cover->save( get_thumb_path( amzn_data->mbid ), "png" );
                m_pixbuf_cache.insert( std::make_pair( amzn_data->mbid, cover ));
                Signals.GotCover.emit( amzn_data->mbid );
                delete amzn_data;
            }
        }
        else
        {
            next_image:

            ++(amzn_data->n);
            if(amzn_data->n > 5)
            {
                if( amzn_data->uri.length() )
                {
                    if( cache_inline( amzn_data->mbid, amzn_data->uri ) )
                    {
                        delete amzn_data;
                        return;
                    }
                }

                if(!amzn_data->amapi)
                {
                    site_fetch_and_save_cover_amapi(amzn_data);
                }
            }
            else
            {
                site_fetch_and_save_cover_amazn(amzn_data);
            }
        }
    }

    void
    Covers::local_save_cover(
        CoverFetchData * local_data
    )
    {
        std::string cover_file_name = local_cover_file(local_data->uri);

        Glib::RefPtr<Gio::File> source = Gio::File::create_for_path( cover_file_name );
        Glib::RefPtr<Gio::File> dest   = Gio::File::create_for_path( get_thumb_path(local_data->mbid) );

        g_message("Copying %s to %s", source->get_path().c_str(), dest->get_path().c_str());

        source->copy( dest );

        RefPtr<Gdk::Pixbuf> cover = Gdk::Pixbuf::create_from_file( dest->get_path() );
        m_pixbuf_cache.insert( std::make_pair( local_data->mbid, cover ));
        Signals.GotCover.emit( local_data->mbid );

        delete local_data;
    }
    
    std::string
    Covers::get_thumb_path(
        const std::string& mbid
    )
    {
        std::string basename = (boost::format ("%s.png") % mbid).str ();
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), "mpx", "covers", basename.c_str(), NULL));
        return std::string(path.get());
    }

    std::string
    Covers::local_cover_file(
        const std::string& track_uri
    )
    {
        Glib::RefPtr<Gio::File> directory = Gio::File::create_for_uri( track_uri )->get_parent( );
        Glib::RefPtr<Gio::FileEnumerator> files = directory->enumerate_children( );
    
        RefPtr<Gio::FileInfo> file ;
        while ( (file = files->next_file()) != 0 )
        {
            if( file->get_name().rfind("png") != std::string::npos )
            {
                return directory->get_child(file->get_name())->get_path();
            }
        }
    }

    void 
    Covers::cache(
        const std::string& mbid,
        const std::string& asin,
        const std::string& uri,
        const std::string& artist,
        const std::string& album,
        bool               acquire
    )
    {
        Glib::Mutex::Lock L (RequestKeeperLock);
        if( RequestKeeper.count(mbid) )
        {
            return;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if( file_test( thumb_path, FILE_TEST_EXISTS ))
        {
            Signals.GotCover.emit(mbid);
            return; 
        }

        if( acquire )
        {
            if ( !local_cover_file(uri).empty() )
            {
                CoverFetchData * data = new CoverFetchData( asin, mbid, uri, artist, album );
                g_message("Has cover art at %s", local_cover_file(uri).c_str()); 

                local_save_cover( data );
            }
            else
            if( mbid.substr(0,4) == "mpx-" )
            {
                CoverFetchData * data = new CoverFetchData( asin, mbid, uri, artist, album );

                if(asin.empty())
                    site_fetch_and_save_cover_amapi( data );
                else
                    site_fetch_and_save_cover_amazn( data );
            }
            else
            {
                CoverFetchData * data = new CoverFetchData( asin, mbid, uri, artist, album );

                if(asin.empty())
                    site_fetch_and_save_cover_mbxml( data );
                else
                    site_fetch_and_save_cover_amazn( data );
            }
        }
    }

    bool
    Covers::cache_inline(
        const std::string& mbid,
        const std::string& uri
    )
    {
        Glib::RefPtr<Gdk::Pixbuf> cover;

        try{
            MPEG::File opfile (uri.c_str());
            if(opfile.isOpen() && opfile.isValid())
            {
                using TagLib::ID3v2::FrameList;
                ID3v2::Tag * tag = opfile.ID3v2Tag (false);
                if( tag )
                {
                    FrameList const& list = tag->frameList();
                    for(FrameList::ConstIterator i = list.begin(); i != list.end(); ++i)
                    {
                        TagLib::ID3v2::Frame const* frame = *i;
                        if( frame )
                        {
                            TagLib::ID3v2::AttachedPictureFrame const* picture =
                                dynamic_cast<TagLib::ID3v2::AttachedPictureFrame const*>(frame);

                            if( picture && picture->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover )
                            {
                                RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
                                ByteVector picdata = picture->picture();
                                loader->write (reinterpret_cast<const guint8*>(picdata.data()), picdata.size());
                                loader->close ();
                                cover = loader->get_pixbuf();
                            }
                        }
                    }
                }
            }
        } catch( ... )
        {
        }

        if( cover )
        {
            cover->save (get_thumb_path(mbid), "png");
            m_pixbuf_cache.insert (std::make_pair (mbid, cover));
            Signals.GotCover.emit(mbid);
            return true;
        }

        return false;
    }

    bool
    Covers::fetch(
        const std::string&      mbid,
        RefPtr<Gdk::Pixbuf>&    cover
    )
    {
        MPixbufCache::const_iterator i = m_pixbuf_cache.find(mbid);
        if (i != m_pixbuf_cache.end())
        {
            cover = (*i).second; 
            return true;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test( thumb_path, FILE_TEST_EXISTS ))
        {
            try{
              cover = Gdk::Pixbuf::create_from_file( thumb_path ); 
              m_pixbuf_cache.insert (std::make_pair(mbid, cover));
              return true;
            } catch( Gdk::PixbufError )
            {
            }
        }

        return false;
    }

    bool
    Covers::fetch(
        const std::string&                  mbid,
        Cairo::RefPtr<Cairo::ImageSurface>& surface,
        CoverSize                           size
    )
    {
        MSurfaceCache::const_iterator i = m_surface_cache[size].find (mbid);
        if (i != m_surface_cache[size].end())
        {
            surface = (*i).second; 
            return true;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test (thumb_path, FILE_TEST_EXISTS))
        {
            int px = pixel_size (size);
            
            try{
                surface = Util::cairo_image_surface_from_pixbuf(
                              Gdk::Pixbuf::create_from_file(thumb_path)->scale_simple(
                                                                            px,
                                                                            px,
                                                                            Gdk::INTERP_BILINEAR
                ));
                m_surface_cache[size].insert( std::make_pair( mbid, surface ));
                return true;
            } catch( Gdk::PixbufError )
            {
            }
        }

        return false;
    }
}
