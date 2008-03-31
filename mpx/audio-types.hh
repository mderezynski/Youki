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

#ifndef GST_TYPE_BUS
struct GstBus;
#endif // !GST_TYPE_BUS

#ifndef GST_TYPE_MESSAGE
struct GstMessage;
#endif // !GST_TYPE_MESSAGE

namespace MPX
{
  typedef sigc::signal<void, GstBus*, GstMessage*> SignalBusWatchCascade; 
  typedef std::vector<float> Spectrum; 
  typedef std::map<std::string, bool> MPXFileExtensions;

  enum MPXPlaystatus
  {
    PLAYSTATUS_NONE    = 0,
    PLAYSTATUS_STOPPED = 1 << 0,
    PLAYSTATUS_PLAYING = 1 << 1,
    PLAYSTATUS_PAUSED  = 1 << 2,
    PLAYSTATUS_SEEKING = 1 << 3,
    PLAYSTATUS_WAITING = 1 << 4
  };

  enum StateFlags
  {
    DONT_SET_STATE = 0
  };

  enum MPXGstMetadataField
  {
    FIELD_TITLE,
    FIELD_ALBUM,
    FIELD_IMAGE,
    FIELD_AUDIO_BITRATE,
    FIELD_AUDIO_CODEC,
    FIELD_VIDEO_CODEC,
  };

  struct MPXGstMetadata
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
