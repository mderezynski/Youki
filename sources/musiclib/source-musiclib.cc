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
#include "config.h"
#ifdef HAVE_TR1
#include <tr1/unordered_map>
#endif //HAVE_TR1
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

#include "mpx/mpx-hal.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/playlistparser-xspf.hh"
#include "mpx/playlistparser-pls.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/widgetloader.hh"

#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/widgets/cell-renderer-count.hh"
#include "mpx/widgets/cell-renderer-vbox.hh"
#include "mpx/widgets/cell-renderer-expander.h"
#include "mpx/widgets/rounded-layout.hh"

#include "mpx/com/file-system-tree.hh"
#include "mpx/com/view-tracklist.hh"
#include "mpx/com/view-albums.hh"
#include "mpx/com/view-collections.hh"

#include "source-musiclib.hh"
#include "musiclib-py.hh"
#include "glib-marshalers.h"

#include "view.hh"
#include "model.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using boost::get;
using boost::algorithm::trim;

namespace
{
        const int N_STARS = 6;

        const char ACTION_CLEAR [] = "musiclib-playlist-action-clear";
        const char ACTION_REMOVE_ITEMS [] = "musiclib-playlist-action-remove-items";
        const char ACTION_REMOVE_REMAINING [] = "musiclib-playlist-action-remove-remaining";
        const char ACTION_REMOVE_PRECEDING [] = "musiclib-playlist-action-remove-preceding";
        const char ACTION_PLAY [] = "musiclib-playlist-action-play";
        const char ACTION_SHOW_CCDIALOG [] = "musiclib-playlist-action-show-ccdialog";

        const char ui_playlist_popup [] =
                "<ui>"
                ""
                "<menubar name='musiclib-playlist-popup-playlist-list'>"
                ""
                "   <menu action='dummy' name='musiclib-playlist-menu-playlist-list'>"
                "       <menuitem action='musiclib-playlist-action-play' name='musiclib-playlist-action-play'/>"
                "       <menuitem action='musiclib-playlist-action-remove-items'/>"
                "     <separator/>"
                "       <menuitem action='musiclib-playlist-action-clear'/>"
                "       <menuitem action='musiclib-playlist-action-remove-preceding'/>"
                "       <menuitem action='musiclib-playlist-action-remove-remaining'/>"
                "     <separator/>"
                "       <menuitem action='musiclib-playlist-action-show-ccdialog'/>"
                "     <separator/>"
                "       <placeholder name='musiclib-playlist-placeholder-playlist'/>"
                "   </menu>"
                ""
                "</menubar>"
                ""
                "</ui>";

        char const * ui_source =
                "<ui>"
                ""
                "<menubar name='MenubarMain'>"
                "   <placeholder name='placeholder-source'>"
                "     <menu action='menu-source-musiclib'>"
                "         <menuitem action='musiclib-show-albums'/>"
                "         <menuitem action='musiclib-show-alltracks'/>"
                "         <menuitem action='musiclib-show-collections'/>"
                "         <separator/>"
                "         <menu action='menu-musiclib-albums'>"
                "           <menuitem action='musiclib-albums-show-all'/>"
                "           <menuitem action='musiclib-albums-show-albums'/>"
                "           <menuitem action='musiclib-albums-show-singles'/>"
                "           <menuitem action='musiclib-albums-show-compilations'/>"
                "           <menuitem action='musiclib-albums-show-eps'/>"
                "           <menuitem action='musiclib-albums-show-live'/>"
                "           <menuitem action='musiclib-albums-show-remix'/>"
                "           <menuitem action='musiclib-albums-show-soundtracks'/>"
                "           <menuitem action='musiclib-albums-show-other'/>"
                "         </menu>"
                "         <menu action='menu-musiclib-sort'>"
                "           <menuitem action='musiclib-sort-by-name'/>"
                "           <menuitem action='musiclib-sort-by-alphabet'/>"
                "           <menuitem action='musiclib-sort-by-date'/>"
                "           <menuitem action='musiclib-sort-by-playscore'/>"
                "           <menuitem action='musiclib-sort-by-rating'/>"
                "         </menu>"
                "         <menu action='menu-musiclib-highlight'>"
                "           <menuitem action='musiclib-highlight-equal'/>"
                "           <menuitem action='musiclib-highlight-unplayed'/>"
                "           <menuitem action='musiclib-highlight-played'/>"
                "         </menu>"
                "         <separator/>"
                "         <menuitem action='musiclib-show-only-new'/>"
                "         <separator/>"
                "         <menuitem action='musiclib-recache-covers'/>"
                "     </menu>"
                "   </placeholder>"
                "</menubar>"
                ""
                "</ui>"
                "";

        std::string
                get_timestr_from_time_t (time_t atime)
                {
                        struct tm atm, btm;
                        localtime_r (&atime, &atm);
                        time_t curtime = time(NULL);
                        localtime_r (&curtime, &btm);

                        static boost::format date_f ("%s, %s");

                        if (atm.tm_year == btm.tm_year &&
                                        atm.tm_yday == btm.tm_yday)
                        {
                                char btime[64];
                                strftime (btime, 64, "%H:00", &atm); // we just ASSUME that no podcast updates more than once an hour, for cleaner readbility

                                return locale_to_utf8 ((date_f % _("Today") % btime).str());
                        }
                        else
                        {
                                char bdate[64];
                                strftime (bdate, 64, "%d %b %Y", &atm);

                                char btime[64];
                                strftime (btime, 64, "%H:00", &atm); // we just ASSUME that no podcast updates more than once an hour, for cleaner readbility

                                return locale_to_utf8 ((date_f % bdate % btime).str());
                        }
                }

        ReleaseType
                determine_release_type (const std::string& type)
                {
                        if( type == "Album" )
                                return RT_ALBUM;

                        if( type == "Single" )
                                return RT_SINGLE;

                        if( type == "Compilation" )
                                return RT_COMPILATION;

                        if( type == "Ep" )
                                return RT_EP;

                        if( type == "Live" )
                                return RT_LIVE;

                        if( type == "Remix" )
                                return RT_REMIX;

                        if( type == "Soundtrack" )
                                return RT_SOUNDTRACK;

                        return RT_OTHER;
                }

        enum Order
        {
                NO_ORDER,
                ORDER
        };

        std::string
                ovariant_get_string (MPX::OVariant & self)
                {
                        if(!self.is_initialized())
                        {
                                return std::string();
                        }

                        std::string output;

                        switch(self.get().which())
                        {
                                case 0:
                                        output = boost::lexical_cast<std::string>(boost::get<gint64>(self.get()));
                                        break;
                                case 1:
                                        output = boost::lexical_cast<std::string>(boost::get<double>(self.get()));
                                        break;
                                case 2:
                                        output = boost::get<std::string>(self.get());
                                        break;
                        }

                        return output;
                }

        // Column-Control

        char const* attribute_names[] =
        {
                N_("File Location"),
                N_("Name"),
                N_("Genre"),
                N_("Comment"),
                N_("MusicIP PUID"),
                N_("File Hash"),
                N_("MusicBrainz Track ID"),
                N_("Artist"),
                N_("Artist Sort Name"),
                N_("MusicBrainz Artist ID"),
                N_("Album"),
                N_("MusicBrainz Album ID"),
                N_("Release Date"),
                N_("Release Country"),
                N_("Release Type"),
                N_("Amazon ASIN"),
                N_("Album Artist"),
                N_("Album Artist Sort Name"),
                N_("MusicBrainz Album Artist ID"),
                N_("MIME Type"),
                N_("HAL Volume UDI"),
                N_("HAL Device UDI"),
                N_("Volume-relative Path"),
                N_("Insert Path"),
                N_("Location Name"),
                N_("Track"),
                N_("Time"),
                N_("Rating"),
                N_("Date"),
                N_("Change Time"),
                N_("Bitrate"),
                N_("Samplerate"),
                N_("Play Count"),
                N_("Play Date"),
                N_("Insert Date"),
                N_("Is MB Album Artist"),
                N_("Active"),
                N_("Audio Quality"),
                N_("MPX Track ID"),
                N_("MPX Album ID")
        };

        typedef sigc::signal<void, int, bool> SignalColumnState;

        class ColumnControlView : public WidgetLoader<Gtk::TreeView>
        {
                public:

                        struct Signals_t 
                        {
                                SignalColumnState ColumnState;
                        };

                        Signals_t Signals;

                        class Columns_t : public Gtk::TreeModelColumnRecord
                        {
                            public: 

                                Gtk::TreeModelColumn<Glib::ustring> Name;
                                Gtk::TreeModelColumn<int>           ID;
                                Gtk::TreeModelColumn<bool>          Active;

                                Columns_t ()
                                {
                                        add(Name);
                                        add(ID);
                                        add(Active);
                                };
                        } ;

                        Columns_t                       Columns;
                        Glib::RefPtr<Gtk::ListStore>    Store;

                        ColumnControlView (const Glib::RefPtr<Gnome::Glade::Xml>& xml)
                                : WidgetLoader<Gtk::TreeView>(xml, "cc-treeview")
                        {
                                Store = Gtk::ListStore::create(Columns); 
                                set_model(Store);

                                TreeViewColumn *col = manage( new TreeViewColumn(_("Active")));
                                CellRendererToggle *cell1 = manage( new CellRendererToggle );
                                cell1->property_xalign() = 0.5;
                                cell1->signal_toggled().connect(
                                                sigc::mem_fun(
                                                        *this,
                                                        &ColumnControlView::on_cell_toggled
                                                        ));
                                col->pack_start(*cell1, true);
                                col->add_attribute(*cell1, "active", Columns.Active);
                                append_column(*col);            
                                append_column(_("Column"), Columns.Name);            

                                mcs->domain_register("PlaybackSourceMusicLib-PlaylistColumns");

                                for( int n = 0; n < N_ATTRIBUTES_INT; ++n )
                                {
                                        TreeIter iter = Store->append();

                                        (*iter)[Columns.Name]   = _(attribute_names[n]);
                                        (*iter)[Columns.ID]     = n;
                                        (*iter)[Columns.Active] = false;

                                        mcs->key_register("PlaybackSourceMusicLib-PlaylistColumns", (boost::format("Column%d") % n ).str(), false);
                                }
                        };

                        void
                                on_cell_toggled(Glib::ustring const& path)
                                {
                                        TreeIter iter = Store->get_iter(path);

                                        bool active = (*iter)[Columns.Active];
                                        (*iter)[Columns.Active] = !active;

                                        Signals.ColumnState.emit((*iter)[Columns.ID], !active);

                                        int n = TreePath(path)[0];
                                        mcs->key_set<bool>("PlaybackSourceMusicLib-PlaylistColumns", (boost::format ("Column%d") % n).str(), !active); 
                                }

                        void
                                restore_column_states ()
                                {
                                        for( int n = 0; n < N_ATTRIBUTES_INT; ++n )
                                        {
                                                bool active = mcs->key_get<bool>("PlaybackSourceMusicLib-PlaylistColumns", (boost::format("Column%d") % n ).str());
                                                if ( active )
                                                {
                                                    TreePath path (TreePath::size_type(1), TreePath::value_type(n));
                                                    on_cell_toggled( path.to_string() );
                                                }
                                        }
                                }
        };

        class ColumnControlDialog : public WidgetLoader<Gtk::Dialog>
        {
                ColumnControlView *m_ControlView;

                public:

                ColumnControlDialog(const Glib::RefPtr<Gnome::Glade::Xml>& xml)
                        : WidgetLoader<Gtk::Dialog>(xml, "cc-dialog")
                {
                        m_ControlView = new ColumnControlView(xml) ;
                }

                virtual ~ColumnControlDialog(
                )
                {
                        delete m_ControlView ;
                }

                SignalColumnState&
                        signal_column_state()
                        {
                                return m_ControlView->Signals.ColumnState;
                        }

                void
                        restore_column_states()
                        {
                                m_ControlView->restore_column_states();
                        }
        };


        class ArtistSelectView : public WidgetLoader<Gtk::TreeView>
        {
                public:
            
                        typedef sigc::signal<void, gint64> SignalAlbumArtistSelected;

                        struct Signals_t 
                        {
                                SignalAlbumArtistSelected   AlbumArtistSelected;
                        };

                        Signals_t Signals;

                        class Columns_t : public Gtk::TreeModelColumnRecord
                {
                        public: 

                                Gtk::TreeModelColumn<Glib::ustring> Name;
                                Gtk::TreeModelColumn<int>           ID;

                                Columns_t ()
                                {
                                        add(Name);
                                        add(ID);
                                };
                };


                        Columns_t                       Columns;
                        Glib::RefPtr<Gtk::ListStore>    Store;

                        ArtistSelectView(
                            const Glib::RefPtr<Gnome::Glade::Xml>&  xml,
                            const std::string&                      name,
                            MPX::Library&                           lib
                        )
                        : WidgetLoader<Gtk::TreeView>(xml, name)
                        {
                                using boost::get;
    
                                Store = Gtk::ListStore::create(Columns); 
                                append_column(_("Column"), Columns.Name);            

                                SQL::RowV v;
                                lib.getSQL(v, "SELECT * FROM album_artist");
                                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                {
                                    TreeIter iter = Store->append();

                                    (*iter)[Columns.ID] = get<gint64>((*i)["id"]);
                                    (*iter)[Columns.Name] = get<std::string>((*i)["album_artist"]);
                                }
                                
                                set_model(Store);
                        };
        };
}

namespace MPX
{
        typedef std::vector<Gtk::TreePath> PathV;
        typedef std::vector<Gtk::TreeRowReference> ReferenceV;
        typedef ReferenceV::iterator ReferenceVIter;

        class ReferenceCollect
        {
                public:

                                ReferenceCollect(
                                    const Glib::RefPtr<Gtk::TreeModel>& model,
                                    ReferenceV&                         references
                                )
                                : m_model       (model)
                                , m_references  (references)
                                {} 

                        void
                                operator()(Gtk::TreePath const& p)
                                {
                                        m_references.push_back (TreeRowReference (m_model, p));
                                }

                private:

                        Glib::RefPtr<Gtk::TreeModel> const& m_model; 
                        ReferenceV & m_references;
        };  

        struct MusicLibPrivate
        {
                Glib::RefPtr<Gnome::Glade::Xml>     m_RefXml;
                Gtk::Widget                       * m_UI;

                class PlaylistTreeView
                        :   public WidgetLoader<Gtk::TreeView>
                {
                        typedef std::set<TreeIter>            IterSet_t;
                        typedef std::map<gint64, IterSet_t>   IdIterMap_t ; 
   
                        IdIterMap_t                           m_IdIterMap; 

                        ColumnControlDialog                 * m_CCDialog ;

                        MPX::Source::PlaybackSourceMusicLib & m_MLib ;

                        Glib::RefPtr<Gdk::Pixbuf>             m_Playing ;
                        Glib::RefPtr<Gdk::Pixbuf>             m_Bad ;

                        TreePath                              m_PathButtonPress ;
                        int                                   m_ButtonDepressed ;

                        Glib::RefPtr<Gtk::UIManager>          m_UIManager ;
                        Glib::RefPtr<Gtk::ActionGroup>        m_ActionGroup ;      

                        public:

                        boost::optional<Gtk::TreeIter>        m_CurrentIter ;
                        boost::optional<Gtk::TreeIter>        m_PlayInitIter ;
                        boost::optional<gint64>               m_CurrentId ;
                        Glib::RefPtr<Gdk::Pixbuf>             m_Stars[N_STARS] ;
                        PlaylistColumnsT                      PlaylistColumns ;
                        Glib::RefPtr<Gtk::ListStore>          ListStore ;

                        Library                             * m_Library;

                        enum Column
                        {
                                C_PLAYING,
                                C_TITLE,
                                C_LENGTH,
                                C_ARTIST,
                                C_ALBUM,
                                C_TRACK,
                                C_RATING,
                                C_COUNT
                        };

                        static const int N_FIRST_CUSTOM = 8;

                        virtual ~PlaylistTreeView(
                        )
                        {
                            delete m_CCDialog;
                        }

                        PlaylistTreeView(
                            Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                            Glib::RefPtr<Gtk::UIManager>    const& ui_manager,
                            MPX::Source::PlaybackSourceMusicLib  & mlib
                        )
                        : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-playlist")
                        , m_MLib(mlib)
                        , m_ButtonDepressed(0)
                        {
                                m_Library = services->get<Library>("mpx-service-library").get();

                                set_has_tooltip();
                                set_rules_hint();

                                m_Playing = /*Gdk::Pixbuf::create_from_file(Glib::build_filename(Glib::build_filename(DATA_DIR,"images"),"play.png"));*/
                                            render_icon(Gtk::StockID(GTK_STOCK_MEDIA_PLAY), Gtk::ICON_SIZE_MENU)->scale_simple(16,16,Gdk::INTERP_HYPER);
                                m_Bad = render_icon (StockID (MPX_STOCK_ERROR), ICON_SIZE_SMALL_TOOLBAR);

                                for(int n = 0; n < N_STARS; ++n)
                                {
                                        m_Stars[n] = Gdk::Pixbuf::create_from_file(
                                                        build_filename(
                                                                build_filename(
                                                                        DATA_DIR,
                                                                        "images"
                                                                        ),
                                                                (boost::format("stars%d.png") % n).str()
                                        ));
                                }

                                TreeViewColumn * col = manage (new TreeViewColumn(""));
                                CellRendererPixbuf * cell = manage (new CellRendererPixbuf);
                                col->pack_start(*cell, true);
                                col->set_min_width(24);
                                col->set_max_width(24);
                                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncIcon ));
                                append_column(*col);

                                append_column(_("Name"), PlaylistColumns.Name);

                                col = manage (new TreeViewColumn(_("Time")));
                                CellRendererText * cell2 = manage (new CellRendererText);
                                col->property_alignment() = 1.;
                                col->pack_start(*cell2, true);
                                col->set_cell_data_func(*cell2, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncTime ));
                                col->set_sort_column_id(PlaylistColumns.Length);
                                g_object_set(G_OBJECT(cell2->gobj()), "xalign", 1.0f, NULL);
                                append_column(*col);

                                append_column(_("Artist"), PlaylistColumns.Artist);
                                append_column(_("Album"), PlaylistColumns.Album);
                                append_column(_("Track"), PlaylistColumns.Track);

                                col = manage (new TreeViewColumn(_("My Rating")));
                                cell = manage (new CellRendererPixbuf);
                                col->pack_start(*cell, false);
                                col->set_min_width(66);
                                col->set_max_width(66);
                                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncRating ));
                                append_column(*col);

                                append_column(_("Count"), PlaylistColumns.Playcount);

                                //////////////////////////////// 

                                cell2 = manage (new CellRendererText);
                                for( int i = 0; i < N_ATTRIBUTES_INT; ++i)
                                {
                                        col = manage (new TreeViewColumn(_(attribute_names[i])));
                                        col->pack_start(*cell2, true);
                                        col->set_cell_data_func(*cell2, sigc::bind(sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncCustom ), i ));
                                        col->property_visible()= false;
                                        append_column(*col);
                                }

                                ////////////////////////////////

                                get_column(C_TITLE)->set_sort_column_id(PlaylistColumns.Name);
                                get_column(C_LENGTH)->set_sort_column_id(PlaylistColumns.Length);
                                get_column(C_ARTIST)->set_sort_column_id(PlaylistColumns.Artist);
                                get_column(C_ALBUM)->set_sort_column_id(PlaylistColumns.Album);
                                get_column(C_TRACK)->set_sort_column_id(PlaylistColumns.Track);
                                get_column(C_RATING)->set_sort_column_id(PlaylistColumns.Rating);
                                get_column(C_COUNT)->set_sort_column_id(PlaylistColumns.Playcount);

                                get_column(0)->set_resizable(false);
                                get_column(1)->set_resizable(true);
                                get_column(2)->set_resizable(false);
                                get_column(3)->set_resizable(true);
                                get_column(4)->set_resizable(true);
                                get_column(5)->set_resizable(false);
                                get_column(6)->set_resizable(false);
                                get_column(7)->set_resizable(false);

                                /*
                                for( int n = 1; n <= 5; ++ n)
                                {
                                    CellRenderer * cell = get_column( n )->get_first_cell_renderer();
                                    get_column( n )->set_cell_data_func(
                                        *cell,
                                        sigc::mem_fun(
                                            *this,
                                            &PlaylistTreeView::cellDataFuncCurrentRow
                                    ));
                                }*/

                                ListStore = Gtk::ListStore::create(PlaylistColumns);

                                ListStore->set_sort_func(PlaylistColumns.Artist,
                                                sigc::mem_fun( *this, &PlaylistTreeView::slotSortByArtist ));
                                ListStore->set_sort_func(PlaylistColumns.Album,
                                                sigc::mem_fun( *this, &PlaylistTreeView::slotSortByAlbum ));
                                ListStore->set_sort_func(PlaylistColumns.Track,
                                                sigc::mem_fun( *this, &PlaylistTreeView::slotSortByTrack ));
                                ListStore->set_default_sort_func(
                                                sigc::mem_fun( *this, &PlaylistTreeView::slotSortDefault ));

                                get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
                                get_selection()->signal_changed().connect( sigc::mem_fun( *this, &PlaylistTreeView::on_selection_changed ) );

                                set_headers_clickable();
                                set_model(ListStore);

                                m_UIManager = ui_manager;

                                m_ActionGroup = ActionGroup::create ("Actions_UiPartMusicLib-PlaylistList");
                                m_ActionGroup->add(Action::create("dummy","dummy"));

                                m_ActionGroup->add(
                                                Action::create(
                                                    ACTION_PLAY,
                                                    Gtk::StockID (GTK_STOCK_MEDIA_PLAY),
                                                    _("Play")
                                                ),
                                                sigc::mem_fun(
                                                    mlib,
                                                    &MPX::Source::PlaybackSourceMusicLib::action_cb_play
                                                ));

                                m_ActionGroup->add(
                                                Action::create(
                                                    ACTION_CLEAR,
                                                    Gtk::StockID (GTK_STOCK_CLEAR),
                                                    _("Clear Playlist")
                                                ),
                                                AccelKey("<ctrl>d"),
                                                sigc::mem_fun(
                                                    *this,
                                                    &PlaylistTreeView::clear
                                                ));

                                m_ActionGroup->add(
                                                Action::create(
                                                    ACTION_REMOVE_ITEMS,
                                                    Gtk::StockID (GTK_STOCK_REMOVE),
                                                    _("Remove selected Tracks")
                                                ),
                                                sigc::mem_fun(
                                                    *this,
                                                    &PlaylistTreeView::action_cb_playlist_remove_items 
                                                ));

                                m_ActionGroup->add(
                                                Action::create(
                                                    ACTION_REMOVE_REMAINING,
                                                    _("Remove remaining Tracks")
                                                ),
                                                AccelKey("<ctrl>r"),
                                                sigc::mem_fun(
                                                    *this,
                                                    &PlaylistTreeView::action_cb_playlist_remove_remaining
                                                ));

                                m_ActionGroup->add(
                                                Action::create(
                                                    ACTION_REMOVE_PRECEDING,
                                                    _("Remove preceding Tracks")
                                                ),
                                                AccelKey("<ctrl>p"),
                                                sigc::mem_fun(
                                                    *this,
                                                    &PlaylistTreeView::action_cb_playlist_remove_preceding
                                                ));

                                m_ActionGroup->add(
                                                Action::create(
                                                    ACTION_SHOW_CCDIALOG,
                                                _("Configure Columns...")
                                                ),
                                                sigc::mem_fun(
                                                    *this,
                                                    &PlaylistTreeView::action_cb_show_ccdialog
                                                ));

                                m_UIManager->insert_action_group (m_ActionGroup);
                                m_UIManager->add_ui_from_string (ui_playlist_popup);

                                Gtk::Widget * item = m_UIManager->get_widget("/ui/musiclib-playlist-popup-playlist-list/musiclib-playlist-menu-playlist-list/musiclib-playlist-action-play");
                                Gtk::Label * label = dynamic_cast<Gtk::Label*>(dynamic_cast<Gtk::Bin*>(item)->get_child());
                                label->set_markup(_("<b>Play</b>"));

                                m_CCDialog = new ColumnControlDialog(xml);
                                m_CCDialog->signal_column_state().connect(
                                                sigc::mem_fun(
                                                        *this,
                                                        &PlaylistTreeView::on_column_toggled
                                                        ));
                                m_CCDialog->restore_column_states();

                                set_tooltip_text(_("Drag and drop albums, tracks and files here to add them to the playlist."));

                                std::vector<TargetEntry> Entries;
                                Entries.push_back(TargetEntry("mpx-album"));
                                Entries.push_back(TargetEntry("mpx-track"));
                                drag_dest_set(Entries, DEST_DEFAULT_MOTION);
                                drag_dest_add_uri_targets();

                                m_Library->signal_track_updated().connect(
                                    sigc::mem_fun(
                                        *this,
                                        &PlaylistTreeView::on_library_track_updated
                                ));
                        }

                        void
                                on_library_track_updated(
                                      Track& track
                                    , gint64
                                    , gint64
                                )
                                {
                                    gint64 id = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get());
                                    if( m_IdIterMap.count(id))
                                    {
                                        IterSet_t s = m_IdIterMap.find( id )->second; // intentional copy
                                        m_IdIterMap.erase(id);
        
                                        for( IterSet_t::const_iterator i = s.begin(); i != s.end(); ++i )
                                        {
                                            place_track( track, *i );
                                        }
                                    }
                                }

                        void
                                erase_track_from_store(
                                      gint64 id
                                    , const TreeIter& iter
                                )
                                {
                                    if( m_IdIterMap.count(id))
                                    {
                                        m_IdIterMap[id].erase(iter);
                                        ListStore->erase(iter);
                                    }
                                }

                        void
                                on_column_toggled( int column, bool state )
                                {
                                        get_column(N_FIRST_CUSTOM + column)->property_visible() = state;
                                }

                        void
                                action_cb_show_ccdialog ()
                                {
                                        m_CCDialog->run();
                                        m_CCDialog->hide();
                                }

                        virtual void
                                action_cb_go_selected_album ()
                                {
                                        PathV p = get_selection()->get_selected_rows();
                                        TreeIter iter = ListStore->get_iter(p[0]);
                                        Track t = (*iter)[PlaylistColumns.MPXTrack];
                                        if(t[ATTRIBUTE_MPX_ALBUM_ID])
                                        {
                                                gint64 id = get<gint64>(t[ATTRIBUTE_MPX_ALBUM_ID].get());
                                                m_MLib.action_cb_go_to_album(id);
                                        }
                                        else
                                                g_message("%s: No Album ID", G_STRLOC);
                                }

                        virtual void
                                action_cb_playlist_remove_items ()
                                {
                                        PathV p = get_selection()->get_selected_rows();
                                        ReferenceV v;
                                        std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, v));
                                        ReferenceVIter i;
                                        for (i = v.begin() ; !v.empty() ; )
                                        {
                                                TreeIter iter = ListStore->get_iter (i->get_path());
                                                if(m_CurrentIter && m_CurrentIter.get() == iter)
                                                {
                                                        m_CurrentIter.reset();
                                                }
                                                erase_track_from_store((*iter)[PlaylistColumns.RowId], iter);
                                                i = v.erase (i);
                                        }

                                        m_MLib.check_nextprev_caps();
                                        m_MLib.send_caps ();

                                        if(ListStore->children().empty())
                                        {
                                                m_MLib.plist_end(true);
                                        }

                                        check_for_end ();
                                }

                        virtual void
                                action_cb_playlist_remove_remaining ()
                                {
                                        TreePath path = ListStore->get_path(m_CurrentIter.get());
                                        path.next();

                                        PathV p; 
                                        while( guint(path.get_indices().data()[0]) < ListStore->children().size() )
                                        {
                                                p.push_back(path);
                                                path.next ();
                                        }

                                        ReferenceV v;
                                        std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, v));

                                        ReferenceVIter i;
                                        for (i = v.begin() ; !v.empty() ; )
                                        {
                                                TreeIter iter = ListStore->get_iter (i->get_path());
                                                erase_track_from_store((*iter)[PlaylistColumns.RowId], iter);
                                                i = v.erase (i);
                                        }

                                        m_MLib.check_nextprev_caps();
                                        m_MLib.send_caps ();

                                        check_for_end ();
                                }

                        virtual void
                                action_cb_playlist_remove_preceding ()
                                {
                                        TreeIter iter = m_CurrentIter.get();
                    					TreePath path_iter (ListStore->get_path(iter));
                    					TreePath path (TreePath::size_type(1), TreePath::value_type(0));

                                        PathV p; 
                                        while( path < path_iter ) 
                                        {
                                                p.push_back(path);
                                                path.next ();
                                        }

                                        ReferenceV v;
                                        std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, v));

                                        ReferenceVIter i;
                                        for (i = v.begin() ; !v.empty() ; )
                                        {
                                                TreeIter iter = ListStore->get_iter (i->get_path());
                                                erase_track_from_store((*iter)[PlaylistColumns.RowId], iter);
                                                i = v.erase (i);
                                        }

                                        m_MLib.check_nextprev_caps();
                                        m_MLib.send_caps ();

                                        check_for_end ();
                                }

                        void
                                on_selection_changed ()
                                {
                                        if(get_selection()->count_selected_rows() == 1)
                                                m_MLib.set_play();
                                        else
                                                m_MLib.clear_play();
                                }

                        virtual void
                                scroll_to_track ()
                                {
                                        if(m_CurrentIter)
                                        {
                                                TreePath path = ListStore->get_path(m_CurrentIter.get());
                                                scroll_to_row(path, 0.5);
                                        }
                                }

                        virtual void
                                clear ()  
                                {
                                        ListStore->clear ();
                                        m_CurrentIter.reset ();
                                        m_MLib.check_nextprev_caps();
                                        m_MLib.send_caps ();
                                        columns_autosize();
                                }

                        void
                                check_for_end ()
                                {
                                        if(m_CurrentIter)
                                        {
                                                if(TreePath (m_CurrentIter.get()) == TreePath(1, ListStore->children().size() - 1))
                                                {
                                                        m_MLib.plist_end(false);
                                                }
                                        }
                                }

                        void
                                place_track(SQL::Row & r, Gtk::TreeIter const& iter)
                                {
                                        Track_sp t = m_Library->sqlToTrack( r );
                                        place_track( *(t.get()), iter );
                                }

                        void
                                place_track(MPX::Track & track, Gtk::TreeIter const& iter)
                                {
                                        if(track[ATTRIBUTE_ARTIST])
                                                (*iter)[PlaylistColumns.Artist] = get<std::string>(track[ATTRIBUTE_ARTIST].get()); 

                                        if(track[ATTRIBUTE_ALBUM])
                                                (*iter)[PlaylistColumns.Album] = get<std::string>(track[ATTRIBUTE_ALBUM].get()); 

                                        if(track[ATTRIBUTE_TRACK])
                                                (*iter)[PlaylistColumns.Track] = guint64(get<gint64>(track[ATTRIBUTE_TRACK].get()));

                                        if(track[ATTRIBUTE_TITLE])
                                                (*iter)[PlaylistColumns.Name] = get<std::string>(track[ATTRIBUTE_TITLE].get()); 

                                        if(track[ATTRIBUTE_TIME])
                                                (*iter)[PlaylistColumns.Length] = guint64(get<gint64>(track[ATTRIBUTE_TIME].get()));

                                        if(track[ATTRIBUTE_MB_ARTIST_ID])
                                                (*iter)[PlaylistColumns.ArtistSort] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());

                                        if(track[ATTRIBUTE_MB_ALBUM_ID])
                                                (*iter)[PlaylistColumns.AlbumSort] = get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get());

                                        if(track[ATTRIBUTE_RATING])
                                                (*iter)[PlaylistColumns.Rating] = get<gint64>(track[ATTRIBUTE_RATING].get());

                                        if(track[ATTRIBUTE_COUNT])
                                                (*iter)[PlaylistColumns.Playcount] = get<gint64>(track[ATTRIBUTE_COUNT].get());

                                        gint64 id = 0;

                                        if(track[ATTRIBUTE_MPX_TRACK_ID])
                                        {
                                                id = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()); 
                                                (*iter)[PlaylistColumns.RowId] = id; 
                                                if(!m_CurrentIter && m_CurrentId && get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) == m_CurrentId.get())
                                                {
                                                        m_CurrentIter = iter; 
                                                        queue_draw();
                                                }
                                        }

                                        (*iter)[PlaylistColumns.MPXTrack] = track; 
                                        (*iter)[PlaylistColumns.IsMPXTrack] = track[ATTRIBUTE_MPX_TRACK_ID] ? true : false; 
                                        (*iter)[PlaylistColumns.IsBad] = false; 

                                        m_IdIterMap[id].insert( iter );
                                }

                        void
                                append_album (gint64 id)
                                {
                                        SQL::RowV v;
                                        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = '%lld' ORDER BY track") % id).str()); 
                                        TreeIter iter = ListStore->append();
                                        for(SQL::RowV::const_iterator i = v.begin(); i != v.end(); ++i)
                                        {
                                                SQL::Row r = *i;
                                                if(i != v.begin())
                                                        iter = ListStore->insert_after(iter);
                                                place_track(r, iter); 
                                        }
                                }

                        void          
                                append_tracks (IdV const& tid_v, Order order = NO_ORDER)
                                {
                                        if(tid_v.empty())
                                                return;

                                        std::stringstream numbers;
                                        IdV::const_iterator end = tid_v.end();
                                        end--;

                                        for(IdV::const_iterator i = tid_v.begin(); i != tid_v.end(); ++i)
                                        {
                                                numbers << *i;
                                                if(i != end)
                                                        numbers << ",";
                                        }

                                        SQL::RowV v;
                                        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id IN (%s) %s") % numbers.str() % ((order == ORDER) ? "ORDER BY track_view.track" : "")).str()); 

                                        TreeIter iter = ListStore->append();

                                        if(!m_CurrentIter)
                                                m_PlayInitIter = iter;

                                        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                        {
                                                SQL::Row & r = *i;

                                                if(i != v.begin())
                                                        iter = ListStore->insert_after(iter);

                                                place_track(r, iter);
                                        }
                                } 

                        void
                                append_uris (const Util::FileList& uris, Gtk::TreeIter & iter, bool & begin)
                                {
                                        Util::FileList dirs;

                                        for(Util::FileList::const_iterator i = uris.begin(); i != uris.end(); ++i)
                                        {
                                                if(file_test(filename_from_uri(*i), FILE_TEST_IS_DIR))
                                                {
                                                        dirs.push_back(*i);
                                                        continue;
                                                }

                                                // Check for playlist types
                                                PlaylistParser::Base * pp = 0;

                                                if(Util::str_has_suffix_nocase(*i, "xspf"))
                                                {
                                                        pp = new PlaylistParser::XSPF;
                                                }
                                                else
                                                        if(Util::str_has_suffix_nocase(*i, "pls"))
                                                        {
                                                                pp = new PlaylistParser::PLS;
                                                        }

                                                if( pp )
                                                {
                                                        Track_v v;

                                                        pp->read(*i, v); 

                                                        for(Track_v::const_iterator i = v.begin(); i != v.end(); ++i)
                                                        {
                                                                MPX::Track t = *i;
                                                                if(!begin)
                                                                        iter = ListStore->insert_after(iter);
                                                                place_track(t, iter); 
                                                        }

                                                        delete pp;
                                                }

                                                Track track;
                                                services->get<Library>("mpx-service-library")->getMetadata(*i, track);

                                                if(!begin)
                                                {
                                                        iter = ListStore->insert_after(iter);
                                                }

                                                begin = false;

                                                place_track(track, iter);
                                        }

                                        if(!dirs.empty())
                                        {
                                                for(Util::FileList::const_iterator i = dirs.begin(); i != dirs.end(); ++i)
                                                {
                                                        Util::FileList files;
                                                        Util::collect_audio_paths(*i, files);
                                                        append_uris(files,iter,begin);
                                                }
                                        }
                                }

                        virtual void
                                on_drag_data_received (const Glib::RefPtr<Gdk::DragContext>&, int x, int y,
                                                const SelectionData& data, guint, guint)
                                {
                                        ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                                        TreePath path;
                                        TreeViewDropPosition pos;
                                        TreeIter iter;

                                        if( !get_dest_row_at_pos (x, y, path, pos) )
                                        {
                                                int cell_x, cell_y;
                                                TreeViewColumn * col;
                                                if(get_path_at_pos (x, y, path, col, cell_x, cell_y))
                                                        iter = ListStore->insert (ListStore->get_iter (path));
                                                else
                                                        iter = ListStore->append ();
                                        }
                                        else
                                        {
                                                switch (pos)
                                                {
                                                        case TREE_VIEW_DROP_BEFORE:
                                                        case TREE_VIEW_DROP_INTO_OR_BEFORE:
                                                                iter = ListStore->insert (ListStore->get_iter (path));
                                                                break;

                                                        case TREE_VIEW_DROP_AFTER:
                                                        case TREE_VIEW_DROP_INTO_OR_AFTER:
                                                                iter = ListStore->insert_after (ListStore->get_iter (path));
                                                                break;
                                                }
                                        }

                                        if(data.get_data_type() == "mpx-album")
                                        {
                                                gint64 id = *(reinterpret_cast<const gint64*>(data.get_data()));

                                                SQL::RowV v;
                                                services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % id).str()); 

                                                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                                {
                                                        SQL::Row & r = *i;

                                                        if(i != v.begin())
                                                                iter = ListStore->insert_after(iter);

                                                        place_track(r, iter);
                                                }
                                        }
                                        if(data.get_data_type() == "mpx-track")
                                        {
                                                gint64 id = *(reinterpret_cast<const gint64*>(data.get_data()));

                                                SQL::RowV v;
                                                services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = %lld;") % id).str()); 

                                                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                                {
                                                        SQL::Row & r = *i;

                                                        if(i != v.begin())
                                                                iter = ListStore->insert_after(iter);

                                                        place_track(r, iter);
                                                }
                                        }
                                        if(data.get_data_type() == "mpx-idvec")
                                        {
                                                IdV const& idv = *(reinterpret_cast<const IdV*>(data.get_data()));

                                                for(IdV::const_iterator i = idv.begin(); i != idv.end(); ++i) 
                                                {
                                                        SQL::RowV v;
                                                        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = %lld;") % *i).str()); 
                                                        SQL::Row & r = v[0]; 

                                                        if(i != idv.begin())
                                                                iter = ListStore->insert_after(iter);

                                                        place_track(r, iter);
                                                }
                                        }

                                        else
                                        {
                                                bool begin = true;
                                                Util::FileList uris = data.get_uris();
                                                append_uris (uris, iter, begin);
                                        }

                                        m_MLib.check_nextprev_caps ();
                                        m_MLib.send_caps ();
                                }

                        virtual bool
                                on_button_press_event (GdkEventButton* event)
                                {
                                        int cell_x, cell_y ;
                                        TreeViewColumn *col ;
                                        TreePath path;

                                        if(get_path_at_pos (event->x, event->y, path, col, cell_x, cell_y))
                                        {
                                                if(col == get_column(6))
                                                {
                                                        int rating = (cell_x + 7) / 12;
                                                        g_return_val_if_fail(((rating >= 0) && (rating <= 5)), false);
                                                        TreeIter iter = ListStore->get_iter(path);
                                                        (*iter)[PlaylistColumns.Rating] = rating;   
                                                        services->get<Library>("mpx-service-library")->trackRated(gint64((*iter)[PlaylistColumns.RowId]), rating);
                                                }
                                        }
                                        TreeView::on_button_press_event(event);
                                        return false;
                                }

                        virtual bool
                                on_drag_motion (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
                                {
                                        TreeModel::Path path;
                                        TreeViewDropPosition pos;

                                        if( get_dest_row_at_pos (x, y, path, pos) )
                                        {
                                                switch (pos)
                                                {
                                                        case TREE_VIEW_DROP_INTO_OR_BEFORE:
                                                        case TREE_VIEW_DROP_BEFORE:
                                                                set_drag_dest_row (path, TREE_VIEW_DROP_BEFORE);
                                                                break;

                                                        case TREE_VIEW_DROP_INTO_OR_AFTER:
                                                        case TREE_VIEW_DROP_AFTER:
                                                                set_drag_dest_row (path, TREE_VIEW_DROP_AFTER);
                                                                break;
                                                }
                                        }

                                        return true;
                                }

                        virtual bool
                                on_event (GdkEvent * ev)
                                {
                                        if ((ev->type == GDK_BUTTON_RELEASE) ||
                                                        (ev->type == GDK_2BUTTON_PRESS) ||
                                                        (ev->type == GDK_3BUTTON_PRESS))
                                        {
                                                if((ev->type == GDK_BUTTON_RELEASE) && m_CurrentIter)
                                                {
                                                        check_for_end ();
                                                }
                                                m_ButtonDepressed = false;
                                                return false;
                                        }
                                        else
                                                if( ev->type == GDK_BUTTON_PRESS )
                                                {
                                                        GdkEventButton * event = reinterpret_cast <GdkEventButton *> (ev);

                                                        if( event->button == 1 && (event->window == get_bin_window()->gobj()))
                                                        {
                                                                TreeViewColumn      * column;
                                                                int                   cell_x,
                                                                                      cell_y;
                                                                int                   tree_x,
                                                                                      tree_y;
                                                                TreePath              path;

                                                                tree_x = event->x; tree_y = event->y;
                                                                m_ButtonDepressed  = get_path_at_pos (tree_x, tree_y, path, column, cell_x, cell_y) ;
                                                                if(m_ButtonDepressed)
                                                                        m_PathButtonPress = path;  
                                                        }
                                                        else
                                                                if( event->button == 3 )
                                                                {
                                                                        m_ActionGroup->get_action (ACTION_CLEAR)->set_sensitive
                                                                                (ListStore->children().size());

                                                                        m_ActionGroup->get_action (ACTION_REMOVE_ITEMS)->set_sensitive
                                                                                (get_selection()->count_selected_rows());

                                                                        m_ActionGroup->get_action (ACTION_REMOVE_REMAINING)->set_sensitive
                                                                                (m_CurrentIter);

                                                                        m_ActionGroup->get_action (ACTION_REMOVE_PRECEDING)->set_sensitive
                                                                                (m_CurrentIter);

                                                                        m_ActionGroup->get_action (ACTION_PLAY)->set_sensitive
                                                                                (get_selection()->count_selected_rows());

                                                                        Gtk::Menu * menu = dynamic_cast < Gtk::Menu* > (Util::get_popup (m_UIManager, "/musiclib-playlist-popup-playlist-list/musiclib-playlist-menu-playlist-list"));
                                                                        if (menu) // better safe than screwed
                                                                        {
                                                                                menu->popup (event->button, event->time);
                                                                        }
                                                                        return true;
                                                                }
                                                }
                                        return false;
                                }

                        virtual bool
                                on_motion_notify_event (GdkEventMotion * event)
                                {
                                        TreeView::on_motion_notify_event (event);

                                        if( m_ButtonDepressed && event->window == get_bin_window()->gobj())
                                        {
                                                TreeViewColumn    * column;
                                                TreeModel::Path     path;

                                                int                 cell_x,
                                                                    cell_y;

                                                int                 tree_x,
                                                                    tree_y;

                                                tree_x = event->x;
                                                tree_y = event->y;
                                                if( get_path_at_pos (tree_x, tree_y, path, column, cell_x, cell_y) )
                                                {
                                                        if( path != m_PathButtonPress )
                                                        {
                                                                if( path < m_PathButtonPress )
                                                                        move_selected_rows_up ();
                                                                else
                                                                        if( path > m_PathButtonPress )
                                                                                move_selected_rows_down ();

                                                                m_PathButtonPress = path;
                                                        }
                                                }
                                        }
                                        return false;
                                }

                        void
                                move_selected_rows_up ()
                                {
                                        PathV p = get_selection()->get_selected_rows();

                                        if( p.empty() )
                                                return;

                                        if( p[0].get_indices().data()[0] < 1 )
                                                return; // we don't move rows if the uppermost would be pushed out of the list

                                        ReferenceV r;
                                        std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, r));
                                        ReferenceVIter i = r.begin();

                                        ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                                        for ( ; !r.empty() ; )
                                        {
                                                TreeModel::Path path_a (i->get_path());
                                                TreeModel::Path path_b (i->get_path());
                                                path_a.prev();
                                                path_b.next();
                                                TreeModel::iterator iter_a (ListStore->get_iter (path_a));
                                                TreeModel::iterator iter_b (ListStore->get_iter (path_b));

                                                if( G_UNLIKELY( TreeNodeChildren::size_type( path_b.get_indices().data()[0] ) == ListStore->children().size()) )
                                                {
                                                        ListStore->iter_swap (iter_a, ListStore->get_iter (i->get_path()));
                                                }
                                                else
                                                {
                                                        ListStore->move (iter_a, iter_b);
                                                }

                                                i = r.erase (i);
                                        }
                                }

                        void
                                move_selected_rows_down ()
                                {
                                        PathV p = get_selection()->get_selected_rows();

                                        if( p.empty() )
                                                return;

                                        if( TreeNodeChildren::size_type( p[p.size()-1].get_indices().data()[0] + 1 ) == (ListStore->children().size()) )
                                                return; // we don't move rows if the LOWER-most would be pushed out of the list, either

                                        ReferenceV r;
                                        std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, r));
                                        ReferenceVIter i = r.begin();

                                        ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                                        for ( ; !r.empty() ; )
                                        {
                                                TreeModel::Path path_a (i->get_path());
                                                TreeModel::Path path_b (i->get_path());
                                                path_b.next();
                                                TreeModel::iterator iter_a (ListStore->get_iter (path_a));
                                                TreeModel::iterator iter_b (ListStore->get_iter (path_b));

                                                ListStore->move (iter_b, iter_a);
                                                i = r.erase (i);
                                        }
                                }

                        void
                                on_drag_leave (RefPtr<Gdk::DragContext> const& context, guint time)
                                {
                                        gtk_tree_view_set_drag_dest_row (gobj(), NULL, GTK_TREE_VIEW_DROP_BEFORE);
                                }

                        bool
                                on_drag_drop (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
                                {
                                        ustring target (drag_dest_find_target (context));

                                        if( !target.empty() )
                                        {
                                                drag_get_data (context, target, time);
                                                context->drag_finish  (true, false, time);
                                                return true;
                                        }
                                        else
                                        {
                                                context->drag_finish  (false, false, time);
                                                return false;
                                        }
                                }

                        void
                                cellDataFuncTime (CellRenderer * basecell, TreeModel::iterator const &iter)
                                {
                                        CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                                        guint64 Length = (*iter)[PlaylistColumns.Length]; 
                                        g_object_set(G_OBJECT(cell_t->gobj()), "xalign", 1.0f, NULL);
                                        cell_t->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
                                }

                        void
                                cellDataFuncIcon (CellRenderer * basecell, TreeModel::iterator const &iter)
                                {
                                        CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);

                                        if( (*iter)[PlaylistColumns.IsBad] )
                                        {
                                                cell_p->property_pixbuf() = m_Bad; 
                                                return;
                                        }

                                        if( m_CurrentIter && m_CurrentIter.get() == iter )
                                                cell_p->property_pixbuf() = m_Playing;
                                        else
                                                cell_p->property_pixbuf() = Glib::RefPtr<Gdk::Pixbuf>(0);
                                }

                        void
                                cellDataFuncRating (CellRenderer * basecell, TreeModel::iterator const &iter)
                                {
                                        CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);
                                        if(!(*iter)[PlaylistColumns.IsMPXTrack])
                                        {
                                                cell_p->property_sensitive() = false; 
                                                cell_p->property_pixbuf() = Glib::RefPtr<Gdk::Pixbuf>(0);
                                        }
                                        else
                                        {
                                                cell_p->property_sensitive() = true; 
                                                gint64 i = ((*iter)[PlaylistColumns.Rating]);
                                                g_return_if_fail((i >= 0) && (i <= 5));
                                                cell_p->property_pixbuf() = m_Stars[i];
                                        }
                                }

                        void
                                cellDataFuncCustom (CellRenderer * basecell, TreeModel::iterator const &iter, int attribute)
                                {
                                        CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                                        MPX::Track track = (*iter)[PlaylistColumns.MPXTrack]; 

                                        if(track.has(attribute))
                                        {
                                                cell_t->property_text() = ovariant_get_string(track[attribute]);
                                        }
                                        else
                                        {
                                                cell_t->property_text() = ""; 
                                        }
                                }

                        void
                                cellDataFuncCurrentRow (CellRenderer * basecell, TreeModel::iterator const &iter)
                                {
                                        CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                                        if( m_CurrentIter && m_CurrentIter.get() == iter )
                                                cell_t->property_weight() = Pango::WEIGHT_BOLD;
                                        else
                                                cell_t->property_weight() = Pango::WEIGHT_NORMAL;
                                }


                        int
                                slotSortDefault(const TreeIter& iter_a, const TreeIter& iter_b)
                                {
                                        return 0;
                                }

                        int
                                slotSortById(const TreeIter& iter_a, const TreeIter& iter_b)
                                {
                                        guint64 id_a = (*iter_a)[PlaylistColumns.RowId];
                                        guint64 id_b = (*iter_b)[PlaylistColumns.RowId];

                                        return (id_a - id_b); // FIXME: int overflow
                                }

                        int
                                slotSortByTrack(const TreeIter& iter_a, const TreeIter& iter_b)
                                {
                                        ustring alb_a = (*iter_a)[PlaylistColumns.AlbumSort];
                                        ustring alb_b = (*iter_b)[PlaylistColumns.AlbumSort];
                                        guint64 trk_a = (*iter_a)[PlaylistColumns.Track];
                                        guint64 trk_b = (*iter_b)[PlaylistColumns.Track];

                                        if(alb_a != alb_b)
                                                return 0;

                                        return (trk_a - trk_b); // FIXME: int overflow
                                }

                        int
                                slotSortByAlbum(const TreeIter& iter_a, const TreeIter& iter_b)
                                {
                                        ustring arts_a = (*iter_a)[PlaylistColumns.ArtistSort];
                                        ustring arts_b = (*iter_b)[PlaylistColumns.ArtistSort];
                                        ustring alb_a = (*iter_a)[PlaylistColumns.Album];
                                        ustring alb_b = (*iter_b)[PlaylistColumns.Album];

                                        return alb_a.compare(alb_b); 
                                }

                        int
                                slotSortByArtist(const TreeIter& iter_a, const TreeIter& iter_b)
                                {
                                        ustring art_a = (*iter_a)[PlaylistColumns.Artist];
                                        ustring art_b = (*iter_b)[PlaylistColumns.Artist];

                                        return art_a.compare(art_b); 
                                }

                        void
                                prepare_uris (Util::FileList const& uris, bool init_iter)
                                {
                                        bool begin = true;

                                        TreeIter iter = ListStore->append();

                                        if(init_iter)
                                        {
                                                m_PlayInitIter = iter;
                                        }

                                        append_uris (uris, iter, begin);
                                        m_MLib.check_nextprev_caps();
                                        m_MLib.send_caps ();
                                }

                        bool
                                check_current_exists ()
                                {
                                        g_return_val_if_fail(m_CurrentIter,false);

                                        MPX::Track t ((*m_CurrentIter.get())[PlaylistColumns.MPXTrack]);
                                        std::string location;

                                        try{
                                            location = services->get<Library>("mpx-service-library")->trackGetLocation( t );
                                        } catch( Library::FileQualificationError & cxe )
                                        {
                                            g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
                                        }

                                        if( !location.empty() ) 
                                        {
                                                try{
                                                        Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri( location );
                                                        bool exists = file->query_exists();
                                                        (*m_CurrentIter.get())[PlaylistColumns.IsBad] = !exists; 
                                                        return exists;
                                                } catch( Glib::Error & cxe ) {
                                                        g_warning("%s: %s", G_STRFUNC, cxe.what().c_str());
                                                }

                                        }

                                        return false;
                                }
                };

                struct AllTracksColumnsT : public Gtk::TreeModel::ColumnRecord 
                {
                        Gtk::TreeModelColumn<Glib::ustring> Artist;
                        Gtk::TreeModelColumn<Glib::ustring> Album;
                        Gtk::TreeModelColumn<guint64>       Track;
                        Gtk::TreeModelColumn<Glib::ustring> Name;
                        Gtk::TreeModelColumn<guint64>       Length;

                        // These hidden columns are used for sorting
                        // They don't contain sortnames, as one might 
                        // think from their name, but instead the MB
                        // IDs (if not available, then just the plain name)
                        // They are used only for COMPARISON FOR EQUALITY.
                        // Obviously, comparing them with compare() is
                        // useless if they're MB IDs

                        Gtk::TreeModelColumn<Glib::ustring> ArtistSort;
                        Gtk::TreeModelColumn<Glib::ustring> AlbumSort;
                        Gtk::TreeModelColumn<gint64>        RowId;
                        Gtk::TreeModelColumn<std::string>   Location;
                        Gtk::TreeModelColumn< ::MPX::Track> MPXTrack;
                        Gtk::TreeModelColumn<gint64>        Rating;
                        Gtk::TreeModelColumn<bool>          IsMPXTrack;
                        Gtk::TreeModelColumn<bool>          IsBad;

                        AllTracksColumnsT ()
                        {
                                add (Artist);
                                add (Album);
                                add (Track);
                                add (Name);
                                add (Length);
                                add (ArtistSort);
                                add (AlbumSort);
                                add (RowId);
                                add (Location);
                                add (MPXTrack);
                                add (Rating);
                                add (IsMPXTrack);
                                add (IsBad);
                        };
                };

                class AllTracksView
                {
                        MPX::Source::PlaybackSourceMusicLib & m_MLib;
                        ListView                            * m_ListView;
                        DataModelFilterP                      m_FilterModel;
                        RoundedLayout*                        m_LabelShowing;

                        public:

                        AllTracksView(
                                        Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                                        MPX::Source::PlaybackSourceMusicLib  & mlib
                                     )
                        : m_MLib(mlib)
                        {
                                Gtk::ScrolledWindow     * scrollwin = dynamic_cast<Gtk::ScrolledWindow*>(xml->get_widget("musiclib-alltracks-sw")); 
                                Gtk::Entry              * entry     = dynamic_cast<Gtk::Entry*>(xml->get_widget("musiclib-alltracks-filter-entry")); 
                                m_ListView                          = new ListView;
                                m_LabelShowing                      = new RoundedLayout(xml, "da-showing-alltracks");

                                DataModelP m (new DataModel);

                                SQL::RowV v;
                                services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY album_artist, album, track_view.track")).str()); 
                                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                {
                                        SQL::Row & r = *i;
                                        m->append_track(r, (*(services->get<Library>("mpx-service-library")->sqlToTrack(r).get())));
                                }

                                m_FilterModel = DataModelFilterP (new DataModelFilter(m));

                                entry->signal_changed().connect(
                                                sigc::bind(
                                                        sigc::mem_fun(
                                                                *this,
                                                                &AllTracksView::on_entry_changed
                                                                ),
                                                        m_FilterModel,
                                                        entry
                                                        ));

                                ColumnP c1 (new Column(_("Name")));
                                c1->set_column(0);

                                ColumnP c2 (new Column(_("Artist")));
                                c2->set_column(1);

                                ColumnP c3 (new Column(_("Album")));
                                c3->set_column(2);

                                m_ListView->append_column(c1);
                                m_ListView->append_column(c2);
                                m_ListView->append_column(c3);

                                m_ListView->set_model(m_FilterModel);

                                scrollwin->add(*m_ListView);
                                m_ListView->show();
                                scrollwin->show_all();

                                m_ListView->signal_track_activated().connect(
                                                sigc::mem_fun(
                                                        *this,
                                                        &AllTracksView::on_track_activated
                                                        ));

                                Gtk::CheckButton * cb;
                                xml->get_widget("alltracks-highlight", cb);

                                cb->signal_toggled().connect(

                                                sigc::bind(

                                                        sigc::mem_fun(
                                                                *this,
                                                                &AllTracksView::on_highlight_toggled
                                                                ),

                                                        cb
                                                        ));

                                services->get<Library>("mpx-service-library")->signal_new_track().connect( 
                                                sigc::mem_fun(
                                                        *this,
                                                        &AllTracksView::on_new_track
                                                        ));


                                services->get<Library>("mpx-service-library")->signal_track_deleted().connect( 
                                                sigc::mem_fun(
                                                        *this,
                                                        &AllTracksView::on_track_deleted
                                                        ));
                        }

                        void
                                set_advanced (bool advanced)
                                {
                                        m_ListView->set_advanced( advanced );
                                }

                        void
                                on_highlight_toggled( Gtk::CheckButton * cb )
                                {
                                        m_ListView->set_highlight( cb->get_active() );
                                }   

                        void
                                on_new_track(Track & track, gint64 album_id, gint64 artist_id)
                                {
                                        m_FilterModel->append_track( track );
                                }

                        void
                                on_track_deleted(gint64 id)
                                {
                                        m_FilterModel->erase_track( id );
                                }

                        void
                                on_track_activated (gint64 id, bool play)
                                {
                                        IdV v (1, id);
                                        m_MLib.play_tracks(v, play);
                                }

                        void
                                on_entry_changed (DataModelFilterP model, Gtk::Entry* entry)
                                {
                                        model->set_filter(entry->get_text());
                                        m_LabelShowing->set_text ((boost::format (_("%lld of %lld")) % model->m_mapping.size() % model->m_realmodel->size()).str());
                                }
                };

                PlaylistTreeView        *   m_TreeViewPlaylist;
                AlbumTreeView           *   m_TreeViewAlbums;
                CollectionTreeView      *   m_TreeViewCollections;
                AllTracksView           *   m_ViewAllTracks;
                FileSystemTree          *   m_TreeViewFS;
                View                    *   m_View;
                ViewModel               *   m_ViewModel;

                MusicLibPrivate(
                    MPX::Player&                            player,
                    MPX::Source::PlaybackSourceMusicLib&    mlib,
                    const Glib::RefPtr<Gnome::Glade::Xml>&  xml,
                    const Glib::RefPtr<Gtk::UIManager>&     ui_manager
                )
                {
                        m_RefXml = xml;

                        m_UI = m_RefXml->get_widget("source-musiclib");

                        m_TreeViewAlbums = new AlbumTreeView(m_RefXml, "source-musiclib-treeview-albums", "albums-showing", "search-entry", "search-alignment", "search-plugin-cbox", ui_manager);
                        m_TreeViewCollections = new CollectionTreeView(m_RefXml, "source-musiclib-treeview-collections", "collections-showing", "collections-filter-entry", ui_manager);

                        m_ViewAllTracks = new AllTracksView(m_RefXml, mlib);
                        m_TreeViewPlaylist = new PlaylistTreeView(m_RefXml, ui_manager, mlib);

                        //m_TreeViewFS = new FileSystemTree(m_RefXml, "musiclib-treeview-file-system");
                        /*
                        m_TreeViewFS->build_file_system_tree("/");
                        m_TreeViewFS->signal_uri().connect(
                                        sigc::mem_fun(
                                                mlib,
                                                &::MPX::Source::PlaybackSourceMusicLib::play_uri
                        ));
                        */

                        /*
                        m_View = new View;
                        m_ViewModel = new ViewModel;
                        m_ViewModel->set_metrics( 90, 24 );

                        Gtk::ScrolledWindow * win;
                        m_RefXml->get_widget("albums2-sw", win);
                        win->add(*m_View);

                        SQL::RowV v;
                        services->get<Library>("mpx-service-library")->getSQL(v, "SELECT * FROM album ORDER BY id;");

                        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                        {
                            SQL::Row & r = *i;

                            Row_p r1 (new Row_t);
                            r1->AlbumData = r;
                            m_Covers.get().fetch(get<std::string>(r["mb_album_id"]), r1->Cover, COVER_SIZE_ALBUM);
                            if( r1->Cover )
                            {
                                r1->Cover = Util::cairo_image_surface_round(r1->Cover, 6.);
                                Gdk::Color c = m_View->get_style()->get_black();
                                Util::cairo_image_surface_rounded_border(r1->Cover, .5, 6., c.get_red_p(), c.get_green_p(), c.get_blue_p(), 1.);
                            }

                            SQL::RowV v2;
                            services->get<Library>("mpx-service-library")->getSQL(v2, (boost::format ("SELECT * FROM track_view WHERE album_j = '%lld'") % get<gint64>(r["id"])).str());

                            for(SQL::RowV::iterator n = v2.begin(); n != v2.end(); ++n)
                            {
                                r1->ChildData.push_back(services->get<Library>("mpx-service-library")->sqlToTrack(*n));
                            }

                            m_ViewModel->append_row( r1 );

                            g_message("%s: Appended album %lld out of %lld", G_STRFUNC, gint64(std::distance(v.begin(), i)), gint64(v.size()));
                        }

                        m_View->set_model( m_ViewModel );
                        m_View->show_all();
                        */

                        m_TreeViewAlbums->signal_play_tracks().connect(
                                                sigc::mem_fun(
                                                        mlib,
                                                        &::MPX::Source::PlaybackSourceMusicLib::play_tracks
                        ));

                        m_TreeViewAlbums->signal_play_album().connect(
                                                sigc::mem_fun(
                                                        mlib,
                                                        &::MPX::Source::PlaybackSourceMusicLib::play_album
                        ));

                        Gtk::CheckButton * cb;
                        m_RefXml->get_widget("tracks-advanced-cb", cb);
                        cb->signal_toggled().connect(
                            sigc::bind(
                                sigc::mem_fun(
                                    *this,
                                    &MusicLibPrivate::on_tracks_advanced_cb_toggled
                                ),
                                cb
                        ));
                }

                void
                on_tracks_advanced_cb_toggled (Gtk::CheckButton const* cb)
                {
                    m_ViewAllTracks->set_advanced(cb->get_active());
                }
        };
}

namespace MPX
{
        namespace Source
        {
                bool PlaybackSourceMusicLib::m_signals_installed = false;

                PlaybackSourceMusicLib::PlaybackSourceMusicLib (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
                : PlaybackSource(ui_manager, _("Music Library"), C_CAN_SEEK)
                , m_MainUIManager(ui_manager)
                , m_Skipped(false)
                {
                        add_cap(C_CAN_SEEK);

                        mpx_musiclib_py_init();

                        if(!m_signals_installed)
                        {
                                signals[PSM_SIGNAL_PLAYLIST_TOOLTIP] =
                                        g_signal_new ("playlist-tooltip",
                                                        G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                        GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                                                        0,
                                                        NULL, NULL,
                                                        psm_glib_marshal_OBJECT__OBJECT_BOXED, G_TYPE_OBJECT, 2, G_TYPE_OBJECT, GTK_TYPE_TREE_ITER);

                                signals[PSM_SIGNAL_PLAYLIST_END] = 
                                        g_signal_new ("playlist-end",
                                                        G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                        GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                                                        0,
                                                        NULL, NULL,
                                                        g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

                                m_signals_installed = true;
                        }

                        const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
                        m_RefXml = Gnome::Glade::Xml::create (path);

                        m_Private = new MusicLibPrivate(player,*this,m_RefXml,ui_manager);
                        m_Private->m_TreeViewPlaylist->signal_row_activated().connect( sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_plist_row_activated ) );
                        m_Private->m_TreeViewPlaylist->signal_query_tooltip().connect( sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_plist_query_tooltip ) );


                        gtk_widget_realize(GTK_WIDGET((dynamic_cast<Gtk::Widget*>(m_Private->m_UI))->gobj()));
                        gtk_widget_show(GTK_WIDGET((dynamic_cast<Gtk::Widget*>(m_Private->m_UI))->gobj()));

                        gtk_widget_realize(GTK_WIDGET(m_Private->m_UI->gobj()));
                        (dynamic_cast<Gtk::VPaned*>(m_Private->m_UI))->set_position(mcs->key_get<int>("PlaybackSourceMusicLib", "divider-position"));

                        m_MainActionGroup = ActionGroup::create("ActionsMusicLib");
                        m_MainActionGroup->add(Action::create("menu-source-musiclib", _("Music _Library")));
                        m_MainActionGroup->add(Action::create("menu-musiclib-albums", _("Release Types...")));
                        m_MainActionGroup->add(Action::create("menu-musiclib-highlight", _("Highlight...")));
                        m_MainActionGroup->add(Action::create("menu-musiclib-sort", _("Sort Albums By...")));

                        Gtk::RadioButtonGroup gr1;

                        m_MainActionGroup->add(
                                        RadioAction::create( gr1, "musiclib-sort-by-name", _("Artist, Album by Year")),
                                        sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_albums_sort_column_change ));

                        m_MainActionGroup->add(
                                        RadioAction::create( gr1, "musiclib-sort-by-date", _("Time of Adding")),
                                        sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_albums_sort_column_change ));

                        m_MainActionGroup->add(
                                        RadioAction::create( gr1, "musiclib-sort-by-rating", _("Rating")),
                                        sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_albums_sort_column_change ));

                        m_MainActionGroup->add(
                                        RadioAction::create( gr1, "musiclib-sort-by-alphabet", _("Artist, Album by Name")),
                                        sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_albums_sort_column_change ));

                        m_MainActionGroup->add(
                                        RadioAction::create( gr1, "musiclib-sort-by-playscore", _("PlayScore")),
                                        sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_albums_sort_column_change ));

                        m_MainActionGroup->add(
                                        ToggleAction::create(     "musiclib-show-only-new", _("Show New Albums Only")),
                                        sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_view_option_show_new_albums ));

                        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-name"))->property_value() = 0;
                        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-date"))->property_value() = 1;
                        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-rating"))->property_value() = 2;
                        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-alphabet"))->property_value() = 3;
                        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-playscore"))->property_value() = 4;


                        Gtk::RadioButtonGroup gr2;

                        m_MainActionGroup->add(

                                        RadioAction::create(
                                                gr2,
                                                "musiclib-show-albums",
                                                _("_Albums")
                                                ),

                                        AccelKey("<alt>1"),

                                        sigc::mem_fun(
                                                *this,
                                                &PlaybackSourceMusicLib::on_view_change
                                                )
                                        );
                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-show-albums"))->property_value() = 0;

                        m_MainActionGroup->add(

                                        RadioAction::create(
                                                gr2,
                                                "musiclib-show-alltracks",
                                                _("_Tracks")
                                                ),

                                        AccelKey("<alt>2"),

                                        sigc::mem_fun(
                                                *this,
                                                &PlaybackSourceMusicLib::on_view_change
                                                )
                                        );
                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-show-alltracks"))->property_value() = 1;

                        m_MainActionGroup->add(

                                        RadioAction::create(
                                                gr2,
                                                "musiclib-show-collections",
                                                _("_Collections")
                                                ),

                                        AccelKey("<alt>3"),

                                        sigc::mem_fun(
                                                *this,
                                                &PlaybackSourceMusicLib::on_view_change
                                                )
                                        );
                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-show-collections"))->property_value() = 2;

                        const ReleaseTypeActionInfo action_infos[] =
                        {
                                {N_("Show All Release Types"), "musiclib-albums-show-all", RT_ALL},
                                {N_("Show Albums"), "musiclib-albums-show-albums", RT_ALBUM},
                                {N_("Show Singles"), "musiclib-albums-show-singles", RT_SINGLE},
                                {N_("Show Compilations"), "musiclib-albums-show-compilations", RT_COMPILATION},
                                {N_("Show EPs"), "musiclib-albums-show-eps", RT_EP},
                                {N_("Show Live Recordings"), "musiclib-albums-show-live", RT_LIVE},
                                {N_("Show Remixes"), "musiclib-albums-show-remix", RT_REMIX},
                                {N_("Show Soundtracks"), "musiclib-albums-show-soundtracks", RT_SOUNDTRACK},
                                {N_("Show Releases of Unknown Type"), "musiclib-albums-show-other", RT_OTHER},
                        };

                        Gtk::RadioButtonGroup gr3;

                        for( unsigned int n = 0; n < G_N_ELEMENTS(action_infos); ++n )
                        {
                                m_MainActionGroup->add(
                                                RadioAction::create(
                                                        gr3,
                                                        action_infos[n].Name,
                                                        _(action_infos[n].Label)
                                                        ),
                                                sigc::mem_fun(
                                                        *this, 
                                                        &PlaybackSourceMusicLib::action_cb_toggle_albums_type_filter
                                                        ));

                                RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action(action_infos[n].Name))->property_value() = action_infos[n].Value; 
                        }

                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-albums-show-all"))->set_current_value(RT_ALL);

                        Gtk::RadioButtonGroup gr4;

                        m_MainActionGroup->add(

                                        RadioAction::create(
                                                gr4,
                                                "musiclib-highlight-equal",
                                                _("Do Not Highlight")
                                                ),

                                        sigc::mem_fun(
                                                *this,
                                                &PlaybackSourceMusicLib::on_view_option_album_highlight
                                                )
                                        );
                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-highlight-equal"))->property_value() = 0;

                        m_MainActionGroup->add(

                                        RadioAction::create(
                                                gr4,
                                                "musiclib-highlight-unplayed",
                                                _("Unplayed")
                                                ),

                                        sigc::mem_fun(
                                                *this,
                                                &PlaybackSourceMusicLib::on_view_option_album_highlight
                                                )
                                        );
                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-highlight-unplayed"))->property_value() = 1;

                        m_MainActionGroup->add(

                                        RadioAction::create(
                                                gr4,
                                                "musiclib-highlight-played",
                                                _("Played")
                                                ),

                                        sigc::mem_fun(
                                                *this,
                                                &PlaybackSourceMusicLib::on_view_option_album_highlight
                                                )
                                        );
                        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-highlight-played"))->property_value() = 2;

                        m_MainUIManager->insert_action_group(m_MainActionGroup);
                }

                PlaybackSourceMusicLib::~PlaybackSourceMusicLib ()
                {
                        g_message("Saving divider position");
                        mcs->key_set<int>("PlaybackSourceMusicLib", "divider-position", int((dynamic_cast<Gtk::VPaned*>(m_Private->m_UI))->get_position()));
                        delete m_Private;
                }

                void
                        PlaybackSourceMusicLib::action_cb_toggle_albums_type_filter ()
                        {
                                int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-albums-show-all"))->get_current_value();
                                m_Private->m_TreeViewAlbums->set_release_type_filter(value);
                        }

                void
                        PlaybackSourceMusicLib::action_cb_play ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.m_CurrentIter = boost::optional<Gtk::TreeIter>(); 
                                playlist.m_CurrentId = boost::optional<gint64>();
                                Signals.PlayRequest.emit();
                        }

                void
                        PlaybackSourceMusicLib::action_cb_go_to_album(gint64 id)
                        {
                                m_Private->m_TreeViewAlbums->go_to_album(id);
                        }

                PyObject*
                        PlaybackSourceMusicLib::get_py_obj ()
                        {
                                using namespace boost::python;
                                object obj(boost::ref(this));
                                Py_INCREF(obj.ptr());
                                return obj.ptr();
                        }

                std::string
                        PlaybackSourceMusicLib::get_guid ()
                        {
                                return "36068e19-dfb3-49cd-85b4-52cea16fe0fd";
                        }

                std::string
                        PlaybackSourceMusicLib::get_class_guid ()
                        {
                                return "ecd01fd1-9b4d-4fef-8e85-6c4b16965ef8";
                        }

                guint
                        PlaybackSourceMusicLib::add_menu ()
                        {
                                guint id = m_MainUIManager->add_ui_from_string(ui_source);
                                return id;
                        }

                void
                        PlaybackSourceMusicLib::on_plist_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.m_CurrentIter = playlist.ListStore->get_iter( path );

                                Signals.PlayRequest.emit();
                        }

                void
                        PlaybackSourceMusicLib::on_albums_sort_column_change ()
                        {
                                int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-sort-by-name"))->get_current_value();
                                m_Private->m_TreeViewAlbums->AlbumsTreeStore->set_sort_column(value, Gtk::SORT_ASCENDING);    
                        }

                void
                        PlaybackSourceMusicLib::on_view_option_show_new_albums ()
                        {
                                bool value = RefPtr<Gtk::ToggleAction>::cast_static (m_MainActionGroup->get_action ("musiclib-show-only-new"))->get_active();
                                m_Private->m_TreeViewAlbums->set_new_albums_state(value);
                        }

                void
                        PlaybackSourceMusicLib::on_view_option_album_highlight ()
                        {
                                int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-highlight-equal"))->get_current_value();
                                m_Private->m_TreeViewAlbums->set_highlight_mode(AlbumHighlightMode(value));
                        }

                void
                        PlaybackSourceMusicLib::on_view_change ()
                        {
                                int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-show-albums"))->get_current_value();
                                dynamic_cast<Gtk::Notebook*>(m_RefXml->get_widget("musiclib-notebook-views"))->set_current_page(value);
                        }

                bool
                        PlaybackSourceMusicLib::on_plist_query_tooltip(int x,int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip> & tooltip)
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                GtkWidget * tooltip_widget = 0;
                                int cell_x, cell_y;
                                TreePath path;
                                TreeViewColumn *col;
                                int x2, y2;
                                playlist.convert_widget_to_bin_window_coords(x, y, x2, y2);
                                if(playlist.get_path_at_pos (x2, y2, path, col, cell_x, cell_y))
                                {
                                        TreeIter iter = playlist.ListStore->get_iter(path);
                                        GtkTreeIter const* iter_c = iter->gobj();
                                        g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_TOOLTIP], 0, G_OBJECT(playlist.ListStore->gobj()), iter_c, &tooltip_widget);
                                        if( tooltip_widget )
                                        {
                                                g_object_ref(G_OBJECT(tooltip_widget));
                                                gtk_widget_show_all(GTK_WIDGET(tooltip_widget));
                                                tooltip->set_custom(*Glib::wrap(tooltip_widget, false)); 
                                                return true;
                                        }
                                }

                                return false;        
                        }

                void
                        PlaybackSourceMusicLib::plist_end(bool cleared)
                        {
                                g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_END], 0, gboolean(cleared));
                        }

                void
                        PlaybackSourceMusicLib::plist_sensitive(bool sensitive)
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.set_sensitive(sensitive);
                        }

                Glib::RefPtr<Gtk::ListStore>
                        PlaybackSourceMusicLib::get_playlist_model ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                return playlist.ListStore;
                        }

                PyObject*
                        PlaybackSourceMusicLib::get_playlist_current_path ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                if(!playlist.m_CurrentIter)
                                {
                                        Py_INCREF(Py_None);
                                        return Py_None;
                                }
        
                                TreePath path (playlist.m_CurrentIter.get());
                                return pygtk_tree_path_to_pyobject(path.gobj());
                        }

                Glib::RefPtr<Gtk::TreeStore>
                        PlaybackSourceMusicLib::get_albums_model ()
                        {
                                AlbumTreeView & albums (*m_Private->m_TreeViewAlbums);

                                return albums.AlbumsTreeStore;
                        }

                PyObject*
                        PlaybackSourceMusicLib::get_albums_selected_path ()
                        {
                                AlbumTreeView & albums (*m_Private->m_TreeViewAlbums);

                                if(! albums.get_selection()->count_selected_rows() )
                                {
                                    Py_INCREF(Py_None);
                                    return Py_None;
                                }

                                TreePath path (albums.AlbumsTreeStoreFilter->convert_path_to_child_path(TreePath(albums.get_selection()->get_selected())));
                                return pygtk_tree_path_to_pyobject(path.gobj());
                        }


                void
                        PlaybackSourceMusicLib::play_album(gint64 id, bool play)
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                if( play )
                                        playlist.clear();

                                playlist.append_album(id);

                                if( play )
                                        Signals.PlayRequest.emit();
                        }

                void
                        PlaybackSourceMusicLib::play_tracks(IdV const& idv, bool play)
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                if( play )
                                        playlist.clear();

                                playlist.append_tracks(idv, NO_ORDER);
                                check_nextprev_caps ();
                                send_caps();

                                if( play )
                                        Signals.PlayRequest.emit();
                        }

                std::string
                        PlaybackSourceMusicLib::get_uri () 
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                g_return_val_if_fail(playlist.m_CurrentIter, std::string());

                                MPX::Track t ((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.MPXTrack]);
                                
                                try{
                                    return services->get<Library>("mpx-service-library")->trackGetLocation( t );
                                } catch( Library::FileQualificationError & cxe ) 
                                {
                                        g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
                                }

                                return std::string();
                        }

                std::string
                        PlaybackSourceMusicLib::get_type ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                g_return_val_if_fail(playlist.m_CurrentIter, std::string());

                                MPX::Track t ((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.MPXTrack]);

                                return t[ATTRIBUTE_TYPE] ? 
                                         get<std::string>(t[ATTRIBUTE_TYPE].get())
                                       : std::string() ;
                        }

                bool
                        PlaybackSourceMusicLib::play ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                std::vector<Gtk::TreePath> rows = playlist.get_selection()->get_selected_rows();

                                bool exists = false;

                                if(playlist.m_PlayInitIter)
                                {
                                        playlist.m_CurrentIter = playlist.m_PlayInitIter.get();
                                        playlist.m_PlayInitIter.reset ();
                                }

                                if /* still */ (!playlist.m_CurrentIter)
                                {
                                        if( !rows.empty() )
                                        {
                                                playlist.m_CurrentIter = playlist.ListStore->get_iter (rows[0]); 
                                                playlist.m_CurrentId = (*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.RowId];
                                                playlist.queue_draw();
                                        }
                                        else
                                        {
                                                if( playlist.ListStore->children().size() )
                                                {
                                                        TreePath path (TreePath::size_type(1), TreePath::value_type(0));
                                                        playlist.m_CurrentIter = playlist.ListStore->get_iter(path);
                                                }
                                        }
                                }

                                if /* finally */ ( playlist.m_CurrentIter )
                                {
                                        exists = playlist.check_current_exists();
                                }

                                if( !exists )
                                {
                                        playlist.m_CurrentIter.reset();
                                        playlist.m_PlayInitIter.reset();
                                        playlist.queue_draw();
                                }

                                check_nextprev_caps ();

                                return exists;
                        }

                bool
                        PlaybackSourceMusicLib::go_next ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                TreeIter &iter = playlist.m_CurrentIter.get();
                                TreePath path1 = playlist.ListStore->get_path(iter);
                                TreePath path2 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

                                if (path1[0] < path2[0]) 
                                {
                                        iter++;
                                        return true;
                                }

                                return false;
                        }

                bool
                        PlaybackSourceMusicLib::go_prev ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                g_return_val_if_fail(bool(playlist.m_CurrentIter), false);

                                TreeIter & iter = playlist.m_CurrentIter.get();

                                TreePath path1 = playlist.ListStore->get_path(iter);
                                TreePath path2 = playlist.ListStore->get_path(playlist.ListStore->children().begin());

                                if (path1[0] > path2[0])
                                {
                                        iter--;
                                        return true;
                                }

                                return false;
                        }

                void
                        PlaybackSourceMusicLib::stop () 
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.m_CurrentIter = boost::optional<Gtk::TreeIter>();
                                playlist.m_CurrentId = boost::optional<gint64>(); 
                                playlist.queue_draw();
                                check_nextprev_caps ();
                        }

                void
                        PlaybackSourceMusicLib::set_play ()
                        {
                                add_cap(C_CAN_PLAY);
                                send_caps ();
                        }

                void
                        PlaybackSourceMusicLib::clear_play ()
                        {
                                rem_cap(C_CAN_PLAY);
                                send_caps ();
                        }

                void
                        PlaybackSourceMusicLib::check_nextprev_caps ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                if (!playlist.m_CurrentIter)
                                {
                                        rem_cap(C_CAN_GO_NEXT);
                                        rem_cap(C_CAN_GO_PREV);
                                        return;
                                }

                                TreeIter & iter = playlist.m_CurrentIter.get();

                                TreePath path1 = playlist.ListStore->get_path(iter);
                                TreePath path2 = playlist.ListStore->get_path(playlist.ListStore->children().begin());
                                TreePath path3 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

                                if(path1[0] > path2[0])
                                        add_cap(C_CAN_GO_PREV);
                                else
                                        rem_cap(C_CAN_GO_PREV);

                                if(path1[0] < path3[0])
                                        add_cap(C_CAN_GO_NEXT);
                                else
                                        rem_cap(C_CAN_GO_NEXT);
                        }

                void
                        PlaybackSourceMusicLib::send_metadata ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                g_return_if_fail(bool(playlist.m_CurrentIter));

                                MPX::Metadata m = (MPX::Track((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.MPXTrack]));
                                Signals.Metadata.emit( m );
                        }

                void
                        PlaybackSourceMusicLib::play_post () 
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.m_CurrentId = (*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.RowId];

                                add_cap(C_CAN_PAUSE);
                                add_cap(C_CAN_PROVIDE_METADATA);
                                check_nextprev_caps ();

                                m_Private->m_TreeViewPlaylist->scroll_to_track();

                                TreeIter &iter = playlist.m_CurrentIter.get();
                                TreePath path1 = playlist.ListStore->get_path(iter);
                                TreePath path2 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

                                if(path1[0] == path2[0])
                                {
                                        plist_end(false);
                                }
                        }

                void
                        PlaybackSourceMusicLib::next_post () 
                        { 
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.m_CurrentId = (*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.RowId];

                                playlist.queue_draw();
                                check_nextprev_caps ();
                                playlist.scroll_to_track();

                                TreeIter &iter = playlist.m_CurrentIter.get();
                                TreePath path1 = playlist.ListStore->get_path(iter);
                                TreePath path2 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

                                if (path1[0] < path2[0]) 
                                {
                                        if((path1[0]+1) == path2[0])
                                        {
                                                g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_END], 0);
                                        }
                                }
                        }

                void
                        PlaybackSourceMusicLib::prev_post ()
                        {
                                MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

                                playlist.m_CurrentId = (*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.RowId];

                                playlist.queue_draw();
                                check_nextprev_caps ();
                                playlist.scroll_to_track();
                        }

                void
                        PlaybackSourceMusicLib::restore_context ()
                        {}

                void
                        PlaybackSourceMusicLib::skipped () 
                        {
                                m_Skipped = true;
                        }

                void
                        PlaybackSourceMusicLib::segment () 
                        {}

                void
                        PlaybackSourceMusicLib::buffering_done () 
                        {}

                Glib::RefPtr<Gdk::Pixbuf>
                        PlaybackSourceMusicLib::get_icon ()
                        {
                                try{
                                        return IconTheme::get_default()->load_icon("source-library", 64, ICON_LOOKUP_NO_SVG);
                                } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
                        }

                Gtk::Widget*
                        PlaybackSourceMusicLib::get_ui ()
                        {
                                return m_Private->m_UI;
                        }

                UriSchemes 
                        PlaybackSourceMusicLib::get_uri_schemes ()
                        {
                                UriSchemes s;
                                s.push_back("file");
                                s.push_back("lastfm-tag");
                                return s;
                        }

                void    
                        PlaybackSourceMusicLib::process_uri_list (Util::FileList const& uris, bool play)
                        {
                                m_Private->m_TreeViewPlaylist->prepare_uris (uris, play);
                                if(play)
                                {
                                        Signals.PlayRequest.emit();
                                }
                        }

                void    
                        PlaybackSourceMusicLib::play_uri (Glib::ustring const& uri)
                        {
                                Util::FileList uris (1, uri);
                                m_Private->m_TreeViewPlaylist->prepare_uris (uris, true);
                                Signals.PlayRequest.emit();
                        }

        } // end namespace Source
} // end namespace MPX 
