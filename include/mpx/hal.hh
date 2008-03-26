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

#ifndef MPX_HAL_HH
#define MPX_HAL_HH

#include "config.h"

#include <string>
#include <vector>
#include <map>

#include <glib/gtypes.h>
#include <glibmm/ustring.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libhal.h>
#include <libhal-storage.h>

#include "sql.hh"
#include "hal++.hh"

namespace MPX
{
    class HAL
    {
      public:

#include "mpx/exception.hh"

        EXCEPTION(NotInitializedError)
        EXCEPTION(NoVolumeForUriError)
        EXCEPTION(NoMountPathForVolumeError)
        EXCEPTION(InvalidVolumeSpecifiedError)

        struct Volume
        {
          std::string	    volume_udi;
          std::string	    device_udi;
          std::string	    label;
          gint64            size;

          std::string	    mount_point;
          std::time_t       mount_time;
          std::string	    device_file;

          std::string	    drive_serial;
          Hal::DriveBus     drive_bus;
          Hal::DriveType    drive_type;
          gint64            drive_size;

          bool		        disc;

          Volume () {}

          Volume (SQL::Row const& row);
          Volume (Hal::RefPtr<Hal::Context> const& context,
                  Hal::RefPtr<Hal::Volume>  const& volume) throw ();
        };

        typedef std::vector < Volume > VVolumes;

        typedef std::pair < std::string, std::string >              VolumeKey;
        typedef std::pair < Volume, Hal::RefPtr<Hal::Volume> >      StoredVolume;
        typedef std::map  < VolumeKey , StoredVolume >              Volumes;
        typedef std::map  < VolumeKey , bool >                      VolumesMounted;

        typedef sigc::signal  <void, Volume const&>             SignalVolume;
        typedef sigc::signal  <void, std::string, std::string > SignalCDDAInserted;
        typedef sigc::signal  <void, std::string >              SignalDeviceRemoved;
        typedef sigc::signal  <void, std::string >              SignalEjected;

        HAL ();
        ~HAL ();

        Volumes const&
        volumes () const;

        void
        volumes (VVolumes & volumes) const;

        bool
        volume_is_mounted (VolumeKey const& volume_key) const;

        bool
        volume_is_mounted (Volume const& volume) const;

        std::string
        get_mount_point_for_volume (std::string const& volume_udi, std::string const& device_udi) const;

        Volume
        get_volume_for_uri (Glib::ustring const& uri) const;

        Glib::ustring
        get_volume_drive_bus (Volume const& volume) const;

        SignalVolume&
        signal_volume_removed ();

        SignalVolume&
        signal_volume_added ();

        SignalDeviceRemoved&
        signal_device_removed ();

        SignalCDDAInserted&
        signal_cdda_inserted ();

        SignalEjected&
        signal_ejected ();

        bool
        is_initialized () const
        {
          return m_initialized;
        }

      private:

        bool  hal_init ();

        SignalVolume              signal_volume_removed_;
        SignalVolume              signal_volume_added_;
        SignalDeviceRemoved       signal_device_removed_;
        SignalCDDAInserted        signal_cdda_inserted_;
        SignalEjected             signal_ejected_;

        Hal::RefPtr<Hal::Context>   m_context;
        Volumes		                m_volumes;
        VolumesMounted              m_volumes_mounted;

        SQL::SQLDB                * m_SQL;
        bool                        m_initialized;

        void  cdrom_policy                (Hal::RefPtr<Hal::Volume> const& volume);
        void  register_update_volume      (Volume const& volume);

        void  process_udi                 (std::string const& udi);
        void  process_volume              (std::string const& udi);

        void  device_condition            (std::string const& udi,
                                           std::string const& cond_name,
                                           std::string const& cond_name );
        void  device_added                (std::string const& udi );
        void  device_removed              (std::string const& udi );
        void  device_property             (std::string const& udi,
                                           std::string const& key,
                                           bool is_removed,
                                           bool is_added);
    };
}
#endif //!MPX_HAL_HH
