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
#ifndef MPX_MUSICLIB_VIEW_ALBUMS_HH
#define MPX_MUSICLIB_VIEW_ALBUMS_HH
#include "config.h"
#include <tr1/unordered_map>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <glib.h>
#include <giomm.h>
#include <libglademm.h>
#include <Python.h>
#define NO_IMPORT
#include <pygobject.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "mpx/mpx-library.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/widgetloader.hh"
#include "mpx/mpx-protected-access.hh"

#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/widgets/cell-renderer-count.hh"
#include "mpx/widgets/cell-renderer-vbox.hh"
#include "mpx/widgets/cell-renderer-expander.h"
#include "mpx/widgets/rounded-layout.hh"
#include "mpx/widgets/timed-confirmation.hh"

#include "mpx/algorithm/aque.hh"

namespace MPX
{
                typedef sigc::signal<void, gint64, bool>      SignalPlayAlbum;
                typedef sigc::signal<void, IdV, bool>         SignalPlayTracks;

                const int N_STARS = 6;

                enum AlbumRowType
                {
                        ROW_ALBUM   =   1,
                        ROW_TRACK   =   2,
                };

                enum AlbumHighlightMode
                {
                        HIGHLIGHT_EQUAL,
                        HIGHLIGHT_UNPLAYED,
                        HIGHLIGHT_PLAYED
                };

                enum AlbumFlags
                {
                    ALBUMS_STATE_NO_FLAGS                =   0,
                    ALBUMS_STATE_SHOW_NEW                =   1 << 0,
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

                struct ReleaseTypeActionInfo
                {
                        char const*     Label;
                        char const*     Name;
                        int             Value;
                };

                class AlbumTreeView
                        :   public Gnome::Glade::WidgetLoader<Gtk::TreeView>
                {
                    public:

                        typedef std::set<Gtk::TreeIter>                         IterSet;
                        typedef std::tr1::unordered_map<std::string, IterSet>   MBIDIterMap;
                        typedef std::tr1::unordered_map<gint64, Gtk::TreeIter>  IdIterMap; 

                        struct ColumnsT : public Gtk::TreeModel::ColumnRecord 
                        {
                                Gtk::TreeModelColumn<AlbumRowType>                          RowType;
                                Gtk::TreeModelColumn<ReleaseType>                           RT;
                                // we use an MPX::Track to store album attributes
                                Gtk::TreeModelColumn<MPX::Track>                            AlbumTrack;

                                Gtk::TreeModelColumn<Cairo::RefPtr<Cairo::ImageSurface> >   Image;
                                Gtk::TreeModelColumn<Glib::ustring>                         Text;

                                Gtk::TreeModelColumn<bool>                                  HasTracks;
                                Gtk::TreeModelColumn<bool>                                  NewAlbum;

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

                                ColumnsT ()
                                {
                                        add (RowType);
                                        add (RT);
                                        add (AlbumTrack);

                                        add (Image);
                                        add (Text);

                                        add (HasTracks);
                                        add (NewAlbum);

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
                                }
                        };

                        public:

                        // Treemodel stuff

                          Glib::RefPtr<Gtk::TreeStore>          AlbumsTreeStore;
                          Glib::RefPtr<Gtk::TreeModelFilter>    AlbumsTreeStoreFilter;
                          ColumnsT                              Columns;

                        protected:

                          std::string                           m_Name;

                        // UI

                          Glib::RefPtr<Gtk::UIManager>          m_UIManager;
                          Glib::RefPtr<Gtk::ActionGroup>        m_ActionGroup;      

                        // Objects

                          PAccess<MPX::Library>                 m_Lib;
                          PAccess<MPX::Covers>                  m_Covers;

                        // View mappings

                          MBIDIterMap                           m_Album_MBID_Iter_Map;
                          IdIterMap                             m_Album_Iter_Map;
                          IdIterMap                             m_Track_Iter_Map;

                        // Disc+rating pixbufs

                            enum EmblemType
                            {
                                EM_COMPILATION,
                                EM_SOUNDTRACK,
    
                                N_EMS
                            };

                            Cairo::RefPtr<Cairo::ImageSurface>    m_DiscDefault;
                            Glib::RefPtr<Gdk::Pixbuf>             m_DiscDefault_DND;
                            Glib::RefPtr<Gdk::Pixbuf>             m_Emblem[N_EMS];
                            Glib::RefPtr<Gdk::Pixbuf>             m_Stars[N_STARS];

                        // DND state variables

                            boost::optional<std::string>          m_DragAlbumMBID;
                            boost::optional<gint64>               m_DragAlbumId;
                            boost::optional<gint64>               m_DragTrackId;

                        // Event handling data

                            Gtk::TreePath                         m_PathButtonPress;
                            bool                                  m_ButtonPressed;

                        // State variables

                            Glib::ustring                         m_FilterText;
                            AQE::Constraints_t                    m_Constraints;

                            struct Options_t
                            {
                                    AlbumHighlightMode    HighlightMode;
                                    int                   Flags;
                                    int                   Type;
                                    int                   Advanced;
                            };

                            Options_t                             Options;

                        // Widgets

                            Gtk::Entry*                           m_FilterEntry;
                            RoundedLayout*                        m_LabelShowing;
                            Gtk::CheckButton*                     m_AdvancedQueryCB;

                            int                                   m_motion_x,
                                                                  m_motion_y;

                        // Signals

                            struct Signals_t
                            {
                                SignalPlayAlbum         PlayAlbum;
                                SignalPlayTracks        PlayTracks; 
                            };

                            Signals_t Signals;

                        public:

                            SignalPlayTracks&
                            signal_play_tracks()
                            {
                                return Signals.PlayTracks;
                            }

                            SignalPlayAlbum&
                            signal_play_album()
                            {
                                return Signals.PlayAlbum;
                            }

                            AlbumTreeView(
                                const Glib::RefPtr<Gnome::Glade::Xml>&,    
                                const std::string&,
                                const std::string&,
                                const std::string&,
                                const std::string&,
                                Glib::RefPtr<Gtk::UIManager>,
                                const PAccess<MPX::Library>&,
                                const PAccess<MPX::Covers>&
                            );

                        protected:

                            virtual void
                                    on_row_expanded (const Gtk::TreeIter &iter_filter,
                                                     const Gtk::TreePath &path);

                            virtual void
                                    on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>&, Gtk::SelectionData&, guint, guint);

                            virtual void
                                    on_drag_begin (const Glib::RefPtr<Gdk::DragContext>&);


                            virtual void
                                    on_drag_data_received (const Glib::RefPtr<Gdk::DragContext>&, int x, int y,
                                                const Gtk::SelectionData& data, guint, guint);

                            virtual bool
                                    on_motion_notify_event(GdkEventMotion*);

                            virtual bool
                                    on_button_press_event (GdkEventButton*);

                            virtual bool
                                    on_button_release_event (GdkEventButton*);

                            virtual bool
                                    on_event (GdkEvent * ev);

                            virtual void
                                    run_rating_comment_dialog(int, gint64);

                            virtual void
                                    on_album_show_info ();

                            virtual void
                                    on_got_cover(const std::string&);

                            virtual Gtk::TreeIter 
                                    place_album (SQL::Row&, gint64);

                            virtual void
                                    update_album (SQL::Row&, gint64);

                            virtual void
                                    place_album_iter_real(
                                        Gtk::TreeIter&  iter,
                                        SQL::Row&       r,
                                        gint64          id
                                    );

                            virtual void
                                    album_list_load ();

                            virtual void
                                    on_album_updated(gint64);

                            virtual void
                                    on_new_album(gint64);

                            virtual void
                                    on_new_track(Track&, gint64, gint64);

                            virtual void
                                    on_album_deleted(gint64);

                            virtual void
                                    on_track_deleted(gint64);

                            virtual int
                                    slotSortRating(const Gtk::TreeIter&, const Gtk::TreeIter&);

                            virtual int
                                    slotSortAlpha(const Gtk::TreeIter&, const Gtk::TreeIter&);

                            virtual int
                                    slotSortDate(const Gtk::TreeIter&, const Gtk::TreeIter&);

                            virtual int
                                    slotSortStrictAlpha(const Gtk::TreeIter&, const Gtk::TreeIter&);

                            virtual int
                                    slotSortPlayScore(const Gtk::TreeIter&, const Gtk::TreeIter&);

                            virtual int
                                    sortTracks(const Gtk::TreeIter&, const Gtk::TreeIter&);

                            virtual void
                                    cellDataFuncCover (Gtk::CellRenderer *, Gtk::TreeModel::iterator const&);

                            virtual void
                                    cellDataFuncText1 (Gtk::CellRenderer *, Gtk::TreeModel::iterator const&);

                            virtual void
                                    cellDataFuncText2 (Gtk::CellRenderer *, Gtk::TreeModel::iterator const&);

                            virtual void
                                    cellDataFuncText3 (Gtk::CellRenderer *, Gtk::TreeModel::iterator const&);

                            virtual void
                                    cellDataFuncText4 (Gtk::CellRenderer *, Gtk::TreeModel::iterator const&);

                            virtual void
                                    cellDataFuncText5 (Gtk::CellRenderer *, Gtk::TreeModel::iterator const&);

                            static void
                                    rb_sourcelist_expander_cell_data_func(
                                        GtkTreeViewColumn*,
                                        GtkCellRenderer*,
                                        GtkTreeModel*,
                                        GtkTreeIter*,
                                        gpointer 
                                    ); 

                            virtual bool
                                    album_visible_func (Gtk::TreeIter const&);

                            virtual void
                                    update_album_count_display ();

                            virtual void
                                    on_row_added_or_deleted ();

                            virtual void
                                    on_filter_entry_changed ();

                            virtual void
                                    on_advanced_query_cb_toggled ();

                        public:

                            virtual void
                                    go_to_album(gint64 id);

                            virtual void
                                    set_new_albums_state (bool);

                            virtual void
                                    set_release_type_filter (int);

                            virtual void
                                    set_highlight_mode (AlbumHighlightMode);
                };

} // end namespace MPX 

#endif
