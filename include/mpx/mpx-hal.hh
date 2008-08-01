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
#include <set>
#include <tr1/unordered_map>
#include <ctime>
#include <glib/gtypes.h>
#include <glibmm/ustring.h>
#include <boost/functional/hash.hpp>
#include "hal++.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-services.hh"

namespace MPX
{
    class HAL : public Service::Base
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

            typedef std::vector<Hal::RefPtr<Hal::Volume> >                                      VolumesV;
            typedef std::pair<std::string, std::string >                                        VolumeKey;
            typedef std::pair<Volume, Hal::RefPtr<Hal::Volume> >                                StoredVolume;
            typedef std::tr1::unordered_map<VolumeKey , StoredVolume, boost::hash<VolumeKey> >  Volumes;
            typedef std::tr1::unordered_map<VolumeKey , bool, boost::hash<VolumeKey> >          VolumesMounted;
            typedef std::set<std::string>                                                       MountedPaths;

            typedef sigc::signal  <void, Volume const&>                 SignalVolume;
            typedef sigc::signal  <void, std::string, std::string >     SignalCDDAInserted;
            typedef sigc::signal  <void, std::string >                  SignalDeviceRemoved;
            typedef sigc::signal  <void, std::string >                  SignalEjected;

            HAL (MPX::Service::Manager&);
            ~HAL ();

            Hal::RefPtr<Hal::Context>
            get_context ();

            Volumes const&
            volumes () const;

            void
            volumes (VolumesV & volumes) const;

            bool
            path_is_mount_path (std::string const& path);

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

            bool
            hal_init ();

            void
            cdrom_policy (Hal::RefPtr<Hal::Volume> const& volume);

            void
            volume_register (Volume const& volume);

            void
            volume_insert (std::string const& udi);

            void
            volume_remove (std::string const& udi);

            void
            volume_process (std::string const& udi);

            void
            device_condition (std::string const& udi,
                              std::string const& cond_name,
                              std::string const& cond_details );
            void
            device_added (std::string const& udi );

            void
            device_removed (std::string const& udi );

            void
            device_property (std::string const& udi,
                             std::string const& key,
                             bool is_removed,
                             bool is_added);

            SignalVolume                    signal_volume_removed_;
            SignalVolume                    signal_volume_added_;
            SignalCDDAInserted              signal_cdda_inserted_;
            SignalEjected                   signal_ejected_;

            Hal::RefPtr<Hal::Context>       m_context;
            Volumes		                    m_volumes;
            VolumesMounted                  m_volumes_mounted;
            MountedPaths                    m_mounted_paths;

            SQL::SQLDB                    * m_SQL;
            bool                            m_initialized;
    };
}
#endif //!MPX_HAL_HH
