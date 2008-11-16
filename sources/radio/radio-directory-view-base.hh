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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifndef MPX_RADIO_DIRECTORY_VIEW_BASE_HH
#define MPX_RADIO_DIRECTORY_VIEW_BASE_HH

#include <vector>
#include <glibmm/ustring.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeview.h>
#include "mpx/mpx-minisoup.hh"
#include "mpx/mpx-main.hh"
#include <libglademm.h>

namespace MPX
{
  typedef std::vector<Glib::ustring> VUri;

  namespace RadioDirectory
  {
    class ViewBase :
      public Gtk::TreeView
    {
      protected:

        struct SignalsT
        {
          SignalStartStop   Start;
          SignalStartStop   Stop;
        };

        SignalsT Signals;

      public:

        ViewBase (BaseObjectType * cobj, Glib::RefPtr<Gnome::Glade::Xml> const& xml);
        virtual ~ViewBase () {}

        void set_filter (Glib::ustring const& filter);
        void set_stream_list (StreamListT const& entries);
        void get (Glib::ustring & title, Glib::ustring & uri);

        SignalStartStop&
        signal_start ();

        SignalStartStop&
        signal_stop ();

      protected:

        class Columns
          : public Gtk::TreeModel::ColumnRecord
        {
          public:

            Gtk::TreeModelColumn<Glib::ustring> name;
            Gtk::TreeModelColumn<unsigned int>  bitrate;
            Gtk::TreeModelColumn<Glib::ustring> genre;
            Gtk::TreeModelColumn<Glib::ustring> current;
            Gtk::TreeModelColumn<Glib::ustring> uri;

            Columns ()
            {
              add (name);
              add (bitrate);
              add (genre);
              add (current);
              add (uri);
            }
        };
        Columns columns; 

        bool visible_func (Gtk::TreeIter const& iter);
        void column_clicked (int column);

        struct BaseDataT
        {
          Soup::RequestRefP                   Request; 
          Glib::RefPtr<Gtk::ListStore>        Streams;
          Glib::RefPtr<Gtk::TreeModelFilter>  Filtered;
          Glib::ustring                       Filter;
          int                                 MinimalBitrate;
        };

        BaseDataT BaseData;

        void        
        on_bitrate_changed (MCS_CB_DEFAULT_SIGNATURE);

        static int instance_counter;

    };  // class ViewBase
  }  // namespace RadioDirectory 
}  // namespace MPX 

#endif // !STREAM_LISTER_HH
