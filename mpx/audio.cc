//  MPX - The Dumb Music Player
//  Copyright (C) 2005-2007 MPX development team.
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

#include <cstring>
#include <glibmm.h>

#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gst/interfaces/mixer.h>
#include <gst/interfaces/mixertrack.h>
#include <gst/interfaces/mixeroptions.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "audio.hh"
#include "mpx/uri.hh"
#include "mpx/util-string.hh"

namespace MPX
{
  namespace Audio
  {
    namespace
    {
      class ElementPropertySetter : public boost::static_visitor<>
      {
      public:

        ElementPropertySetter (GstElement*          element,
                               Element::Attr const& attr)
          : m_element (element),
            m_attr    (attr)
        {}

        template <typename T>
        void operator() (T const& value) const
        {
          static boost::format msg_fmt ("Setting property '%1%' to '%2%'");
          g_object_set (G_OBJECT (m_element), m_attr.name.c_str (), value);
        }

      private:

        GstElement*          m_element;
        Element::Attr const& m_attr;
      };
    }
  
    std::string
    get_ext_for_type (std::string const& type)
    {
      struct {
        char const * type;
        char const * ext;
      } type_list[] = { 
        {"application/ogg",       "ogg"},
        {"application/x-id3",     "mp3"},
        {"audio/mpeg",            "mp3"},
        {"video/x-ms-asf",        "wma"},
        {"audio/x-wav",           "wav"},
        {"audio/x-m4a",           "m4a"},
        {"application/x-apetag",  "mp3"} //FIXME: Use decodebin to find the actual stream type
      };

      std::string ext;

      for (unsigned int n = 0; n < G_N_ELEMENTS(type_list); ++n)
      {
        if (type == type_list[n].type)
        {
          ext = type_list[n].ext;
          break;
        } 
      }

      return ext;
    };

    bool
    is_audio_file (std::string const& uri)
    {
      static char const* extensions[] =
      { ".mp3",
        ".aac",
        ".mp4",
        ".m4a",
        ".m4b",
        ".m4p",
        ".m4v",
        ".mp4v",
        ".flac",
        ".wma",
        ".asf",
        ".sid",
        ".psid",
        ".mod",
        ".oct",
        ".xm",
        ".669",
        ".sht",
        ".mpc",
        ".ogg",
        ".wav",
        0 };

      try{
            MPX::URI u (uri);
            return Util::str_has_suffixes_nocase (u.path, extensions);
        } catch (...) {
            return false;
        }
    }

    bool
    test_element (std::string const& name)
    {
      GstElementFactory* factory = gst_element_factory_find (name.c_str ());
      bool exists = (factory != NULL);
      if (factory)
        gst_object_unref (factory);
      return exists;
    }

    Caps
    get_caps ()
    {
      Caps caps = CAPS_NONE;

      if (test_element ("mpx-neonhttpsrc"))
        caps = Caps (caps | CAPS_HTTP);

#if defined (HAVE_CDPARANOIA)
      if (test_element ("cdparanoiasrc"))
#elif defined (HAVE_CDIO)
      if (test_element ("cdiocddasrc"))
#endif
        caps = Caps (caps | CAPS_CDDA);

      if (test_element ("mmssrc"))
        caps = Caps (caps | CAPS_MMS);

      return caps;
    }

    void
    have_type_handler (GstElement*    typefind,
                       guint          probability,
                       GstCaps const* caps,
                       GstCaps**      p_caps)
    {
      if (p_caps)
        {
          *p_caps = gst_caps_copy (caps);
        }
    }

    std::string ElementNames[] =
      {
        "source",
        "decoder",
        "tee",
        "queue",
        "convert",
        "volume",
        "sink"
      };

    void
    link_pad (GstElement* element,
              GstPad*     pad,
              gboolean    last,
              gpointer    data)
    {
      GstPad* pad2 = GST_PAD (data);
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

    void
    outfit_element (GstElement*           element,
                    Element::Attrs const& attrs)
    {
      Glib::ScopedPtr<char> c_name (gst_element_get_name (element));
      std::string name (c_name.get());

      for (Element::Attrs::const_iterator iter = attrs.begin (); iter != attrs.end (); ++iter)
      {
          ElementPropertySetter setter (element, *iter);
          boost::apply_visitor (setter);
      }
    }

    GstElement*
    create_pipeline (GstElement*    src,
                     Element const& sink)
    {
      GstElement* pipeline = 0;
      GstElement* e[N_ELEMENTS];

      e[SOURCE] = src;

      //create sink
      e[SINK] = gst_element_factory_make (sink.name.c_str (), ElementNames[SINK].c_str ());

      if (!e[SINK])
        throw PipelineError ((boost::format ("Pipeline could not be created: Unable to create sink element '%s'") % sink.name).str ());

      outfit_element (e[SINK], sink.attrs);

      e[DECODER] = gst_element_factory_make ("decodebin",    ElementNames[DECODER].c_str ());
      e[CONVERT] = gst_element_factory_make ("audioconvert", ElementNames[CONVERT].c_str ());
      e[VOLUME]  = gst_element_factory_make ("volume",       ElementNames[VOLUME].c_str ());
      e[TEE]     = gst_element_factory_make ("tee",          ElementNames[TEE].c_str ());
      e[QUEUE]   = gst_element_factory_make ("queue",        ElementNames[QUEUE].c_str ());

      pipeline = gst_pipeline_new ("pipeline_file");

      gst_bin_add_many (GST_BIN (pipeline),
                        e[SOURCE],
                        e[DECODER],
                        e[TEE],
                        e[QUEUE],
                        e[CONVERT],
                        e[VOLUME],
                        e[SINK],
                        NULL);

      gst_element_link_many (e[SOURCE],
                             e[DECODER],
                             NULL);

      gst_element_link_many (e[TEE],
                             e[QUEUE],
                             e[CONVERT],
                             e[VOLUME],
                             e[SINK],
                             NULL);

      g_signal_connect (G_OBJECT (e[DECODER]),
                        "new-decoded-pad",
                        G_CALLBACK (link_pad),
                        gst_element_get_static_pad (e[TEE], "sink"));

      return pipeline;
    }

    GstElement*
    create_pipeline (Element const& source,
                     Element const& sink)
    {
      GstElement* e_source = 0;

      e_source = gst_element_factory_make (source. name. c_str (), ElementNames[SOURCE].c_str ());
      if (!e_source)
        {
          throw PipelineError ((boost::format
                                ("Pipeline could not be created: Unable to create source element '%s'")
                                % ElementNames[SOURCE].c_str ()).str());
        }
      outfit_element (e_source, source.attrs);
      return create_pipeline (e_source, sink);
    }

    bool
    typefind (std::string const& uri,
              std::string&       type)
    {
      GstStateChangeReturn state_change;
      GstElement* pipeline  = 0;
      GstElement* source    = 0;
      GstElement* typefind  = 0;
      GstElement* fakesink  = 0;
      GstCaps*    caps      = 0;
      GstState    state;

      if(uri.empty())
        return false;

      pipeline  = gst_pipeline_new ("pipeline");
      source    = gst_element_factory_make ("giosrc", "source");
      typefind  = gst_element_factory_make ("typefind", "typefind");
      fakesink  = gst_element_factory_make ("fakesink", "fakesink");

      gst_bin_add_many (GST_BIN (pipeline), source, typefind, fakesink, NULL);
      gst_element_link_many (source, typefind, fakesink, NULL);

      g_signal_connect (G_OBJECT (typefind), "have-type", G_CALLBACK (have_type_handler), &caps);

      g_object_set (source, "location", uri.c_str (), NULL);
      gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);

      state_change = gst_element_get_state (GST_ELEMENT (pipeline), &state, NULL, 5 * GST_SECOND);

      switch (state_change)
      {
        case GST_STATE_CHANGE_FAILURE:
          {
            //GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
            //gst_object_unref (bus);
            break;
          }

        case GST_STATE_CHANGE_SUCCESS:
          {
            if (caps)
              {
                char* caps_cstr = gst_caps_to_string (caps);
                gst_caps_unref (caps);

                std::string caps_str = caps_cstr;
                g_free (caps_cstr);

                std::vector<std::string> subs;
                boost::algorithm::split (subs, caps_str, boost::algorithm::is_any_of(","));

                if (subs.empty ())
                  {
                    type = caps_str;
                  }
                else
                  {
                    type = subs[0];
                  }

                gst_element_set_state (pipeline, GST_STATE_NULL);
                gst_object_unref (pipeline);
                return true;
              }
            else
              {
                gst_element_set_state (pipeline, GST_STATE_NULL);
                gst_object_unref (pipeline);
                return false;
              }
            break;
          }
        default: break;
      }

      gst_element_set_state (pipeline, GST_STATE_NULL);
      gst_object_unref (pipeline);
      return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    ProcessorBase::ProcessorBase ()
      : Glib::ObjectBase                  (typeid (this)),
        pipeline                          (0),
        prop_stream_time_                 (*this, "stream-time", 0),
        prop_stream_time_report_interval_ (*this, "stream-time-report-interval", 250),
        prop_length_                      (*this, "stream-length", 0),
        prop_volume_                      (*this, "volume", 50),
        prop_state_                       (*this, "processor-state", STATE_STOPPED)
    {
      prop_stream_time_report_interval () = 250;

      prop_stream_time_report_interval ().signal_changed ().connect
        (sigc::mem_fun (this, &ProcessorBase::position_send_interval_change));

      prop_volume().signal_changed().connect
        (sigc::mem_fun (this, &ProcessorBase::prop_volume_changed));
      conn_state = prop_state ().signal_changed ().connect
        (sigc::mem_fun (this, &ProcessorBase::prop_state_changed));
    }

    void
    ProcessorBase::prop_volume_changed ()
    {
      GstElement* element = gst_bin_get_by_name (GST_BIN (pipeline), ElementNames[VOLUME].c_str ());

      double volume = prop_volume_.get_value()/100.;

      g_object_set (element, "volume", volume, NULL);
      gst_object_unref (element);
    }

    void
    ProcessorBase::prop_state_changed ()
    {
      switch (prop_state_.get_value ())
        {
        case STATE_STOPPED:
          stop ();
          break;

        case STATE_PAUSED:
          pause ();
          break;

        case STATE_RUNNING:
          run ();
          break;
        }
    }

    ProcessorBase::~ProcessorBase ()
    {
    }

    void
    ProcessorBase::link_pad (GstElement* element,
                             GstPad*     srcpad,
                             gboolean    last,
                             gpointer    data)
    {
      GstPad* sinkpad = static_cast<GstPad*> (data);
      gst_pad_link (srcpad, sinkpad);
      gst_object_unref (sinkpad);
    }

    gboolean
    ProcessorBase::bus_watch (GstBus*     bus,
                              GstMessage* message,
                              gpointer    data)
    {
      ProcessorBase* processor = static_cast<ProcessorBase*> (data);

      switch (GST_MESSAGE_TYPE (message))
        {
        case GST_MESSAGE_TAG:
          {
            break;
          }

        case GST_MESSAGE_STATE_CHANGED:
          {
            GstState old_state, new_state, pending_state;

            gst_message_parse_state_changed (message,
                                             &old_state,
                                             &new_state,
                                             &pending_state);

            if ((new_state == GST_STATE_PLAYING) && !processor->conn_position.connected())
              {
                processor->position_send_stop ();
                processor->position_send_start ();
              }
            else if ((old_state >= GST_STATE_PAUSED) && (new_state < GST_STATE_PAUSED))
              {
                processor->position_send_stop ();
              }
            break;
          }

        case GST_MESSAGE_ERROR:
          {
            std::string location;

            GstElement* element = GST_ELEMENT (GST_MESSAGE_SRC (message));
            Glib::ScopedPtr<char> c_name (gst_object_get_name (GST_OBJECT (element)));
            gst_object_unref (element);
            std::string name (c_name.get());

            std::string message_str = std::string ("<b>ERROR [Element: <i>") + name + "</i>]</b>\n";

            GError *error = 0;
            Glib::ScopedPtr<char> c_debug;
            gst_message_parse_error (message, &error, c_debug.addr());
            std::string debug_str (c_debug.get());

            if (error->domain == GST_CORE_ERROR)
            {
                switch (error->code)
                {
                  case GST_CORE_ERROR_MISSING_PLUGIN:
                  {
                      message_str += "(E101) There is no plugin available to play "
                        "this track\n";
                      break;
                  }

                  case GST_CORE_ERROR_SEEK:
                  {
                      message_str += "(E102) An error occured during seeking\n";
                      break;
                  }

                  case GST_CORE_ERROR_STATE_CHANGE:
                  {
                      typedef struct _StringPairs StringPairs;
                      struct _StringPairs {
                        std::string   state_pre;
                        std::string   state_post;
                  };

                  const StringPairs string_pairs[] =
                  {
                      {"0",       "READY"},
                      {"READY",   "PAUSED"},
                      {"PAUSED",  "PLAYING"},
                      {"PLAYING", "PAUSED"},
                      {"PAUSED",  "READY"},
                      {"READY",   "0"},
                  };

                  message_str += "(E103) An error occured during changing the state "
                        "from "
                        + string_pairs[GST_STATE_PENDING (message->src)].state_pre
                        + " to "
                        + string_pairs[GST_STATE_PENDING (message->src)].state_post
                        + " (note that these are internal states and don't neccesarily "
                        "reflect what you might understand by e.g. 'play' or 'pause'\n";
                  break;
                }

                case GST_CORE_ERROR_PAD:
                {
                  message_str += "(E104) An error occured during pad "
                                     "linking/unlinking\n";
                  break;
                }

                case GST_CORE_ERROR_NEGOTIATION:
                {
                  message_str =+ "(E105) An error occured during element(s) "
                                     "negotiation\n";
                  break;
                }

                default: break;
              }
            }
            else if (error->domain == GST_RESOURCE_ERROR)
            {
                switch (error->code)
                {

                  case GST_RESOURCE_ERROR_SEEK:
                  {
                      message_str += "(E201) An error occured during seeking\n";
                      break;
                  }

                  case GST_RESOURCE_ERROR_NOT_FOUND:
                  case GST_RESOURCE_ERROR_OPEN_READ:
                  {
                      message_str += "(E202) Couldn't open the stream/track\n";
                      break;
                  }

                  default: break;
              }
            }
            else if (error->domain == GST_STREAM_ERROR)
            {
                switch (error->code)
                {
                  case GST_STREAM_ERROR_CODEC_NOT_FOUND:
                  {
                      message_str += "(E301) There is no codec installed to handle "
                                     "this stream\n";
                      break;
                  }

                  case GST_STREAM_ERROR_DECODE:
                  {
                      message_str += "(E302) There was an error decoding the stream\n";
                      break;
                  }

                  case GST_STREAM_ERROR_DEMUX:
                  {
                      message_str += "(E303) There was an error demultiplexing "
                                     "the stream\n";
                      break;
                  }

                  case GST_STREAM_ERROR_FORMAT:
                  {
                      message_str += "(E304) There was an error parsing the "
                                     "format of the stream\n";
                      break;
                  }

                  default: break;
               }
            }

            g_error_free (error);

            message_str += "\n<b>Current URI:</b>\n"+ location + "\n\n<b>Detailed debugging "
                           "information:</b>\n" + debug_str;

            processor->stop ();
            processor->signal_error_.emit(message_str);

            break;
        }

        case GST_MESSAGE_WARNING:
        {
            GError* error      = 0;
            gchar*  debug_cstr = 0;

            gst_message_parse_warning (message, &error, &debug_cstr);
            g_print ("%s WARNING: %s (%s)\n", G_STRLOC, error->message, debug_cstr);
            g_free (debug_cstr);
            g_error_free (error);
            break;
        }

        case GST_MESSAGE_EOS:
        {
            processor->conn_position.disconnect ();
            processor->signal_eos_.emit();
            break;
        }

        default: break;
      }

      return TRUE;
    }

    bool
    ProcessorBase::emit_stream_position ()
    {
      if ((GST_STATE (pipeline) != GST_STATE_PLAYING))
      {
          return false;
      }

      GstFormat format = GST_FORMAT_TIME;
      gint64    time_nsec;
      gst_element_query_position (pipeline, &format, &time_nsec);

      int time_sec = time_nsec / GST_SECOND;

      if ((time_sec >= 0) && (GST_STATE (pipeline) >= GST_STATE_PLAYING))
      {
          signal_position_.emit(time_sec);
          return true;
      }
      else
      {
          signal_position_.emit(0);
          return false;
      }
    }

    //Properties/Property proxies
    Glib::PropertyProxy<unsigned int>
    ProcessorBase::prop_stream_time ()
    {
      return prop_stream_time_.get_proxy ();
    }

    Glib::PropertyProxy<unsigned int>
    ProcessorBase::prop_stream_time_report_interval ()
    {
      return prop_stream_time_report_interval_.get_proxy ();
    }

    Glib::PropertyProxy<ProcessorState>
    ProcessorBase::prop_state ()
    {
      return prop_state_.get_proxy ();
    }

    Glib::PropertyProxy<int>
    ProcessorBase::prop_volume()
    {
      return prop_volume_.get_proxy();
    }

    Glib::PropertyProxy<unsigned int>
    ProcessorBase::prop_length()
    {
      GstFormat format = GST_FORMAT_TIME;
      gint64    length_nsec;
      gst_element_query_duration (pipeline, &format, &length_nsec);

      prop_length_ = length_nsec / GST_SECOND;

      return prop_length_.get_proxy();
    }

    void
    ProcessorBase::position_send_stop ()
    {
      if (conn_position.connected()) conn_position.disconnect ();
      signal_position_.emit(0);
    }

    void
    ProcessorBase::position_send_start ()
    {
      conn_position = Glib::signal_timeout().connect
        (sigc::mem_fun (this, &ProcessorBase::emit_stream_position),
         prop_stream_time_report_interval_.get_value ());
    }

    void
    ProcessorBase::position_send_interval_change ()
    {
      position_send_stop ();
      if (prop_state_.get_value () == STATE_RUNNING)
        {
          position_send_start ();
        }
    }

    //Signals
    SignalEos&
    ProcessorBase::signal_eos ()
    {
      return signal_eos_;
    }

    SignalError&
    ProcessorBase::signal_error ()
    {
      return signal_error_;
    }


    SignalPosition&
    ProcessorBase::signal_position ()
    {
      return signal_position_;
    }

    SignalStreamProperties&
    ProcessorBase::signal_stream_properties ()
    {
      return signal_stream_properties_;
    }


    GstStateChangeReturn
    ProcessorBase::stop ()
    {
      if (!pipeline)
        return GST_STATE_CHANGE_FAILURE; //FIXME: Exception

      conn_state.disconnect ();
      conn_position.disconnect ();

      prop_state_ = STATE_STOPPED;

      GstStateChangeReturn state_change = gst_element_set_state (pipeline, GST_STATE_READY);
      return state_change;
    }

    GstStateChangeReturn
    ProcessorBase::pause ()
    {
      if (!pipeline)
        return GST_STATE_CHANGE_FAILURE; //FIXME: Exception

      conn_state.disconnect ();
      prop_state_ = STATE_PAUSED;
      GstStateChangeReturn state_change = gst_element_set_state (pipeline, GST_STATE_PAUSED);
      conn_state = prop_state ().signal_changed ().connect (sigc::mem_fun (*this, &ProcessorBase::prop_state_changed));
      return state_change;
    }

    GstStateChangeReturn
    ProcessorBase::run ()
    {
      if (!pipeline)
        return GST_STATE_CHANGE_FAILURE; //FIXME: Exception

      conn_position.disconnect ();
      conn_state.disconnect ();
      prop_state_ = STATE_RUNNING;
      GstStateChangeReturn state_change = gst_element_set_state (pipeline, GST_STATE_PLAYING);
      conn_state = prop_state ().signal_changed ().connect (sigc::mem_fun (*this, &ProcessorBase::prop_state_changed));
      return state_change;
    }

    GstElement*
    ProcessorBase::tap ()
    {
      return gst_bin_get_by_name (GST_BIN(pipeline), ElementNames[TEE].c_str ());
    }

    bool
    ProcessorBase::verify_pipeline ()
    {
      GstIterator* iter = 0;
      GstIteratorResult result;

      iter = gst_bin_iterate_elements (GST_BIN (pipeline));

      while (iter)
      {
          gpointer elem;
          result = gst_iterator_next (iter, &elem);
          switch (result)
          {
            case GST_ITERATOR_DONE:
              gst_iterator_free (iter);
              return true;
              break;

            case GST_ITERATOR_OK:
              if (!GST_IS_ELEMENT (elem))
              {
                gst_iterator_free (iter);
                return false;
              }
              break;

            case GST_ITERATOR_ERROR:
              gst_iterator_free (iter);
              return false;
              break;

            case GST_ITERATOR_RESYNC:
              gst_iterator_resync (iter);
              break;
          }
      }
      return false;
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ProcessorURISink::ProcessorURISink  ()
      : Glib::ObjectBase (typeid (this))
    {
    }

    ProcessorURISink::~ProcessorURISink ()
    {
    }

    void
    ProcessorURISink::create_pipeline ()
    {
      if (pipeline)
      {
          gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
          gst_object_unref (pipeline);
      }

      pipeline = MPX::Audio::create_pipeline (source, sink);
    }

    void
    ProcessorURISink::set_uri (Glib::ustring const& _uri,
                               Element const&       sink)
    {
      URI u (_uri);
      URI::Protocol protocol = u.get_protocol ();

      stop ();
      this->stream = _uri;

      if (protocol != current_protocol)
      {
          switch (protocol)
          {
            case URI::PROTOCOL_FILE:
            {
                source = Element ("filesrc");
                source.add (Element::Attr ("location", Glib::filename_from_uri (_uri)));
                break;
            }

            case URI::PROTOCOL_HTTP:
            {
                source = Element ("mpx-jnethttpsrc");
                source.add (Element::Attr ("location", std::string (_uri)));
                source.add (Element::Attr ("iradio-mode", true));
                break;
            }

            case URI::PROTOCOL_CDDA:
            {
                int track = std::atoi (u.path.c_str () + 1);
#if defined (HAVE_CDPARANOIA)
                source = Element ("cdparanoiasrc");
#elif defined (HAVE_CDIO)
                source = Element ("cdiocddasrc");
#endif // HAVE_CDAPARNOIA/HAVE_CDIO
                source.add (Element::Attr ("track", track));
                break;
            }

            default: break; //FIXME: error condition
        }

        this->sink = sink;
        create_pipeline ();
      }
      else
      {
          switch (protocol)
          {
            case URI::PROTOCOL_FILE:
            {
                GstElement* element = gst_bin_get_by_name (GST_BIN (pipeline), ElementNames[SOURCE].c_str ());
                g_object_set (element, "location", Glib::filename_from_uri (_uri).c_str (), NULL);
                gst_object_unref (element);

                break;
            }

            case URI::PROTOCOL_HTTP:
            {
                GstElement* element = gst_bin_get_by_name (GST_BIN (pipeline), ElementNames[SOURCE].c_str ());
                g_object_set (element, "location", _uri.c_str (), NULL);
                gst_object_unref (element);

                break;
            }

            case URI::PROTOCOL_CDDA:
            {
                int track = std::atoi (u.path.c_str () + 1);

                GstElement* element = gst_bin_get_by_name (GST_BIN (pipeline), ElementNames[SOURCE].c_str ());
                g_object_set (element, "track", track, NULL);
                gst_object_unref (element);

                break;
            }

            default: break; //FIXME: error condition
        }
      }
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void
    ProcessorPUID::stream_eos ()
    {
      stop ();

      char* puid   = 0;
      char* artist = 0;
      char* title  = 0;

      GstElement* elmt = gst_bin_get_by_name (GST_BIN (pipeline), "puid");
      g_object_get (G_OBJECT (elmt),
                    "puid", &puid,
                    "artist", &artist,
                    "title", &title,
                    NULL);
      gst_object_unref (elmt);

      if (puid && strlen (puid))
      {
          m_puid = puid;
          g_free (puid);
      }

      if (artist && strlen (artist))
      {
          m_artist = artist;
          g_free (artist);
      }

      if (title && strlen (title))
      {
          m_title = title;
          g_free (title);
      }
    }

    boost::optional<Glib::ustring>
    ProcessorPUID::get_puid () const
    {
      return m_puid;
    }
    boost::optional<Glib::ustring>
    ProcessorPUID::get_artist () const
    {
      return m_artist;
    }
    boost::optional<Glib::ustring>
    ProcessorPUID::get_title () const
    {
      return m_title;
    }

    ProcessorPUID::ProcessorPUID ()
      : Glib::ObjectBase (typeid (this))
    {
      create_pipeline ();
      signal_eos().connect (sigc::mem_fun (this, &ProcessorPUID::stream_eos));
    }

    ProcessorPUID::~ProcessorPUID ()
    {
      gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
      gst_object_unref (GST_OBJECT (pipeline));
    }

    void
    ProcessorPUID::set_uri (Glib::ustring const& uri)
    {
      m_stream = uri;
      URI u (uri);
      URI::Protocol protocol = u.get_protocol();

      if (protocol != URI::PROTOCOL_FILE)
      {
        throw InvalidUriError ((boost::format ("Unable to parse URI '%s'") % uri.c_str ()).str());
      }

      GstElement* src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
      g_object_set (G_OBJECT (src), "location", Glib::filename_from_uri (m_stream).c_str (), NULL);
      gst_object_unref (src);
    }

    void
    ProcessorPUID::create_pipeline ()
    {
      GstElement* elements[N_ELEMENTS];

      elements[SOURCE] =
        gst_element_factory_make ("filesrc",
                                  "src");

      elements[DECODER] =
        gst_element_factory_make ("decodebin",
                                  "decoder");

      elements[CONVERT] =
        gst_element_factory_make ("audioconvert",
                                  "audioconvert");

      elements[PUID] =
        gst_element_factory_make ("puid-mpx",
                                  "puid");

      elements[SINK] =
        gst_element_factory_make ("fakesink",
                                  "sink");

      g_object_set (G_OBJECT (elements[SINK]), "sync", FALSE, NULL);
      g_object_set (G_OBJECT (elements[PUID]), "musicdns-id", "a7f6063296c0f1c9b75c7f511861b89b", NULL);

      pipeline = gst_pipeline_new ("pipeline");

      gst_bin_add_many (GST_BIN (pipeline),
                        elements[SOURCE],
                        elements[DECODER],
                        elements[CONVERT],
                        elements[PUID],
                        elements[SINK],
                        NULL);

      gst_element_link_many (elements[SOURCE],
                             elements[DECODER], NULL);

      gst_element_link_many (elements[CONVERT],
                             elements[PUID],
                             elements[SINK], NULL);

      g_signal_connect (G_OBJECT (elements[DECODER]),
                        "new-decoded-pad",
                        G_CALLBACK (link_pad),
                        gst_element_get_static_pad (elements[CONVERT], "sink"));

      GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
      gst_bus_add_watch (bus, GstBusFunc (bus_watch), this);
      gst_object_unref (bus);
    }

    ProcessorCDDA_Vorbis::ProcessorCDDA_Vorbis (std::string const& filename,
                                                unsigned int       track,
                                                std::string const& device,
                                                int                quality)
      : Glib::ObjectBase (typeid(this)),
        ProcessorTranscode (filename, track, device, quality)
    {
      ProcessorCDDA_Vorbis::create_pipeline ();
    }

    ProcessorCDDA_Vorbis::ProcessorCDDA_Vorbis ()
      : Glib::ObjectBase (typeid(this)),
        ProcessorTranscode ()
    {
      ProcessorCDDA_Vorbis::create_pipeline ();
    }

    ProcessorCDDA_Vorbis::~ProcessorCDDA_Vorbis ()
    {
      gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
      gst_object_unref (GST_OBJECT (pipeline));
    }

    void
    ProcessorCDDA_Vorbis::create_pipeline ()
    {
      GstElement* elements[N_ELEMENTS];

#if defined (HAVE_CDPARANOIA)
      elements[SOURCE]    = gst_element_factory_make ("cdparanoiasrc",  "src");
#elif defined (HAVE_CDIO)
      elements[SOURCE]    = gst_element_factory_make ("cdiocddasrc",  "src");
#endif // HAVE_CDPARANOIA/HAVE_CDIO

      elements[CONVERT]   = gst_element_factory_make ("audioconvert",   "audioconvert");
      elements[VORBISENC] = gst_element_factory_make ("vorbisenc",      "vorbisenc");
      elements[OGGMUX]    = gst_element_factory_make ("oggmux",         "oggmux");
      elements[SINK]      = gst_element_factory_make ("filesink",       "sink");

      if (!(elements[SOURCE] &&
            elements[CONVERT] &&
            elements[VORBISENC] &&
            elements[OGGMUX] &&
            elements[SINK]))
        {
          throw PipelineError ("Unable to create Pipeline");
        }

      pipeline = gst_pipeline_new ("pipeline");

      gst_bin_add_many (GST_BIN (pipeline),
                        elements[SOURCE],
                        elements[CONVERT],
                        elements[VORBISENC],
                        elements[OGGMUX],
                        elements[SINK],
                        NULL);

      gst_element_link_many (elements[SOURCE],
                             elements[CONVERT],
                             elements[VORBISENC],
                             elements[OGGMUX],
                             elements[SINK],
                             NULL);

      if (!verify_pipeline())
      {
          throw PipelineError ("Pipeline could not be created");
      }

      switch (m_quality)
      {
        case 0:
          g_object_set (G_OBJECT (elements[VORBISENC]),  "quality", 0.1, NULL);
          break;

        case 1:
          g_object_set (G_OBJECT (elements[VORBISENC]),  "quality", 0.4, NULL);
          break;

        case 2:
          g_object_set (G_OBJECT (elements[VORBISENC]),  "quality", 0.7, NULL);
          break;
      }

      g_object_set (G_OBJECT (elements[SINK]),
                    "location", m_filename.c_str (),
                    NULL);

      g_object_set (G_OBJECT (elements[SOURCE]),
                    "track", m_track,
                    "device", m_device.c_str (),
                    NULL);

      gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (pipeline)), GstBusFunc(bus_watch), this);
      prop_stream_time_report_interval() = 2000;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    ProcessorCDDA_MP3::ProcessorCDDA_MP3 (std::string const& filename,
                                          unsigned int       track,
                                          std::string const& device,
                                          int                quality)
      : Glib::ObjectBase (typeid(this)),
        ProcessorTranscode (filename, track, device, quality)
    {
      ProcessorCDDA_MP3::create_pipeline ();
    }

    ProcessorCDDA_MP3::ProcessorCDDA_MP3 ()
      : Glib::ObjectBase (typeid(this)),
        ProcessorTranscode ()
    {
      ProcessorCDDA_MP3::create_pipeline ();
    }

    ProcessorCDDA_MP3::~ProcessorCDDA_MP3 ()
    {
      gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
      gst_object_unref (GST_OBJECT (pipeline));
    }

    void
    ProcessorCDDA_MP3::create_pipeline ()
    {
      GstElement* elements[N_ELEMENTS];

#if defined (HAVE_CDPARANOIA)
      elements[SOURCE]  = gst_element_factory_make ("cdparanoiasrc", "src");
#elif defined (HAVE_CDIO)
      elements[SOURCE]  = gst_element_factory_make ("cdiocddasrc", "src");
#endif
      elements[CONVERT] = gst_element_factory_make ("audioconvert",  "audioconvert");
      elements[LAME]    = gst_element_factory_make ("lame",          "lame");
      elements[SINK]    = gst_element_factory_make ("filesink",      "sink");

      if (!(elements[SOURCE] && elements[CONVERT] && elements[LAME] && elements[SINK]))
      {
        throw PipelineError ("Pipeline could not be created");
      }

      g_object_set (G_OBJECT (elements[SINK]),
                    "location",
                    m_filename.c_str (),
                    NULL);

      g_object_set (G_OBJECT (elements[SOURCE]),
                    "track", m_track,
                    "device", m_device.c_str (),
                    NULL);

      pipeline = gst_pipeline_new ("pipeline");

      gst_bin_add_many (GST_BIN (pipeline),
                        elements[SOURCE],
                        elements[CONVERT],
                        elements[LAME],
                        elements[SINK], NULL);

      gst_element_link_many (elements[SOURCE],
                             elements[CONVERT],
                             elements[LAME],
                             elements[SINK],
                             NULL);

      if (!verify_pipeline())
      {
        throw PipelineError ("Pipeline could not be created");
      }

      switch (m_quality)
      {
        case 0:
          g_object_set (G_OBJECT (elements[LAME]),
                        "vbr", 4,
                        "vbr-mean-bitrate", 96,
                        "mode", 1,
                        NULL);
          break;

        case 1:
          g_object_set (G_OBJECT (elements[LAME]),
                        "vbr", 4,
                        "vbr-mean-bitrate", 128,
                        "mode", 1,
                        NULL);
          break;

        case 2:
          g_object_set (G_OBJECT (elements[LAME]),
                        "vbr", 4,
                        "vbr-mean-bitrate", 256,
                        "mode", 1,
                        NULL);
          break;
      }

      GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
      gst_bus_add_watch (bus, GstBusFunc (bus_watch), this);
      gst_object_unref (bus);

      prop_stream_time_report_interval () = 2000;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    ProcessorCDDA_FLAC::ProcessorCDDA_FLAC (std::string const& filename,
                                            unsigned int       track,
                                            std::string const& device,
                                            int                quality)
      : Glib::ObjectBase (typeid(this)),
        ProcessorTranscode (filename, track, device, quality)
    {
      ProcessorCDDA_FLAC::create_pipeline ();
    }

    ProcessorCDDA_FLAC::ProcessorCDDA_FLAC ()
      : Glib::ObjectBase (typeid(this)),
        ProcessorTranscode ()
    {
      ProcessorCDDA_FLAC::create_pipeline ();
    }

    ProcessorCDDA_FLAC::~ProcessorCDDA_FLAC ()
    {
      gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
      gst_object_unref (GST_OBJECT (pipeline));
    }

    void
    ProcessorCDDA_FLAC::create_pipeline ()
    {
      GstElement* elements[N_ELEMENTS];

#if defined (HAVE_CDPARANOIA)
      elements[SOURCE]  = gst_element_factory_make ("cdparanoiasrc", "src");
#elif defined (HAVE_CDIO)
      elements[SOURCE]  = gst_element_factory_make ("cdiocddasrc", "src");
#endif
      elements[CONVERT] = gst_element_factory_make ("audioconvert",  "audioconvert");
      elements[FLACENC] = gst_element_factory_make ("flacenc",       "flacenc");
      elements[SINK]    = gst_element_factory_make ("filesink",      "sink");

      if (!(elements[SOURCE] && elements[CONVERT] && elements[FLACENC] && elements[SINK]))
      {
        throw PipelineError ("Pipeline could not be created");
      }

      g_object_set (G_OBJECT (elements[SINK]),
                    "location", m_filename.c_str (),
                    NULL);

      g_object_set (G_OBJECT (elements[SOURCE]),
                    "track", m_track,
                    "device", m_device.c_str (),
                    NULL);

      pipeline = gst_pipeline_new ("pipeline");
      gst_bin_add_many (GST_BIN (pipeline),
                        elements[SOURCE],
                        elements[CONVERT],
                        elements[FLACENC],
                        elements[SINK],
                        NULL);

      gst_element_link_many (elements[SOURCE],
                             elements[CONVERT],
                             elements[FLACENC],
                             elements[SINK],
                             NULL);

      if (!verify_pipeline ())
      {
        throw PipelineError ("Pipeline could not be created");
      }

      switch (m_quality)
      {
        case 0:
          g_object_set (G_OBJECT (elements[FLACENC]), "quality", 3, NULL);
          break;

        case 1:
          g_object_set (G_OBJECT (elements[FLACENC]), "quality", 5, NULL);
          break;

        case 2:
          g_object_set (G_OBJECT (elements[FLACENC]), "quality", 7, NULL);
          break;
      }

      GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
      gst_bus_add_watch (bus, GstBusFunc (bus_watch), this);
      gst_object_unref (bus);

      prop_stream_time_report_interval () = 2000;
    }

    ProcessorBase*
    ProcessorFactory::get_processor (ProcessorType      type,
                                     std::string const& filename,
                                     unsigned int       track,
                                     std::string const& device,
                                     int                quality)
    {
      ProcessorBase *p = 0;
      switch (type)
      {
        case PROC_MP3:
        {
          p = new MPX::Audio::ProcessorCDDA_MP3 ( filename,
                                                  track, 
                                                  device,
                                                  quality ); 
          break;
        }

        case PROC_VORBIS:
        {
          p = new MPX::Audio::ProcessorCDDA_Vorbis ( filename,
                                                     track, 
                                                     device,
                                                     quality ); 
          break;
        }

        case PROC_FLAC:
        {
          p = new MPX::Audio::ProcessorCDDA_FLAC ( filename, 
                                                   track, 
                                                   device, 
                                                   quality ); 
          break;
        }

        default:
          p = 0;
          break;
      }
      return p;
    }

    bool
    ProcessorFactory::test_processor (ProcessorType type)
    {
      ProcessorBase *p = 0;

      try{
          switch (type)
          {
            case PROC_MP3:
            {
              p = new MPX::Audio::ProcessorCDDA_MP3 ();
              break;
            }

            case PROC_VORBIS:
            {
              p = new MPX::Audio::ProcessorCDDA_Vorbis ();
              break;
            }

            case PROC_FLAC:
            {
              p = new MPX::Audio::ProcessorCDDA_FLAC ();
              break;
            }

            default:
              p = 0;
              break;
          }
        }
      catch (PipelineError & cxe)
        { 
          if (p) delete p;
          return false;
        }

      if (p)
      {
        delete p;
        return true;
      }
      return false;
    }

    void
    ProcessorFileTranscode::create_pipeline ()
    {
      switch (m_type)
      {
        case CONV_OGG_VORBIS:
        {
          GstElement* element[8];
          element[0] = gst_element_factory_make ("filesrc",       "src");
          element[1] = gst_element_factory_make ("decodebin",     NULL); 
          element[2] = gst_element_factory_make ("audioconvert",  NULL); 
          element[3] = gst_element_factory_make ("audioresample", NULL); 
          element[4] = gst_element_factory_make ("audiorate",     NULL); 
          element[5] = gst_element_factory_make ("vorbisenc",     NULL); 
          element[6] = gst_element_factory_make ("oggmux",        NULL); 
          element[7] = gst_element_factory_make ("filesink",      "sink");

          for (int n=0; n<8; ++n)
          {
            if (!element[n])
            {
              g_warning ("FileTranscodeOGG: Pipeline could not be created, element %d doesn't check out", n);
              throw PipelineError ();
            }
          }

          g_object_set (G_OBJECT (element[0]),
                        "location", m_filename_src.c_str(),
                        NULL);

          g_object_set (G_OBJECT (element[7]),
                        "location", m_filename_dest.c_str (),
                        NULL);

          pipeline = gst_pipeline_new ("pipeline");
          gst_bin_add_many (GST_BIN (pipeline),
                            element[0],
                            element[1],
                            element[2],
                            element[3],
                            element[4],
                            element[5],
                            element[6],
                            element[7],
                            NULL);

          gst_element_link_many (element[0],
                                 element[1],
                                 NULL);

          gst_element_link_many (element[2],
                                 element[3],
                                 element[4],
                                 element[5],
                                 element[6],
                                 element[7],
                                 NULL);

          g_signal_connect (G_OBJECT (element[1]),
                            "new-decoded-pad",
                            G_CALLBACK (link_pad),
                            gst_element_get_static_pad (element[2], "sink"));

          if (!verify_pipeline ())
          {
            g_warning ("FileTranscodeOGG: Pipeline could not be created: Not verified");
            throw PipelineError ();
          }

#if 0
          switch (m_quality)
          {
            case 0:
              g_object_set (G_OBJECT (elements[FLACENC]), "quality", 3, NULL);
              break;

            case 1:
              g_object_set (G_OBJECT (elements[FLACENC]), "quality", 5, NULL);
              break;

            case 2:
              g_object_set (G_OBJECT (elements[FLACENC]), "quality", 7, NULL);
              break;
          }
#endif

        }
        break;

        case CONV_MPEG1_L3:
        {
          GstElement* element[7];
          element[0] = gst_element_factory_make ("filesrc",       "src");
          element[1] = gst_element_factory_make ("decodebin",     NULL); 
          element[2] = gst_element_factory_make ("audioconvert",  NULL);
          element[3] = gst_element_factory_make ("audioresample", NULL); 
          element[4] = gst_element_factory_make ("lame",          NULL); 
          element[5] = gst_element_factory_make ("id3v2mux",      NULL); 
          element[6] = gst_element_factory_make ("filesink",      "sink");

          for (int n=0; n<7; ++n)
          {
            if (!element[n])
            {
              g_warning ("FileTranscodeMP3: Pipeline could not be created, element %d doesn't check out", n);
              throw PipelineError ();
            }
          }

          if (!m_filename_src.empty())
            g_object_set (G_OBJECT (element[0]),
                          "location", m_filename_src.c_str(),
                          NULL);

          if (!m_filename_dest.empty())
            g_object_set (G_OBJECT (element[6]),
                          "location", m_filename_dest.c_str (),
                          NULL);

          pipeline = gst_pipeline_new ("pipeline");
          gst_bin_add_many (GST_BIN (pipeline),
                            element[0],
                            element[1],
                            element[2],
                            element[3],
                            element[4],
                            element[5],
                            element[6],
                            NULL);

          gst_element_link_many (element[0],
                                 element[1],
                                 NULL);

          gst_element_link_many (element[2],
                                 element[3],
                                 element[4],
                                 element[5],
                                 element[6],
                                 NULL);

          g_signal_connect (G_OBJECT (element[1]),
                            "new-decoded-pad",
                            G_CALLBACK (link_pad),
                            gst_element_get_static_pad (element[2], "sink"));

          if (!verify_pipeline ())
          {
            g_warning ("FileTranscodeMP3: Pipeline could not be created: Not verified");
            throw PipelineError ();
          }

#if 0
          switch (m_quality)
          {
            case 0:
              g_object_set (G_OBJECT (elements[FLACENC]), "quality", 3, NULL);
              break;

            case 1:
              g_object_set (G_OBJECT (elements[FLACENC]), "quality", 5, NULL);
              break;

            case 2:
              g_object_set (G_OBJECT (elements[FLACENC]), "quality", 7, NULL);
              break;
          }
#endif

        }
        break;
      }

      GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
      gst_bus_add_watch (bus, GstBusFunc (bus_watch), this);
      gst_object_unref (bus);
      prop_stream_time_report_interval () = 1000;
    }

    ProcessorFileTranscode::ProcessorFileTranscode (std::string const&  filename_src,
                                                    std::string const&  filename_dest,
                                                    ConversionType type)
      : Glib::ObjectBase  (typeid (this)),
        ProcessorBase     (),
        m_filename_src    (filename_src),
        m_filename_dest   (filename_dest),
        m_type            (type)
    {
      create_pipeline ();
    }

    ProcessorFileTranscode::ProcessorFileTranscode (ConversionType type)
      : Glib::ObjectBase  (typeid (this)),
        ProcessorBase     (),
        m_type            (type)
    {
      create_pipeline ();
    }

  } // Audio namespace
} // MPX namespace
