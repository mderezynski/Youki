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

#include "mpx/com/view-albums-filter-plugin.hh"

namespace MPX
{
                typedef sigc::signal<void, gint64, bool>      SignalPlayAlbum;
                typedef sigc::signal<void, IdV, bool>         SignalPlayTracks;

                const int N_STARS = 6;

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

                struct ReleaseTypeActionInfo
                {
                        char const*     Label;
                        char const*     Name;
                        int             Value;
                };

                typedef boost::shared_ptr<ViewAlbumsFilterPlugin::Base> Plugin_p;
                typedef std::vector<Plugin_p> Plugin_pv;

                class AlbumTreeView
                        :   public Gnome::Glade::WidgetLoader<Gtk::TreeView>
                {
                        public:

                            typedef std::set<Gtk::TreeIter>                         IterSet;
                            typedef std::tr1::unordered_map<std::string, IterSet>   MBIDIterMap;
                            typedef std::tr1::unordered_map<gint64, Gtk::TreeIter>  IdIterMap; 

                        public:

                        // Treemodel stuff

                            Glib::RefPtr<Gtk::TreeStore>          AlbumsTreeStore;
                            Glib::RefPtr<Gtk::TreeModelFilter>    AlbumsTreeStoreFilter;
                            ViewAlbumsColumnsT                    Columns;

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

                        // Filter Plugins

                            Plugin_pv                             m_FilterPlugins;
                            Plugin_p                              m_FilterPlugin_Current;
                            Gtk::Widget                         * m_FilterPluginUI;
                            Gtk::Alignment                      * m_FilterPluginUI_Alignment;
                            Gtk::ComboBox                       * m_FilterPluginsCBox;
                            sigc::connection                      m_ConnFilterEntry_Changed, m_ConnFilterEntry_Activate;

                            void
                            on_plugin_cbox_changed ();
                        

                        // State variables

                            Glib::ustring                         m_FilterText;

                            struct Options_t
                            {
                                    AlbumHighlightMode    HighlightMode;
                                    int                   Flags;
                                    int                   Type;
                            };

                            Options_t                             Options;

                        // Widgets

                            Gtk::Entry*                           m_FilterEntry;
                            RoundedLayout*                        m_LabelShowing;

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
                                    on_drag_motion (const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);

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


                            virtual void
                                    on_filter_entry_changed ();
    
                            virtual void
                                    on_filter_entry_activate ();

                            virtual bool
                                    album_visible_func (Gtk::TreeIter const&);

                            virtual void
                                    refilter ();



                            virtual void
                                    update_album_count_display ();

                            virtual void
                                    on_row_added_or_deleted ();


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
