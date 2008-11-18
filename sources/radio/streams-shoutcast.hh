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

#ifndef MPX_STREAMS_SHOUTCAST_HH
#define MPX_STREAMS_SHOUTCAST_HH

#include <glibmm/ustring.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeview.h>
#include <libglademm/xml.h>
#include <sigc++/signal.h>
#include <boost/optional.hpp>
#include "mpx/mpx-minisoup.hh"
#include "mpx/xml/xml.hh"
#include "radio-directory-types.hh"

namespace MPX
{
  namespace RadioDirectory
  {
    typedef sigc::signal <void, StreamListT const&, boost::optional<std::string> const& > SignalListUpdated;

    class Columns
      : public Gtk::TreeModel::ColumnRecord
    {
      public:
        Gtk::TreeModelColumn<Glib::ustring> name;
        Columns ()
        {
          add (name);
        }
    };

    class Shoutcast
      : public Gtk::TreeView
    {
        struct SignalsT
        {
          SignalStartStop   Start;
          SignalStartStop   Stop;
          SignalListUpdated ListUpdated;
        };

        SignalsT Signals;

      public:

        Shoutcast (BaseObjectType                       * obj,
                   Glib::RefPtr<Gnome::Glade::Xml> const& xml);

        virtual ~Shoutcast ();

        void  refresh ();

        SignalListUpdated&
        signal_list_updated ();

        SignalStartStop&
        signal_start ();

        SignalStartStop&
        signal_stop ();

      protected:

        void on_row_activated (Gtk::TreeModel::Path const& path, Gtk::TreeViewColumn * column);

      private:

        Columns columns;

        void  genre_cell_data_func (Gtk::CellRenderer *cell, Gtk::TreeIter const& iter);
        void  refresh_wrapper ();
        void  refresh_parse_and_emit_updated (xmlDocPtr doc);
        void  refresh_callback (char const* data, guint size, guint code, std::string const& genre);
        void  rebuild_list (bool force = false);

        struct DataT
        {
          Glib::RefPtr<Gtk::ListStore>  Genres; 
          Gtk::TreeModel::iterator      CurrentIter; 
          GHashTable*                   Cache;
          StreamListT                   StreamList;
          Soup::RequestRefP             Request; 
          boost::optional<std::string>  CustomSearch;
        };

        DataT   Data;


    }; // end class Shoutcast
  } // end namespace RadioDirectory
} // end namespace MPX

#endif // !MPX_STREAMS_SHOUTCAST_HH
