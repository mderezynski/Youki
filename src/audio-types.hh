//
// (C) 2007 DEREZYNSKI Milosz
//

#ifndef MPX_AUDIO_TYPES_HH
#define MPX_AUDIO_TYPES_HH

#include <sigc++/sigc++.h>
#include <vector>
#include <map>
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <gdkmm/pixbuf.h>
#include <gst/gst.h>

namespace MPX
{
  typedef sigc::signal<void, GstBus*, GstMessage*> SignalBusWatchCascade; 
  typedef std::vector<float> Spectrum; 
  typedef std::map<std::string, bool> MPXFileExtensions;

  enum StateFlags
  {
    DONT_SET_STATE = 0
  };

  enum GstMetadataField
  {
    FIELD_TITLE,
    FIELD_ALBUM,
    FIELD_IMAGE,
    FIELD_AUDIO_BITRATE,
    FIELD_AUDIO_CODEC,
    FIELD_VIDEO_CODEC,
  };

  struct GstMetadata
  {
    boost::optional<Glib::ustring>                m_title;
    boost::optional<Glib::ustring>                m_album;
    boost::optional<Glib::RefPtr<Gdk::Pixbuf> >   m_image;
    boost::optional<unsigned int>                 m_audio_bitrate;
    boost::optional<Glib::ustring>                m_audio_codec;
    boost::optional<Glib::ustring>                m_video_codec;

    void reset ()
    {
      m_title.reset ();
      m_album.reset ();
      m_image.reset ();
      m_audio_bitrate.reset ();
      m_audio_codec.reset ();
      m_video_codec.reset ();
      m_audio_bitrate.reset ();
    }
  };
}

#endif //!_MPX_PLAY_TYPES_HH
