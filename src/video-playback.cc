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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H


#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <boost/format.hpp>

#include "mpx/mpx-main.hh" // for mcs

#include "video-playback.hh"

namespace
{
    GstElement *
    element (const char * name, const char * id = NULL)
    {
        GstElement * x = gst_element_factory_make (name, id);
        if (!x) throw MPX::NoSuchElementError ((boost::format ("Element %s could not be created") % name).str());
        return x;
    }

    std::string
    stdstring (char * c_str)
    {
        std::string str; 

        if (c_str)
        {
            str = c_str;
            g_free (c_str);
        }

        return str;
    }

    bool
    caps_have_structure (GstCaps const * caps, std::string const& field_name)
    {
        for( guint n = 0; n < gst_caps_get_size (caps); ++n )
        {
            GstStructure * structure = gst_caps_get_structure (caps, n);

            if( structure && gst_structure_get_name( structure ) == field_name )
            {
                return true;
            }
        }

        return false;
    }
   
    void link_pad (GstElement* element,
                    GstPad*     pad,
                    gboolean    last,
                    gpointer    data) ;
}

namespace MPX
{
    void
    VideoPipe::bus_watch (GstBus*     bus,
                          GstMessage* message,
                          gpointer    data)
    {
        VideoPipe & pipe = *(reinterpret_cast<VideoPipe*>(data));

        if (message->structure && gst_structure_has_name (message->structure, "prepare-xwindow-id"))
        {
            ::Window id = pipe.signal_.emit ();
            gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (pipe["videosink1"]), id);
            return;
        }

        pipe.signal_cascade_.emit (bus, message);
    }

    VideoPipe::~VideoPipe ()
    {
        gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
        gst_object_unref (pipeline);
    }
  
    void
    VideoPipe::set_audio_elmt (GstElement * audio_out)
    {
        gst_element_set_state(
              audio_out
            , GST_STATE_NULL
        ) ;

        gst_bin_add_many(
              GST_BIN(pipeline)
            , audio_out
            , NULL
        ) ;

        gst_element_link_many(
              queue2
            , audio_out
            , NULL
        ) ;

        m_elements.insert( ElementPair( "audio_out", audio_out )) ;
    }
  
    void
    VideoPipe::rem_audio_elmt ()
    {
        if( m_elements.count("audio_out") ) 
        {
            gst_element_unlink(
                  queue2
                , m_elements.find("audio_out")->second
            ) ;

            gst_element_set_state(
                  m_elements.find( "audio_out" )->second
                , GST_STATE_NULL
            ) ;

            gst_bin_remove_many(
                  GST_BIN(pipeline)
                , m_elements.find( "audio_out" )->second
                , NULL
            ) ;
        }
    }

    void
    VideoPipe::set_window_id ( ::Window id)
    {
        gst_x_overlay_set_xwindow_id (GST_X_OVERLAY ((*this)["videosink1"]), id);
    }

    void
    VideoPipe::expose ()
    {
        gst_x_overlay_expose (GST_X_OVERLAY ((*this)["videosink1"]));
    }

    VideoPipe::VideoPipe () 
    {
        pipeline = gst_pipeline_new ("Pipe1");
        videoscale = 0;

        try{
            filesrc             = element ("filesrc", "Video_FileSrc") ;
            decodebin           = element ("decodebin", "Video_Decoder") ;
            queue               = element ("queue") ;
            ffmpegcolorspace    = element ("ffmpegcolorspace") ;

            int video_out = mcs->key_get<int>( "audio", "video-output" ) ;

            switch (video_out)
            {
                case 0: 
                  imagesink = element ("xvimagesink"); 
                  break;

                case 1:
                  imagesink = element ("ximagesink"); 
                  videoscale = element ("videoscale");
                  g_object_set(
                          G_OBJECT(videoscale)
                        , "method"
                        , int(1)
                        , NULL
                  ) ;
                  break;
            }

            queue2 = element ("queue", "Queue2");

        } catch (NoSuchElementError & cxe) 
        {
            g_warning ("%s: Couldn't create video pipeline!", G_STRLOC);
            throw UnableToConstructError();
        }

        m_elements.insert (ElementPair ("filesrc", filesrc));
        m_elements.insert (ElementPair ("decodebin", decodebin));
        m_elements.insert (ElementPair ("queue", queue));
        m_elements.insert (ElementPair ("ffmpegcolorspace", ffmpegcolorspace));
        m_elements.insert (ElementPair ("videoscale", videoscale));
        m_elements.insert (ElementPair ("videosink1", imagesink));
        m_elements.insert (ElementPair ("queue2", queue2));

        if( videoscale )
        {
            gst_bin_add_many(
                  GST_BIN (pipeline)
                , filesrc
                , decodebin
                , queue
                , ffmpegcolorspace
                , videoscale
                , imagesink
                , queue2
                , NULL
            ) ; 

            gst_element_link_many(
                  filesrc
                , decodebin
                , NULL
            ) ; 

            gst_element_link_many(
                  queue
                , ffmpegcolorspace
                , videoscale
                , imagesink
                , NULL
            ) ;
        }
        else
        {
            gst_bin_add_many(
                  GST_BIN (pipeline)
                , filesrc
                , decodebin
                , queue
                , ffmpegcolorspace
                , imagesink
                , queue2
                , NULL
            ) ; 

            gst_element_link_many(
                  filesrc
                , decodebin
                , NULL
            ) ; 

            gst_element_link_many(
                  queue
                , ffmpegcolorspace
                , imagesink
                , NULL
            ) ;
        }

        g_signal_connect(
              G_OBJECT (decodebin)
            , "new-decoded-pad"
            , G_CALLBACK (link_pad)
            , this
        ) ;

        GstBus * bus =

          gst_pipeline_get_bus(
              GST_PIPELINE (pipeline)
          ) ;

        gst_bus_add_signal_watch( bus ) ;

        g_signal_connect(
              G_OBJECT (bus)
            , "message"
            , GCallback (bus_watch)
            , this
        ) ;

        gst_object_unref( bus ) ;
    }

    void
    VideoPipe::link_pads_and_unref (GstPad * pad1, GstPad * pad2, MPX::VideoPipe & pipe)
    {
      switch (gst_pad_link (pad1, pad2))
      {
          case GST_PAD_LINK_OK: 
          break;

          case GST_PAD_LINK_WRONG_HIERARCHY:
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Trying to link pads %p and %p with non common ancestry.", G_STRFUNC, pad1, pad2); 
          break;

          case GST_PAD_LINK_WAS_LINKED: 
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pad %p was already linked", G_STRFUNC, pad1); 
          break;

          case GST_PAD_LINK_WRONG_DIRECTION:
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pad %p is being linked into the wrong direction", G_STRFUNC, pad1); 
          break;

          case GST_PAD_LINK_NOFORMAT: 
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pads %p and %p have no common format", G_STRFUNC, pad1, pad2); 
          break;

          case GST_PAD_LINK_NOSCHED: 
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pads %p and %p can not cooperate in scheduling", G_STRFUNC, pad1, pad2); 
          break;

          case GST_PAD_LINK_REFUSED:
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pad %p refused the link", G_STRFUNC, pad1); 
          break;
      }
      gst_object_unref (pad2);
    }

    void
    VideoPipe::link_pad (GstElement* element,
                         GstPad*     pad,
                         gboolean    last,
                         gpointer    data)
    {
      MPX::VideoPipe & pipe = *(reinterpret_cast<MPX::VideoPipe*>(data));

      GstCaps * new_pad_caps = gst_pad_get_caps (pad);

      if (caps_have_structure (new_pad_caps, "video/x-raw-yuv") ||
          caps_have_structure (new_pad_caps, "video/x-raw-rgb"))
      {
        g_message ("%s: Video pad (%s)", G_STRLOC, GST_OBJECT_NAME (pad));
        GstPad * video_sink_pad = gst_element_get_pad (GST_ELEMENT (pipe["queue"]), "sink");
        g_return_if_fail( video_sink_pad ) ;
        link_pads_and_unref( pad, video_sink_pad, pipe );
      }

      if (caps_have_structure (new_pad_caps, "audio/x-raw-int"))
      {
        g_message ("%s: Audio pad (%s)", G_STRLOC, GST_OBJECT_NAME (pad));
        GstPad * audio_sink_pad = gst_element_get_pad (GST_ELEMENT (pipe["queue2"]), "sink");
        g_return_if_fail( audio_sink_pad ) ;
        link_pads_and_unref (pad, audio_sink_pad, pipe);
      }
    }
}
