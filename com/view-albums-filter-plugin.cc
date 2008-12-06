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
#include "mpx/util-string.hh"
#include "mpx/algorithm/aque.hh"

#include "mpx/mpx-uri.hh"
#include "mpx/xml/xmltoc++.hh"
#include "xsd-tag-topalbums.hxx"

#include <boost/format.hpp>

using namespace MPX::AQE;

namespace MPX
{
    namespace ViewAlbumsFilterPlugin
    {
            TextMatch::TextMatch ()
            {
                m_UI = manage( new Gtk::CheckButton( _("Advanced Query")));
                m_UI->signal_toggled().connect(
                    sigc::mem_fun(
                            *this,
                            &TextMatch::on_advanced_toggled
                ));
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
            TextMatch::on_advanced_toggled ()
            {
                if( m_UI->get_active() )
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

                if( m_UI->get_active() )
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
            TextMatch::filter_delegate(const Gtk::TreeIter& iter, const ViewAlbumsColumnsT& columns)
            {
                if( m_FilterEffective.empty() && m_Constraints.empty() ) 
                {
                    return true;
                }
                else
                {
                    bool truth = true;
                    if( !m_Constraints.empty() )
                    {
                        MPX::Track track = (*iter)[columns.AlbumTrack]; 
                        truth = AQE::match_track( m_Constraints, track );
                    }

                    return truth && Util::match_keys( Glib::ustring((*iter)[columns.Text]).lowercase(), m_FilterEffective ); 
                }
            }
    }
} // end namespace MPX 

namespace MPX
{
    namespace ViewAlbumsFilterPlugin
    {
            LFMTopAlbums::LFMTopAlbums ()
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

                try{
                        URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/tag/%s/topalbums.xml") % m_FilterText.c_str()).str(), true);
                        MPX::XmlInstance<LastFM::lfm> * Xml = new MPX::XmlInstance<LastFM::lfm>(Glib::ustring(u));

                        for( LastFM::topalbums::album_sequence::const_iterator i = Xml->xml().topalbums().album().begin(); i != Xml->xml().topalbums().album().end(); ++i )
                        {
                            m_Names.insert( (*i).name() );
                        }

                        delete Xml;
                }
                catch( ... ) {
                }


                Signals.Refilter.emit();
            }

            void
            LFMTopAlbums::on_filter_changed( const Glib::ustring& G_GNUC_UNUSED )
            {
            }

            bool
            LFMTopAlbums::filter_delegate(const Gtk::TreeIter& iter, const ViewAlbumsColumnsT& columns)
            {
                return m_Names.count( (*iter)[columns.Album] );
            }
    }
} // end namespace MPX 
