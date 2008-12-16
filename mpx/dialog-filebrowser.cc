//  MPX
//  Copyright (C) 2005-2007 MPX development.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm/i18n.h>

#include "mpx/mpx-audio.hh"
#include "mpx/mpx-main.hh"
#include "mpx/util-file.hh"

#include "dialog-filebrowser.hh"

using namespace Glib;
using namespace Gtk;

namespace MPX
{
  FileBrowser*
  FileBrowser::create ()
  {
    const std::string path = build_filename (DATA_DIR, "glade" G_DIR_SEPARATOR_S "dialog-filebrowser.glade");
    RefPtr<Gnome::Glade::Xml> glade_xml = Gnome::Glade::Xml::create (path);
    FileBrowser *browser = 0;
    glade_xml->get_widget_derived ("window", browser);
    return browser;
  }

  FileBrowser::~FileBrowser ()
  {
    mcs->key_set<std::string>("mpx", "file-chooser-path", get_current_folder ());
  }

  FileBrowser::FileBrowser (BaseObjectType                 * cobj,
                            RefPtr<Gnome::Glade::Xml> const& xml)
  : FileChooserDialog  (cobj)
  , m_ref_xml          (xml)
  {
    struct {
      const char *name;
      const char *glob;
    } filter_list[] = {
      { "FLAC", "*.[fF][lL][aA][cC]" },
      { "M4A" , "*.[mM]4[aA]" },
      { "MP3" , "*.[mM][pP]3" },
      { "MPC" , "*.[mM][pP][cC]" },
      { "OGG" , "*.[oO][gG][gG]" },
      { "WMA" , "*.[wW][mM][aA]" }
    };

    {
      FileFilter f;
      f.set_name (_("All Files"));
      f.add_pattern ("*");
      add_filter (f);
    }

    {
      FileFilter f;
      f.set_name (_("Audio Files"));
      f.add_custom (FILE_FILTER_URI, sigc::mem_fun (*this, &MPX::FileBrowser::audio_files_filter));
      add_filter (f);
    }

    for (unsigned int n = 0; n < G_N_ELEMENTS(filter_list); ++n)
    {
      FileFilter f;
      f.set_name (filter_list[n].name);
      f.add_pattern (filter_list[n].glob);
      add_filter (f);
    }
    set_current_folder (mcs->key_get<std::string>("mpx", "file-chooser-path"));
  }

  bool
  FileBrowser::audio_files_filter (const FileFilter::Info& info)
  {
    return (::MPX::Audio::is_audio_file(info.uri));
  }

  int
  FileBrowser::run ()
  {
    return Dialog::run ();
  }
}
