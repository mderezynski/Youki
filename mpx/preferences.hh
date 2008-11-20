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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifndef MPX_PREFERENCES_HH
#define MPX_PREFERENCES_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include "mpx/mpx-audio.hh"
#include "mpx/widgets/widgetloader.hh"
#include "play.hh"

#include <set>
#include <string>
#include <vector>

#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <mcs/mcs.h>
#include <mcs/gtk-bind.h>

namespace MPX
{
  class CoverArtSourceView;

  /** Preferences dialog
   *
   * MPX::Preferences is a complex dialog for adjusting run time parameters
   * of MPX trough the GUI instead of having to manipulate the configuration
   * file.
   */
  class Preferences
    : public Gnome::Glade::WidgetLoader<Gtk::Window>
  {
    protected:

      virtual void on_hide ();

      Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;

    public:

      Preferences (Glib::RefPtr<Gnome::Glade::Xml> const&, MPX::Play&);

      static
      Preferences*
      create (MPX::Play&);

      virtual
      ~Preferences()
      {
      }

    private:

      class AudioSystemColumnRecord
        : public Gtk::TreeModel::ColumnRecord
      {
        public:

          Gtk::TreeModelColumn<Glib::ustring> description;
          Gtk::TreeModelColumn<std::string>   name;
          Gtk::TreeModelColumn<int>           tab;
          Gtk::TreeModelColumn<Audio::Sink>   sink;

          AudioSystemColumnRecord()
          {
              add (description);
              add (name);
              add (tab);
              add (sink);
          }
      };

      AudioSystemColumnRecord   audio_system_columns;
      std::vector<int>          audio_system_cbox_ids;

      Gtk::StockID
      get_plugin_stock(bool /*available*/);

      void
      setup_audio_widgets ();

      void
      setup_audio ();

      void
      audio_system_apply_set_sensitive ();
    
      void
      audio_system_apply_set_insensitive ();

      void
      audio_system_changed ();

      void
      audio_system_apply ();

      Gtk::Button                       * m_button_audio_system_apply;
      Gtk::Button                       * m_button_audio_system_reset;
      Gtk::ComboBox                     * m_cbox_audio_system;
      Gtk::HBox                         * m_warning_audio_system_changed;
      Gtk::Notebook                     * m_notebook_audio_system;
      std::set<std::string>               m_sinks;
      Glib::RefPtr<Gtk::ListStore>        m_list_store_audio_systems;

      Play & m_Play;

#ifdef HAVE_ALSA
      // ALSA
      struct AlsaDevice
      {
          std::string   m_handle;
          int	        m_card_id;
          int	        m_device_id;
          std::string   m_name;

          AlsaDevice  () {}
          AlsaDevice  ( std::string const&  handle,
                        int                 card_id,
                        int                 device_id,
                        std::string const&  name)

          : m_handle    (handle),
            m_card_id   (card_id),
            m_device_id (device_id),
            m_name      (name) {}
      };
      typedef std::vector <AlsaDevice> AlsaDevices;

      struct AlsaCard
      {
          std::string   m_handle;
          int           m_card_id;
          std::string   m_id;
          std::string   m_name;
          std::string   m_longname;
          std::string   m_driver;
          std::string   m_mixer;

          AlsaDevices   m_devices;

          AlsaCard () {}
          AlsaCard (std::string const&  handle,
                    int                 card_id,
                    std::string const&  id,
                    std::string const&  name,
                    std::string const&  longname,
                    std::string const&  driver,
                    std::string const&  mixer,
                    AlsaDevices      &  devices)

          : m_handle    (handle),
            m_card_id   (card_id),
            m_id        (id),
            m_name      (name),
            m_longname  (longname),
            m_driver    (driver),
            m_mixer     (mixer)
          {
            std::swap (devices, m_devices);
          }

          operator Glib::ustring ()
          {
            return Glib::ustring ((m_longname.size() ? m_longname : m_name));
          }
      };
      typedef std::vector <AlsaCard> AlsaCards;

      class AlsaCardColumnRecord
        : public Gtk::TreeModel::ColumnRecord
      {
        public:

          Gtk::TreeModelColumn <Glib::ustring> name;
          Gtk::TreeModelColumn <AlsaCard>      card;

          AlsaCardColumnRecord ()
          {
            add (name);
            add (card);
          }
      };

      class AlsaDeviceColumnRecord
        : public Gtk::TreeModel::ColumnRecord
      {
        public:

          Gtk::TreeModelColumn <Glib::ustring> name;
          Gtk::TreeModelColumn <AlsaDevice>    device;

          AlsaDeviceColumnRecord ()
          {
            add (name);
            add (device);
          }
      };

      AlsaCardColumnRecord                m_alsa_card_columns;
      AlsaDeviceColumnRecord              m_alsa_device_columns;
      Gtk::ComboBox                     * m_cbox_alsa_card;
      Gtk::ComboBox                     * m_cbox_alsa_device;
      Gtk::SpinButton                   * m_alsa_buffer_time;
      Gtk::Entry                        * m_alsa_device_string;
      Glib::RefPtr<Gtk::ListStore>        m_list_store_alsa_cards;
      Glib::RefPtr<Gtk::ListStore>        m_list_store_alsa_device;
      sigc::connection                    m_conn_alsa_card_changed;
      sigc::connection                    m_conn_alsa_device_changed;
      sigc::connection                    m_conn_alsa_device_string_changed;

      AlsaCards get_alsa_cards ();
      void on_alsa_card_changed ();
      void on_alsa_device_changed ();
      void on_alsa_device_string_changed ();
#endif //HAVE_ALSA

      // Video
      Gtk::ComboBox                     * m_cbox_video_out;

      // OSS
      Gtk::ComboBoxEntry                * m_oss_cbe_device;
      Gtk::SpinButton                   * m_oss_buffer_time;

      // ESD
      Gtk::Entry                        * m_esd_host;
      Gtk::SpinButton                   * m_esd_buffer_time;

      // PulseAudio
      Gtk::Entry                        * m_pulse_server;
      Gtk::Entry                        * m_pulse_device;
      Gtk::SpinButton                   * m_pulse_buffer_time;

      // Jack
      Gtk::Entry                        * m_jack_server;
      Gtk::SpinButton                   * m_jack_buffer_time;

#ifdef HAVE_SUN
      Gtk::ComboBoxEntry                * m_sun_cbe_device;
      Gtk::SpinButton                   * m_sun_buffer_time;
#endif // HAVE_SUN

#ifdef HAVE_HAL
      Gtk::Entry                        * m_halaudio_udi;
#endif // HAVE_SUN

      // MM Keys
      struct KeyControls
      {
        gint key, mask;

        KeyControls () : key(0), mask(0) {}
      };

      std::vector<KeyControls> m_mm_key_controls;
      int m_mm_option;

      void  set_keytext (gint entry_id, gint key, gint mask);
      bool  on_entry_key_press_event(GdkEventKey * event, gint entry_id);
      bool  on_entry_key_release_event(GdkEventKey * event, gint entry_id);
      void  on_clear_keyboard (gint entry_id);
      void  on_mm_option_changed (gint option);
      void  mm_load ();
      void  mm_apply ();
      void  mm_toggled ();

    public:

        typedef sigc::signal<void> Signal;
        struct Signals_t
        {
          Signal  HotkeyEditBegin;
          Signal  HotkeyEditEnd;
        };

    private:

        Signals_t Signals;

    public:

        Signal&
        signal_mm_edit_begin ()
        {
          return Signals.HotkeyEditBegin;
        }

        Signal&
        signal_mm_edit_done ()
        {
          return Signals.HotkeyEditEnd;
        }

    private:

        Gtk::SpinButton*	m_Radio_MinimalBitrate;
      	CoverArtSourceView*	m_Covers_CoverArtSources; 

        Gtk::CheckButton*	m_Library_RescanAtStartup;
        Gtk::CheckButton*	m_Library_RescanInIntervals;
        Gtk::SpinButton*	m_Library_RescanInterval;
        Gtk::HBox*          m_Library_RescanIntervalBox;

  }; // class Preferences
} // namespace MPX

#endif // MPX_PREFERENCES_HH
