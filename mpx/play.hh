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

#ifndef MPX_PLAY_HH
#define MPX_PLAY_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <map>
#include <string>

#include <glibmm/object.h>
#include <glibmm/property.h>
#include <glibmm/propertyproxy.h>
#include <glibmm/ustring.h>
#include <sigc++/signal.h>
#include <sigc++/connection.h>

#include <boost/optional.hpp>

#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gst/interfaces/mixer.h>
#include <gst/interfaces/mixertrack.h>
#include <gst/interfaces/mixeroptions.h>

#include "mcs/mcs.h"

#include "mpx/glibaddons.hh"
#include "mpx/main.hh"
#include "mpx/uri.hh"

#include "video-playback.hh"
#include "audio-types.hh"
#include "messages.hh"

using namespace std;

namespace MPX
{
    const int SPECT_BANDS = 24;

    /** Playback Engine
     *
     * MPX::Play is the playback engine of BMP. It is based on GStreamer 0.10
     * using a rather simple design. (http://www.gstreamer.net)
     *
     */
    class Play
      : public Glib::Object
    {
      private:

        enum PipelineId
        {
          PIPELINE_NONE = 0,
          PIPELINE_HTTP,
          PIPELINE_HTTP_MAD,
          PIPELINE_MMSX,
          PIPELINE_FILE,
          PIPELINE_CDDA,
          PIPELINE_VIDEO // not really a *pipeline* but anyway
        };

        enum BinId
        {
          BIN_OUTPUT = 0,
          BIN_HTTP,
          BIN_HTTP_MAD,
          BIN_MMSX,
          BIN_FILE,
          BIN_CDDA,

          N_BINS,
        };

        PipelineId m_pipeline_id;

		GAsyncQueue *m_MessageQueue;
		Glib::Mutex m_QueueLock;
		Glib::Mutex m_BusLock;

		void
		process_queue ();

		void
		push_message (Audio::Message const& message);

      public:

        Play ();
        ~Play ();

        MPXGstMetadata const&        
        get_metadata ();

        void  request_status (MPXPlaystatus status);
        void  switch_stream (Glib::ustring const& stream, Glib::ustring const& type = Glib::ustring());
        void  set_custom_httpheader (char const*);
        void  seek (gint64 position);
        void  set_window_id ( ::Window id);

        void  reset ();

        ProxyOf<PropString>::ReadWrite  property_stream ();
        ProxyOf<PropInt>::ReadWrite     property_volume ();

        ProxyOf<PropInt>::ReadOnly      property_status() const;
        ProxyOf<PropBool>::ReadOnly     property_sane() const; 
        ProxyOf<PropInt>::ReadOnly      property_position() const; 
        ProxyOf<PropInt>::ReadOnly      property_duration() const; 

        typedef sigc::signal<void>                    SignalEos;
        typedef sigc::signal<void, gint64>            SignalSeek;
        typedef sigc::signal<void, gint64>            SignalPosition;
        typedef sigc::signal<void, int>               SignalHttpStatus;
        typedef sigc::signal<void>                    SignalLastFMSync;
        typedef sigc::signal<void, double>            SignalBuffering;
        typedef sigc::signal<void, GstState>          SignalPipelineState;
        typedef sigc::signal<void, MPXPlaystatus>     SignalMPXPlaystatus;
        typedef sigc::signal<void, Spectrum const&>   SignalSpectrum;
        typedef sigc::signal<void, int, int, GValue const*> SignalVideoGeom;

        typedef sigc::signal<void, Glib::ustring const&   /* element name */
                                 , Glib::ustring const&   /* location     */
                                 , Glib::ustring const&   /* debug string */
                                 , GError*                /* error        */
                                 , GstElement const*      /* error source */> SignalError;
                                                  
        typedef sigc::signal<void, MPXGstMetadataField> SignalMetadata;

        bool has_video ();
        void video_expose ();
        GstElement * x_overlay ();
      
        /** Signal emitted when video output requests a window ID
          *
          */ 
        VideoPipe::SignalRequestWindowId&
        signal_request_window_id();

        /** Signal emitted on error state 
         *
         */
        SignalError &
        signal_error();

        /** Signal emitted on spectrum data 
         *
         */
        SignalSpectrum &
        signal_spectrum();

        /** Signal emitted when the engine state changes 
         *
         */
        SignalMPXPlaystatus &
        signal_playstatus();

        /** Signal emitted when a stream title is incoming 
         *
         */
        SignalPipelineState &
        signal_pipeline_state();

        /** Signal emitted on end of stream
         *
         */
        SignalEos &  
        signal_eos();

        /** Signal emitted on a successful seek event
         *
         */
        SignalSeek &
        signal_seek();

        /** Signal emitted on stream position change
         *
         */
        SignalPosition &
        signal_position();

        /** Signal emitted on Last.fm HTTP "Status Code" 
         *
         */
        SignalHttpStatus &
        signal_http_status();

        /** Signal emitted when prebuffering live stream data 
         *
         */
        SignalBuffering &
        signal_buffering();

        /** Signal emitted for video geometry 
         *
         */
        SignalVideoGeom &
        signal_video_geom ();

        //////////// METADATA SIGNALS

        /** Signal on new metadata 
         *
         */
        SignalMetadata &
        signal_metadata();

      private:

        Glib::Mutex           m_state_lock,
                              m_stream_lock;

        SignalMPXPlaystatus   signal_playstatus_;
        SignalMetadata        signal_metadata_;
        SignalSeek            signal_seek_;
        SignalBuffering       signal_buffering_;
        SignalSpectrum        signal_spectrum_;
        SignalPosition        signal_position_;
        SignalHttpStatus      signal_http_status_;
        SignalPipelineState   signal_pipeline_state_;
        SignalEos             signal_eos_;
        SignalError           signal_error_;
        SignalVideoGeom       signal_video_geom_;

        MPXGstMetadata        m_metadata;
        Spectrum              m_spectrum, m_zero_spectrum;

        void
        eq_band_changed (MCS_CB_DEFAULT_SIGNATURE, unsigned int band);

        static void
        queue_underrun (GstElement *element,
                        gpointer data);

        static void
        http_status  (GstElement *element,
                      int status,
                      gpointer data);

        static void
        link_pad  (GstElement *element,
                   GstPad     *pad,
                   gboolean    last,
                   gpointer    data);

        static void
        bus_watch (GstBus     *bus,
                   GstMessage *message,
                   gpointer    data);

        static gboolean
        foreach_structure (GQuark	       field_id,
                           const GValue	*value,
                           gpointer	     data);

        static void
        for_each_tag (const GstTagList * list,
                            const gchar * tag,
                            gpointer data);

        // Properties
        
        PropString  property_stream_;
        PropInt     property_volume_;
        PropInt     property_status_;
        PropBool    property_sane_;
        PropInt     property_position_;
        PropInt     property_duration_;

        ustring             m_stream_type;
        URI::Protocol       m_current_protocol;

        GstElement        * m_pipeline;
        GstElement        * m_bin[N_BINS];
        GstElement        * m_Equalizer;
        GstElement        * m_play_elmt;
        VideoPipe         * m_video_pipe;

        sigc::connection    m_conn_stream_position;
        bool                m_seeking;

        void  destroy_bins ();
        void  create_bins ();

        void  stop_stream ();
        void  play_stream ();
        void  pause_stream ();
        void  readify_stream ();

        bool  tick ();

        void  on_stream_changed ();
        void  on_volume_changed ();

        void  pipeline_configure (PipelineId id);
        void  request_status_real (MPXPlaystatus status);
        void  switch_stream_real (Glib::ustring const& stream, Glib::ustring const& type = Glib::ustring());

        GstElement * control_pipe () const;
    };
}

#endif // MPX_PLAY_HH
