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
#include <glibmm/i18n.h>

#include "mpx/com/view-albums-filter-plugin.hh"

#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-uri.hh"

#include "mpx/util-string.hh"

#include "xmlcpp/xsd-topalbums-2.0.hxx"
#include "xmlcpp/xsd-artist-similar-2.0.hxx"

#include "mpx/xml/xmltoc++.hh"

#include "mpx/algorithm/aque.hh"

#include "mpx/widgets/widgetloader.hh"

#include <boost/format.hpp>
#include <sigx/sigx.h>


using namespace MPX::AQE;
using namespace Gnome::Glade;
using namespace Gtk;
using boost::get;

namespace MPX
{
    typedef std::set<gint64>        IdSet_t;
    typedef sigc::signal<void>      SignalChanged_t;

    class ArtistListView
    : public Gtk::TreeView
    , public sigx::glib_auto_dispatchable
    {
            class Columns_t : public Gtk::TreeModelColumnRecord
            {
                public:

                    Gtk::TreeModelColumn<std::string>   Name;
                    Gtk::TreeModelColumn<std::string>   SortKey;
                    Gtk::TreeModelColumn<gint64>        ID;

                    Columns_t ()
                    {
                        add(Name);
                        add(SortKey);
                        add(ID);
                    };
            };

            Columns_t                       Columns;
            Glib::RefPtr<Gtk::TreeStore>    Store;
            IdSet_t                         m_FilterIDs;
            SignalChanged_t                 signal_changed_;
    
            typedef std::map<gint64, TreeIter>  IdIterMap_t;

            IdIterMap_t m_IdIterMap;

        public:

            SignalChanged_t&
            signal_changed(
            )
            {
                return signal_changed_;
            }

            const IdSet_t&
            get_filter_ids(
            )
            {
                return m_FilterIDs;
            }

            ArtistListView()
            {
                Store = Gtk::TreeStore::create(Columns);
                Store->set_default_sort_func(
                    sigc::mem_fun(
                        *this,
                        &ArtistListView::sort
                ));

                append_column(_("Name"), Columns.Name);

                MPX::SQL::RowV v;
                boost::shared_ptr<Library> library = services->get<Library>("mpx-service-library");

                library->signal_new_artist().connect(
                    sigc::mem_fun(
                          *this
                        , &ArtistListView::on_new_artist
                ));

                library->scanner()->signal_entity_deleted().connect(
                    sigc::mem_fun(
                          *this
                        , &ArtistListView::on_entity_deleted
                ));

                library->getSQL(
                      v
                    , "SELECT id, album_artist, album_artist_sortname, mb_album_artist_id FROM album_artist"
                );

                TreeIter iter = Store->append();
                (*iter)[Columns.Name] = "ALL"; 

                for( MPX::SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                {
                    TreeIter iter = Store->append();

                    (*iter)[Columns.Name] = (*i).count("album_artist_sortname")
                                                ? get<std::string>((*i)["album_artist_sortname"])
                                                : get<std::string>((*i)["album_artist"]);
                    (*iter)[Columns.SortKey] = Glib::ustring((*iter)[Columns.Name]).collate_key();
                    (*iter)[Columns.ID] = get<gint64>((*i)["id"]);

                    m_IdIterMap.insert(std::make_pair( get<gint64>((*i)["id"]), iter));
                }

                set_model(Store);

                get_selection()->signal_changed().connect(
                    sigc::mem_fun(
                        *this,
                        &ArtistListView::on_selection_changed
                ));
            }

            int
            sort(
                  const TreeIter& iter_a
                , const TreeIter& iter_b
            )
            {
                return std::string((*iter_a)[Columns.SortKey]).compare(std::string((*iter_b)[Columns.SortKey]));
            }

            void
            on_new_artist(
                gint64 id
            )
            {
                MPX::SQL::RowV v;
                boost::shared_ptr<Library> library = services->get<Library>("mpx-service-library");

                library->getSQL(
                      v
                    , (boost::format("SELECT album_artist, album_artist_sortname, mb_album_artist_id FROM album_artist WHERE id = '%lld'") % id).str()
                );

                TreeIter iter = Store->append();

                (*iter)[Columns.Name] = v[0].count("album_artist_sortname")
                                            ? get<std::string>(v[0]["album_artist_sortname"])
                                            : get<std::string>(v[0]["album_artist"]);
                (*iter)[Columns.SortKey] = Glib::ustring((*iter)[Columns.Name]).collate_key();
                (*iter)[Columns.ID] = get<gint64>(v[0]["id"]);

                m_IdIterMap.insert(std::make_pair( get<gint64>(v[0]["id"]), iter));
            }

            void
            on_entity_deleted(
                  gint64     id
                , EntityType type
            )
            {
                if( type == ENTITY_ALBUM_ARTIST && m_IdIterMap.count(id))
                {
                    m_IdIterMap.erase(id); 
                    on_selection_changed();
                }
            }

            void
            on_selection_changed()
            {
                if( !get_selection()->count_selected_rows())
                {
                    m_FilterIDs.clear();
                    return;
                }

                TreeIter iter = get_selection()->get_selected();
                if( iter == Store->children().begin() )
                {
                    m_FilterIDs.clear();
                }
                else
                {
                    m_FilterIDs.clear();
                    m_FilterIDs.insert( (*iter)[Columns.ID] );
                }

                signal_changed_.emit();
            }
    };
}

namespace MPX
{
    namespace ViewAlbumsFilterPlugin
    {
            TextMatch::TextMatch(
                          Glib::RefPtr<Gtk::TreeStore>&     store
                        , ViewAlbumsColumns_t&                columns
            )
            : Base( store, columns )
            {
                m_UI = manage( new Gtk::VBox );
                m_Advanced_CB = manage( new Gtk::CheckButton( _("Advanced Query")));
                m_Advanced_CB->signal_toggled().connect(
                    sigc::mem_fun(
                            *this,
                            &TextMatch::on_advanced_toggled
                ));

                Gtk::ScrolledWindow * sw = manage( new Gtk::ScrolledWindow );

                m_ArtistListView = new ArtistListView; 
                m_ArtistListView->set_rules_hint();
                m_ArtistListView->signal_changed().connect(
                    sigc::mem_fun(
                        *this,
                        &TextMatch::on_artist_filter_changed
                ));

                sw->add( *m_ArtistListView );
                sw->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
                sw->set_size_request( -1, 180 );

                m_UI->pack_start( *m_Advanced_CB, false, false );
                m_UI->pack_start( *sw );
                m_UI->show_all();
            }

            TextMatch::~TextMatch ()
            {
            }

            Gtk::Widget*
            TextMatch::get_ui ()
            {
                return m_UI;
            }

            void
            TextMatch::on_artist_filter_changed(
            )
            {
                const IdSet_t& s = m_ArtistListView->get_filter_ids();

                for( TreeNodeChildren::iterator i = Store->children().begin(); i != Store->children().end(); ++i )
                {
                    (*i)[Columns.Visible] = s.empty();
                }

                if( !s.empty())
                {
                        for( TreeNodeChildren::iterator i = Store->children().begin(); i != Store->children().end(); ++i )
                        {
                            if( s.count( (*i)[Columns.AlbumArtistId] ))
                            {
                                (*i)[Columns.Visible] = true;
                            }
                        }
                }

                Signals.Refilter.emit();
            }

            void
            TextMatch::on_advanced_toggled ()
            {
                if( m_Advanced_CB->get_active() )
                {
                    m_Constraints.clear();
                    m_FilterEffective = AQE::parse_advanced_query( m_Constraints, m_FilterText ); 
                }
                else
                {
                    m_FilterEffective = m_FilterText;
                }

                Signals.Refilter.emit();
            }

            void
            TextMatch::on_filter_issued( const Glib::ustring& G_GNUC_UNUSED )
            {
                // for textmach, we don't need to do anything
            }

            void
            TextMatch::on_filter_changed( const Glib::ustring& text )
            {
                m_FilterText = text;

                if( m_Advanced_CB->get_active() )
                {
                    m_Constraints.clear();
                    m_FilterEffective = AQE::parse_advanced_query( m_Constraints, m_FilterText ); 
                }
                else
                {
                    m_FilterEffective = text;
                }

                Signals.Refilter.emit();
            }

            bool
            TextMatch::filter_delegate(const Gtk::TreeIter& iter)
            {
                if( ! bool((*iter)[Columns.Visible]))
                    return false;

                if( m_FilterEffective.empty() && m_Constraints.empty() ) 
                {
                    return true;
                }
                else
                {
                    bool truth = true;
                    if( !m_Constraints.empty() )
                    {
                        MPX::Track& track = *(Track_sp((*iter)[Columns.AlbumTrack]));
                        truth = AQE::match_track( m_Constraints, track );
                    }

                    return truth && Util::match_keys( *(ustring_sp((*iter)[Columns.Text])), m_FilterEffective ); 
                }
            }
    }
} // end namespace MPX 

namespace MPX
{
    namespace ViewAlbumsFilterPlugin
    {
            LFMTopAlbums::LFMTopAlbums(
                          Glib::RefPtr<Gtk::TreeStore>&     store
                        , ViewAlbumsColumns_t&                columns
            )
            : Base( store, columns )
            {
            }

            LFMTopAlbums::~LFMTopAlbums ()
            {
            }

            Gtk::Widget*
            LFMTopAlbums::get_ui ()
            {
                return 0;
            }

            void
            LFMTopAlbums::on_filter_issued( const Glib::ustring& text )
            {
                m_FilterText = text;
                m_Names.clear();

                if( !m_FilterText.empty() )
                {
                        try{
                                URI u ((boost::format ("http://ws.audioscrobbler.com/2.0/?method=tag.gettopalbums&tag=%s&api_key=37cd50ae88b85b764b72bb4fe4041fe4") % m_FilterText.c_str()).str(), true);
                                MPX::XmlInstance<lfm> * Xml = new MPX::XmlInstance<lfm>(Glib::ustring(u));

                                for( topalbums::album_sequence::const_iterator i = Xml->xml().topalbums().album().begin(); i != Xml->xml().topalbums().album().end(); ++i )
                                {
                                    m_Names.insert( AlbumQualifier_t((*i).name(),(*i).artist().name()));
                                }

                                delete Xml;
                        }
                        catch( ... ) {
                                g_message("Exception!");
                        }
                }

                Signals.Refilter.emit();
            }

            void
            LFMTopAlbums::on_filter_changed( const Glib::ustring& G_GNUC_UNUSED )
            {
            }

            bool
            LFMTopAlbums::filter_delegate(const Gtk::TreeIter& iter)
            {
                return m_FilterText.empty() || m_Names.count( AlbumQualifier_t((*iter)[Columns.Album], (*iter)[Columns.AlbumArtist]));
            }
    }
} // end namespace MPX 

namespace MPX
{
    namespace ViewAlbumsFilterPlugin
    {
            LFMSimilarArtists::LFMSimilarArtists(
                          Glib::RefPtr<Gtk::TreeStore>&     store
                        , ViewAlbumsColumns_t&              columns
            )
            : Base( store, columns )
            {
            }

            LFMSimilarArtists::~LFMSimilarArtists ()
            {
            }

            Gtk::Widget*
            LFMSimilarArtists::get_ui ()
            {
                return 0;
            }

            void
            LFMSimilarArtists::on_filter_issued( const Glib::ustring& text )
            {
                m_FilterText = text;
                m_Names.clear();

                if( !m_FilterText.empty() )
                {
                        try{
                                URI u ((boost::format ("http://ws.audioscrobbler.com/2.0/?method=artist.getsimilar&artist=%s&api_key=37cd50ae88b85b764b72bb4fe4041fe4") % m_FilterText.c_str()).str(), true);
                                MPX::XmlInstance<LastFM_SimilarArtists::lfm> * Xml = new MPX::XmlInstance<LastFM_SimilarArtists::lfm>(Glib::ustring(u));

                                for( LastFM_SimilarArtists::similarartists::artist_sequence::const_iterator i = Xml->xml().similarartists().artist().begin(); i != Xml->xml().similarartists().artist().end(); ++i )
                                {
                                    m_Names.insert( (*i).name() );
                                }

                                delete Xml;
                        }
                        catch( ... ) {
                                g_message("Exception!");
                        }
                }

                Signals.Refilter.emit();
            }

            void
            LFMSimilarArtists::on_filter_changed( const Glib::ustring& G_GNUC_UNUSED )
            {
            }

            bool
            LFMSimilarArtists::filter_delegate(const Gtk::TreeIter& iter)
            {
                return m_FilterText.empty() || m_Names.count( (*iter)[Columns.AlbumArtist] );
            }
    }
} // end namespace MPX 
