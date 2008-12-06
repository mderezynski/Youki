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
#ifndef MPX_MUSICLIB_VIEW_ALBUMS_FILTER_PLUGINS_HH
#define MPX_MUSICLIB_VIEW_ALBUMS_FILTER_PLUGINS_HH
#include "config.h"
#include <gtkmm.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <cairomm/cairomm.h>
#include "mpx/mpx-types.hh"
#include "mpx/algorithm/aque.hh"
#include <tr1/unordered_set>

namespace MPX
{
    enum AlbumRowType
    {
            ROW_ALBUM   =   1,
            ROW_TRACK   =   2,
    };

    enum ReleaseType
    {
        RT_NONE             =   0,   
        RT_ALBUM            =   1 << 0,
        RT_SINGLE           =   1 << 1,
        RT_COMPILATION      =   1 << 2,
        RT_EP               =   1 << 3,
        RT_LIVE             =   1 << 4,
        RT_REMIX            =   1 << 5,
        RT_SOUNDTRACK       =   1 << 6,
        RT_OTHER            =   1 << 7,
        RT_ALL              =   (RT_ALBUM|RT_SINGLE|RT_COMPILATION|RT_EP|RT_LIVE|RT_REMIX|RT_SOUNDTRACK|RT_OTHER)
    };

    struct ViewAlbumsColumnsT : public Gtk::TreeModel::ColumnRecord 
    {
            Gtk::TreeModelColumn<AlbumRowType>                          RowType;
            Gtk::TreeModelColumn<ReleaseType>                           RT;

            Gtk::TreeModelColumn<MPX::Track>                            AlbumTrack;

            Gtk::TreeModelColumn<Cairo::RefPtr<Cairo::ImageSurface> >   Image;
            Gtk::TreeModelColumn<Glib::ustring>                         Text;

            Gtk::TreeModelColumn<bool>                                  HasTracks;
            Gtk::TreeModelColumn<bool>                                  NewAlbum;

            Gtk::TreeModelColumn<std::string>                           Album;
            Gtk::TreeModelColumn<std::string>                           Artist;
            Gtk::TreeModelColumn<std::string>                           AlbumSort;
            Gtk::TreeModelColumn<std::string>                           ArtistSort;

            Gtk::TreeModelColumn<gint64>                                Id;
            Gtk::TreeModelColumn<std::string>                           MBID;
            Gtk::TreeModelColumn<std::string>                           AlbumArtistMBID;

            Gtk::TreeModelColumn<gint64>                                Date;
            Gtk::TreeModelColumn<gint64>                                InsertDate;
            Gtk::TreeModelColumn<gint64>                                Rating;
            Gtk::TreeModelColumn<double>                                PlayScore;

            Gtk::TreeModelColumn<Glib::ustring>                         TrackTitle;
            Gtk::TreeModelColumn<Glib::ustring>                         TrackArtist;
            Gtk::TreeModelColumn<std::string>                           TrackArtistMBID;
            Gtk::TreeModelColumn<gint64>                                TrackNumber;
            Gtk::TreeModelColumn<gint64>                                TrackLength;
            Gtk::TreeModelColumn<gint64>                                TrackId;

            Gtk::TreeModelColumn<bool>                                  Visible;

            ViewAlbumsColumnsT ()
            {
                    add (RowType);
                    add (RT);
                    add (AlbumTrack);

                    add (Image);
                    add (Text);

                    add (HasTracks);
                    add (NewAlbum);

                    add (Album);
                    add (Artist);

                    add (AlbumSort);
                    add (ArtistSort);

                    add (Id);
                    add (MBID);
                    add (AlbumArtistMBID);

                    add (Date);
                    add (InsertDate);
                    add (Rating);
                    add (PlayScore);

                    add (TrackTitle);
                    add (TrackArtist);
                    add (TrackArtistMBID);
                    add (TrackNumber);
                    add (TrackLength);
                    add (TrackId);
    
                    add (Visible);
            }
    };

    namespace ViewAlbumsFilterPlugin
    {
            typedef sigc::signal<void> SignalRefilter;

            struct VAFPSignals_t
            {
                SignalRefilter  Refilter;
            };

            class Base
            {
                public:

                    Base ()
                    {}

                    virtual ~Base ()
                    {}

                    virtual void 
                    on_filter_issued( const Glib::ustring& ) = 0;

                    virtual void
                    on_filter_changed( const Glib::ustring& ) = 0;

                    virtual bool
                    filter_delegate(const Gtk::TreeIter&, const ViewAlbumsColumnsT& columns) = 0;

                    virtual Gtk::Widget*
                    get_ui () = 0;
    
                    virtual SignalRefilter&
                    signal_refilter ()
                    {
                        return Signals.Refilter;
                    }

                protected:

                    VAFPSignals_t Signals;
            };

            class TextMatch : public Base
            {
                public:                    

                    TextMatch ();

                    virtual ~TextMatch ();

                    virtual void 
                    on_filter_issued( const Glib::ustring& );

                    virtual void
                    on_filter_changed( const Glib::ustring& );

                    virtual bool
                    filter_delegate(const Gtk::TreeIter&, const ViewAlbumsColumnsT& columns);

                    virtual Gtk::Widget*
                    get_ui ();

                private:

                    void
                    on_advanced_toggled ();

                    AQE::Constraints_t  m_Constraints;
                    Glib::ustring       m_FilterText, m_FilterEffective;
                    Gtk::CheckButton  * m_UI;
            };

            class LFMTopAlbums : public Base
            {
                public:                    

                    LFMTopAlbums ();

                    virtual ~LFMTopAlbums ();

                    virtual void 
                    on_filter_issued( const Glib::ustring& );

                    virtual void
                    on_filter_changed( const Glib::ustring& );

                    virtual bool
                    filter_delegate(const Gtk::TreeIter&, const ViewAlbumsColumnsT& columns);

                    virtual Gtk::Widget*
                    get_ui ();

                private:
            
                    typedef std::pair<std::string, std::string> AlbumQualifier_t;
                    typedef std::set<AlbumQualifier_t> StringSet_t;
        
                    StringSet_t         m_Names;
                    Glib::ustring       m_FilterText;
            };

            class LFMSimilarArtists : public Base
            {
                public:                    

                    LFMSimilarArtists ();

                    virtual ~LFMSimilarArtists ();

                    virtual void 
                    on_filter_issued( const Glib::ustring& );

                    virtual void
                    on_filter_changed( const Glib::ustring& );

                    virtual bool
                    filter_delegate(const Gtk::TreeIter&, const ViewAlbumsColumnsT& columns);

                    virtual Gtk::Widget*
                    get_ui ();

                private:
            
                    typedef std::set<std::string> StringSet_t;
        
                    StringSet_t         m_Names;
                    Glib::ustring       m_FilterText;
            };

    }
} // end namespace MPX 

#endif
