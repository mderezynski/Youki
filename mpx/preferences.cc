//  MPX
//  Copyright (C) 2003-2007 MPX Development
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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <utility>
#include <iostream>

#include <glibmm.h>
#include <glib/gi18n.h>
#include <gtkmm.h>
#include <libglademm.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#ifdef HAVE_ALSA
#  define ALSA_PCM_NEW_HW_PARAMS_API
#  define ALSA_PCM_NEW_SW_PARAMS_API

#  include <alsa/global.h>
#  include <alsa/asoundlib.h>
#  include <alsa/pcm_plugin.h>
#  include <alsa/control.h>
#endif //HAVE_ALSA

#include <mcs/mcs.h>

#include "mpx/mpx-audio.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/widgetloader.hh"

#include "preferences.hh"

using namespace Glib;
using namespace Gtk;
using namespace MPX::Audio;

namespace MPX
{
  namespace
  {
    static boost::format gbyte_f  (_("%.2Lf GiB"));
    static boost::format mbyte_f  (_("%.2f MiB"));
    static boost::format uint64_f ("%llu");

    const double MBYTE = 1048576;

    std::string
    get_size_string (guint64 size_)
    {
      double size = size_/MBYTE;
      return (mbyte_f % size).str();

      /* FIXME
      if (size > 1024)
        return (gbyte_f % (size / 1024.)).str();
      else
        return (mbyte_f % size).str();
      */ 
    }

    struct AudioSystem
    {
        char const* description;
        char const* name;
        gint        tab;
        Sink        sink;
    };

    AudioSystem audiosystems[] =
    {
        {"Automatic Choice",
         "autoaudiosink",
         0,
         SINK_AUTO},

        {"GNOME Configured Audio Output",
         "gconfaudiosink",
         1,
         SINK_GCONF},

#ifdef HAVE_ALSA
        {"ALSA Audio Output",
         "alsasink",
         2,
         SINK_ALSA},
#endif //HAVE_ALSA

        {"OSS",
         "osssink",
         3,
         SINK_OSS},

#ifdef HAVE_SUN
        {"Sun/Solaris Audio",
         "sunaudiosink",
         4,
         SINK_SUNAUDIO},
#endif // HAVE_SUN

        {"ESD",
         "esdsink",
         5,
         SINK_ESD},

#ifdef HAVE_HAL
        {"HAL",
         "halaudiosink",
         6,
         SINK_HAL},
#endif // HAVE_HAL

        {"PulseAudio",
         "pulsesink",
         7,
         SINK_PULSEAUDIO},

        {"JACK",
         "jackaudiosink",
         8,
         SINK_JACKSINK},

    };

    char const* sources[] =
    {
        N_("Local files (folder.jpg, cover.jpg, etc)"),
        N_("MusicBrainz AR"),
        N_("Amazon ASIN"),
        N_("Amazon search"),
        N_("Inline covers")
    };
  } // <anonymous> namespace
    

  using namespace Gnome::Glade;

  typedef sigc::signal<void, int, bool> SignalColumnState;

  class CoverArtSourceView : public WidgetLoader<Gtk::TreeView>
  {
      public:
             
              struct Signals_t 
              {
                  SignalColumnState ColumnState;
              };

              Signals_t Signals;

              class Columns_t : public Gtk::TreeModelColumnRecord
              {
                  public: 

                          Gtk::TreeModelColumn<Glib::ustring> Name;
                          Gtk::TreeModelColumn<int>           ID;
                          Gtk::TreeModelColumn<bool>          Active;

                          Columns_t ()
                          {
                              add(Name);
                              add(ID);
                              add(Active);
                          };
              };


              Columns_t                       Columns;
              Glib::RefPtr<Gtk::ListStore>    Store;

              CoverArtSourceView (const Glib::RefPtr<Gnome::Glade::Xml>& xml)
              : WidgetLoader<Gtk::TreeView>(xml, "preferences-treeview-coverartsources")
              {
                  Store = Gtk::ListStore::create(Columns); 
                  set_model(Store);

                  TreeViewColumn *col = manage( new TreeViewColumn(_("Active")));
                  CellRendererToggle *cell1 = manage( new CellRendererToggle );
                  cell1->property_xalign() = 0.5;
                  cell1->signal_toggled().connect(
                      sigc::mem_fun(
                          *this,
                          &CoverArtSourceView::on_cell_toggled
                  ));
                  col->pack_start(*cell1, true);
                  col->add_attribute(*cell1, "active", Columns.Active);
                  append_column(*col);            

                  append_column(_("Column"), Columns.Name);            

                  for( int i = 0; i < G_N_ELEMENTS(sources); ++i )
                  {
                      TreeIter iter = Store->append();
                      
                      int source = mcs->key_get<int>("Preferences-CoverArtSources", (boost::format ("Source%d") % i).str());

                      (*iter)[Columns.Name]   = _(sources[source]);
                      (*iter)[Columns.ID]     = source; 
                      (*iter)[Columns.Active] = mcs->key_get<bool>("Preferences-CoverArtSources", (boost::format ("SourceActive%d") % i).str());
                  }
              };

              void
              update_configuration ()
              {
                using namespace Gtk;

                TreeNodeChildren const& children = Store->children();

                for(TreeNodeChildren::const_iterator i = children.begin(); i != children.end(); ++i) 
                {
                    int pos = std::distance(children.begin(), i);

                    mcs->key_set<int>("Preferences-CoverArtSources", (boost::format ("Source%d") % pos).str(), (*i)[Columns.ID]);
                    mcs->key_set<bool>("Preferences-CoverArtSources", (boost::format ("SourceActive%d") % pos).str(), (*i)[Columns.Active]);
                }
              }

              void
              on_cell_toggled(Glib::ustring const& path)
              {
                  TreeIter iter = Store->get_iter(path);

                  bool active = (*iter)[Columns.Active];
                  (*iter)[Columns.Active] = !active;

                  Signals.ColumnState.emit((*iter)[Columns.ID], !active);

                  update_configuration ();
              }

              virtual void
              on_rows_reordered (const TreeModel::Path& path, const TreeModel::iterator& iter, int* new_order)
              {
                  update_configuration ();
              }
  };

  void
  Preferences::audio_system_changed ()
  {
    m_notebook_audio_system->set_current_page ((*m_cbox_audio_system->get_active ())[audio_system_columns.tab]);
  }

  void
  Preferences::audio_system_apply_set_sensitive ()
  {
    m_warning_audio_system_changed->set_sensitive (true);

    m_button_audio_system_apply->set_sensitive (true);
    m_button_audio_system_reset->set_sensitive (true);

#ifdef HAVE_ALSA
    if ((m_cbox_alsa_card->get_active_row_number () != -1)
       && (m_cbox_alsa_device->get_active_row_number () != -1))
    {
        TreeModel::iterator iter (m_cbox_alsa_device->get_active ());
        m_ignore_device_string_set = true;
        m_alsa_device_string->set_text(AlsaDevice ((*iter)[m_alsa_device_columns.device]).m_handle);
    }
#endif // HAVE_ALSA
  }

  void
  Preferences::audio_system_apply ()
  {
    m_Play.request_status (PLAYSTATUS_STOPPED);

    m_warning_audio_system_changed->set_sensitive (false);

    m_button_audio_system_apply->set_sensitive (false);
    m_button_audio_system_reset->set_sensitive (false);

    TreeModel::iterator iter (m_cbox_audio_system->get_active ());
    Sink sink = (*iter)[audio_system_columns.sink];
    std::string name = (*iter)[audio_system_columns.name];
    mcs->key_set<std::string> ("audio", "sink", name);

    switch (sink)
    {
#ifdef HAVE_ALSA
      case SINK_ALSA:
      {
        mcs->key_set<std::string> ("audio", "device-alsa", m_alsa_device_string->get_text()); 
        break;
      }
#endif //HAVE_ALSA
      default: ;
    }
    mcs->key_set <int> ("audio", "video-output", m_cbox_video_out->get_active_row_number());
    m_Play.reset();
    setup_audio();
  }

#ifdef HAVE_ALSA
  void
  Preferences::on_alsa_device_string_changed ()
  {
    if( m_ignore_device_string_set )
    {
        m_ignore_device_string_set = false;
        return;
    }

    m_cbox_alsa_card->set_active (-1);
    m_cbox_alsa_device->set_active (-1);
  } 

  void
  Preferences::on_alsa_card_changed ()
  {
    m_ignore_device_string_set = true;

    m_list_store_alsa_device->clear ();
    int row = m_cbox_alsa_card->get_active_row_number ();

    if( row == -1 )
      return;

    AlsaCard const& card = (*m_cbox_alsa_card->get_active ())[m_alsa_card_columns.card];

    if (row == 0)
    {
        m_alsa_device_string->set_text("default");
    }

    if ((row == 0) || card.m_devices.empty())
    {
      m_cbox_alsa_device->set_sensitive (false);
      return;
    }

    for (AlsaDevices::const_iterator i = card.m_devices.begin () ; i != card.m_devices.end() ; ++i)
    {
      TreeModel::iterator iter (m_list_store_alsa_device->append ());
      (*iter)[m_alsa_device_columns.name]   = i->m_name;
      (*iter)[m_alsa_device_columns.device] = *i;
    }

    m_cbox_alsa_device->set_active (0);
    m_cbox_alsa_device->set_sensitive (true);
  }

  Preferences::AlsaCards
  Preferences::get_alsa_cards ()
  {
    AlsaCards cards;
    int card_id (-1);
    while (!snd_card_next (&card_id) && (card_id > -1))
    {
      snd_ctl_t* control (0);
      if (!snd_ctl_open (&control, (boost::format ("hw:%d") % card_id).str().c_str(), SND_CTL_ASYNC))
      {
        snd_ctl_card_info_t * card_info (0);
        snd_ctl_card_info_malloc (&card_info);

        if (!snd_ctl_card_info (control, card_info))
        {
          using namespace std;

          string   card_handle   (snd_ctl_name (control));
          int      card_card_id  (snd_ctl_card_info_get_card (card_info));
          string   card_id       (snd_ctl_card_info_get_id (card_info));
          string   card_name     (snd_ctl_card_info_get_name (card_info));
          string   card_longname (snd_ctl_card_info_get_longname (card_info));
          string   card_driver   (snd_ctl_card_info_get_driver (card_info));
          string   card_mixer    (snd_ctl_card_info_get_mixername (card_info));

          AlsaDevices _devices;

          int device_id (-1);
          while (!snd_ctl_pcm_next_device (control, &device_id) && (device_id > -1))
          {
            snd_pcm_info_t * pcm_info (0);
            snd_pcm_info_malloc (&pcm_info);
            snd_pcm_info_set_device (pcm_info, device_id);

            if (!snd_ctl_pcm_info (control, pcm_info))
            {
              if (snd_pcm_info_get_stream (pcm_info) == SND_PCM_STREAM_PLAYBACK)
              {
                string device_handle  ((boost::format ("%s,%d") % snd_ctl_name (control) % device_id).str());
                string device_name    (snd_pcm_info_get_name (pcm_info));

                _devices.push_back (AlsaDevice (device_handle,
                                                card_card_id,
                                                device_id,
                                                device_name));
              }
            }
            if (pcm_info) snd_pcm_info_free (pcm_info);
          }

          if (_devices.size())
          {
            cards.push_back (AlsaCard ( card_handle,
                                        card_card_id,
                                        card_id,
                                        card_name,
                                        card_longname,
                                        card_driver,
                                        card_mixer,
                                        _devices ));
          }
        }
        if (card_info) snd_ctl_card_info_free (card_info);
      }
      if (control) snd_ctl_close (control);
    }
    return cards;
  }
#endif //HAVE_ALSA

#define PRESENT_SINK(n) ((m_sinks.find (n) != m_sinks.end()))
#define CURRENT_SINK(n) ((sink == n))
#define NONE_SINK (-1)

  Gtk::StockID
  Preferences::get_plugin_stock (bool truth)
  {
    return (truth ? StockID(MPX_STOCK_PLUGIN) : StockID(MPX_STOCK_PLUGIN_DISABLED));
  }

  void
  Preferences::setup_audio_widgets ()
  {
    audio_system_cbox_ids.resize(16);
 
    m_ref_xml->get_widget ("cbox_video_out", m_cbox_video_out);

    CellRendererText * cell;
    m_list_store_audio_systems = ListStore::create (audio_system_columns);
    cell = manage (new CellRendererText() );
    m_cbox_audio_system->clear ();
    m_cbox_audio_system->pack_start (*cell);
    m_cbox_audio_system->add_attribute (*cell, "text", 0);
    m_cbox_audio_system->set_model (m_list_store_audio_systems);

    std::string sink = mcs->key_get<std::string> ("audio", "sink");
    int cbox_counter = 0;
    for (unsigned int n = 0; n < G_N_ELEMENTS (audiosystems); n++)
    {
        if (test_element (audiosystems[n].name))
        {
              audio_system_cbox_ids[n] = cbox_counter++;
              m_sinks.insert (audiosystems[n].name);
              TreeIter iter = m_list_store_audio_systems->append ();
              (*iter)[audio_system_columns.description] = audiosystems[n].description;
              (*iter)[audio_system_columns.name] = audiosystems[n].name;
              (*iter)[audio_system_columns.tab] = audiosystems[n].tab;
              (*iter)[audio_system_columns.sink] = audiosystems[n].sink;
        }
    }

#ifdef HAVE_ALSA
    if (PRESENT_SINK ("alsasink"))
    {
        m_list_store_alsa_cards = ListStore::create (m_alsa_card_columns);
        m_list_store_alsa_device = ListStore::create (m_alsa_device_columns);

        cell = manage (new CellRendererText () );
        m_cbox_alsa_card->clear ();
        m_cbox_alsa_card->pack_start (*cell);
        m_cbox_alsa_card->add_attribute (*cell, "text", 0);

        cell = manage (new CellRendererText () );
        m_cbox_alsa_device->clear ();
        m_cbox_alsa_device->pack_start (*cell);
        m_cbox_alsa_device->add_attribute (*cell, "text", 0);

        m_cbox_alsa_device->set_model (m_list_store_alsa_device);
        m_cbox_alsa_card->set_model (m_list_store_alsa_cards);

        m_cbox_alsa_card->signal_changed ().connect
          (sigc::mem_fun (*this, &Preferences::on_alsa_card_changed));

        m_alsa_device_string->signal_changed().connect(
          sigc::mem_fun (*this, &Preferences::on_alsa_device_string_changed
        ));

        m_ignore_device_string_set = false;

        mcs_bind->bind_spin_button (*m_alsa_buffer_time, "audio", "alsa-buffer-time");

        m_alsa_buffer_time->signal_value_changed().connect
          (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));

        m_cbox_alsa_device->signal_changed().connect
          (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));

        m_cbox_alsa_card->signal_changed().connect(
          sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive
        ));
    }
#endif //HAVE_ALSA

#ifdef HAVE_SUN
    if (PRESENT_SINK("sunaudiosink"))
    {
      mcs_bind->bind_cbox_entry (*m_sun_cbe_device, "audio", "device-sun");
      mcs_bind->bind_spin_button (*m_sun_buffer_time, "audio", "sun-buffer-time");

      m_sun_buffer_time->signal_value_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
      m_sun_cbe_device->signal_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
    }
#endif //HAVE_SUN

    if (PRESENT_SINK("osssink"))
    {
      mcs_bind->bind_cbox_entry (*m_oss_cbe_device, "audio", "device-oss");
      mcs_bind->bind_spin_button (*m_oss_buffer_time, "audio", "oss-buffer-time");

      m_oss_cbe_device->signal_changed().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
      m_oss_buffer_time->signal_value_changed().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
    }

    if (PRESENT_SINK("esdsink"))
    {
      mcs_bind->bind_entry (*m_esd_host, "audio", "device-esd");
      mcs_bind->bind_spin_button (*m_esd_buffer_time, "audio", "esd-buffer-time");

      m_esd_buffer_time->signal_value_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
      m_esd_host->signal_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
    }

    if (PRESENT_SINK("pulsesink"))
    {
      mcs_bind->bind_entry (*m_pulse_server, "audio", "pulse-server");
      mcs_bind->bind_entry (*m_pulse_device, "audio", "pulse-device");
      mcs_bind->bind_spin_button (*m_pulse_buffer_time, "audio", "pulse-buffer-time");

      m_pulse_buffer_time->signal_value_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
      m_pulse_server->signal_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
      m_pulse_device->signal_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
    }

    if (PRESENT_SINK("jackaudiosink"))
    {
      mcs_bind->bind_entry (*m_jack_server, "audio", "jack-server");
      mcs_bind->bind_spin_button (*m_jack_buffer_time, "audio", "jack-buffer-time");

      m_jack_buffer_time->signal_value_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
      m_jack_server->signal_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
    }

#ifdef HAVE_HAL
    if (PRESENT_SINK("halaudiosink"))
    {
      m_ref_xml->get_widget("halaudio_udi", m_halaudio_udi);
      mcs_bind->bind_entry (*m_halaudio_udi, "audio", "hal-udi");
      m_halaudio_udi->signal_changed ().connect
        (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));
    }
#endif // HAVE_HAL

    m_button_audio_system_apply->signal_clicked().connect
      (sigc::mem_fun (*this, &Preferences::audio_system_apply));

    m_button_audio_system_reset->signal_clicked().connect
      (sigc::mem_fun (*this, &Preferences::setup_audio));

    m_cbox_audio_system->signal_changed().connect
      (sigc::mem_fun (*this, &Preferences::audio_system_changed));

    m_cbox_audio_system->signal_changed().connect
      (sigc::mem_fun (*this, &Preferences::audio_system_apply_set_sensitive));

    // Setup Support Status Indicators
#if defined (HAVE_CDPARANOIA)
    bool has_cdda = test_element ("cdparanoiasrc");
#elif defined (HAVE_CDIO)
    bool has_cdda = test_element ("cdiocddasrc");
#endif
    bool has_mmsx = test_element ("mmssrc");
    bool has_http = true; // built-in http src 

    dynamic_cast<Image*> (m_ref_xml->get_widget ("img_status_cdda"))->set
       (get_plugin_stock (has_cdda), ICON_SIZE_SMALL_TOOLBAR);

    dynamic_cast<Image*> (m_ref_xml->get_widget ("img_status_http"))->set
       (get_plugin_stock (has_http), ICON_SIZE_SMALL_TOOLBAR);

    dynamic_cast<Image*> (m_ref_xml->get_widget ("img_status_mms"))->set
       (get_plugin_stock (has_mmsx), ICON_SIZE_SMALL_TOOLBAR);

    struct SupportCheckTuple
    {
      char const* widget;
      char const* element;
    };

    const SupportCheckTuple support_check[] =
    {
      {"img_status_mpc",    "musepackdec"},
      {"img_status_flac",   "flacdec"},
      {"img_status_m4a",    "faad"},
      {"img_status_wma",    "ffdec_wmav2"},
      {"img_status_sid",    "siddec"},
      {"img_status_wav",    "wavparse"},
      {"img_status_mod",    "modplug"},
      {"img_status_spc",    "spcdec"},
    };

    // Check for either mad or flump3dec
    dynamic_cast<Image*> (m_ref_xml->get_widget ("img_status_mp3"))->set
        (get_plugin_stock ((test_element ("mad") ||
                  test_element ("flump3dec")) &&
                  test_element("id3demux") &&
                  test_element ("apedemux")),
        ICON_SIZE_SMALL_TOOLBAR);

    dynamic_cast<Image*> (m_ref_xml->get_widget ("img_status_ogg"))->set
      (get_plugin_stock (test_element ("oggdemux") && test_element("vorbisdec")), ICON_SIZE_SMALL_TOOLBAR);

    for (unsigned int n = 0; n < G_N_ELEMENTS (support_check); ++n)
    {
      dynamic_cast<Image*> (m_ref_xml->get_widget (support_check[n].widget))->set
        (get_plugin_stock (test_element (support_check[n].element)), ICON_SIZE_SMALL_TOOLBAR);
    }

    bool video = (test_element ("filesrc") && test_element ("decodebin") && test_element ("queue") &&
                  test_element ("ffmpegcolorspace") && test_element ("xvimagesink"));
    
    dynamic_cast<Image*> (m_ref_xml->get_widget ("img_status_video"))->set (get_plugin_stock (video), ICON_SIZE_SMALL_TOOLBAR);
    m_cbox_video_out->signal_changed().connect
      (sigc::mem_fun( *this, &Preferences::audio_system_apply_set_sensitive) );

    dynamic_cast<ToggleButton *>(m_ref_xml->get_widget ("enable-eq"))->signal_toggled().connect
      (sigc::mem_fun( *this, &Preferences::audio_system_apply_set_sensitive) );
  }

  void
  Preferences::setup_audio ()
  {
    m_notebook_audio_system->set_current_page( 0 );

    std::string sink = mcs->key_get<std::string> ("audio", "sink");
    int x = NONE_SINK;

    for (unsigned int n = 0; n < G_N_ELEMENTS (audiosystems); n++)
    {
      if (PRESENT_SINK(audiosystems[n].name) && CURRENT_SINK(audiosystems[n].name))
      {
        x = n;
        break;
      }
    }

    if( x == NONE_SINK )
    {
        x = SINK_AUTO; // we fallback to autoaudiosink if garbage or unavailable output is in the config file
    }
	
    m_cbox_audio_system->set_active (audio_system_cbox_ids[x]);
  
    if( x != NONE_SINK )
    {
      m_notebook_audio_system->set_current_page (audiosystems[x].tab);
    }

#ifdef HAVE_ALSA
    if (PRESENT_SINK ("alsasink"))
    {
      AlsaCards cards (get_alsa_cards());

      m_list_store_alsa_cards->clear ();
      m_list_store_alsa_device->clear ();

      TreeModel::iterator iter = m_list_store_alsa_cards->append ();
      (*iter)[m_alsa_card_columns.name] = _("System Default");

      for (AlsaCards::iterator i = cards.begin (); i != cards.end (); ++i)
      {
        iter = m_list_store_alsa_cards->append ();
        (*iter)[m_alsa_card_columns.name] = ustring (*i);
        (*iter)[m_alsa_card_columns.card] = *i;
      }

      if (CURRENT_SINK ("alsasink"))
      {
        std::string device (mcs->key_get<std::string> ("audio", "device-alsa"));

        if (device.substr(0,2) == "hw")
        {
            std::vector<std::string> subs;

            using namespace boost::algorithm;
            split (subs, device, is_any_of (":,"));

            if (subs.size() && subs[0] == "hw")
            {
                int card (atoi (subs[1].c_str ()));
                int device (atoi (subs[2].c_str ()));
                m_cbox_alsa_card->set_active (card);
                m_cbox_alsa_device->set_active (device);
            }
        }
        else
        if (device == "default")
        {
            m_cbox_alsa_card->set_active (0);
        }
        else
        {
            m_cbox_alsa_card->set_active (-1);
            m_cbox_alsa_device->set_active (-1);
            m_alsa_device_string->set_text(device);
        }

      }
    }
#endif //HAVE_ALSA

#ifdef HAVE_SUN
    if (PRESENT_SINK("sunaudiosink"))
    {
      mcs->key_push ("audio", "device-sun");
      mcs->key_push ("audio", "sun-buffer-time");
    }
#endif //HAVE_SUN

    if (PRESENT_SINK("osssink"))
    {
      mcs->key_push ("audio", "device-oss");
      mcs->key_push ("audio", "oss-buffer-time");
    }

    if (PRESENT_SINK("esdsink"))
    {
      mcs->key_push ("audio", "device-esd");
      mcs->key_push ("audio", "esd-buffer-time");
    }

    if (PRESENT_SINK("pulsesink"))
    {
      mcs->key_push ("audio", "pulse-server");
      mcs->key_push ("audio", "pulse-device");
      mcs->key_push ("audio", "pulse-buffer-time");
    }

    if (PRESENT_SINK("jackaudiosink"))
    {
      mcs->key_push ("audio", "jack-server");
      mcs->key_push ("audio", "jack-buffer-time");
    }

#ifdef HAVE_HAL
    if (PRESENT_SINK("halaudiosink"))
    {
      mcs->key_push ("audio", "hal-udi");
    }
#endif // HAVE_HAL

    m_cbox_video_out->set_active (mcs->key_get <int> ("audio", "video-output"));

    m_warning_audio_system_changed->set_sensitive (0);
    m_button_audio_system_apply->set_sensitive (0);
    m_button_audio_system_reset->set_sensitive (0);
  }


  //// Preferences

  Preferences*
  Preferences::create (MPX::Play & play)
  {
      return new Preferences (Gnome::Glade::Xml::create (build_filename(DATA_DIR, "glade" G_DIR_SEPARATOR_S "preferences.glade")), play); 
  }

  Preferences::Preferences (RefPtr<Gnome::Glade::Xml> const& xml, MPX::Play & play)
  : Gnome::Glade::WidgetLoader<Gtk::Window>(xml, "preferences")
  , m_ref_xml(xml)
  , m_Play(play)
  {
      mcs_bind = new Mcs::Bind(mcs);

      dynamic_cast<Button*>(m_ref_xml->get_widget ("close"))->signal_clicked().connect(
        sigc::mem_fun(
            *this,
            &Preferences::hide
      ));

      m_ref_xml->get_widget ("cbox_audio_system", m_cbox_audio_system);

#ifdef HAVE_ALSA
      m_ref_xml->get_widget ("cbox_alsa_card", m_cbox_alsa_card);
      m_ref_xml->get_widget ("cbox_alsa_device", m_cbox_alsa_device);
      m_ref_xml->get_widget ("alsa_buffer_time", m_alsa_buffer_time);
      m_ref_xml->get_widget ("alsa_device_string", m_alsa_device_string);
#endif //HAVE_ALSA

#ifdef HAVE_SUN
      m_ref_xml->get_widget ("sun_cbe_device",  m_sun_cbe_device);
      m_ref_xml->get_widget ("sun_buffer_time", m_sun_buffer_time);
#endif //HAVE_SUN

      m_ref_xml->get_widget ("oss_cbe_device", m_oss_cbe_device);
      m_ref_xml->get_widget ("oss_buffer_time", m_oss_buffer_time);

      m_ref_xml->get_widget ("esd_host", m_esd_host);
      m_ref_xml->get_widget ("esd_buffer_time", m_esd_buffer_time);

      m_ref_xml->get_widget ("pulse_server", m_pulse_server);
      m_ref_xml->get_widget ("pulse_device", m_pulse_device);
      m_ref_xml->get_widget ("pulse_buffer_time", m_pulse_buffer_time);

      m_ref_xml->get_widget ("jack_server", m_jack_server);
      m_ref_xml->get_widget ("jack_buffer_time", m_jack_buffer_time);

      m_ref_xml->get_widget ("notebook_audio_system", m_notebook_audio_system);
      m_ref_xml->get_widget ("audio-system-apply-changes", m_button_audio_system_apply);
      m_ref_xml->get_widget ("audio-system-reset-changes", m_button_audio_system_reset);
      m_ref_xml->get_widget ("audio-system-changed-warning", m_warning_audio_system_changed);

      /* audio */ 
      setup_audio_widgets ();
      setup_audio ();

      /* various toggle buttons */ 
      struct DomainKeyPair
      {
        char const * domain;
        char const * key;
        char const * widget;
      };

      DomainKeyPair buttons[] =
      {
        { "audio", "enable-eq", "enable-eq" },
      };

      for (unsigned int n = 0; n < G_N_ELEMENTS (buttons); ++n)
      {
        ToggleButton* button = dynamic_cast<ToggleButton*> (m_ref_xml->get_widget (buttons[n].widget));

        if (button)
            mcs_bind->bind_toggle_button (*button, buttons[n].domain, buttons[n].key);
        else
            g_warning ("%s: Widget '%s' not found in 'preferences.glade'", G_STRLOC, buttons[n].widget);

      }

     /* mm-keys */
      for(int n = 1; n < 6; ++n)
      {
        Gtk::Entry * entry;
        m_ref_xml->get_widget ((boost::format ("mm-entry-%d") % n).str(), entry);
        entry->signal_key_press_event().connect( sigc::bind( sigc::mem_fun( *this, &Preferences::on_entry_key_press_event ), n));
        entry->signal_key_release_event().connect( sigc::bind( sigc::mem_fun( *this, &Preferences::on_entry_key_release_event ), n));
        Gtk::Button * button;
        m_ref_xml->get_widget ((boost::format ("mm-clear-%d") % n).str(), button);
        button->signal_clicked().connect( sigc::bind( sigc::mem_fun( *this, &Preferences::on_clear_keyboard ), n));
        m_mm_key_controls.resize( n );
      }

      int active = mcs->key_get<int>("hotkeys","system") + 1;
      for(int n = 1; n < 4; ++n)
      {
        Gtk::RadioButton * button;
        m_ref_xml->get_widget ((boost::format ("mm-rb-%d") % n).str(), button);
        button->signal_toggled().connect( sigc::bind( sigc::mem_fun( *this, &Preferences::on_mm_option_changed), n));
        if( n == active )
        {
          button->set_active ();
        }
      }

      dynamic_cast<Gtk::Button*>(m_ref_xml->get_widget ("mm-revert"))->signal_clicked().connect(
        sigc::mem_fun( *this, &Preferences::mm_load ));

      dynamic_cast<Gtk::Button*>(m_ref_xml->get_widget ("mm-apply"))->signal_clicked().connect(
        sigc::mem_fun( *this, &Preferences::mm_apply ));

      Gtk::ToggleButton * mm_enable;
      m_ref_xml->get_widget ("mm-enable", mm_enable);
      bool mm_enabled = mcs->key_get<bool>("hotkeys","enable");
      mm_enable->set_active(mm_enabled);
      m_ref_xml->get_widget("mm-vbox")->set_sensitive(mm_enabled);
      mm_enable->signal_toggled().connect(sigc::mem_fun( *this, &Preferences::mm_toggled ));

      mm_load ();

      m_CoverArtSources = new CoverArtSourceView(m_ref_xml);
  }

  void
  Preferences::on_hide ()
  {
    setup_audio();
    Gtk::Widget::on_hide();
  }

  /* mm-keys */
  void
  Preferences::mm_toggled ()
  {
    Gtk::ToggleButton * b;
    m_ref_xml->get_widget ("mm-enable", b);
    bool active = b->get_active();

    Signals.HotkeyEditBegin.emit();
    mcs->key_set<bool>("hotkeys","enable", active);
    m_ref_xml->get_widget("mm-vbox")->set_sensitive(active);
    Signals.HotkeyEditEnd.emit();
  } 

  void
  Preferences::mm_apply ()
  {
    mcs->key_set<int>("hotkeys","system", m_mm_option);

    for(int n = 1; n < 6; ++n)
    {
      KeyControls & c = m_mm_key_controls[n-1];
      mcs->key_set<int>("hotkeys", (boost::format ("key-%d") % n).str(), c.key);
      mcs->key_set<int>("hotkeys", (boost::format ("key-%d-mask") % n).str(), c.mask);
    }

    m_ref_xml->get_widget ("mm-apply")->set_sensitive (false);
    m_ref_xml->get_widget ("mm-revert")->set_sensitive (false);

    Signals.HotkeyEditEnd.emit();
  }

  void
  Preferences::mm_load ()
  {
    for(int n = 1; n < 6; ++n)
    {
      KeyControls c;  
      c.key = mcs->key_get<int>("hotkeys", (boost::format ("key-%d") % n).str());
      c.mask = mcs->key_get<int>("hotkeys", (boost::format ("key-%d-mask") % n).str());
      m_mm_key_controls[n-1] = c;
      set_keytext( n, c.key, c.mask );
    }

    int sys = mcs->key_get<int>("hotkeys","system");
    if( sys < 0 || sys > 2) sys = 1;
    dynamic_cast<Gtk::RadioButton*>(m_ref_xml->get_widget ((boost::format ("mm-rb-%d") % (sys+1)).str()))->set_active();

    m_ref_xml->get_widget ("mm-apply")->set_sensitive (false);
    m_ref_xml->get_widget ("mm-revert")->set_sensitive (false);

    Signals.HotkeyEditEnd.emit();
  }

  void
  Preferences::set_keytext (gint entry_id, gint key, gint mask)
  {
    Signals.HotkeyEditBegin.emit();

    std::string text; 

    if (key == 0 && mask == 0)
    {
      text = (_("(none)"));
    } else {
      char const* modifier_string[] = { "Control", "Shift", "Alt", "Mod2", "Mod3", "Super", "Mod5" };
      const unsigned int modifiers[] = { ControlMask, ShiftMask, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask };

      std::string keytext;
      std::vector<std::string> strings;
      int i, j;
      KeySym keysym;

      keysym = XKeycodeToKeysym(gdk_x11_display_get_xdisplay (get_display()->gobj()), key, 0);
      if (keysym == 0 || keysym == NoSymbol)
      {
        keytext = (boost::format ("#%3d") % key).str();
      } else {
        keytext = XKeysymToString(keysym);
      }

      for (i = 0, j=0; j<7; j++)
      {
        if (mask & modifiers[j])
          strings.push_back (modifier_string[j]);
      }
      if (key != 0)
      {
        strings.push_back( keytext );
      }

      text = Util::stdstrjoin(strings, " + ");
    }

    Gtk::Entry * entry;
    m_ref_xml->get_widget ((boost::format ("mm-entry-%d") % entry_id).str(), entry); 

    entry->set_text (text);
    entry->set_position(-1);
  }

  bool 
  Preferences::on_entry_key_press_event(GdkEventKey * event, gint entry_id)
  {
    KeyControls & controls = m_mm_key_controls[entry_id-1]; 
    int is_mod;
    int mod;

    if (event->keyval == GDK_Tab) return false;
    mod = 0;
    is_mod = 0;

    if ((event->state & GDK_CONTROL_MASK) | (!is_mod && (is_mod = (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R))))
            mod |= ControlMask;

    if ((event->state & GDK_MOD1_MASK) | (!is_mod && (is_mod = (event->keyval == GDK_Alt_L || event->keyval == GDK_Alt_R))))
            mod |= Mod1Mask;

    if ((event->state & GDK_SHIFT_MASK) | (!is_mod && (is_mod = (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R))))
            mod |= ShiftMask;

    if ((event->state & GDK_MOD5_MASK) | (!is_mod && (is_mod = (event->keyval == GDK_ISO_Level3_Shift))))
            mod |= Mod5Mask;

    if ((event->state & GDK_MOD4_MASK) | (!is_mod && (is_mod = (event->keyval == GDK_Super_L || event->keyval == GDK_Super_R))))
            mod |= Mod4Mask;

    if (!is_mod) {
      controls.key = event->hardware_keycode;
      controls.mask = mod;
    } else controls.key = 0;

    set_keytext (entry_id, is_mod ? 0 : event->hardware_keycode, mod);
    m_ref_xml->get_widget ("mm-apply")->set_sensitive (true);
    m_ref_xml->get_widget ("mm-revert")->set_sensitive (true);
    return false;
  }

  bool 
  Preferences::on_entry_key_release_event(GdkEventKey * event, gint entry_id)
  {
    KeyControls & controls = m_mm_key_controls[entry_id-1]; 
    if (controls.key == 0)
    {
      controls.mask = 0;
    }
    set_keytext (entry_id, controls.key, controls.mask);
    m_ref_xml->get_widget ("mm-apply")->set_sensitive (true);
    m_ref_xml->get_widget ("mm-revert")->set_sensitive (true);
    return false;
  }

  void
  Preferences::on_clear_keyboard (gint entry_id)
  {
    KeyControls & controls = m_mm_key_controls[entry_id-1]; 
    controls.key = 0;
    controls.mask = 0;
    set_keytext(entry_id, 0, 0);
    m_ref_xml->get_widget ("mm-apply")->set_sensitive (true);
    m_ref_xml->get_widget ("mm-revert")->set_sensitive (true);
  }

  void
  Preferences::on_mm_option_changed (gint option)
  {
    Signals.HotkeyEditBegin.emit();

    switch( option )
    {
      case 1:
      case 2:
        m_ref_xml->get_widget ("mm-table")->set_sensitive (false);
        break;  

      case 3:
        m_ref_xml->get_widget ("mm-table")->set_sensitive (true);
        break; 
    }
    m_mm_option = option - 1;
    m_ref_xml->get_widget ("mm-apply")->set_sensitive (true);
  }

} // namespace MPX
