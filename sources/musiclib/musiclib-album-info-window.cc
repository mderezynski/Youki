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
#include "musiclib-album-info-window.hh"
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

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/widgetloader.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using boost::get;
using boost::algorithm::trim;

namespace
{
    const int N_STARS = 6;
}

namespace MPX
{
                AlbumRatingsList::AlbumRatingsList(
                    const Glib::RefPtr<Gnome::Glade::Xml>&  xml
                )
                : WidgetLoader<Gtk::TreeView>(xml, "album-info-window-ratings-list")
                {
                        Store = Gtk::ListStore::create(Columns); 

                        append_column(_("Date"), Columns.DateString);            

                        TreeViewColumn * col = manage (new TreeViewColumn(_("Rating")));
                        CellRendererPixbuf * cell = manage (new CellRendererPixbuf);
                        col->pack_start(*cell, false);
                        col->set_min_width(66);
                        col->set_max_width(66);
                        col->set_cell_data_func(
                                        *cell,
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumRatingsList::cellDataFuncRating
                                                ));
                        append_column(*col);

                        append_column(_("Abstract"), Columns.ShortText);            

                        for(int n = 0; n < N_STARS; ++n)
                        {
                                Stars[n] = Gdk::Pixbuf::create_from_file(
                                                build_filename(
                                                        build_filename(
                                                                DATA_DIR,
                                                                "images"
                                                                ),
                                                        (boost::format("stars%d.png") % n).str()
                                                        ));
                        }

                        Gtk::TextView * view;
                        m_Xml->get_widget("albumcommentview", view);
                        Buffer = view->get_buffer();

                        get_selection()->signal_changed().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumRatingsList::on_selection_changed
                                                ));

                        set_model(Store);
                };

                void
                        AlbumRatingsList::on_selection_changed ()
                        { 
                                if( get_selection()->count_selected_rows() )
                                {
                                        TreeIter iter = get_selection()->get_selected();
                                        Buffer->set_text((*iter)[Columns.Text]);
                                }
                                else
                                {
                                        Buffer->set_text("");
                                }
                        }

                void
                        AlbumRatingsList::load_ratings(gint64 id, MPX::Library & lib)
                        {
                                using boost::get;

                                Store->clear();

                                SQL::RowV v; 
                                lib.albumGetAllRatings(id, v);
                                for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                                {
                                        TreeIter iter = Store->append();

                                        gint64 rating = get<gint64>((*i)["rating"]);
                                        time_t time = time_t(get<gint64>((*i)["time"]));
                                        std::string text = get<std::string>((*i)["comment"]);
                                        gint64 id = get<gint64>((*i)["id"]);

                                        char * time_s = ctime(&time);
                                        std::string time_ccs (time_s);
                                        boost::algorithm::trim (time_ccs);

                                        (*iter)[Columns.DateString] = time_ccs;
                                        (*iter)[Columns.Rating]     = rating; 
                                        (*iter)[Columns.ShortText]  = Glib::ustring(text).substr(0,64) + "...";
                                        (*iter)[Columns.Text]       = text; 
                                        (*iter)[Columns.Id]         = id; 
                                }
                        }

                void
                        AlbumRatingsList::cellDataFuncRating (CellRenderer * basecell, TreeModel::iterator const &iter)
                        {
                                CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);
                                gint64 i = ((*iter)[Columns.Rating]);
                                g_return_if_fail((i >= 0) && (i <= 5));
                                cell_p->property_pixbuf() = Stars[i];
                        }

        AlbumInfoWindow*
                AlbumInfoWindow::create(
                    gint64                      id,
                    MPX::Library              & obj_library,
                    MPX::Covers               & obj_covers
                )
                {
                        const std::string path = DATA_DIR G_DIR_SEPARATOR_S "glade" G_DIR_SEPARATOR_S "album-info-window.glade";
                        Glib::RefPtr<Gnome::Glade::Xml> glade_xml = Gnome::Glade::Xml::create (path);
                        AlbumInfoWindow * p = new AlbumInfoWindow(glade_xml, id, obj_library, obj_covers); 
                        return p;
                }


                AlbumInfoWindow::AlbumInfoWindow(
                    const Glib::RefPtr<Gnome::Glade::Xml>&  xml,
                    gint64                                  id,
                    MPX::Library                          & obj_library,    
                    MPX::Covers                           & obj_covers
                )
                : WidgetLoader<Gtk::Window>(xml, "album-info-window")
                , m_Id(id)
                , m_Lib(obj_library)
                , m_Covers(obj_covers)
                {
                        glade_xml_signal_autoconnect(xml->gobj());

                        m_Xml->get_widget("album-info-window-cover", m_ImageCover); 
                        m_Xml->get_widget("album-info-window-label-artist", m_l1);
                        m_Xml->get_widget("album-info-window-label-album", m_l2);
                        m_Xml->get_widget("album-info-window-delete-rating", m_delete_rating);

                        m_delete_rating->signal_clicked().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumInfoWindow::delete_rating
                                                ));

                        m_AlbumRatingsList = new AlbumRatingsList(xml);

                        m_AlbumRatingsList->get_selection()->signal_changed().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumInfoWindow::on_rating_list_selection_changed
                                                ));

                        m_Lib.signal_album_updated().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &AlbumInfoWindow::display_album
                                                ));

                        display_album(id);
                }

        void
                AlbumInfoWindow::on_rating_list_selection_changed ()
                { 
                        m_delete_rating->set_sensitive( m_AlbumRatingsList->get_selection()->count_selected_rows() );
                }

        void
                AlbumInfoWindow::delete_rating ()
                {
                        if( m_AlbumRatingsList->get_selection()->count_selected_rows() )
                        {
                                TreeIter iter = m_AlbumRatingsList->get_selection()->get_selected();
                                gint64 rating_id = (*iter)[m_AlbumRatingsList->Columns.Id];
                                m_Lib.albumDeleteRating(rating_id, m_Id);
                                display_album(m_Id);
                        }
                }

        void
                AlbumInfoWindow::display_album(gint64 id)
                {
                        m_AlbumRatingsList->load_ratings(id, m_Lib);

                        SQL::RowV v;
                        m_Lib.getSQL(v, (boost::format ("SELECT * FROM album JOIN album_artist ON album_artist_j = album_artist.id WHERE album.id = '%lld'") % id).str());
                        if( !v.empty ())
                        {
                                std::string mbid = get<std::string>(v[0]["mb_album_id"]);
                                Cairo::RefPtr<Cairo::ImageSurface> surface;
                                m_Covers.fetch(mbid, surface, COVER_SIZE_DEFAULT);

                                if( surface )
                                {
                                        surface = Util::cairo_image_surface_round(surface, 9.5);
                                        Util::cairo_image_surface_rounded_border(surface, 1., 9.5);
                                        m_ImageCover->set(Util::cairo_image_surface_to_pixbuf(surface));
                                }

                                m_l1->set_markup((boost::format ("<big>%s</big>")
                                                        % Glib::Markup::escape_text(get<std::string>(v[0]["album_artist"])).raw()
                                                 ).str());

                                m_l2->set_markup((boost::format ("<big><b>%s</b></big>")
                                                        % Glib::Markup::escape_text(get<std::string>(v[0]["album"])).raw()
                                                 ).str());
                        }
                }
}
