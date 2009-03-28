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

#ifndef VIDEO_PLAYBACK_HH
#define VIDEO_PLAYBACK_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <string>
#include <map>

#include <gst/gst.h>
#include <gdk/gdkx.h>
#include <sigc++/sigc++.h>
#include <boost/format.hpp>
#include "mpx/mpx-audio.hh"
#include "mpx/mpx-audio-types.hh"

namespace MPX
{

    AUDIOEXCEPTION(NoSuchElementError)
    AUDIOEXCEPTION(UnableToConstructError)

    class VideoPipe
    {
        public:

            typedef sigc::signal< ::Window> SignalRequestWindowId;
            typedef sigc::signal<void, int, int> SignalVideoGeom;

      
        private:

            SignalRequestWindowId signal_;
            SignalBusWatchCascade signal_cascade_;

        public:
            
            SignalRequestWindowId&
            signal_request_window_id ()
            {
              return signal_;
            }

            SignalBusWatchCascade&
            signal_cascade ()
            {
              return signal_cascade_;
            }

        private:

            static void bus_watch (GstBus*     bus,
                                   GstMessage* message,
                                   gpointer    data);

            static void link_pad (GstElement* element,
                                  GstPad*     pad,
                                  gboolean    last,
                                  gpointer    data);

            static void link_pads_and_unref (GstPad*, GstPad*, VideoPipe &);

            GstElement  * pipeline ;
            GstElement  * filesrc ;
            GstElement  * decodebin ;
            GstElement  * queue ; 
            GstElement  * ffmpegcolorspace ; 
            GstElement  * imagesink ;
            GstElement  * videoscale ; // only used when no hw scaling is available
            GstElement  * queue2 ;

            typedef std::map<std::string, GstElement *> ElementMap;
            typedef ElementMap::value_type              ElementPair;
            ElementMap                                  m_elements;
            
        public:

            void  set_audio_elmt (GstElement * audio_out);
            void  rem_audio_elmt ();
            void  set_window_id (::Window id);
            void  expose ();
       
            VideoPipe() ;
            virtual ~VideoPipe() ;

            GstElement *
            pipe ()
            {
                return pipeline;
            }

            GstElement *
            operator[] (std::string const& id)
            {
                if( m_elements.find( id ) == m_elements.end() )
                {
                    throw NoSuchElementError( (boost::format ("No such element '%s' in this pipeline") % id).str() );
                }

                return m_elements.find( id )->second;
            }
    };
}

#endif
