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

#ifndef MPX_UI_PART_LASTFM_HH
#define MPX_UI_PART_LASTFM_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <string>
#include <sigc++/connection.h>
#include <boost/optional.hpp>

#include <glibmm/ustring.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/uimanager.h>
#include <libglademm/xml.h>

#include "mpx/mpx-public.hh"
#include "mpx/i-playbacksource.hh"
#include "mpx/tagview.hh"

#include "lastfm.hh"

using namespace Glib;

namespace MPX
{
    // LastFMArtist
    struct LastFMArtist
    {
      ustring name;
      ustring mbid;
      ustring url;
      ustring thumbnail;
      ustring image;
      guint64 count;
      guint64 rank;
      bool    streamable;

      LastFMArtist () : streamable(false) {}
    };
    typedef std::vector <LastFMArtist> LastFMArtistV;

    class ArtistParser
      : public Glib::Markup::Parser
    {
      public:
        ArtistParser (LastFMArtistV & x) : m_artists (x), m_state (0) {}
        virtual ~ArtistParser () {}
      protected:
        virtual void
        on_start_element  (Glib::Markup::ParseContext   &context,
                           Glib::ustring const          &element_name,
                           AttributeMap const           &attributes);
        virtual void
        on_end_element    (Glib::Markup::ParseContext   &context,
                           Glib::ustring const          &element_name);
        virtual void
        on_text	          (Glib::Markup::ParseContext   &context,
                           Glib::ustring const          &text);
      private:

        enum Element
        {
          E_NONE	      = 0,
          E_ARTIST      = 1 << 0,
          E_NAME        = 1 << 1,
          E_MBID        = 1 << 2,
          E_COUNT       = 1 << 3,
          E_RANK        = 1 << 4,
          E_URL         = 1 << 5,
          E_THUMBNAIL   = 1 << 6,
          E_IMAGE       = 1 << 7,
          E_IMAGE_SMALL = 1 << 8,
          E_MATCH       = 1 << 9,
          E_STREAMABLE  = 1 << 10
        };

        LastFMArtistV & m_artists;
        LastFMArtist    m_current;
        int             m_state;
    };

    enum IdType
    {
      ID_TAG,
      ID_ARTIST,
      ID_AUTO,
    };

    class TagInfoViewT
      : public Gtk::TreeView
    {
      public:

        TagInfoViewT (BaseObjectType*            obj,
                      Glib::RefPtr<Gnome::Glade::Xml> const& xml);
        virtual ~TagInfoViewT () {};

        void
        set_ui_manager (Glib::RefPtr<Gtk::UIManager> const &ui_manager);

        void
        clear ();

        void
        idSetAuto (Glib::ustring const& id);
    
        void
        idSet (IdType type, Glib::ustring const& id);

        bool
        isEmpty ();

      public:

        typedef sigc::signal<void, Glib::ustring const&> SignalString;

      private:

        struct SignalsT
        {
          SignalString ArtistActivated;
          SignalString GoToMBID;
        };

        SignalsT Signals;

      public:

        SignalString&
        signalArtistActivated()
        {
          return Signals.ArtistActivated;
        }

        SignalString&
        signalGoToMBID()
        {
          return Signals.GoToMBID;
        }

      private:                    

        class Columns
          : public Gtk::TreeModel::ColumnRecord
        {
          public:

            Gtk::TreeModelColumn< ::Cairo::RefPtr< ::Cairo::ImageSurface> > image;
            Gtk::TreeModelColumn<LastFMArtist> artist; 
            Gtk::TreeModelColumn<Glib::ustring>      info; 
            Gtk::TreeModelColumn<bool>         hasAlbums; 

            Columns ()
            {
              add (image);
              add (artist);
              add (info);
              add (hasAlbums);
            }
        };

        Columns mColumns;
        Glib::RefPtr<Gtk::ListStore> mStore;

        ::Cairo::RefPtr< ::Cairo::ImageSurface> mUnknownArtist;
        Gtk::Notebook        * mNotebook;
        Gtk::Entry           * mEntry;
        Gtk::ComboBox        * mCbox;
        guint             mTopRank;
        sigc::connection  mProcessIdler;
        LastFMArtistV::iterator mIterator;
        Gtk::TreeIter          mTreeIterator;
        LastFMArtistV     mArtists;
        Glib::Mutex       mDisplayLock;
        bool              mEndDisplay;
        ::Cairo::RefPtr< ::Cairo::ImageSurface > mImage;
        LastFMArtist      mArtist;
        Soup::RequestRefP mArtistReq;

        bool
        on_event (GdkEvent*);

        bool
        idler ();

        void
        cellDataFunc (Gtk::CellRenderer* cell, Gtk::TreeIter const& m_iter, int column);

        void
        displayThread ();

        void
        showArtist ();

        void  
        data_cb (char const * data, guint size, guint code);

        void
        on_selection_changed ();

        void
        on_view_in_library ();

        void
        on_play_lastfm ();

        Glib::RefPtr<Gtk::UIManager> m_ui_manager;
        Glib::RefPtr<Gtk::ActionGroup> m_actions;
    };


namespace Source
{
    class LastFM
      : public PlaybackSource
    {
      public:

        LastFM (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
        virtual ~LastFM () {}
  
      private:

		Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;
		Glib::RefPtr<Gtk::ActionGroup>  m_actions;
		Glib::RefPtr<Gtk::UIManager>    m_ui_manager;
		Glib::RefPtr<Gtk::UIManager>    m_ui_manager_main;
		Gtk::Widget					  * m_UI;
        Gtk::Entry                    * m_URL_Entry;
        Gtk::ComboBox                 * m_CBox_Sel;
        Gtk::HBox                     * m_HBox_Error;
        Gtk::Label                    * m_Label_Error;
        Gtk::Button                   * m_Button_Error_Hide;
        TagInfoViewT                  * m_TagInfoView;
    
        MPX::LastFMRadio                m_LastFMRadio;
        boost::optional<XSPF::Playlist> m_Playlist;
        XSPF::ItemIter                  m_PlaylistIter;
        Glib::Mutex                     m_PlaylistLock;
        

        MPX::Player                   & m_Player;

        void
        on_playlist (XSPF::Playlist const&);

        void
        on_no_playlist ();

        void
        on_tuned ();

        void
        on_tune_error (Glib::ustring const&);


        void
        on_url_entry_activated ();

      protected:

	    virtual Glib::RefPtr<Gdk::Pixbuf>
		get_icon ();

	    virtual Gtk::Widget*
		get_ui ();

        virtual std::string
        get_uri ();

        virtual bool
        go_next ();

        virtual void
        go_next_async ();

        virtual bool
        go_prev ();

        virtual void
        go_prev_async ();

        virtual void
        stop ();

        virtual bool
        play ();

        virtual void
        play_async ();

        virtual void
        play_post ();

        virtual void
        next_post ();

        virtual void
        restore_context ();

		virtual void
		send_metadata ();

        // UriHandler
        virtual UriSchemes 
        Get_Schemes (); 

        virtual void    
        Process_URI_List (Util::FileList const&);
    };
  }
}
#endif // !MPX_UI_PART_LASTFM
