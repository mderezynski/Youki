//  hal.cc
//
//  Authors:
//      Milosz Derezynski <milosz@backtrace.info>
//
//  (C) 2008 MPX Project
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
#include "config.h"

#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>

#include <gtkmm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libhal.h>
#include <libhal-storage.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "hal++.hh"

#include "mpx/mpx-sql.hh"
#include "mpx/mpx-hal.hh"

using namespace Glib;
using namespace Gtk;
using namespace Hal;
using namespace std;
using namespace MPX::SQL;

namespace
{
  const char* HALBus[] =
  {
    _("<Unknown>"),
    "IDE",
    "SCSI",
    "USB",
    "IEEE1394",
    "CCW"
  };
}

namespace MPX
{
      struct AttributeInfo
      {
        char const* name;
        char const* id;
        int         type;
      };

      AttributeInfo
      volume_attributes[] = 
      {
        {
          N_("Volume UDI"),
          "volume_udi",
          SQL::VALUE_TYPE_STRING,
        },
        {
          N_("Device UDI"),
          "device_udi",
          SQL::VALUE_TYPE_STRING,
        },
        {
          N_("Mount Path"),
          "mount_point",
          SQL::VALUE_TYPE_STRING,
        },
        {
          N_("Device Serial"),
          "drive_serial",
          SQL::VALUE_TYPE_STRING,
        },
        {
          N_("Volume Name"),
          "label",
          SQL::VALUE_TYPE_STRING,
        },
        {
          N_("Device File"),
          "device_node",
          SQL::VALUE_TYPE_STRING,
        },
        {
          N_("Drive Bus"),
          "drive_bus",
          SQL::VALUE_TYPE_INT,
        },
        {
          N_("Drive Type"),
          "drive_type",
          SQL::VALUE_TYPE_INT,
        },
        {
          N_("Volume Size"),
          "size",
          SQL::VALUE_TYPE_INT,
        },
        {
          N_("Drive Size"),
          "drive_size",
          SQL::VALUE_TYPE_INT,
        },
        {
          N_("Drive Size"),
          "mount_time",
          SQL::VALUE_TYPE_INT,
        },
      };

      enum VolumeAttribute
      {
        VATTR_VOLUME_UDI,
        VATTR_DEVICE_UDI,
        VATTR_MOUNT_PATH,
        VATTR_DEVICE_SERIAL,
        VATTR_VOLUME_NAME,
        VATTR_DEVICE_FILE,
        VATTR_DRIVE_BUS,         
        VATTR_DRIVE_TYPE,         
        VATTR_SIZE,         
        VATTR_DRIVE_SIZE,         
        VATTR_MOUNT_TIME,         
      };

      HAL::Volume::Volume ( Hal::RefPtr<Hal::Context> const& context,
                            Hal::RefPtr<Hal::Volume>  const& volume) throw ()
      {
        try{
            Hal::RefPtr<Hal::Drive> drive = Hal::Drive::create_from_udi (context, volume->get_storage_device_udi());

            volume_udi      = volume->get_udi();
            device_udi      = volume->get_storage_device_udi(); 

            size            = volume->get_size ();
            label           = volume->get_label(); 

            mount_time      = time( NULL ); 
            mount_point     = volume->get_mount_point(); 

            drive_bus       = drive->get_bus();
            drive_type      = drive->get_type();

    #ifdef HAVE_HAL_058
            drive_size      = drive->get_size(); 
    #else
            drive_size      = 0; 
    #endif //HAVE_HAL_058

            drive_serial    = drive->get_serial(); 
            device_file     = volume->get_device_file(); 
        } catch(...)
        {
            throw;
        }
      }


      HAL::Volume::Volume (MPX::SQL::Row const& row)
      {
        Row::const_iterator i;
        Row::const_iterator end = row.end();

        i = row.find (volume_attributes[VATTR_VOLUME_UDI].id);
        volume_udi = boost::get <string> (i->second).c_str();

        i = row.find (volume_attributes[VATTR_DEVICE_UDI].id);
        device_udi = boost::get <string> (i->second).c_str();

        i = row.find (volume_attributes[VATTR_DRIVE_BUS].id);
        drive_bus = Hal::DriveBus(boost::get <gint64> (i->second));

        i = row.find (volume_attributes[VATTR_DRIVE_TYPE].id);
        drive_type = Hal::DriveType (boost::get <gint64> (i->second));

        i = row.find (volume_attributes[VATTR_SIZE].id);
        size = boost::get <gint64> (i->second);

        i = row.find (volume_attributes[VATTR_DRIVE_SIZE].id);
        drive_size = boost::get <gint64> (i->second);

        i = row.find (volume_attributes[VATTR_MOUNT_TIME].id);
        mount_time = boost::get <gint64> (i->second);

        i = row.find (volume_attributes[VATTR_MOUNT_PATH].id);
        if (i != end)
          mount_point = boost::get <string> (i->second).c_str();

        i = row.find (volume_attributes[VATTR_DEVICE_SERIAL].id);
        if (i != end)
          drive_serial = boost::get <string> (i->second).c_str();

        i = row.find (volume_attributes[VATTR_VOLUME_NAME].id);
        if (i != end)
          label = boost::get <string> (i->second).c_str();

        i = row.find (volume_attributes[VATTR_DEVICE_FILE].id);
        if (i != end)
          device_file = boost::get <string> (i->second).c_str();
      }

      ///////////////////////////////////////////////////////

      void
      HAL::volume_register (Volume const& volume)
      {
        static boost::format insert_volume_f
        ("INSERT INTO hal_volumes (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%lld', '%lld', '%lld', '%lld', '%lld');");

        m_SQL->exec_sql ((insert_volume_f
          % volume_attributes[VATTR_VOLUME_UDI].id
          % volume_attributes[VATTR_DEVICE_UDI].id
          % volume_attributes[VATTR_MOUNT_PATH].id
          % volume_attributes[VATTR_DEVICE_SERIAL].id
          % volume_attributes[VATTR_VOLUME_NAME].id
          % volume_attributes[VATTR_DEVICE_FILE].id
          % volume_attributes[VATTR_DRIVE_BUS].id
          % volume_attributes[VATTR_DRIVE_TYPE].id
          % volume_attributes[VATTR_SIZE].id
          % volume_attributes[VATTR_DRIVE_SIZE].id
          % volume_attributes[VATTR_MOUNT_TIME].id
          % volume.volume_udi
          % volume.device_udi
          % volume.mount_point
          % volume.drive_serial
          % volume.label
          % volume.device_file
          % gint64(volume.drive_bus)
          % gint64(volume.drive_type)
          % gint64(volume.size)
          % gint64(volume.drive_size)
          % gint64(volume.mount_time)).str());
      }

      Hal::RefPtr<Hal::Context>
      HAL::get_context ()
      {
        return m_context;
      }

      HAL::HAL (MPX::Service::Manager& services)
      : Service::Base("mpx-service-hal")
      {
        if (!hal_init())
        {
          throw NotInitializedError();
        }
      }

      bool
      HAL::hal_init ()
      {
        DBusError error;
        dbus_error_init (&error);

        DBusConnection * c = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

        if (dbus_error_is_set (&error))
        {
          dbus_error_free (&error);
          return false;
        }

        dbus_connection_setup_with_g_main (c, NULL);
        dbus_connection_set_exit_on_disconnect (c, FALSE);

        try{
          m_context = Hal::Context::create (c);

          m_context->signal_device_added().connect(
            sigc::mem_fun(*this, &MPX::HAL::device_added
          ));

          m_context->signal_device_removed().connect(
            sigc::mem_fun (this, &MPX::HAL::device_removed
          ));

          m_SQL = new SQL::SQLDB ("hal", build_filename(g_get_user_data_dir(), "audiosource"), SQL::SQLDB_OPEN);

          try{
              static boost::format sql_create_table_f("CREATE TABLE IF NOT EXISTS %s (%s, PRIMARY KEY ('%s','%s') ON CONFLICT REPLACE);");
              static boost::format sql_attr_f ("'%s'");
    
              std::string attrs;
              for (unsigned int n = 0; n < G_N_ELEMENTS(volume_attributes); ++n)
              {
                  attrs.append((sql_attr_f % volume_attributes[n].id).str());

                  switch (volume_attributes[n].type)
                  {
                      case VALUE_TYPE_STRING:
                          attrs.append (" TEXT ");
                          break;

                      case VALUE_TYPE_INT:
                          attrs.append (" INTEGER DEFAULT 0 ");
                          break;

                      default: break;
                  }

                  if (n < (G_N_ELEMENTS(volume_attributes)-1)) attrs.append (", ");
              }

              m_SQL->exec_sql((
                sql_create_table_f
                    % "hal_volumes"
                    % attrs
                    % volume_attributes[VATTR_DEVICE_UDI].id
                    % volume_attributes[VATTR_VOLUME_UDI].id
              ).str());
          }
          catch (MPX::SQL::SqlExceptionC & cxe)
          {
              return false;
          }

          Hal::StrV list = m_context->find_device_by_capability ("volume");
          for (Hal::StrV::const_iterator n = list.begin(); n != list.end(); ++n) 
          {
            volume_insert (*n);
          }
        }
        catch (...)
        {
            return false;
        }

        return true;
      }

      HAL::~HAL ()
      {
        delete m_SQL;
      }

      void
      HAL::volumes (VolumesV & volumes) const
      {
        for(Volumes::const_iterator i = m_volumes.begin(); i != m_volumes.end(); ++i)
        {
            VolumeKey const& key = i->first;
            if(m_volumes_mounted.count(key) && m_volumes_mounted.find(key)->second)
            {
                volumes.push_back(i->second.second);
            }
        }
      }

      void
      HAL::cdrom_policy (Hal::RefPtr<Hal::Volume> const& volume)
      {
        Hal::RefPtr<Hal::Drive> drive = Hal::Drive::create_from_udi (m_context, volume->get_storage_device_udi ());

        if (drive->get_type() == Hal::DRIVE_TYPE_CDROM &&
            drive->property_exists ("info.locked") &&
            !drive->get_property <bool> ("info.locked"))
        try{
              Hal::DiscProperties props = volume->get_disc_properties();
              if (props & Hal::DISC_HAS_AUDIO)
              {
                signal_cdda_inserted_.emit( volume->get_udi(), volume->get_device_file() );
              }
              else
              if (props & Hal::DISC_HAS_DATA)
              {
              }
              else
              if (props & Hal::DISC_IS_BLANK)
              {
              }
          }
        catch (Hal::UnableToProbeDeviceError)
          {
          }
      }

      Glib::ustring
      HAL::get_volume_drive_bus (Volume const& volume) const
      {
        return Glib::ustring (HALBus[volume.drive_bus]);
      }

      bool
      HAL::path_is_mount_path (std::string const& path)
      {
        std::string path_copy;
        if(path.substr(path.length() - 1) == "/")
            path_copy = path.substr(0, path.length() - 1);
        else
            path_copy = path;
        return m_mounted_paths.count(path_copy);
      }

      bool
      HAL::volume_is_mounted  (VolumeKey const& volume_key) const
      {
        try{
            return m_volumes_mounted.find (volume_key)->second;
          }
        catch (...)
          {
            throw InvalidVolumeSpecifiedError (volume_key.first);
          }
      }

      bool
      HAL::volume_is_mounted  (Volume const& volume) const
      {
        try{
            Hal::RefPtr<Hal::Volume> volume_hal_instance = Hal::Volume::create_from_udi (m_context, volume.volume_udi);
            return volume_hal_instance->is_mounted();
          }
        catch (...)
          {
            throw InvalidVolumeSpecifiedError (volume.volume_udi.c_str());
          }
      }

      HAL::Volume
      HAL::get_volume_for_uri (Glib::ustring const& uri) const
      {
        string filename (filename_from_uri (uri));
        string::size_type l1 = 0;
        Volumes::const_iterator m = m_volumes.end();

        for (Volumes::const_iterator i = m_volumes.begin (); i != m_volumes.end (); ++i)
        {
          std::string::size_type l2 (i->second.first.mount_point.length());
          if ((i->second.first.mount_point == filename.substr (0, l2)) && (l2 > l1))
          {
            m = i;
            l1 = l2;
          } 
        }

        if (m != m_volumes.end())
          return m->second.first; 
        else
          throw NoVolumeForUriError ((boost::format ("No volume for Uri %s") % uri.c_str()).str());
      }

      std::string
      HAL::get_mount_point_for_volume (std::string const& volume_udi, std::string const& device_udi) const
      {
        VolumeKey match (volume_udi, device_udi);
        Volumes::const_iterator v_iter = m_volumes.find (match);

        if (v_iter == m_volumes.end())
        {
            g_message("%s: Got no volume for %s, %s", G_STRFUNC, volume_udi.c_str(), device_udi.c_str());
            throw NoMountPathForVolumeError ((boost::format ("No mount path for given volume: device: %s, volume: %s") % device_udi % volume_udi).str());
        }
        else
        {
            return v_iter->second.first.mount_point; 
        }
      }

      void
      HAL::volume_process (std::string const &udi)
      {
        try{
          Hal::RefPtr<Hal::Volume> volume = Hal::Volume::create_from_udi (m_context, udi);
          if (volume->is_disc())
          {
            cdrom_policy (volume);
          }
          else
          if (volume->is_pmp())
          {
            /* pmp_policy (volume); */
            g_message("%s: Got a media player: %s", G_STRFUNC, udi.c_str());
          }
        }
        catch (DeviceDoesNotExistError)
        {}
        catch (UnableToProbeDeviceError)
        {}
      }

      void
      HAL::volume_remove (std::string const& udi)
      {
        Hal::RefPtr <Hal::Volume> volume_hal_instance;

        try{
          volume_hal_instance = Hal::Volume::create_from_udi (m_context, udi);
        }
        catch (...)
        {
          g_message("%s: Couldn't create HAL Volume from UDI", G_STRFUNC);
          return;
        }

        // Now we know it's a volume

        if (!(volume_hal_instance->get_fsusage() == Hal::VOLUME_USAGE_MOUNTABLE_FILESYSTEM))
        {
            g_message("%s: Got volume UDI, but is not mountable", G_STRFUNC);
            return;
        }

        Volume volume (m_context, volume_hal_instance);
        VolumeKey volume_key (volume.volume_udi, volume.device_udi);

        try{
            m_mounted_paths.erase( volume_hal_instance->get_mount_point() );

            Volumes::iterator i1 = m_volumes.find(volume_key);
            if(i1 != m_volumes.end())
                m_volumes.erase(i1);

            VolumesMounted::iterator i2 = m_volumes_mounted.find(volume_key);
            if(i2 != m_volumes_mounted.end())
                m_volumes_mounted.erase(i2);

            g_message("%s: Volume removed: %s", G_STRFUNC,udi.c_str());

            signal_volume_removed_.emit (volume); 
          }
        catch (...) 
          {
            g_message("%s: Unknown error", G_STRFUNC);
          }
      }


      void
      HAL::volume_insert (std::string const& udi)
      {
        Hal::RefPtr <Hal::Volume> volume_hal_instance;

        try{
          volume_hal_instance = Hal::Volume::create_from_udi (m_context, udi);
        }
        catch (...)
        {
          g_message("%s: Couldn't create HAL Volume from UDI", G_STRFUNC);
          return;
        }

        // Now we know it's a volume

        if (volume_hal_instance->get_fsusage() != Hal::VOLUME_USAGE_MOUNTABLE_FILESYSTEM)
        {
            g_message("%s: Got volume UDI, but is not mountable", G_STRFUNC);
            return;
        }

        try{
            Volume volume (m_context, volume_hal_instance);
            VolumeKey volume_key (volume.volume_udi, volume.device_udi);

            m_volumes.insert (make_pair (volume_key, StoredVolume (volume, volume_hal_instance)));
            Hal::RefPtr<Hal::Volume> const& vol (m_volumes.find (volume_key)->second.second);
            vol->signal_condition().connect
              (sigc::mem_fun (this, &MPX::HAL::device_condition));
            vol->signal_property_modified().connect
              (sigc::mem_fun (this, &MPX::HAL::device_property));

            g_message("%s: Volume added: %s", G_STRFUNC,udi.c_str());
 
            if(volume_hal_instance->is_mounted())
            {
                m_mounted_paths.insert(volume_hal_instance->get_mount_point());
                m_volumes_mounted.insert (make_pair (volume_key, volume_hal_instance->is_mounted()));
            }

            if (!volume_hal_instance->is_disc())
            {
                volume_register( volume );  // we don't store information for discs permanently
                volume_process( udi );
            }

            signal_volume_added_.emit (volume); 
          }
        catch (...) 
          {
            g_message("%s: Unknown error", G_STRFUNC);
          }
      }

      void
      HAL::device_condition  (std::string const& udi,
                              std::string const& cond_name,
                              std::string const& cond_details)
      {
        if (cond_name == "EjectPressed")
        {
            signal_ejected_.emit (udi);
        }
      }

      void
      HAL::device_added (std::string const& udi)
      {
        try{
            if (m_context->device_query_capability (udi, "volume"))
            {
              g_message("%s: Got new Volume: '%s'", G_STRFUNC, udi.c_str());
              volume_insert (udi);       
            }
        } catch (UnableToProbeDeviceError & cxe)
        {
            g_message("%s: Device with udi '%s' doesn't exist", G_STRFUNC, udi.c_str());
        }
      }

      void
      HAL::device_removed (std::string const& udi) 
      {
        try{
            if (m_context->device_query_capability (udi, "volume"))
            {
              g_message("%s: Got Volume removed: %s", G_STRFUNC, udi.c_str());
              volume_remove (udi);       
            }
        } catch (UnableToProbeDeviceError & cxe)
        {
            g_message("%s: Device with udi '%s' doesn't exist", G_STRFUNC, udi.c_str());
        }
      }

      void
      HAL::device_property  
          (std::string const& udi,
           std::string const& key,
           bool               is_removed,
           bool               is_added)
      {
      }

      HAL::SignalVolume&
      HAL::signal_volume_removed ()
      {
        return signal_volume_removed_;
      }

      HAL::SignalVolume&
      HAL::signal_volume_added ()
      {
        return signal_volume_added_;
      }

      HAL::SignalCDDAInserted&
      HAL::signal_cdda_inserted ()
      {
        return signal_cdda_inserted_;
      }

      HAL::SignalEjected&
      HAL::signal_ejected ()
      {
        return signal_ejected_;
      }
}
