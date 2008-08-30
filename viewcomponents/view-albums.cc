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

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "mpx/mpx-library.hh"
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
#include "mpx/widgets/timed-confirmation.hh"

#include "mpx/com/view-albums.hh"
#include "mpx/com/album-info-window.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using namespace MPX::AQE;
using boost::get;
using boost::algorithm::trim;
using boost::algorithm::split;

namespace
{
        const int N_STARS = 6;

        const char ui_albums_popup [] =
                "<ui>"
                ""
                "<menubar name='popup-albumlist-%1%'>"
                ""
                "   <menu action='dummy' name='menu-albumlist-%1%'>"
                "       <menuitem action='action-album-info-%1%'/>"
                "       <placeholder name='action-album-placeholder-%1%'/>"
                "   </menu>"
                ""
                "</menubar>"
                ""
                "</ui>";

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

        enum Order
        {
                NO_ORDER,
                ORDER
        };


        // Release Types

        ReleaseType
                determine_release_type (const std::string& type)
                {
                        if( type == "album" )
                                return RT_ALBUM;

                        if( type == "single" )
                                return RT_SINGLE;

                        if( type == "compilation" )
                                return RT_COMPILATION;

                        if( type == "ep" )
                                return RT_EP;

                        if( type == "live" )
                                return RT_LIVE;

                        if( type == "remix" )
                                return RT_REMIX;

                        if( type == "soundtrack" )
                                return RT_SOUNDTRACK;

                        return RT_OTHER;
                }

        std::string
                get_release_string (ReleaseType rt, bool plural = false)
                {
                            if( rt == RT_ALBUM )
                                return plural ? N_("albums") : N_("Album");

                            if( rt == RT_SINGLE )
                                return plural ? N_("singles") : N_("Single");

                            if( rt == RT_COMPILATION )
                                return plural ? N_("compilations") : N_("Compilation");

                            if( rt == RT_EP )
                                return plural ? N_("EPs") : N_("EP");

                            if( rt == RT_LIVE )
                                return plural ? N_("live recordings") : N_("Live Recording");

                            if( rt == RT_REMIX )
                                return plural ? N_("remixes") : N_("Remix");

                            if( rt == RT_SOUNDTRACK )
                                return plural ? N_("soundtracks") : N_("Soundtrack");

                        return "";
                }

}

namespace MPX
{
                AlbumTreeView::AlbumTreeView(
                        const Glib::RefPtr<Gnome::Glade::Xml>&  xml,    
                        const std::string&                      name,
                        const std::string&                      name_showing_label,
                        const std::string&                      name_filter_entry,
                        const std::string&                      name_advanced_cb,
                        Glib::RefPtr<Gtk::UIManager>            ui_manager,
                        const PAccess<MPX::Library>&            lib,
                        const PAccess<MPX::Covers>&             amzn
                )
                : WidgetLoader<Gtk::TreeView>(xml,name)
                , m_Name(name)
                , m_Lib(lib)
                , m_Covers(amzn)
                , m_ButtonPressed(false)
                {
                        Options.Flags = ALBUMS_STATE_NO_FLAGS;
                        Options.Type = RT_ALL;
                        Options.Advanced = false;

                        const std::string image_base_path = build_filename( DATA_DIR, "images" );

                        for(int n = 0; n < N_STARS; ++n)
                        {
                                m_Stars[n] = Gdk::Pixbuf::create_from_file(
                                                build_filename(
                                                        image_base_path,
                                                        (boost::format("stars%d.png") % n).str()
                                ));
                        }

                        m_Emblem[EM_COMPILATION] = Gdk::Pixbuf::create_from_file(
                                        build_filename(
                                               image_base_path,
                                               "emblem-compilation.png" 
                        ));

                        m_Emblem[EM_SOUNDTRACK] = Gdk::Pixbuf::create_from_file(
                                        build_filename(
                                               image_base_path,
                                               "emblem-soundtrack.png" 
                        ));

                        m_Lib.get().signal_new_album().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::on_new_album
                        ));

                        m_Lib.get().signal_new_track().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::on_new_track
                        ));

                        m_Lib.get().signal_album_deleted().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::on_album_deleted
                        ));

                        m_Lib.get().signal_track_deleted().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::on_track_deleted
                        ));

                        m_Lib.get().signal_album_updated().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::on_album_updated
                        ));

                        m_Lib.get().signal_reload().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::album_list_load
                        ));

                        m_Covers.get().signal_got_cover().connect(
                            sigc::mem_fun(
                                *this,
                                &AlbumTreeView::on_got_cover
                        ));

                        if( !name_showing_label.empty() )
                            m_LabelShowing = new RoundedLayout(xml, name_showing_label);
                        else
                            m_LabelShowing = 0;

                        if( !name_advanced_cb.empty() )
                        {
                            m_Xml->get_widget( name_advanced_cb, m_AdvancedQueryCB ); 
                            m_AdvancedQueryCB->signal_toggled().connect(
                                sigc::mem_fun(
                                    *this,
                                    &AlbumTreeView::on_advanced_query_cb_toggled
                            ));
                        }

                        set_show_expanders( false );
                        set_level_indentation( 56 );

                        TreeViewColumn * col = manage (new TreeViewColumn());
                        GtkCellRenderer * renderer = gossip_cell_renderer_expander_new ();
                        gtk_tree_view_column_pack_start (col->gobj(), renderer, FALSE);
                        gtk_tree_view_column_set_cell_data_func (col->gobj(),
                                        renderer,
                                        GtkTreeCellDataFunc(rb_sourcelist_expander_cell_data_func),
                                        this,
                                        NULL);

                        CellRendererCairoSurface * cellcairo = manage (new CellRendererCairoSurface);
                        col->pack_start(*cellcairo, false);
                        col->set_cell_data_func(*cellcairo, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncCover ));
                        cellcairo->property_xpad() = 4;
                        cellcairo->property_ypad() = 4;
                        cellcairo->property_yalign() = 0.;
                        cellcairo->property_xalign() = 0.;

                        CellRendererVBox *cvbox = manage (new CellRendererVBox);
                        CellRendererText *celltext = manage (new CellRendererText);
                        celltext->property_yalign() = 0.;
                        celltext->property_ypad() = 4;
                        celltext->property_height() = 72;
                        celltext->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
                        cvbox->property_renderer1() = celltext;

                        CellRendererPixbuf *cellpixbuf = manage (new CellRendererPixbuf);
                        cellpixbuf->property_xalign() = 0.;
                        cellpixbuf->property_ypad() = 2;
                        cellpixbuf->property_xpad() = 2;
                        cvbox->property_renderer2() = cellpixbuf;

                        col->pack_start(*cvbox, true);
                        col->set_cell_data_func(
                                        *cvbox,
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::cellDataFuncText1
                                                ));

                        CellRendererCount *cellcount = manage (new CellRendererCount);
                        cellcount->property_box() = BOX_NORMAL;
                        col->pack_start(*cellcount, false);

                        col->set_cell_data_func(
                                        *cellcount,
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::cellDataFuncText2
                                                ));

                        celltext = manage (new CellRendererText);
                        col->pack_start(*celltext, false);

                        col->set_cell_data_func(
                                        *celltext,
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::cellDataFuncText3
                                                ));

                        celltext = manage (new CellRendererText);
                        col->pack_start(*celltext, false);

                        col->set_cell_data_func(
                                        *celltext,
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::cellDataFuncText4
                                                ));

                        celltext = manage (new CellRendererText);
                        col->pack_start(*celltext, false);
                        celltext->property_xalign() = 0.;
                        celltext->property_xpad() = 2;

                        col->set_cell_data_func(
                                        *celltext,
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::cellDataFuncText5
                                                ));

                        append_column(*col);

                        AlbumsTreeStore = Gtk::TreeStore::create(Columns);
                        AlbumsTreeStoreFilter = Gtk::TreeModelFilter::create(AlbumsTreeStore);

                        AlbumsTreeStoreFilter->set_visible_func(
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::album_visible_func
                                                ));

                        AlbumsTreeStoreFilter->signal_row_inserted().connect((
                                                sigc::hide(sigc::hide(sigc::mem_fun(
                                                                        *this,
                                                                        &AlbumTreeView::on_row_added_or_deleted
                                                                        )))));

                        AlbumsTreeStoreFilter->signal_row_deleted().connect((
                                                sigc::hide(sigc::mem_fun(
                                                                *this,
                                                                &AlbumTreeView::on_row_added_or_deleted
                                                                ))));


                        AlbumsTreeStore->set_sort_func(0 , sigc::mem_fun( *this, &AlbumTreeView::slotSortAlpha ));
                        AlbumsTreeStore->set_sort_func(1 , sigc::mem_fun( *this, &AlbumTreeView::slotSortDate ));
                        AlbumsTreeStore->set_sort_func(2 , sigc::mem_fun( *this, &AlbumTreeView::slotSortRating ));
                        AlbumsTreeStore->set_sort_func(3 , sigc::mem_fun( *this, &AlbumTreeView::slotSortStrictAlpha ));
                        AlbumsTreeStore->set_sort_func(4 , sigc::mem_fun( *this, &AlbumTreeView::slotSortPlayScore ));

                        AlbumsTreeStore->set_sort_column(0, Gtk::SORT_ASCENDING);

                        m_DiscDefault_Pixbuf =
                                Gdk::Pixbuf::create_from_file(
                                                build_filename(
                                                        DATA_DIR,
                                                        build_filename("images","disc.png")
                                                        )
                                                )->scale_simple(90,90,Gdk::INTERP_BILINEAR);

                        m_DiscDefault = Util::cairo_image_surface_from_pixbuf(m_DiscDefault_Pixbuf->scale_simple(90,90,Gdk::INTERP_BILINEAR));

                        std::vector<TargetEntry> Entries;
                        Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                        Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                        drag_source_set(Entries); 

                        xml->get_widget(name_filter_entry, m_FilterEntry);

                        m_FilterEntry->signal_changed().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::on_filter_entry_changed
                                                ));

                        m_UIManager = ui_manager;

                        m_ActionGroup = Gtk::ActionGroup::create ((boost::format ("Actions-%s") % name).str());

                        m_ActionGroup->add(Gtk::Action::create("dummy","dummy"));

                        m_ActionGroup->add(

                                        Gtk::Action::create(
                                                (boost::format ("action-album-info-%s") % m_Name).str(),
                                                Gtk::StockID (GTK_STOCK_INFO),
                                                _("Album _Info")
                                                ),

                                        sigc::mem_fun(
                                                *this,
                                                &AlbumTreeView::on_album_show_info
                                                ));

                        m_UIManager->insert_action_group(m_ActionGroup);
                        m_UIManager->add_ui_from_string((boost::format(ui_albums_popup) % m_Name).str());

                        gtk_widget_realize(GTK_WIDGET(gobj()));

                        album_list_load ();

                        set_model(AlbumsTreeStoreFilter);
                }

                void
                        AlbumTreeView::on_filter_entry_changed ()
                        {
                                if(Options.Advanced)
                                {
                                    m_Constraints.clear();
                                    m_FilterText = AQE::parse_advanced_query (m_Constraints, m_FilterEntry->get_text());
                                }
                                else
                                {
                                    m_FilterText = m_FilterEntry->get_text().lowercase();
                                }

                                AlbumsTreeStoreFilter->refilter();
                        }

                void
                        AlbumTreeView::on_advanced_query_cb_toggled ()
                        {
                                Options.Advanced = m_AdvancedQueryCB->get_active();
                                on_filter_entry_changed ();
                        }

                void
                        AlbumTreeView::on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
                        {
                                TreeIter iter = AlbumsTreeStore->get_iter (AlbumsTreeStoreFilter->convert_path_to_child_path(path));
                                if(path.get_depth() == ROW_ALBUM)
                                {
                                        gint64 id = (*iter)[Columns.Id];
                                        Signals.PlayAlbum.emit(id);
                                }
                                else
                                {
                                        gint64 id = (*iter)[Columns.TrackId];
                                        IdV v (1, id);
                                        Signals.PlayTracks.emit(v);
                                }
                        }

                void
                        AlbumTreeView::on_row_expanded (const TreeIter &iter_filter,
                                        const TreePath &path) 
                        {
                                TreeIter iter = AlbumsTreeStoreFilter->convert_iter_to_child_iter(iter_filter);
                                if(!(*iter)[Columns.HasTracks])
                                {
                                        GtkTreeIter children;
                                        bool has_children = (gtk_tree_model_iter_children(GTK_TREE_MODEL(AlbumsTreeStore->gobj()), &children, const_cast<GtkTreeIter*>(iter->gobj())));

                                        std::string album_artist_mb_id = (*iter)[Columns.AlbumArtistMBID];
                                        std::string track_artist_mb_id;

                                        SQL::RowV v;
                                        m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % gint64((*iter)[Columns.Id])).str());

                                        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                        {
                                                SQL::Row & r = *i;

                                                TreeIter child = AlbumsTreeStore->append(iter->children());

                                                gint64 id = get<gint64>(r["id"]);

                                                if(r.count("mb_artist_id"))
                                                {
                                                        track_artist_mb_id = get<std::string>(r["mb_artist_id"]);
                                                }

                                                if( album_artist_mb_id != track_artist_mb_id )
                                                {
                                                        (*child)[Columns.TrackArtist] = Markup::escape_text(get<std::string>(r["artist"]));
                                                }

                                                (*child)[Columns.TrackTitle] = get<std::string>(r["title"]);
                                                (*child)[Columns.TrackNumber] = get<gint64>(r["track"]);
                                                (*child)[Columns.TrackLength] = get<gint64>(r["time"]);
                                                (*child)[Columns.TrackId] = get<gint64>(r["id"]);
                                                (*child)[Columns.RowType] = ROW_TRACK; 

                                                m_Track_Iter_Map.insert(std::make_pair(id, child));
                                        }

                                        if(v.size())
                                        {
                                                (*iter)[Columns.HasTracks] = true;
                                                if(has_children)
                                                {
                                                        gtk_tree_store_remove(GTK_TREE_STORE(AlbumsTreeStore->gobj()), &children);
                                                } 
                                                else
                                                        g_warning("%s:%d : No placeholder row present, state seems corrupted.", __FILE__, __LINE__);
                                        }

                                }
                                scroll_to_row (path, 0.);
                        }

                void
                        AlbumTreeView::on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, SelectionData& selection_data, guint info, guint time)
                        {
                                if(m_DragAlbumId)
                                {
                                        gint64 * id = new gint64(m_DragAlbumId.get());
                                        selection_data.set("mpx-album", 8, reinterpret_cast<const guint8*>(id), 8);
                                }
                                else if(m_DragTrackId)
                                {
                                        gint64 * id = new gint64(m_DragTrackId.get());
                                        selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(id), 8);
                                }

                                m_DragAlbumMBID.reset();
                                m_DragAlbumId.reset();
                                m_DragTrackId.reset();
                        }

                void
                        AlbumTreeView::on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context) 
                        {
                                if(m_DragAlbumId)
                                {
                                        if(m_DragAlbumMBID)
                                        {
                                                Cairo::RefPtr<Cairo::ImageSurface> CoverCairo;
                                                m_Covers.get().fetch(m_DragAlbumMBID.get(), CoverCairo, COVER_SIZE_DEFAULT);
                                                if(CoverCairo)
                                                {
                                                        CoverCairo = Util::cairo_image_surface_round(CoverCairo, 21.3); 
                                                        Glib::RefPtr<Gdk::Pixbuf> CoverPixbuf = Util::cairo_image_surface_to_pixbuf(CoverCairo);
                                                        drag_source_set_icon(CoverPixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                                                        return;
                                                }
                                        }

                                        drag_source_set_icon(m_DiscDefault_Pixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                                }
                                else
                                {
                                        Glib::RefPtr<Gdk::Pixmap> pix = create_row_drag_icon(m_PathButtonPress);
                                        drag_source_set_icon(pix->get_colormap(), pix, Glib::RefPtr<Gdk::Bitmap>(0));
                                }
                        }

                void
                        AlbumTreeView::run_rating_comment_dialog(int rating, gint64 id)
                        {
                                Gtk::TextView   * textview;
                                Gtk::Dialog     * dialog;

                                m_Xml->get_widget("albums-textview-comment", textview);
                                m_Xml->get_widget("albums-dialog-rate-and-comment", dialog);
                                Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
                                buffer->set_text("");

                                int response = dialog->run();

                                if( response == GTK_RESPONSE_OK )
                                {
                                        Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
                                        Glib::ustring text = buffer->get_text();

                                        m_Lib.get().albumAddNewRating(id, rating, text);
                                }

                                dialog->hide();
                        }

                bool
                        AlbumTreeView::on_button_press_event (GdkEventButton* event)
                        {
                                int cell_x, cell_y ;
                                TreeViewColumn *col ;

                                if(get_path_at_pos (event->x, event->y, m_PathButtonPress, col, cell_x, cell_y))
                                {
                                        TreeIter iter = AlbumsTreeStore->get_iter(AlbumsTreeStoreFilter->convert_path_to_child_path(m_PathButtonPress));
                                        if(m_PathButtonPress.get_depth() == ROW_ALBUM)
                                        {
                                                m_DragAlbumMBID = (*iter)[Columns.MBID];
                                                m_DragAlbumId = (*iter)[Columns.Id];
                                                m_DragTrackId.reset(); 

                                                g_atomic_int_set(&m_ButtonPressed, 1);

                                                if( (cell_x >= 124) && (cell_x <= 184) && (cell_y > 72) && (cell_y < 90))
                                                {
                                                        int rating = ((cell_x - 124)+7) / 12;
                                                        (*iter)[Columns.Rating] = rating;  
                                                        queue_draw ();

                                                        run_rating_comment_dialog(rating, m_DragAlbumId.get());

                                                        rating = m_Lib.get().albumGetMeanRatingValue(m_DragAlbumId.get());
                                                        (*iter)[Columns.Rating] = rating;  
                                                        queue_draw ();
                                                }
                                        }
                                        else
                                                if(m_PathButtonPress.get_depth() == ROW_TRACK)
                                                {
                                                        m_DragAlbumMBID.reset(); 
                                                        m_DragAlbumId.reset();
                                                        m_DragTrackId = (*iter)[Columns.TrackId];
                                                }
                                }
                                TreeView::on_button_press_event(event);
                                return false;
                        }

                bool
                        AlbumTreeView::on_button_release_event (GdkEventButton* event)
                        {
                                g_atomic_int_set(&m_ButtonPressed, 0);
                                return false;
                        }

                bool
                        AlbumTreeView::on_event (GdkEvent * ev)
                        {
                                if( ev->type == GDK_BUTTON_PRESS )
                                {
                                        GdkEventButton * event = reinterpret_cast <GdkEventButton *> (ev);
                                        if( event->button == 3 )
                                        {
                                                int cell_x, cell_y ;
                                                TreeViewColumn *col ;
                                                TreePath path;

                                                if(get_path_at_pos (event->x, event->y, path, col, cell_x, cell_y))
                                                {
                                                        get_selection()->select( path );

                                                        Gtk::Menu * menu = dynamic_cast < Gtk::Menu* > (
                                                                        Util::get_popup(
                                                                                m_UIManager,
                                                                                (boost::format ("/popup-albumlist-%s/menu-albumlist-%s") % m_Name % m_Name).str()
                                                                                ));

                                                        if (menu) // better safe than screwed
                                                        {
                                                                menu->popup (event->button, event->time);
                                                        }

                                                        return true;
                                                }
                                        }
                                }
                                return false;
                        }


                void
                        AlbumTreeView::on_album_show_info ()
                        {
                                AlbumInfoWindow * d = AlbumInfoWindow::create(
                                                (*get_selection()->get_selected())[Columns.Id],
                                                m_Lib.get(),
                                                m_Covers.get()
                                                );
                        }

                void
                        AlbumTreeView::on_got_cover(const Glib::ustring& mbid)
                        {
                                Cairo::RefPtr<Cairo::ImageSurface> surface;
                                m_Covers.get().fetch(mbid, surface, COVER_SIZE_ALBUM);
                                surface = Util::cairo_image_surface_round(surface, 6.);

                                Gdk::Color c = get_style()->get_black();
                                Util::cairo_image_surface_rounded_border(surface, .5, 6., c.get_red_p(), c.get_green_p(), c.get_blue_p(), 1.);

                                IterSet & set = m_Album_MBID_Iter_Map[mbid];
                                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                                {       
                                        switch( (*(*i))[Columns.RT] ) 
                                        {
                                            case RT_COMPILATION:
                                                (*(*i))[Columns.Image] = Util::cairo_image_surface_overlay( surface, Util::cairo_image_surface_from_pixbuf(m_Emblem[EM_COMPILATION]), 0., 0., 1.); 
                                                break;

                                            case RT_SOUNDTRACK:
                                                (*(*i))[Columns.Image] = Util::cairo_image_surface_overlay( surface, Util::cairo_image_surface_from_pixbuf(m_Emblem[EM_SOUNDTRACK]), 0., 0., 1.); 
                                                break;

                                            default:
                                                (*(*i))[Columns.Image] = surface;
                                                break;
                                        }
                                }
                        }

                void
                        AlbumTreeView::place_album_iter_real(
                            Gtk::TreeIter&  iter,
                            SQL::Row&       r,
                            gint64          id
                        )
                        {
                                MPX::Track track;

                                std::string asin;
                                std::string year; 
                                std::string country;
                                std::string artist;
                                std::string type;
                                std::string rt_string;
                                double playscore = 0;
                                gint64 rating = 0;
                                ReleaseType rt;

                                std::string album = get<std::string>(r["album"]);

                                try{
                                        rating = m_Lib.get().albumGetMeanRatingValue(id);
                                        track[ATTRIBUTE_RATING] = rating;
                                } catch( std::runtime_error )
                                {
                                }
                                (*iter)[Columns.Rating] = rating;

                                if(r.count("album_playscore"))
                                {
                                        playscore = get<double>(r["album_playscore"]);
                                }

                                if(r.count("album_insert_date"))
                                {
                                        (*iter)[Columns.InsertDate] = get<gint64>(r["album_insert_date"]);
                                }

                                if(r.count("amazon_asin"))
                                {
                                        asin = get<std::string>(r["amazon_asin"]);
                                        track[ATTRIBUTE_ASIN] = asin;
                                }

                                if(r.count("mb_album_id"))
                                {
                                        std::string mbid = get<std::string>(r["mb_album_id"]);

                                        IterSet & s = m_Album_MBID_Iter_Map[mbid];
                                        s.insert(iter);

                                        (*iter)[Columns.MBID] = mbid; 
                                        track[ATTRIBUTE_MB_ALBUM_ID] = mbid;
                                }

                                if(r.count("mb_album_artist_id"))
                                {
                                        std::string mb_album_artist_id = get<std::string>(r["mb_album_artist_id"]);
                                        (*iter)[Columns.AlbumArtistMBID] = mb_album_artist_id; 
                                        track[ATTRIBUTE_MB_ALBUM_ARTIST_ID] = mb_album_artist_id;
                                }

                                if(r.count("mb_release_date"))
                                {
                                        year = get<std::string>(r["mb_release_date"]);
                                        if(year.size())
                                        {
                                                year = year.substr(0,4);
                                                try{
                                                        (*iter)[Columns.Date] = boost::lexical_cast<int>(year);
                                                        track[ATTRIBUTE_DATE] = gint64(boost::lexical_cast<int>(year));
                                                } catch( boost::bad_lexical_cast ) {
                                                } 
                                        }
                                }
                                else
                                {
                                        (*iter)[Columns.Date] = 0; 
                                }

                                if(r.count("mb_release_country"))
                                {
                                        country = get<std::string>(r["mb_release_country"]); 
                                        track[ATTRIBUTE_MB_RELEASE_COUNTRY] = country; 
                                }

                                if(r.count("mb_release_type"))
                                {
                                        type = get<std::string>(r["mb_release_type"]); 
                                        track[ATTRIBUTE_MB_RELEASE_TYPE] = type; 
                                }

                                if(r.find("album_artist_sortname") != r.end())
                                {
                                        artist = get<std::string>(r["album_artist_sortname"]);
                                        track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME] = artist; 
                                }
                                else
                                {
                                        artist = get<std::string>(r["album_artist"]);
                                        track[ATTRIBUTE_ALBUM_ARTIST_SORTNAME] = artist; 
                                }

                                trim(country);
                                trim(year);
                                trim(type);

                                rt = determine_release_type(type);
                                rt_string = _(get_release_string(rt).c_str());

                                if( !country.empty() )
                                {
                                        (*iter)[Columns.Text] =
        (boost::format("<span size='8000'><span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3% %4%\n%5%</span></span>")
                                                 % Markup::escape_text(album).c_str()
                                                 % Markup::escape_text(artist).c_str()
                                                 % country
                                                 % year
                                                 % rt_string
                                                ).str();
                                }
                                else
                                {
                                        (*iter)[Columns.Text] =
        (boost::format("<span size='8000'><span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3%\n%4%</span></span>")
                                                 % Markup::escape_text(album).c_str()
                                                 % Markup::escape_text(artist).c_str()
                                                 % year
                                                 % rt_string
                                                ).str();
                                }


                                (*iter)[Columns.AlbumSort] = ustring(album).collate_key();
                                (*iter)[Columns.ArtistSort] = ustring(artist).collate_key();
                                (*iter)[Columns.RT] = rt; 
                                (*iter)[Columns.PlayScore] = playscore; 
                                (*iter)[Columns.AlbumTrack] = track;
                        } 

                void
                        AlbumTreeView::update_album (SQL::Row & r, gint64 id)
                        {
                                IdIterMap::iterator i = m_Album_Iter_Map.find(id);
                                if (i == m_Album_Iter_Map.end()) return;

                                TreeIter iter = (*i).second; 

                                (*iter)[Columns.NewAlbum] = get<gint64>(r["album_new"]);

                                place_album_iter_real(iter, r, id);
                        } 

                Gtk::TreeIter
                        AlbumTreeView::place_album(
                            SQL::Row&   r,
                            gint64      id
                        )
                        {
                                TreeIter iter = AlbumsTreeStore->append();
                                m_Album_Iter_Map.insert(std::make_pair(id, iter));
                                AlbumsTreeStore->append(iter->children()); //create dummy/placeholder row for tracks

                                (*iter)[Columns.RowType] = ROW_ALBUM; 
                                (*iter)[Columns.HasTracks] = false; 
                                (*iter)[Columns.NewAlbum] = get<gint64>(r["album_new"]);
                                (*iter)[Columns.Id] = id; 

                                place_album_iter_real(iter, r, id);

                                switch( (*iter)[Columns.RT] ) 
                                {
                                    case RT_COMPILATION:
                                        (*iter)[Columns.Image] = Util::cairo_image_surface_overlay( m_DiscDefault, Util::cairo_image_surface_from_pixbuf(m_Emblem[EM_COMPILATION]), 0., 0., 1.); 
                                        break;

                                    case RT_SOUNDTRACK:
                                        (*iter)[Columns.Image] = Util::cairo_image_surface_overlay( m_DiscDefault, Util::cairo_image_surface_from_pixbuf(m_Emblem[EM_SOUNDTRACK]), 0., 0., 1.); 
                                        break;

                                    default:
                                        (*iter)[Columns.Image] = m_DiscDefault; 
                                        break;
                                }

                                return iter;
                        }

                void
                        AlbumTreeView::album_list_load ()
                        {
                                AlbumsTreeStore->clear ();
                                m_Album_MBID_Iter_Map.clear();
                                m_Album_Iter_Map.clear();
                                m_Track_Iter_Map.clear();

                                SQL::RowV v;
                                m_Lib.get().getSQL(v, "SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id;");

                                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                                {
                                        SQL::Row & r = *i; 
                                        place_album(r, get<gint64>(r["id"]));
                                        m_Covers.get().cache(get<std::string>(r["mb_album_id"]));
                                }
                        }

                void
                        AlbumTreeView::on_album_updated(gint64 id)
                        {
                                g_message(G_STRFUNC);

                                SQL::RowV v;
                                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                                g_return_if_fail(!v.empty());

                                SQL::Row & r = v[0];

                                update_album (r, id); 
                        }

                void
                        AlbumTreeView::on_new_album(gint64 id)
                        {
                                SQL::RowV v;
                                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                                g_return_if_fail(!v.empty());

                                SQL::Row & r = v[0];

                                place_album (r, id); 
                        }

                void
                        AlbumTreeView::on_new_track(Track & track, gint64 album_id, gint64 artist_id)
                        {
                                if(m_Album_Iter_Map.count(album_id))
                                {
                                        TreeIter iter = m_Album_Iter_Map[album_id];
                                        if (((*iter)[Columns.HasTracks]))
                                        {
                                                TreeIter child = AlbumsTreeStore->append(iter->children());
                                                if(track[ATTRIBUTE_TITLE])
                                                        (*child)[Columns.TrackTitle] = get<std::string>(track[ATTRIBUTE_TITLE].get());
                                                if(track[ATTRIBUTE_ARTIST])
                                                        (*child)[Columns.TrackArtist] = get<std::string>(track[ATTRIBUTE_ARTIST].get());
                                                if(track[ATTRIBUTE_MB_ARTIST_ID])
                                                        (*child)[Columns.TrackArtistMBID] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());
                                                if(track[ATTRIBUTE_TRACK])
                                                        (*child)[Columns.TrackNumber] = get<gint64>(track[ATTRIBUTE_TRACK].get());
                                                if(track[ATTRIBUTE_TIME])
                                                        (*child)[Columns.TrackLength] = get<gint64>(track[ATTRIBUTE_TIME].get());

                                                (*child)[Columns.TrackId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get());
                                                (*child)[Columns.RowType] = ROW_TRACK; 
                                        }
                                }
                                else
                                        g_warning("%s: Got new track without associated album! Consistency error!", G_STRLOC);
                        }

                void
                        AlbumTreeView::on_album_deleted(gint64 id)
                        {
                            IdIterMap::iterator i = m_Album_Iter_Map.find(id);
                            if( i != m_Album_Iter_Map.end() )
                            {
                                std::string mbid = (*i->second)[Columns.MBID];
                                AlbumsTreeStore->erase( i->second );
                                IterSet & set = m_Album_MBID_Iter_Map[mbid];
                                set.erase( i->second );
                                m_Album_Iter_Map.erase( i );
                            }
                        }

                void
                        AlbumTreeView::on_track_deleted(gint64 id)
                        {
                            IdIterMap::iterator i = m_Track_Iter_Map.find(id);
                            if( i != m_Track_Iter_Map.end() )
                            {
                                AlbumsTreeStore->erase( i->second );
                                m_Track_Iter_Map.erase( i );
                            }
                        }

                int
                        
                        AlbumTreeView::sortTracks(const TreeIter& iter_a, const TreeIter& iter_b)
                        {
                                gint64 trk_a = (*iter_a)[Columns.TrackNumber];
                                gint64 trk_b = (*iter_b)[Columns.TrackNumber];

                                return trk_a - trk_b;
                        }

                int
                        AlbumTreeView::slotSortRating(const TreeIter& iter_a, const TreeIter& iter_b)
                        {
                                AlbumRowType rt_a = (*iter_a)[Columns.RowType];
                                AlbumRowType rt_b = (*iter_b)[Columns.RowType];

                                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                                {
                                        gint64 alb_a = (*iter_a)[Columns.Rating];
                                        gint64 alb_b = (*iter_b)[Columns.Rating];

                                        return alb_b - alb_a;
                                }
                                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                                {
                                        return sortTracks( iter_a, iter_b );
                                }

                                return 0;
                        }

                int
                        AlbumTreeView::slotSortAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
                        {
                                AlbumRowType rt_a = (*iter_a)[Columns.RowType];
                                AlbumRowType rt_b = (*iter_b)[Columns.RowType];

                                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                                {
                                        gint64 alb_a = (*iter_a)[Columns.Date];
                                        gint64 alb_b = (*iter_b)[Columns.Date];
                                        std::string art_a = (*iter_a)[Columns.ArtistSort];
                                        std::string art_b = (*iter_b)[Columns.ArtistSort];

                                        if(art_a != art_b)
                                                return art_a.compare(art_b);

                                        return alb_a - alb_b;
                                }
                                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                                {
                                        return sortTracks( iter_a, iter_b );
                                }

                                return 0;
                        }

                int
                        AlbumTreeView::slotSortDate(const TreeIter& iter_a, const TreeIter& iter_b)
                        {
                                AlbumRowType rt_a = (*iter_a)[Columns.RowType];
                                AlbumRowType rt_b = (*iter_b)[Columns.RowType];

                                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                                {
                                        gint64 alb_a = (*iter_a)[Columns.InsertDate];
                                        gint64 alb_b = (*iter_b)[Columns.InsertDate];

                                        return alb_b - alb_a;
                                }
                                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                                {
                                        return sortTracks( iter_a, iter_b );
                                }

                                return 0;
                        }


                int
                        AlbumTreeView::slotSortStrictAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
                        {
                                AlbumRowType rt_a = (*iter_a)[Columns.RowType];
                                AlbumRowType rt_b = (*iter_b)[Columns.RowType];

                                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                                {
                                        std::string alb_a = (*iter_a)[Columns.AlbumSort];
                                        std::string alb_b = (*iter_b)[Columns.AlbumSort];
                                        std::string art_a = (*iter_a)[Columns.ArtistSort];
                                        std::string art_b = (*iter_b)[Columns.ArtistSort];

                                        if(art_a != art_b)
                                                return art_a.compare(art_b);

                                        return alb_a.compare(alb_b);
                                }
                                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                                {
                                        return sortTracks( iter_a, iter_b );
                                }

                                return 0;
                        }

                int
                        AlbumTreeView::slotSortPlayScore(const TreeIter& iter_a, const TreeIter& iter_b)
                        {
                                AlbumRowType rt_a = (*iter_a)[Columns.RowType];
                                AlbumRowType rt_b = (*iter_b)[Columns.RowType];

                                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                                {
                                        double score_a = (*iter_a)[Columns.PlayScore];
                                        double score_b = (*iter_b)[Columns.PlayScore];

                                        if( score_a < score_b )
                                            return  1;
    
                                        if( score_a > score_b )
                                            return -1;

                                        return 0;
                                }
                                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                                {
                                        return sortTracks( iter_a, iter_b );
                                }

                                return 0;
                        }

                void
                        AlbumTreeView::cellDataFuncCover (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                TreePath path (iter);
                                CellRendererCairoSurface *cell = dynamic_cast<CellRendererCairoSurface*>(basecell);
                                if(path.get_depth() == ROW_ALBUM)
                                {
                                        cell->property_visible() = true;
                                        cell->property_surface() = (*iter)[Columns.Image]; 

                                        double score = (*iter)[Columns.PlayScore]; 

                                        if( Options.HighlightMode == HIGHLIGHT_EQUAL )
                                        {
                                            cell->property_alpha() = 1.0;
                                        }
                                        else if( Options.HighlightMode == HIGHLIGHT_UNPLAYED )
                                        {
                                            if( score < 1 )
                                            {
                                                cell->property_alpha() = 1.;
                                                return;
                                            }

                                            cell->property_alpha() = 0.1; 
                                        }
                                        else if( Options.HighlightMode == HIGHLIGHT_PLAYED )
                                        {
                                            if( score >= 1 )
                                            {
                                                cell->property_alpha() = 1.;
                                                return;
                                            }

                                            cell->property_alpha() = 0.1; 
                                        }
                                }
                                else
                                {
                                        cell->property_visible() = false;
                                }
                        }

                void
                        AlbumTreeView::cellDataFuncText1 (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                TreePath path (iter);
                                CellRendererVBox *cvbox = dynamic_cast<CellRendererVBox*>(basecell);
                                CellRendererText *cell1 = dynamic_cast<CellRendererText*>(cvbox->property_renderer1().get_value());
                                CellRendererPixbuf *cell2 = dynamic_cast<CellRendererPixbuf*>(cvbox->property_renderer2().get_value());
                                if(path.get_depth() == ROW_ALBUM)
                                {
                                        cvbox->property_visible() = true; 

                                        if(cell1)
                                        {
                                                cell1->property_markup() = (*iter)[Columns.Text]; 

                                                double score = (*iter)[Columns.PlayScore]; 

                                                if( Options.HighlightMode == HIGHLIGHT_UNPLAYED )
                                                {
                                                    if( score < 1 )
                                                        cell1->property_sensitive() = true;
                                                    else
                                                        cell1->property_sensitive() = false; 
                                                }
                                                else if( Options.HighlightMode == HIGHLIGHT_PLAYED )
                                                {
                                                    if( score >= 1 )
                                                        cell1->property_sensitive() = true; 
                                                    else
                                                        cell1->property_sensitive() = false; 
                                                }
                                        }

                                        if(cell2)
                                        {
                                                gint64 i = ((*iter)[Columns.Rating]);
                                                g_return_if_fail((i >= 0) && (i <= 5));
                                                cell2->property_pixbuf() = m_Stars[i];
                                        }
                                }
                                else
                                {
                                        cvbox->property_visible() = false; 
                                }
                        }

                void
                        AlbumTreeView::cellDataFuncText2 (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                TreePath path (iter);
                                CellRendererCount *cell = dynamic_cast<CellRendererCount*>(basecell);
                                if(path.get_depth() == ROW_TRACK)
                                {
                                        cell->property_visible() = true; 
                                        cell->property_text() = (boost::format("%lld") % (*iter)[Columns.TrackNumber]).str();
                                }
                                else
                                {
                                        cell->property_visible() = false; 
                                }
                        }

                void
                        AlbumTreeView::cellDataFuncText3 (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                TreePath path (iter);
                                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                                if(path.get_depth() == ROW_TRACK)
                                {
                                        cell->property_visible() = true; 
                                        cell->property_markup() = Markup::escape_text((*iter)[Columns.TrackTitle]);
                                }
                                else
                                {
                                        cell->property_visible() = false; 
                                }
                        }

                void
                        AlbumTreeView::cellDataFuncText4 (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                TreePath path (iter);
                                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                                if(path.get_depth() == ROW_TRACK)
                                {
                                        cell->property_visible() = true; 
                                        gint64 Length = (*iter)[Columns.TrackLength];
                                        cell->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
                                }
                                else
                                {
                                        cell->property_visible() = false; 
                                }
                        }

                void
                        AlbumTreeView::cellDataFuncText5 (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                TreePath path (iter);
                                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                                if(path.get_depth() == ROW_TRACK)
                                {
                                        cell->property_visible() = true; 
                                        cell->property_markup() = (*iter)[Columns.TrackArtist];
                                }
                                else
                                {
                                        cell->property_visible() = false; 
                                }
                        }

                void
                        AlbumTreeView::rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                                        GtkCellRenderer   *cell,
                                        GtkTreeModel      *model,
                                        GtkTreeIter       *iter,
                                        gpointer           data) 
                        {
                                if (gtk_tree_model_iter_has_child (model, iter))
                                {
                                        GtkTreePath *path;
                                        gboolean     row_expanded;

                                        path = gtk_tree_model_get_path (model, iter);
                                        row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (column->tree_view), path);
                                        gtk_tree_path_free (path);

                                        g_object_set (cell,
                                                        "visible", TRUE,
                                                        "render", TRUE,
                                                        "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
                                                        NULL);
                                } else {
                                        g_object_set (cell, "visible", FALSE, "render", FALSE, NULL);
                                }
                        }

                bool
                        AlbumTreeView::album_visible_func (TreeIter const& iter)
                        {
                                TreePath path = AlbumsTreeStore->get_path(iter);

                                if( path.size() == 1 && (Options.Flags & ALBUMS_STATE_SHOW_NEW) && !(*iter)[Columns.NewAlbum])
                                {
                                        return false;
                                } 

                                if( path.size() == 1 && !(Options.Type & (*iter)[Columns.RT]))
                                {
                                        return false;
                                }

                                if( path.size() > 1 ) 
                                {
                                        return true;
                                }

                                if( m_FilterText.empty() && m_Constraints.empty() ) 
                                {
                                        return true;
                                }

                                bool truthvalue = true;

                                if( !m_Constraints.empty() )
                                {
                                    MPX::Track track = (*iter)[Columns.AlbumTrack]; 
                                    truthvalue = AQE::match_track(m_Constraints, track);
                                }

                                return truthvalue && Util::match_keys (ustring((*iter)[Columns.Text]).lowercase(), m_FilterText); 
                        }

                void
                        AlbumTreeView::update_album_count_display ()
                        {
                                TreeNodeChildren::size_type n1 = AlbumsTreeStoreFilter->children().size();
                                TreeNodeChildren::size_type n2 = AlbumsTreeStore->children().size();
                                if( m_LabelShowing )
                                {
                                    static boost::format showing_f ("Out of %lld, showing %lld %s%s");

                                    std::string extra = "";
                                    if (Options.HighlightMode != HIGHLIGHT_EQUAL)
                                    {
                                        if( Options.HighlightMode == HIGHLIGHT_PLAYED )
                                            extra = _(", highlighting played");
                                        else
                                            extra = _(", highlighting unplayed");
                                    }

                                    m_LabelShowing->set_text( (showing_f
                                        % n2
                                        % n1
                                        % ((Options.Type == RT_ALL) ? _("releases of all kind") : get_release_string(ReleaseType(Options.Type), true))
                                        % extra
                                    ).str());
                                }
                        }

                void
                        AlbumTreeView::on_row_added_or_deleted ()
                        {
                                update_album_count_display ();
                        }

                void
                        AlbumTreeView::go_to_album(gint64 id)
                        {
                                if(m_Album_Iter_Map.count(id))
                                {
                                        TreeIter iter = m_Album_Iter_Map.find(id)->second;
                                        scroll_to_row (AlbumsTreeStore->get_path(iter), 0.);
                                }
                        }

                void
                        AlbumTreeView::set_new_albums_state (bool state)
                        {
                                if(state)
                                        Options.Flags |= ALBUMS_STATE_SHOW_NEW;
                                else
                                        Options.Flags &= ~ALBUMS_STATE_SHOW_NEW;
                                AlbumsTreeStoreFilter->refilter();
                        }

                void
                        AlbumTreeView::set_release_type_filter (int state)
                        {
                                Options.Type = state; 
                                update_album_count_display();
                                AlbumsTreeStoreFilter->refilter();
                        }

                void
                        AlbumTreeView::set_highlight_mode (AlbumHighlightMode mode)
                        {
                                Options.HighlightMode = mode;
                                update_album_count_display();
                                queue_draw ();
                        }
} // end namespace MPX 
