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

#include "audio.hh"
#include "uri.hh"
#include "util-file.hh"

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

      bool
      find_file_actual (const std::string &dir_path,
                        const std::string &filename,
                        std::string       &found_path,
                        int                max_depth,
                        int                depth)
      {
          if (max_depth != -1 && depth > max_depth)
          {
              return false;
          }

          Glib::Dir dir (dir_path);
          DirEntries entries (dir.begin (), dir.end ());

          for (DirEntries::const_iterator iter = entries.begin (), end = entries.end ();
               iter != end;
               ++iter)
          {
              std::string path = Glib::build_filename (dir_path, *iter);
              if (path.length () > FILENAME_MAX)
              {
                  g_warning (G_STRLOC ": ignoring path: name too long (%s)",
                             (path).c_str ());
                  continue;
              }

              if (Glib::file_test (path, Glib::FILE_TEST_IS_REGULAR))
              {
                  try
                    {
                      // NOTE: We do a straight comparison first so that should G_FILENAME_ENCODING
                      // be wrongly set, we at least can match filenames character for character.
                      // - descender

                      if (*iter == filename)
                        {
                          found_path = path;
                          return true;
                        }

                     Glib::ustring filename_utf8 = Glib::filename_to_utf8 (filename).casefold ();
                      if (Glib::filename_to_utf8 (*iter).casefold () == filename_utf8)
                      {
                          found_path = path;
                          return true;
                      }
                    }
                  catch (Glib::ConvertError &error)
                    {
                      g_message("%s: Cannot convert filename %s to UTF-8. G_FILENAME_ENCODING may not be properly set", G_STRLOC, (*iter).c_str());
                    }
              }
              else if (Glib::file_test (path, Glib::FILE_TEST_IS_DIR))
                {
                  if (find_file_actual (path, filename, found_path, max_depth, depth + 1))
                      return true;
                }
          }

          return false;
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

    bool
    find_file (const std::string &dir_path,
               const std::string &filename,
               std::string       &found_path,
               int                max_depth)
    {
        found_path = "";
        return find_file_actual (dir_path, filename, found_path, max_depth, 0);
    }

    // Deletes a directory recursively
    void
    del_directory (const std::string &dir_path)
    {
        Glib::Dir dir (dir_path);
        DirEntries entries (dir.begin(), dir.end());

        for (DirEntries::const_iterator iter = entries.begin (), end = entries.end ();
             iter != end;
             ++iter)
        {
            std::string full_path = Glib::build_filename (dir_path, *iter);

            if (Glib::file_test (full_path, Glib::FILE_TEST_IS_DIR))
                del_directory (full_path);
            else
                unlink (full_path.c_str ());
        }

        rmdir (dir_path.c_str ());
    }

    void
    collect_paths (std::string const& dir_path,
                   FileList&          collection,
                   FilePred           pred,
                   bool               clear)
    {
      if (clear)
        collection.clear ();

      try
        {
          Glib::Dir dir (dir_path);
          DirEntries entries (dir.begin(), dir.end ());

          for (DirEntries::const_iterator iter = entries.begin (), end = entries.end ();
               iter != entries.end ();
               ++iter)
            {
              std::string full_path (Glib::build_filename (dir_path, *iter));

              if (Glib::file_test (full_path, Glib::FILE_TEST_IS_REGULAR))
                {
                  if (pred (full_path))
                    collection.push_back (full_path);
                }
              else if (Glib::file_test (full_path, Glib::FILE_TEST_IS_DIR))
                {
                  // pred is getting repeatedly copied for no good reason!
                  collect_paths (full_path, collection, pred, false);
                }
            }
        }
      catch (Glib::FileError& error)
        {}
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
