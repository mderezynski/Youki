//  MPX - The Dumb Music Player
//  Copyright (C) 2005-2007 MPX development team.
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
#endif // HAVE_CONFIG_H

#include <iostream>
#include <cstring>
#include <glibmm.h>
#include <glib/gi18n.h>

#include <gst/gst.h>
#include <gst/gstelement.h>

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>

#include "mpx/mpx-play.hh"

#include "mcs/mcs.h"
#include "mpx/mpx-audio.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-uri.hh"


#define MPX_GST_BUFFER_TIME   ((gint64) 50000)
#define MPX_GST_PLAY_TIMEOUT  ((gint64) 3000000000)

using namespace Glib;

namespace
{
        static boost::format band_f ("band%d");

        char const* NAME_CONVERT    = N_("Raw Audio Format Converter");
        char const* NAME_VOLUME     = N_("Volume Adjustment");
        char const* NAME_RESAMPLE   = N_("Resampler");
        char const* NAME_EQUALIZER  = N_("Equalizer");
        char const* NAME_SPECTRUM   = N_("Spectrum");

        char const* m_pipeline_names[] =
        {
                "(none)",
                "http",
                "httpmad",
                "mmsx",
                "file",
                "cdda"
        };

        bool
                video_type (std::string const& type)
                {
                        return (type.substr (0, 6) == std::string("video/"));
                }

        gboolean
                drop_data (GstPad *        pad,
                                GstMiniObject* mini_obj,
                                gpointer       data)
                {
                        return FALSE;
                }

        char const*
                nullify_string (std::string const& in)
                {
                        return (in.size() ? in.c_str() : NULL);
                }

}

namespace MPX
{
        Play::Play()
        : ObjectBase              ("MPXPlaybackEngine")
        , Service::Base           ("mpx-service-play")
        , m_play_elmt             (0)
        , property_stream_        (*this, "stream", "")
        , property_stream_type_   (*this, "stream-type", "")
        , property_volume_        (*this, "volume", 50)
        , property_status_        (*this, "playstatus", PLAYSTATUS_STOPPED)
        , property_sane_          (*this, "sane", false)
        , property_position_      (*this, "position", 0)
        , property_duration_      (*this, "duration", 0)
        {
                m_message_queue = g_async_queue_new ();

                for (int n = 0; n < SPECT_BANDS; ++n)
                {
                        m_spectrum.push_back (0);
                        m_zero_spectrum.push_back (0);
                }

                property_volume().signal_changed().connect
                        (sigc::mem_fun(this, &Play::on_volume_changed));

                property_stream().signal_changed().connect
                        (sigc::mem_fun(this, &Play::on_stream_changed));

                m_bin[BIN_FILE]     = 0;
                m_bin[BIN_HTTP]     = 0;
                m_bin[BIN_HTTP_MP3] = 0;
                m_bin[BIN_MMSX]     = 0;
                m_bin[BIN_CDDA]     = 0;
                m_bin[BIN_OUTPUT]   = 0;
                m_pipeline          = 0;
                m_video_pipe        = 0;
                reset ();
        }

        //dtor
        Play::~Play ()
        {
                g_async_queue_unref (m_message_queue);
                stop_stream ();
                destroy_bins ();
        }

        inline GstElement*
                Play::control_pipe () const
                {
                        return (m_pipeline_id == PIPELINE_VIDEO) ? m_video_pipe->pipe() : m_pipeline;
                }

        void
                Play::for_each_tag (GstTagList const* list,
                                gchar const*      tag,
                                gpointer          data)
                {
                        Play & play = *(static_cast<Play*> (data));

                        int count = gst_tag_list_get_tag_size (list, tag);

                        for (int i = 0; i < count; ++i)
                        {
                                if (!std::strcmp (tag, GST_TAG_TITLE))
                                {
                                        if (play.m_pipeline_id == PIPELINE_HTTP ||
                                                        play.m_pipeline_id == PIPELINE_HTTP_MP3 ||
                                                        play.m_pipeline_id == PIPELINE_MMSX)
                                        {
                                                Glib::ScopedPtr<char> w;
                                                if (gst_tag_list_get_string_index (list, tag, i, w.addr()));
                                                {
                                                        std::string title (w.get()); 

                                                        if (!play.m_metadata.m_title || (play.m_metadata.m_title && (play.m_metadata.m_title.get() != title)))
                                                        {
                                                                play.m_metadata.m_title = title;
                                                                play.signal_metadata_.emit(FIELD_TITLE);
                                                        }
                                                }
                                        }
                                }
                                if (!std::strcmp (tag, GST_TAG_ALBUM))
                                {
                                        if (play.m_pipeline_id == PIPELINE_HTTP ||
                                                        play.m_pipeline_id == PIPELINE_HTTP_MP3 ||
                                                        play.m_pipeline_id == PIPELINE_MMSX)
                                        {
                                                Glib::ScopedPtr<char> w;
                                                if (gst_tag_list_get_string_index (list, tag, i, w.addr()));
                                                {
                                                        std::string album (w.get()); 

                                                        if (!play.m_metadata.m_album || (play.m_metadata.m_album && (play.m_metadata.m_album.get() != album)))
                                                        {
                                                                play.m_metadata.m_album = album;
                                                                play.signal_metadata_.emit(FIELD_ALBUM);
                                                        }
                                                }
                                        }
                                }
                                else
                                        if (!std::strcmp (tag, GST_TAG_IMAGE)) 
                                        {
                                                GstBuffer* image = gst_value_get_buffer (gst_tag_list_get_value_index (list, tag, i));
                                                if (image)
                                                {
                                                        RefPtr<Gdk::PixbufLoader> loader (Gdk::PixbufLoader::create ());
                                                        try{
                                                                loader->write (GST_BUFFER_DATA (image), GST_BUFFER_SIZE (image));
                                                                loader->close ();
                                                                RefPtr<Gdk::Pixbuf> img (loader->get_pixbuf());
                                                                if (img)
                                                                {
                                                                        play.m_metadata.m_image = img->copy();
                                                                        play.signal_metadata_.emit(FIELD_IMAGE);
                                                                }
                                                        }
                                                        catch (...) {} // pixbufloader is a whacky thing
                                                }
                                        }
                                        else
                                                if (!std::strcmp (tag, GST_TAG_BITRATE)) 
                                                {
                                                        guint bitrate = 0;
                                                        if (gst_tag_list_get_uint_index (list, tag, i, &bitrate))
                                                        {
                                                                play.m_metadata.m_audio_bitrate = bitrate; 
                                                                play.signal_metadata_.emit(FIELD_AUDIO_BITRATE);
                                                        }
                                                }
                                if (!std::strcmp (tag, GST_TAG_AUDIO_CODEC)) 
                                {
                                        Glib::ScopedPtr<char> w;
                                        if (gst_tag_list_get_string_index (list, tag, i, w.addr()));
                                        {
                                                play.m_metadata.m_audio_codec = std::string (w.get()); 
                                                play.signal_metadata_.emit (FIELD_AUDIO_CODEC);
                                        }
                                }
                                if (!std::strcmp (tag, GST_TAG_VIDEO_CODEC)) 
                                {
                                        Glib::ScopedPtr<char> w;
                                        if (gst_tag_list_get_string_index (list, tag, i, w.addr()));
                                        {
                                                play.m_metadata.m_video_codec = std::string (w.get()); 
                                                play.signal_metadata_.emit (FIELD_VIDEO_CODEC);
                                        }
                                }
                        }
                }

        bool
                Play::tick ()
                {
                        if (GST_STATE (control_pipe ()) == GST_STATE_PLAYING)
                        {
                                GstFormat format = GST_FORMAT_TIME;
                                gint64 time_nsec;
                                gst_element_query_position (control_pipe (), &format, &time_nsec);
                                int time_sec = time_nsec / GST_SECOND;
                                signal_position_.emit (time_sec);
                                return true;
                        }

                        return false;
                }

        void
                Play::stop_stream ()
                {
                        if (control_pipe())
                        {
                                gst_element_set_state (control_pipe (), GST_STATE_NULL);
                                gst_element_get_state (control_pipe (), NULL, NULL, GST_CLOCK_TIME_NONE); 
                                property_status_ = PLAYSTATUS_STOPPED;
                        }
                        set_custom_httpheader(NULL);
                }

        void
                Play::readify_stream ()
                {
                        if (control_pipe())
                        {
                                gst_element_set_state (control_pipe (), GST_STATE_NULL);
                                gst_element_get_state (control_pipe (), NULL, NULL, GST_CLOCK_TIME_NONE); 
                                property_status_ = PLAYSTATUS_WAITING;
                        }
                }

        void
                Play::play_stream ()
                {
                        GstStateChangeReturn statechange = gst_element_set_state (control_pipe (), GST_STATE_PLAYING);

                        if (statechange != GST_STATE_CHANGE_FAILURE)
                        {
                                property_status_ = PLAYSTATUS_PLAYING;
                                return;
                        }

                        stop_stream ();
                        return;
                }

        void
                Play::pause_stream ()
                {
                        if (GST_STATE (control_pipe ()) == GST_STATE_PAUSED)
                        {
                                property_status_ = PLAYSTATUS_PLAYING;
                                gst_element_set_state (control_pipe (), GST_STATE_PLAYING);
                                return;
                        }
                        else
                        {
                                property_status_ = PLAYSTATUS_PAUSED;
                                gst_element_set_state (control_pipe (), GST_STATE_PAUSED);
                                return;
                        }

                        stop_stream ();
                }

        void
                Play::on_volume_changed ()
                {
                        GstElement* e = gst_bin_get_by_name (GST_BIN (m_bin[BIN_OUTPUT]), (NAME_VOLUME));
                        g_object_set (G_OBJECT (e), "volume", property_volume().get_value()/100.0, NULL);
                        gst_object_unref (e);
                }

        void
                Play::pipeline_configure (PipelineId id)
                {
                        if (m_pipeline_id != id)
                        {
                                if (m_play_elmt)
                                {
                                        gst_element_set_state (m_play_elmt, GST_STATE_NULL);
                                        gst_element_get_state (m_play_elmt, NULL, NULL, GST_CLOCK_TIME_NONE); 
                                        gst_element_set_state (m_bin[BIN_OUTPUT], GST_STATE_NULL);
                                        gst_element_get_state (m_bin[BIN_OUTPUT], NULL, NULL, GST_CLOCK_TIME_NONE); 
                                        gst_element_unlink (m_play_elmt, m_bin[BIN_OUTPUT]);
                                        gst_bin_remove_many (GST_BIN (m_pipeline), m_play_elmt, m_bin[BIN_OUTPUT], NULL);
                                        m_play_elmt = 0;
                                }
                                else
                                        if (m_pipeline_id == PIPELINE_VIDEO && m_video_pipe)
                                        {
                                                m_video_pipe->rem_audio_elmt ();
                                        } 

                                m_pipeline_id = id;
                                gst_element_set_state (m_pipeline, GST_STATE_NULL);
                                gst_element_get_state (m_pipeline, NULL, NULL, GST_CLOCK_TIME_NONE); 
                                gst_element_set_name (m_pipeline, m_pipeline_names[id]);

                                if (m_pipeline_id == PIPELINE_VIDEO && m_video_pipe)
                                {
                                        m_video_pipe->set_audio_elmt (m_bin[BIN_OUTPUT]);
                                        m_play_elmt = 0;
                                        return;
                                }

                                m_play_elmt = m_bin[BinId (id)];
                                gst_bin_add_many (GST_BIN (m_pipeline), m_bin[BinId (id)], m_bin[BIN_OUTPUT], NULL);
                                gst_element_link_many (m_bin[BinId (id)], m_bin[BIN_OUTPUT], NULL);
                        }
                }

        void
                Play::on_stream_changed ()
                {
                        if (property_sane().get_value() == false)
                                return;

                        URI uri;
                        try{
                                uri = URI (property_stream().get_value());
                        }
                        catch (...)
                        {
                                stop_stream (); 
                                return;
                        }

                        m_current_protocol = uri.get_protocol();
                        switch (m_current_protocol)
                        {
                                case URI::PROTOCOL_ITPC:
                                        /* ITPC doesn't provide streams, it only links to RSS feed files */
                                        break;

                                case URI::PROTOCOL_FILE:
                                        {
                                                if (video_type( property_stream_type_.get_value() ) && m_video_pipe)
                                                {
                                                        g_object_set (m_video_pipe->operator[]("filesrc"), "location", filename_from_uri (property_stream_.get_value()).c_str(), NULL);
                                                        pipeline_configure (PIPELINE_VIDEO);
                                                }
                                                else
                                                {
                                                        try{
                                                                GstElement* src = gst_bin_get_by_name (GST_BIN (m_bin[BIN_FILE]), "src");
                                                                g_object_set (src, "location", filename_from_uri (property_stream().get_value()).c_str(), NULL);
                                                                gst_object_unref (src);
                                                                pipeline_configure (PIPELINE_FILE);
                                                        } catch( ConvertError& cxe )
                                                        {
                                                                // FIXME
                                                        }
                                                }
                                                break;
                                        }

                                case URI::PROTOCOL_MMS:
                                case URI::PROTOCOL_MMSU:
                                case URI::PROTOCOL_MMST:
                                        {
                                                GstElement* src = gst_bin_get_by_name (GST_BIN (m_bin[BIN_MMSX]), "src");
                                                g_object_set (src, "location", property_stream().get_value().c_str(), NULL);
                                                gst_object_unref (src);
                                                pipeline_configure (PIPELINE_MMSX);
                                                break;
                                        }

                                case URI::PROTOCOL_HTTP:
                                        {
                                                if (property_stream_type_.get_value() == "audio/mpeg")
                                                        pipeline_configure (PIPELINE_HTTP_MP3);
                                                else
                                                        pipeline_configure (PIPELINE_HTTP);

                                                GstElement* src = gst_bin_get_by_name (GST_BIN (m_play_elmt), "src");
                                                g_object_set (src, "location", property_stream().get_value().c_str(), NULL);
                                                g_object_set (src, "prebuffer", TRUE, NULL);
                                                gst_object_unref (src);

                                                break;
                                        }

                                case URI::PROTOCOL_CDDA:
                                        {
                                                unsigned int track = static_cast<unsigned int> (std::atoi (uri.path.c_str ()+1));
                                                pipeline_configure (PIPELINE_CDDA);
                                                GstElement* src = gst_bin_get_by_name (GST_BIN (m_bin[BIN_CDDA]), "src");
                                                g_object_set (src, "track", track, NULL);
                                                gst_object_unref (src);

                                                break;
                                        }

                                case URI::PROTOCOL_FTP:
                                case URI::PROTOCOL_QUERY:
                                case URI::PROTOCOL_TRACK:
                                case URI::PROTOCOL_LASTFM:
                                case URI::PROTOCOL_UNKNOWN:
                                        // Yer all are not getting played here dudes
                                        break;
                        }

                }

        void
                Play::switch_stream_real(
                    ustring const& stream,
                    ustring const& type
                )
                {
                        //Glib::Mutex::Lock L (m_stream_lock);

                        set_custom_httpheader(NULL);
                        m_metadata.reset();

                        property_stream_type_ = type;
                        readify_stream (); 

                        property_stream_ = stream;
                        play_stream ();

                        signal_stream_switched_.emit();
                }

        void
                Play::switch_stream(
                    ustring const& stream,
                    ustring const& type
                )
                {
                        Audio::Message message;
                        message.stream = stream;
                        message.type = type;
                        message.id = 1;
                        push_message (message);
                }		

        void
                Play::request_status (PlayStatus status)
                {
                        Audio::Message message;
                        message.status = status;
                        message.id = 0;
                        push_message (message);
                } 

        void
                Play::request_status_real (PlayStatus status)
                {
                        m_state_lock.lock ();

                        switch (status)
                        {
                                case PLAYSTATUS_PAUSED:
                                        pause_stream ();
                                        break;

                                case PLAYSTATUS_STOPPED:

                                        if (BinId (m_pipeline_id) == BIN_HTTP) 
                                        {
                                                g_object_set (G_OBJECT (gst_bin_get_by_name (GST_BIN (m_bin[BIN_HTTP]), "src")),
                                                                "abort",
                                                                TRUE, NULL); 
                                        }
                                        else
                                        if (BinId (m_pipeline_id) == BIN_HTTP_MP3)
                                        {
                                                g_object_set (G_OBJECT (gst_bin_get_by_name (GST_BIN (m_bin[BIN_HTTP_MP3]), "src")),
                                                                "abort",
                                                                TRUE, NULL); 
                                        }

                                        m_metadata.reset();
                                        property_stream_type_ = ustring();
                                        //stop_stream ();
                                        property_status_ = PLAYSTATUS_STOPPED;
                                        fade_init ();
                                        break;

                                case PLAYSTATUS_WAITING:
                                        m_metadata.reset();
                                        property_stream_type_ = ustring();
                                        readify_stream (); 
                                        break;

                                case PLAYSTATUS_PLAYING:
                                        fade_stop ();
                                        play_stream ();
                                        break;

                                default:
                                        {
                                                g_log ( G_LOG_DOMAIN,
                                                                G_LOG_LEVEL_WARNING,
                                                                "%s: Unhandled Playback status request: %d",
                                                                G_STRFUNC,
                                                                int (status));
                                                break;
                                        }
                        }

                        signal_playstatus_.emit (PlayStatus (property_status().get_value()));
                        m_state_lock.unlock ();
                }

        void
                Play::seek (gint64 position)
                {
                        m_conn_stream_position.disconnect ();
                        gst_element_seek_simple (GST_ELEMENT (control_pipe ()), GST_FORMAT_TIME,
                                        GstSeekFlags (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), gint64 (position * GST_SECOND));
                }

        void
                Play::destroy_bins ()
                {
                        for (int n = 0; n < N_BINS; ++n)
                        {
                                if (m_bin[n])
                                {
                                        gst_element_set_state (m_bin[n], GST_STATE_NULL);
                                        gst_element_get_state (m_bin[n], NULL, NULL, GST_CLOCK_TIME_NONE); 
                                        gst_object_unref (m_bin[n]);
                                        m_bin[n] = 0;
                                }
                        }

                        if (m_video_pipe)
                        {
                                delete m_video_pipe;
                                m_video_pipe = 0;
                        }
                }

        void
                Play::link_pad (GstElement* element,
                                GstPad *     pad,
                                gboolean    last,
                                gpointer    data)
                {
                        GstPad * pad2 = gst_element_get_static_pad (GST_ELEMENT (data), "sink"); 

                        char *pad1_name = gst_pad_get_name(pad);
                        char *pad2_name = gst_pad_get_name(pad2);

                        GstObject * parent1 = gst_pad_get_parent(pad); 
                        GstObject * parent2 = gst_pad_get_parent(pad2); 

                        char *pad1_parent_name = gst_object_get_name(parent1);
                        char *pad2_parent_name = gst_object_get_name(parent2);

                        gst_object_unref(parent1);
                        gst_object_unref(parent2);

                        g_free(pad1_name);
                        g_free(pad2_name);
                        g_free(pad1_parent_name);
                        g_free(pad2_parent_name);

                        switch (gst_pad_link (pad, pad2))
                        {
                                case GST_PAD_LINK_OK: 
                                        break;

                                case GST_PAD_LINK_WRONG_HIERARCHY:
                                        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Trying to link pads %p and %p with non common ancestry.", G_STRFUNC, pad, pad2); 
                                        break;

                                case GST_PAD_LINK_WAS_LINKED: 
                                        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pad %p was already linked", G_STRFUNC, pad); 
                                        break;

                                case GST_PAD_LINK_WRONG_DIRECTION:
                                        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pad %p is being linked into the wrong direction", G_STRFUNC, pad); 
                                        break;

                                case GST_PAD_LINK_NOFORMAT: 
                                        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pads %p and %p have no common format", G_STRFUNC, pad, pad2); 
                                        break;

                                case GST_PAD_LINK_NOSCHED: 
                                        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pads %p and %p can not cooperate in scheduling", G_STRFUNC, pad, pad2); 
                                        break;

                                case GST_PAD_LINK_REFUSED:
                                        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "%s: Pad %p refused the link", G_STRFUNC, pad); 
                                        break;
                        }
                        gst_object_unref (pad2);
                }

        bool
                Play::clock_idle_handler ()
                {
                        signal_spectrum_.emit( m_spectrum );
                        return false;
                }
    
        gboolean
                Play::clock_callback(
                    GstClock*       clock,
                    GstClockTime    time,
                    GstClockID      id,
                    gpointer        data
                )
                {
                        Play & play = *(static_cast<Play*>( data ));

                        // Unrefrencing the message will break the spectrum and memory will leak
                        //gst_message_unref(play.m_spectrum_message);

                        play.signal_spectrum_.emit( play.m_spectrum );

                        /*
                        Glib::signal_idle().connect(
                                sigc::mem_fun(
                                        play,
                                        &Play::clock_idle_handler
                        ));*/

                        return FALSE;
                }

        void 
                Play::bus_watch (GstBus*     bus,
                                GstMessage* message,
                                gpointer    data)
                {
                        Play & play = *(static_cast<Play*> (data));

                        Glib::Mutex::Lock L (play.m_bus_lock);

                        static GstState old_state = GstState (0), new_state = GstState (0), pending_state = GstState (0);

                        switch (GST_MESSAGE_TYPE (message))
                        {
                                case GST_MESSAGE_APPLICATION:
                                        {
                                                GstStructure const* s = gst_message_get_structure (message);
                                                if (std::string (gst_structure_get_name (s)) == "buffering")
                                                {
                                                        double size;
                                                        gst_structure_get_double (s, "size", &size);
                                                        play.signal_buffering_.emit(size);
                                                }
                                                else
                                                        if (std::string (gst_structure_get_name (s)) == "buffering-done")
                                                        {
                                                        }
                                                break;
                                        }

                                case GST_MESSAGE_ELEMENT:
                                        {
                                                GstStructure const* s = gst_message_get_structure (message);
                                                if(std::string (gst_structure_get_name (s)) == "spectrum" && !g_atomic_int_get(&play.m_FadeStop))
                                                {
                                                    GstClockTime endtime;

                                                    if(gst_structure_get_clock_time (s, "endtime", &endtime))
                                                    {
                                                      GstClockID clock_id;
                                                      GstClockReturn clock_ret;
                                                      GstClockTimeDiff jitter;
                                                      GstClockTime basetime=gst_element_get_base_time(GST_ELEMENT(GST_MESSAGE_SRC(message)));
                                                      GstClockTime curtime=gst_clock_get_time(gst_pipeline_get_clock((GstPipeline*)(play.control_pipe())))-basetime;
                                                      GstClockTimeDiff waittime=GST_CLOCK_DIFF(curtime,endtime);
                                                      
                                                      clock_id=gst_clock_new_single_shot_id(gst_pipeline_get_clock((GstPipeline*)(play.control_pipe())),basetime+endtime);

                                                      GstStructure const* s = gst_message_get_structure (message);
                                                      GValue const* m = gst_structure_get_value (s, "magnitude");

                                                      for (int i = 0; i < SPECT_BANDS; ++i)
                                                      {
                                                        play.m_spectrum[i] = g_value_get_float(gst_value_list_get_value( m, i )); 
                                                      }

                                                      gst_clock_id_wait_async(clock_id,clock_callback,data);
                                                      gst_clock_id_unref(clock_id);
                                                    }
                                                }
                                                break;
                                        }

                                case GST_MESSAGE_TAG:
                                        {
                                                GstTagList *list = 0;
                                                gst_message_parse_tag (message, &list);

                                                if (list)
                                                {
                                                        gst_tag_list_foreach (list, GstTagForeachFunc (for_each_tag), &play);
                                                        gst_tag_list_free (list);
                                                }

                                                break;
                                        }

                                case GST_MESSAGE_STATE_CHANGED:
                                        {
                                                gst_message_parse_state_changed (message, &old_state, &new_state, &pending_state);

                                                if (old_state != GST_STATE_PLAYING && new_state == GST_STATE_PLAYING)
                                                {
                                                        if (play.m_pipeline_id == PIPELINE_VIDEO)
                                                        {
                                                                GstCaps *caps; 
                                                                if ((caps = gst_pad_get_negotiated_caps (gst_element_get_pad (GST_ELEMENT ((*(play.m_video_pipe))["videosink1"]), "sink"))))
                                                                {
                                                                        GstStructure * structure = gst_caps_get_structure (caps, 0);
                                                                        int width, height;
                                                                        if ((gst_structure_get_int (structure, "width", &width) &&
                                                                                                gst_structure_get_int (structure, "height", &height)))
                                                                        {
                                                                                const GValue * par = gst_structure_get_value (structure, "pixel-aspect-ratio");
                                                                                play.signal_video_geom_.emit (width, height, par);
                                                                        }
                                                                        gst_caps_unref (caps);
                                                                }
                                                        }

                                                        if (!play.m_conn_stream_position.connected())
                                                        {
                                                                play.m_conn_stream_position = signal_timeout().connect(sigc::mem_fun (play, &Play::tick), 500);

                                                        }
                                                }

                                                play.signal_pipeline_state_.emit (new_state);
                                                break;
                                        }

                                case GST_MESSAGE_ERROR:
                                        {
                                                g_message("%s: GST Error, details follow.", G_STRLOC);
                                                GError * error = NULL;
                                                Glib::ScopedPtr<char> debug;
                                                gst_message_parse_error(message, &error, debug.addr());
                                                g_message("%s: Error message: '%s'", G_STRLOC, error->message);
                                                g_message("%s: Debug........: '%s'", G_STRLOC, debug.get());
                                                break;
                                        }

                                case GST_MESSAGE_EOS:
                                        {
                                                play.signal_eos_.emit();
                                                break;
                                        }

                                default: break;
                        }
                }

        void
                Play::queue_underrun (GstElement* element,
                                gpointer    data)
                {
                        unsigned int level;
                        g_object_get (G_OBJECT (element), "current-level-bytes", &level, NULL);
                }

        void
                Play::eq_band_changed (MCS_CB_DEFAULT_SIGNATURE, unsigned int band)

                {
                        g_object_set (G_OBJECT (m_equalizer), (band_f % band).str().c_str (), boost::get<double> (value), NULL);
                }

        void
                Play::create_bins ()
                {
                        // Output Bin
                        {
                                std::string sink = mcs->key_get <std::string> ("audio", "sink");
                                GstElement* sink_element = gst_element_factory_make (sink.c_str (), "sink");

                                if (sink == "autoaudiosink")
                                {
                                        /* nothing to do for it */
                                }
                                else
                                if (sink == "gconfaudiosink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "profile", int (1) /* music/video */, NULL);
                                }
                                else
                                if (sink == "osssink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "device",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "device-oss")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "buffer-time",
                                                        guint64(mcs->key_get <int> ("audio", "oss-buffer-time")), NULL);
                                }
                                else
                                if (sink == "esdsink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "host",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "device-esd")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "buffer-time",
                                                        guint64(mcs->key_get <int> ("audio", "esd-buffer-time")),
                                                        NULL);
                                }
                                else
                                if (sink == "pulsesink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "server",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "pulse-server")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "device",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "pulse-device")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "buffer-time",
                                                        guint64(mcs->key_get <int> ("audio", "pulse-buffer-time")),
                                                        NULL);
                                }
                                else
                                if (sink == "jackaudiosink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "server",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "jack-server")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "buffer-time",
                                                        guint64(mcs->key_get <int> ("audio", "jack-buffer-time")),
                                                        NULL);
                                }
#ifdef HAVE_ALSA
                                else
                                if (sink == "alsasink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "device",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "device-alsa")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "buffer-time",
                                                        guint64(mcs->key_get <int> ("audio", "alsa-buffer-time")),
                                                        NULL);
                                }
#endif //HAVE_ALSA
#ifdef HAVE_SUN
                                else
                                if (sink == "sunaudiosink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "device",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "device-sun")),
                                                        NULL);
                                        g_object_set (G_OBJECT (sink_element), "buffer-time",
                                                        guint64(mcs->key_get <int> ("audio", "sun-buffer-time")),
                                                        NULL);
                                }
#endif //HAVE_SUN
#ifdef HAVE_HAL
                                else
                                if (sink == "halaudiosink")
                                {
                                        g_object_set (G_OBJECT (sink_element), "udi",
                                                        nullify_string (mcs->key_get <std::string> ("audio", "hal-udi")),
                                                        NULL);
                                }
#endif //HAVE_HAL

                                m_bin[BIN_OUTPUT]     = gst_bin_new ("output");
                                GstElement* convert   = gst_element_factory_make ("audioconvert", (NAME_CONVERT));
                                GstElement* resample  = gst_element_factory_make ("audioresample", (NAME_RESAMPLE));
                                GstElement* volume    = gst_element_factory_make ("volume", (NAME_VOLUME));
                                GstElement* vol_fade  = gst_element_factory_make ("volume", "VolumeFade");
                                GstElement* spectrum  = gst_element_factory_make ("spectrum", (NAME_SPECTRUM));

                                if (GST_IS_ELEMENT (spectrum))
                                {
                                        g_object_set (G_OBJECT (spectrum),
                                                        "interval", guint64 (50 * GST_MSECOND),
                                                        "bands", SPECT_BANDS,
                                                        "threshold", int (-72),
                                                        "message-magnitude", gboolean (TRUE),
                                                        "message-phase", gboolean (FALSE),
                                                        "message", gboolean (TRUE), NULL);
                                }

                                if (Audio::test_element ("equalizer-10bands") && mcs->key_get<bool>("audio","enable-eq"))
                                {
                                        m_equalizer = gst_element_factory_make ("equalizer-10bands", (NAME_EQUALIZER));

                                        if (GST_IS_ELEMENT (spectrum))
                                        {
                                                GstElement* idn = gst_element_factory_make ("identity", "identity1"); 

                                                gst_bin_add_many (GST_BIN (m_bin[BIN_OUTPUT]),
                                                                convert,
                                                                resample,
                                                                idn,
                                                                m_equalizer,
                                                                spectrum,
                                                                volume,
                                                                vol_fade,
                                                                sink_element,
                                                                NULL);

                                                gst_element_link_many (convert, resample, idn, m_equalizer, spectrum, volume, vol_fade, sink_element, NULL);
                                        }
                                        else
                                        {
                                                GstElement* idn = gst_element_factory_make ("identity", "identity1"); 
                                                gst_bin_add_many (GST_BIN (m_bin[BIN_OUTPUT]), convert, resample, idn, m_equalizer, volume, vol_fade, sink_element, NULL);
                                                gst_element_link_many (convert, resample, idn, m_equalizer, volume, vol_fade, sink_element, NULL);
                                        }

                                        // Connect MCS to Equalizer Bands
                                        for (unsigned int n = 0; n < 10; ++n)
                                        {
                                                mcs->subscribe(

                                                          "audio"

                                                        , (band_f % n).str()

                                                        , sigc::bind(
                                                            sigc::mem_fun(
                                                                *this,
                                                                &Play::eq_band_changed
                                                          )

                                                        , n
                                                ));

                                                g_object_set(

                                                          G_OBJECT (m_equalizer)

                                                        , (band_f % n).str().c_str()

                                                        , mcs->key_get<double>(
                                                              "audio"
                                                            , (band_f % n).str()
                                                          )

                                                        , NULL
                                                );
                                        }
                                }
                                else
                                {
                                        if (GST_IS_ELEMENT (spectrum))
                                        {
                                                GstElement* idn = gst_element_factory_make ("identity", "identity1"); 

                                                gst_bin_add_many (GST_BIN (m_bin[BIN_OUTPUT]),
                                                                convert,
                                                                resample,
                                                                idn,
                                                                spectrum,
                                                                volume,
                                                                vol_fade,
                                                                sink_element,
                                                                NULL);

                                                gst_element_link_many (convert, resample, idn, spectrum, volume, vol_fade, sink_element, NULL);
                                        }
                                        else
                                        {
                                                GstElement* idn = gst_element_factory_make ("identity", "identity1"); 
                                                gst_bin_add_many (GST_BIN (m_bin[BIN_OUTPUT]), convert, resample, idn, volume, vol_fade, sink_element, NULL);
                                                gst_element_link_many (convert, resample, idn, volume, vol_fade, sink_element, NULL);
                                        }
                                }

                                GstPad * pad = gst_element_get_static_pad (convert, "sink");
                                gst_element_add_pad (m_bin[BIN_OUTPUT], gst_ghost_pad_new ("sink", pad));
                                gst_object_unref (pad);

                                gst_object_ref (m_bin[BIN_OUTPUT]);

                                property_volume() = mcs->key_get <int> ("mpx", "volume");
                                property_sane_ = true;
                        }


                        ////////////////// FILE BIN
                        {
                                GstElement  * source    = gst_element_factory_make ("filesrc", "src");
                                GstElement  * decoder   = gst_element_factory_make ("decodebin", "Decoder File"); 
                                GstElement  * identity  = gst_element_factory_make ("identity", "Identity File"); 

                                if (source && decoder && identity)
                                {
                                        m_bin[BIN_FILE] = gst_bin_new ("bin-file");
                                        gst_bin_add_many (GST_BIN (m_bin[BIN_FILE]), source, decoder, identity, NULL);
                                        gst_element_link_many (source, decoder, NULL);

                                        g_signal_connect (G_OBJECT (decoder),
                                                        "new-decoded-pad",
                                                        G_CALLBACK (Play::link_pad),
                                                        identity);

                                        GstPad * pad = gst_element_get_static_pad (identity, "src");
                                        gst_element_add_pad (m_bin[BIN_FILE], gst_ghost_pad_new ("src", pad));
                                        gst_object_unref (pad);

                                        gst_object_ref (m_bin[BIN_FILE]);
                                }
                        }


                        ////////////////// HTTP BIN
                        {
                                GstElement  * source    = gst_element_factory_make ("jnethttpsrc", "src");
                                GstElement  * decoder   = gst_element_factory_make ("decodebin", "Decoder HTTP"); 
                                GstElement  * identity  = gst_element_factory_make ("identity", "Identity HTTP"); 

                                if (source && decoder && identity)
                                {
                                        m_bin[BIN_HTTP] = gst_bin_new ("bin-http");
                                        gst_bin_add_many (GST_BIN (m_bin[BIN_HTTP]), source, decoder, identity, NULL);
                                        gst_element_link_many (source, decoder, NULL);

                                        g_signal_connect (G_OBJECT (decoder),
                                                        "new-decoded-pad",
                                                        G_CALLBACK (Play::link_pad),
                                                        identity);
                                        g_object_set (G_OBJECT (source),
                                                        "iradio-mode", TRUE, 
                                                        NULL);

                                        GstPad * pad = gst_element_get_static_pad (identity, "src");
                                        gst_element_add_pad (m_bin[BIN_HTTP], gst_ghost_pad_new ("src", pad));
                                        gst_object_unref (pad);

                                        gst_object_ref (m_bin[BIN_HTTP]);
                                }
                        }


                        ////////////////// HTTP MAD/FLUMP3 BIN
                        {
                                GstElement *  source    = gst_element_factory_make ("jnethttpsrc", "src");
                                GstElement *  decoder   = gst_element_factory_make ("mad", "Decoder MAD"); 
                                GstElement  * identity  = gst_element_factory_make ("identity", "Identity MAD"); 

                                if (!decoder)
                                {
                                        decoder = gst_element_factory_make ("flump3dec", "Decoder FluMP3"); 
                                }

                                if (source && decoder && identity)
                                {
                                        m_bin[BIN_HTTP_MP3] = gst_bin_new ("bin-http-mad");
                                        gst_bin_add_many (GST_BIN (m_bin[BIN_HTTP_MP3]), source, decoder, identity, NULL);
                                        gst_element_link_many (source, decoder, identity, NULL);

                                        g_object_set (G_OBJECT (source),
                                                        "iradio-mode", TRUE,
                                                        NULL);

                                        GstPad * pad = gst_element_get_static_pad (identity, "src");
                                        gst_element_add_pad (m_bin[BIN_HTTP_MP3], gst_ghost_pad_new ("src", pad));
                                        gst_object_unref (pad);

                                        gst_object_ref (m_bin[BIN_HTTP_MP3]);
                                }
                        }

                        ////////////////// MMS* BIN
                        {
                                GstElement * source   = gst_element_factory_make ("mmssrc", "src");
                                GstElement * decoder  = gst_element_factory_make ("decodebin", "Decoder MMSX"); 
                                GstElement * identity = gst_element_factory_make ("identity", "Identity MMSX"); 

                                if (source && decoder && identity)
                                {
                                        m_bin[BIN_MMSX] = gst_bin_new ("bin-mmsx");

                                        gst_bin_add_many (GST_BIN (m_bin[BIN_MMSX]), source, decoder, identity, NULL);
                                        gst_element_link_many (source, decoder, NULL);

                                        g_signal_connect (G_OBJECT (decoder),
                                                        "new-decoded-pad",
                                                        G_CALLBACK (Play::link_pad),
                                                        identity);

                                        GstPad * pad = gst_element_get_static_pad (identity, "src");
                                        gst_element_add_pad (m_bin[BIN_MMSX], gst_ghost_pad_new ("src", pad));
                                        gst_object_unref (pad);

                                        gst_object_ref (m_bin[BIN_MMSX]);
                                }
                        }

                        ////////////////// CDDA BIN
                        {
#if defined (HAVE_CDPARANOIA)
                                GstElement * source = gst_element_factory_make ("cdparanoiasrc", "src");
#elif defined (HAVE_CDIO)
                                GstElement * source = gst_element_factory_make ("cdiocddasrc", "src");
#endif

                                if (source)
                                {
                                        m_bin[BIN_CDDA] = gst_bin_new ("bin-cdda");
                                        gst_bin_add_many (GST_BIN (m_bin[BIN_CDDA]), source, NULL);

                                        GstPad * pad = gst_element_get_static_pad (source, "src");
                                        gst_element_add_pad (m_bin[BIN_CDDA], gst_ghost_pad_new ("src", pad));
                                        gst_object_unref (pad);

                                        gst_object_ref (m_bin[BIN_CDDA]);
                                }
                        }

                        // Video Player
                        m_video_pipe = 0; 
                        try{
                                m_video_pipe = new VideoPipe ();
                                m_video_pipe->signal_cascade().connect (sigc::bind (sigc::ptr_fun (&MPX::Play::bus_watch), reinterpret_cast<gpointer>(this)));
                        }
                        catch (UnableToConstructError & cxe)
                        {
                                g_warning ("%s: Couldn't create video player thingie", G_STRLOC);
                        }

                        // Main Pipeline
                        m_pipeline = gst_pipeline_new ("MPX_Pipeline_Main");
                        m_pipeline_id = PIPELINE_NONE;
                        GstBus * bus = gst_pipeline_get_bus (GST_PIPELINE (m_pipeline));
                        gst_bus_add_signal_watch (bus);
                        g_signal_connect (G_OBJECT (bus), "message", GCallback (Play::bus_watch), this);
                        gst_object_unref (bus);
                }

        void
                Play::reset ()
                {
                        if (m_pipeline)
                        {
                                if(property_status().get_value() != PLAYSTATUS_STOPPED)
                                {
                                        stop_stream ();
                                }

                                if (m_play_elmt)
                                {
                                        gst_element_set_state (m_play_elmt, GST_STATE_NULL);
                                        gst_element_get_state (m_play_elmt, NULL, NULL, GST_CLOCK_TIME_NONE); 
                                        gst_element_set_state (m_bin[BIN_OUTPUT], GST_STATE_NULL);
                                        gst_element_get_state (m_bin[BIN_OUTPUT], NULL, NULL, GST_CLOCK_TIME_NONE); 
                                        gst_element_unlink (m_play_elmt, m_bin[BIN_OUTPUT]);
                                        gst_bin_remove_many (GST_BIN (m_pipeline), m_play_elmt, m_bin[BIN_OUTPUT], NULL);
                                        m_play_elmt = 0;
                                }

                                gst_object_unref (m_pipeline);
                                m_pipeline = 0;
                        }

                        destroy_bins ();
                        create_bins ();
                }

        void
                Play::set_window_id ( ::Window id)
                {
                        if (m_video_pipe)
                        {
                                m_video_pipe->set_window_id (id);
                        }
                }

        void
                Play::set_custom_httpheader( char const* header ) 
                {
                        if(!m_bin[BIN_HTTP] && !m_bin[BIN_HTTP_MP3])
                                return;

                        // FIXME: seems a little brash

                        if(m_bin[BIN_HTTP])
                        {
                                GstElement *e = gst_bin_get_by_name (GST_BIN (m_bin[BIN_HTTP]), "src");
                                g_object_set(G_OBJECT(e), "customheader", header, NULL);
                        }

                        if(m_bin[BIN_HTTP_MP3])
                        {
                                GstElement *e = gst_bin_get_by_name (GST_BIN (m_bin[BIN_HTTP_MP3]), "src");
                                g_object_set(G_OBJECT(e), "customheader", header, NULL);
                        }
                }

        ///////////////////////////////////////////////
        /// Object Properties
        ///////////////////////////////////////////////

        ProxyOf<PropString>::ReadWrite
                Play::property_stream_type()
                {
                        return ProxyOf<PropString>::ReadWrite (this, "stream-type");
                }

        ProxyOf<PropString>::ReadWrite
                Play::property_stream()
                {
                        return ProxyOf<PropString>::ReadWrite (this, "stream");
                }

        ProxyOf<PropInt>::ReadWrite
                Play::property_volume()
                {
                        return ProxyOf<PropInt>::ReadWrite (this, "volume");
                }

        // RO Proxies //

        ProxyOf<PropInt>::ReadOnly
                Play::property_status() const
                {
                        return ProxyOf<PropInt>::ReadOnly (this, "playstatus");
                }

        ProxyOf<PropBool>::ReadOnly
                Play::property_sane() const
                {
                        return ProxyOf<PropBool>::ReadOnly (this, "sane");
                }

        ProxyOf<PropInt>::ReadOnly
                Play::property_position() const
                {
                        GstFormat format (GST_FORMAT_TIME);
                        gint64 position = 0;
                        gst_element_query_position (GST_ELEMENT (control_pipe()), &format, &position);
                        g_object_set (G_OBJECT (gobj ()), "position", (position / GST_SECOND), NULL);
                        return ProxyOf<PropInt>::ReadOnly (this, "position");
                }

        ProxyOf<PropInt>::ReadOnly
                Play::property_duration() const
                {
                        GstFormat format (GST_FORMAT_TIME);
                        gint64 duration = 0;
                        gst_element_query_duration (GST_ELEMENT (control_pipe()), &format, &duration);
                        g_object_set (G_OBJECT (gobj ()), "duration", (duration / GST_SECOND), NULL);
                        return ProxyOf<PropInt>::ReadOnly (this, "duration");
                }

        /* Signals --------------------------------------------------------------------*/

        Play::SignalSpectrum &
                Play::signal_spectrum ()
                {
                        return signal_spectrum_;
                }

        Play::SignalPlayStatus &
                Play::signal_playstatus ()
                {
                        return signal_playstatus_;
                }

        Play::SignalPipelineState &
                Play::signal_pipeline_state ()
                {
                        return signal_pipeline_state_;
                }

        Play::SignalEos &
                Play::signal_eos ()
                {
                        return signal_eos_;
                }

        Play::SignalSeek &
                Play::signal_seek ()
                {
                        return signal_seek_;
                }

        Play::SignalPosition &
                Play::signal_position ()
                {
                        return signal_position_;
                }

        Play::SignalHttpStatus &
                Play::signal_http_status ()
                {
                        return signal_http_status_;
                }

        Play::SignalBuffering &
                Play::signal_buffering ()
                {
                        return signal_buffering_;
                }

        Play::SignalError &
                Play::signal_error ()
                {
                        return signal_error_;
                }

        Play::SignalVideoGeom &
                Play::signal_video_geom ()
                {
                        return signal_video_geom_;
                }

        VideoPipe::SignalRequestWindowId&
                Play::signal_request_window_id()
                {
                        return m_video_pipe->signal_request_window_id();
                }

        Play::SignalMetadata &
                Play::signal_metadata ()
                {
                        return signal_metadata_;
                }

        Play::SignalStreamSwitched &
                Play::signal_stream_switched ()
                {
                        return signal_stream_switched_;
                }

        /*--------------------------------------------------------*/

        bool
                Play::has_video ()
                {
                        return bool (m_video_pipe);
                }

        void
                Play::video_expose ()
                {
                        if( has_video() ) 
                        {
                                m_video_pipe->expose ();
                        }
                }

        GstElement*
                Play::x_overlay ()
                {
                        return (*m_video_pipe)["videosink1"];
                }

        GstElement*
                Play::tap ()
                {
                        return gst_bin_get_by_name (GST_BIN (m_bin[BIN_OUTPUT]), "identity1");
                }

        GstElement*
                Play::pipeline ()
                {
                        return m_bin[BIN_OUTPUT]; 
                }

        GstMetadata const&
                Play::get_metadata ()
                {
                        return m_metadata;
                }

        void
                Play::fade_init ()
                {
                        Glib::Mutex::Lock L (m_FadeLock);

                        m_FadeVolume = 1.;
                        update_fade_volume();
                        g_atomic_int_set(&m_FadeStop, 1);
                        m_FadeConn = Glib::signal_timeout().connect( sigc::mem_fun( *this, &Play::fade_timeout ), 15);
                }

        void
                Play::fade_stop ()
                {
                        Glib::Mutex::Lock L (m_FadeLock);

                        m_FadeConn.disconnect();
                        m_FadeVolume = 1.; 
                        update_fade_volume();
                        g_atomic_int_set(&m_FadeStop, 0);
                }

        void
                Play::update_fade_volume ()
                {
                        GstElement *e = gst_bin_get_by_name (GST_BIN (m_bin[BIN_OUTPUT]), "VolumeFade");
                        g_object_set (G_OBJECT (e), "volume", m_FadeVolume, NULL);
                        gst_object_unref (e);
                }

        bool
                Play::fade_timeout ()
                {
                        Glib::Mutex::Lock L (m_FadeLock);

                        m_FadeVolume -= 0.1;
                        update_fade_volume();

                        if(m_FadeVolume < 0.1)
                        {
                            stop_stream ();  
                            m_FadeVolume = 1.0;
                            update_fade_volume ();
                            g_atomic_int_set(&m_FadeStop, 0);
                            return false;
                        }

                        return true;
                }

        void
                Play::process_queue ()
                {
                        while (g_async_queue_length (m_message_queue) > 0)
                        {
                                gpointer data = g_async_queue_pop (m_message_queue);
                                Audio::Message *message = reinterpret_cast<Audio::Message*>(data);

                                switch (message->id)
                                {
                                        case 0: /* FIXME: Enums */
                                                request_status_real (message->status);
                                                break;

                                        case 1:
                                                switch_stream_real (message->stream, message->type);
                                                break;
                                }

                                delete message;
                        }
                }

        void
                Play::push_message (Audio::Message const& message)
                {
                        //Glib::Mutex::Lock L (m_queue_lock);
                        g_async_queue_push (m_message_queue, (gpointer)(new Audio::Message (message)));
                        process_queue ();
                }
}
