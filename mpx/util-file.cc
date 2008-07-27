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
#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <grp.h>
#include <unistd.h>
#include <boost/format.hpp>

#include <glib/gstdio.h>
#include <glibmm.h>
#include <gio/gio.h>

#include "mpx/audio.hh"
#include "mpx/uri.hh"
#include "mpx/util-file.hh"

using namespace Glib;

namespace MPX
{
  namespace Util
  {
    namespace
    {
      typedef std::list<std::string> DirEntries;

      bool
      always_true (std::string const& filename)
      {
        return true;
      }
    } // <anonymous> namespace

    std::string
    create_glob (const std::string &suffix)
    {
        static boost::format fmt ("[%c%c]");

        std::string result;

        for (std::string::const_iterator i = suffix.begin (); i != suffix.end (); ++i)
            result += (fmt % g_ascii_tolower (*i) % g_ascii_toupper (*i)).str ();

        return result;
    }

    void
    collect_paths (std::string const& uri,
                   FileList&          collection,
                   FilePred           pred,
                   bool               clear)
    {
      if (clear)
        collection.clear ();

      GFile * file = g_vfs_get_file_for_uri(g_vfs_get_default(), uri.c_str());
      GFileEnumerator * enm = g_file_enumerate_children(file, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE, GFileQueryInfoFlags(0), NULL, NULL);

      gboolean iterate = TRUE;
      while(iterate && G_IS_FILE_ENUMERATOR(enm))
      {
        GFileInfo * f = g_file_enumerator_next_file(enm, NULL, NULL);
        if(f)
        {
            std::string full_path = uri + G_DIR_SEPARATOR_S + URI::escape_string(g_file_info_get_name(f));
            GFileType t = g_file_info_get_file_type(f);
            g_object_unref(f);
            if (t == G_FILE_TYPE_REGULAR)
            {
                if (pred (full_path))
                {
                  collection.push_back (full_path);
                }
            }
            else if (t == G_FILE_TYPE_DIRECTORY)
            {
                // pred is getting repeatedly copied for no good reason!
                collect_paths (full_path, collection, pred, false);
            }
        }
        else
            iterate = FALSE;
      }
      g_object_unref(enm);
      g_object_unref(file);
    }

    void
    collect_paths (std::string const& dir_path,
                   FileList&          collection,
                   bool               clear)
    {
      collect_paths (dir_path, collection, sigc::ptr_fun (always_true));
    }

    void
    collect_audio_paths (std::string const& dir_path,
                         FileList&          collection,
                         bool               clear)
    {
      collect_paths (dir_path, collection, sigc::ptr_fun (&::MPX::Audio::is_audio_file), clear);
    }

    void
    dir_for_each_entry (const std::string &path,
                        FilePred           pred)
    {
      Glib::Dir dir (path);
      DirEntries entries (dir.begin(), dir.end ());

      for (DirEntries::const_iterator iter = entries.begin (), end = entries.end ();
           iter != end;
           ++iter)
        {
          std::string full_path = Glib::build_filename (path, *iter);

          if (pred (full_path))
            break;
        }
    }

    void
    files_writable (FileList const& list,
                    FileList      & non_writable)
    {
      for (FileList::const_iterator i = list.begin () ; i != list.end () ; ++i)
      {
        if (!access ((*i).c_str(), W_OK))
          continue;
        else
          non_writable.push_back (*i);
      }
    }

    // compares files wrg to existence and filesize
    bool
    compare_files (const std::string &file1,
                   const std::string &file2)
    {
        if (!Glib::file_test (file1, Glib::FILE_TEST_EXISTS))
            return false;

        if (!Glib::file_test (file2, Glib::FILE_TEST_EXISTS))
            return false;

        struct stat stat_a;
        struct stat stat_b;

        stat (file1.c_str (), &stat_a);
        stat (file2.c_str (), &stat_b);

        return (stat_a.st_size == stat_b.st_size);
    }

    void
    copy_file (const std::string &s,
               const std::string &d)
    {
        using namespace std;

        if (s == d) return;

        std::ifstream i;
        std::ofstream o;

        i.exceptions (ifstream::badbit | ifstream::failbit);
        o.exceptions (ofstream::badbit | ofstream::failbit);

        i.open (s.c_str());
        o.open (d.c_str());

        if (i.is_open() && o.is_open())
          o << i.rdbuf();

        if (o.is_open())
          o.close ();

        if (i.is_open())
          i.close ();
    }
  } // namespace Util
} // namespace MPX

/////////////////////////////////////////////////////////////////////
