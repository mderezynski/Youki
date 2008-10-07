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
#include "mpx/com/mb-import-album.hh"
#include "config.h"
#ifdef HAVE_TR1
#include <tr1/unordered_map>
#endif //HAVE_TR1
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <glib.h>
#include <libglademm.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/widgetloader.hh"
#include "mpx/metadatareader-taglib.hh"
#include "musicbrainz/mbxml-v2.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using namespace MusicBrainzXml;
using boost::get;
using boost::algorithm::trim;

namespace
{
    const int N_STARS = 6;
}

namespace MPX
{
        MB_ImportAlbum*
                MB_ImportAlbum::create(
                    MPX::Library              & obj_library,
                    MPX::Covers               & obj_covers
                )
                {
                        const std::string path = DATA_DIR G_DIR_SEPARATOR_S "glade" G_DIR_SEPARATOR_S "mb-import-album.glade";
                        Glib::RefPtr<Gnome::Glade::Xml> glade_xml = Gnome::Glade::Xml::create (path);
                        MB_ImportAlbum * p = new MB_ImportAlbum(glade_xml, obj_library, obj_covers); 
                        return p;
                }


                MB_ImportAlbum::MB_ImportAlbum(
                    const Glib::RefPtr<Gnome::Glade::Xml>&  xml,
                    MPX::Library                          & obj_library,    
                    MPX::Covers                           & obj_covers
                )
                : WidgetLoader<Gtk::Window>(xml, "mb-import-album")
                , m_Lib(obj_library)
                , m_Covers(obj_covers)
                {
                        glade_xml_signal_autoconnect(xml->gobj());

                        m_Xml->get_widget("mbrel-cover", m_iCover); 
                        m_Xml->get_widget("button-search", m_bSearch);
                        m_Xml->get_widget("button-import", m_bImport);
                        m_Xml->get_widget("button-cancel", m_bCancel);
                        m_Xml->get_widget("button-add-files", m_bAddFiles);
                        m_Xml->get_widget("treeview-tracks", m_tTracks);
                        m_Xml->get_widget("treeview-release", m_tRelease);

                        m_Xml->get_widget("cbe-artist", m_cbeArtist);
                        m_Xml->get_widget("cbe-album", m_cbeAlbum);
                        m_Xml->get_widget("cbe-genre", m_cbeGenre);
                        m_Xml->get_widget("e-mbid", m_eMBID);

                        m_Model_CBE_Artist = Gtk::ListStore::create( m_CBE_Text_ID_Columns );
                        m_Model_CBE_Album  = Gtk::ListStore::create( m_CBE_Text_ID_Columns );

                        m_cbeArtist->set_model( m_Model_CBE_Artist );
                        m_cbeAlbum->set_model( m_Model_CBE_Album );
       
                        m_bSearch->signal_clicked().connect(
                            sigc::mem_fun(
                                *this,
                                &MB_ImportAlbum::on_button_search
                        ));

                        m_bImport->set_sensitive( false ); 
                        m_bImport->signal_clicked().connect(
                            sigc::mem_fun(
                                *this,
                                &MB_ImportAlbum::on_button_import
                        ));

                        m_bCancel->signal_clicked().connect(
                            sigc::mem_fun(
                                *this,
                                &MB_ImportAlbum::on_button_cancel
                        ));

                        m_bAddFiles->signal_clicked().connect(
                            sigc::mem_fun(
                                *this,
                                &MB_ImportAlbum::on_button_add_files
                        ));

                        m_Model_Release = Gtk::ListStore::create(m_ModelColumns);
                        m_Model_Tracks = Gtk::ListStore::create(m_ModelColumns);

                        m_tRelease->set_model(m_Model_Release);
                        m_tTracks->set_model(m_Model_Tracks);

                        setup_view(*m_tRelease);
                        setup_view(*m_tTracks);
                }

        void
                MB_ImportAlbum::setup_view(Gtk::TreeView& view)
                {
                        Gtk::CellRenderer * cell = manage (new Gtk::CellRendererText);
                        Gtk::TreeViewColumn * col = 0;

                        col = manage( new Gtk::TreeViewColumn("#"));
                        col->pack_start(*cell, false);
                        col->set_cell_data_func( *cell, sigc::bind( sigc::mem_fun( *this, &MB_ImportAlbum::cell_data_func_track ), COL_TRACK));
                        view.append_column(*col);

                        col = manage( new Gtk::TreeViewColumn(_("Title")));
                        col->pack_start(*cell, true);
                        col->set_cell_data_func( *cell, sigc::bind( sigc::mem_fun( *this, &MB_ImportAlbum::cell_data_func_track ), COL_TITLE));
                        view.append_column(*col);

                        col = manage( new Gtk::TreeViewColumn(_("Album")));
                        col->pack_start(*cell, true);
                        col->set_cell_data_func( *cell, sigc::bind( sigc::mem_fun( *this, &MB_ImportAlbum::cell_data_func_track ), COL_ALBUM));
                        view.append_column(*col);

                        col = manage( new Gtk::TreeViewColumn(_("Artist")));
                        col->pack_start(*cell, true);
                        col->set_cell_data_func( *cell, sigc::bind( sigc::mem_fun( *this, &MB_ImportAlbum::cell_data_func_track ), COL_ARTIST));
                        view.append_column(*col);

                        col = manage( new Gtk::TreeViewColumn(_("Time")));
                        col->pack_start(*cell, true);
                        col->set_cell_data_func( *cell, sigc::bind( sigc::mem_fun( *this, &MB_ImportAlbum::cell_data_func_track ), COL_TIME));
                        view.append_column(*col);
                }

        void
                MB_ImportAlbum::cell_data_func_track( Gtk::CellRenderer *basecell, const Gtk::TreeIter& iter, Columns col)
                { 
                        Gtk::CellRendererText & cell = *(dynamic_cast<Gtk::CellRendererText*>(basecell));

                        MPX::Track track = (*iter)[m_ModelColumns.Track];

                        std::string text;
                        text.reserve(256);

                        switch( col )
                        {
                            case COL_TITLE:
                                if( track.has(ATTRIBUTE_TITLE) )
                                {
                                    text = get<std::string>(track[ATTRIBUTE_TITLE].get()); 
                                }
                                break;

                            case COL_ALBUM:
                                if( track.has(ATTRIBUTE_ALBUM) )
                                {
                                    text = get<std::string>(track[ATTRIBUTE_ALBUM].get()); 
                                }
                                break;

                            case COL_ARTIST:
                                if( track.has(ATTRIBUTE_ARTIST) )
                                {
                                    text = get<std::string>(track[ATTRIBUTE_ARTIST].get()); 
                                }
                                break;

                            case COL_TIME:
                                if( track.has(ATTRIBUTE_TIME) )
                                {
                                    gint64 t = get<gint64>(track[ATTRIBUTE_TIME].get());
                                    text = (boost::format ("%lld:%02lld") % (t / 60) % (t % 60)).str();
                                }
                                break;

                            case COL_TRACK:
                                if( track.has(ATTRIBUTE_TRACK) )
                                {
                                    gint64 t = get<gint64>(track[ATTRIBUTE_TRACK].get());
                                    text = (boost::format ("%lld") % t).str(); 
                                }
                                break;

                            default: break;
                        }

                        cell.property_text() = text;
                }

        void
                MB_ImportAlbum::run ()
                {
                    show_all();
                }

        void
                MB_ImportAlbum::on_button_search ()
                {
                   MusicBrainzReleaseV releases; 
                
                   if( !m_eMBID->get_text().empty() )
                   {
                        mb_releases_by_id (m_eMBID->get_text(), releases);
                   }
                   else
                   {
                        mb_releases_query (m_cbeArtist->get_entry()->get_text(), m_cbeAlbum->get_entry()->get_text(), releases);
                   }

                    populate_matches( releases );
                }

        void
                MB_ImportAlbum::on_button_import ()
                {
                }

        void
                MB_ImportAlbum::on_button_cancel ()
                {
                    hide ();
                    m_Model_Tracks->clear();
                    m_Model_Release->clear();
                    /* TODO: Reset release data */
                }

        void
                MB_ImportAlbum::on_button_add_files ()
                {
                    Gtk::FileChooserDialog dialog (_("AudioSource: Select Album Tracks to Import")); 
                    dialog.add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
                    dialog.add_button( Gtk::Stock::ADD, Gtk::RESPONSE_OK );
                    dialog.set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
                    dialog.set_select_multiple();
                    dialog.set_current_folder(mcs->key_get<std::string>("mpx", "file-chooser-path"));

                    int response = dialog.run();
                    dialog.hide();

                    if( response == Gtk::RESPONSE_OK )
                    {
                        m_Uris = dialog.get_uris();
                        reload_tracks ();
                    }
                }

        void
                MB_ImportAlbum::reload_tracks ()
                {
                    m_Model_Tracks->clear();

                    MetadataReaderTagLib & reader = *(services->get<MetadataReaderTagLib>("mpx-service-taglib"));
                    for(Uris_t::const_iterator i = m_Uris.begin(); i != m_Uris.end(); ++i)
                    {
                            Track track;
                            reader.get( *i, track );
                            (*m_Model_Tracks->append())[m_ModelColumns.Track] = track;
                    }
                }

        void
                MB_ImportAlbum::populate_matches( const MusicBrainzReleaseV& releases )
                {
                      m_cbeArtist->get_entry()->set_text ("");
                      m_cbeAlbum->get_entry()->set_text ("");
                      m_eMBID->set_text ("");

                      m_Model_CBE_Artist->clear ();
                      m_Model_CBE_Album->clear ();

                      std::set<std::string> set_artists;

                      for( MusicBrainzReleaseV::const_iterator i = releases.begin (); i != releases.end(); ++i )
                      {
                        const MusicBrainzRelease& release = *i; 

                        if( !set_artists.count( release.mArtist.artistId ) )
                        {
                          set_artists.insert( release.mArtist.artistId );

                          TreeIter iter = m_Model_CBE_Artist->append ();
                          (*iter)[m_CBE_Text_ID_Columns.Text] = release.mArtist.artistName;
                          (*iter)[m_CBE_Text_ID_Columns.ID]   = release.mArtist.artistId;

                        }
                      }

                      m_cbeArtist->set_active (0);
                      m_cbeAlbum->set_active (0);
                }
}
