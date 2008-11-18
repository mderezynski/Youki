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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <glibmm.h>
#include <glibmm/markup.h>
#include <glibmm/i18n.h>
#include <glib.h>
#include <gtk/gtkliststore.h>
#include <gtkmm.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include "mpx/util-string.hh"
using namespace Glib;
using namespace Gtk;

#include "radio-directory-types.hh"
#include "radio-directory-view-base.hh"

namespace MPX
{
    namespace RadioDirectory
    {
        int ViewBase::instance_counter = 0;

        SignalStartStop&
        ViewBase::signal_start ()
        {
            return Signals.Start;
        }

        SignalStartStop&
        ViewBase::signal_stop ()
        {
            return Signals.Stop; 
        }

        void
        ViewBase::set_stream_list (StreamListT const& entries)
        {
            BaseData.Streams->clear ();
            for (StreamListT::const_iterator i = entries.begin() ;  i != entries.end() ; ++i) 
            {
              StreamInfo const& entry (*i);
              TreeModel::iterator m_iter = BaseData.Streams->append ();
              gtk_list_store_set (BaseData.Streams->gobj(), const_cast<GtkTreeIter*>(m_iter->gobj()),
                                  0, entry.name.c_str(),
                                  1, entry.bitrate,
                                  2, entry.genre.c_str(),
                                  3, entry.current.c_str(),
                                  4, entry.uri.c_str(), -1);
              while (gtk_events_pending ()) gtk_main_iteration();
            }
        }

        bool
        ViewBase::visible_func (TreeModel::iterator const& m_iter)
        {
            int bitrate = (*m_iter)[columns.bitrate];

            if( bitrate < BaseData.MinimalBitrate )
            {
                return false;
            }

            if( BaseData.Filter.empty() )
            {
                return true;
            }

            ustring name ( ustring((*m_iter)[columns.name]).casefold()), genre ( ustring((*m_iter)[columns.genre]).casefold()), needle (BaseData.Filter.casefold ());
            return ((Util::match_keys (name, needle)) || (Util::match_keys (genre, needle)));
        }

        void
        ViewBase::set_filter (Glib::ustring const& text)
        {
            BaseData.Filter = text;
            BaseData.Filtered->refilter ();
        }

        void        
        ViewBase::on_bitrate_changed (MCS_CB_DEFAULT_SIGNATURE)
        {
            BaseData.MinimalBitrate = mcs->key_get<int>("radio","minimal-bitrate");
            BaseData.Filtered->refilter ();
        }

        void
        ViewBase::column_clicked (int column)
        {
            SortType sort_type, sort_type_new;
            int           sort_id;

            BaseData.Streams->get_sort_column_id (sort_id, sort_type);

            if ((sort_id >= 0) && (sort_id != column))
            {
              get_column (sort_id)->set_sort_indicator (false);
            }

            if (sort_id >= 0)
              sort_type_new = (sort_type == SORT_ASCENDING) ? SORT_DESCENDING : SORT_ASCENDING;
            else
              sort_type_new = SORT_ASCENDING;

            BaseData.Streams->set_sort_column_id (column, sort_type_new);
            get_column (column)->set_sort_indicator (true);
            get_column (column)->set_sort_order (sort_type_new);
        }

        void 
        ViewBase::get (Glib::ustring & title, Glib::ustring & uri)
        {
            TreeModel::iterator m_iter = get_selection()->get_selected ();
            uri   = (*m_iter)[columns.uri];
            title = (*m_iter)[columns.name];
        }

        void
        ViewBase::cell_data_func_text1 (CellRenderer * basecell, TreeModel::iterator const &iter)
        {
            CellRendererText *cell_p = dynamic_cast<CellRendererText*>(basecell);
            std::string str = (*iter)[columns.name];
            if( BaseData.Highlight )
            {
                cell_text_highlight( cell_p, str );
            }
            else
            {
                cell_p->property_text() = str; 
            }
        }
    
        void
        ViewBase::cell_data_func_text2 (CellRenderer * basecell, TreeModel::iterator const &iter)
        {
            CellRendererText *cell_p = dynamic_cast<CellRendererText*>(basecell);
            std::string str = (*iter)[columns.genre];
            if( BaseData.Highlight )
            {
                cell_text_highlight( cell_p, str );
            }
            else
            {
                cell_p->property_text() = str; 
            }
        }

        void
        ViewBase::cell_data_func_text3 (CellRenderer * basecell, TreeModel::iterator const &iter)
        {
            CellRendererText *cell_p = dynamic_cast<CellRendererText*>(basecell);
            std::string str = (*iter)[columns.current];
            if( BaseData.Highlight )
            {
                cell_text_highlight( cell_p, str );
            }
            else
            {
                cell_p->property_text() = str; 
            }
        }

        void
        ViewBase::cell_text_highlight( CellRendererText *cell_p, std::string& str)
        {
                    using namespace boost::algorithm;

                    typedef boost::iterator_range<std::string::iterator>    Range;
                    typedef std::vector< std::string >                      SplitVectorType;
                    typedef std::map<std::string::size_type, int>           IndexSet; 
                    typedef std::list<Range>                                Results;

                    SplitVectorType split_vec; // #2: Search for tokens
                    boost::algorithm::split( split_vec, BaseData.Highlight.get(), boost::algorithm::is_any_of(" ") );

                    std::sort( split_vec.begin(), split_vec.end() );
                    std::reverse( split_vec.begin(), split_vec.end());

                    IndexSet i_begin;
                    IndexSet i_end;

                    for( SplitVectorType::const_iterator i = split_vec.begin(); i != split_vec.end(); ++i )
                    {
                        Results x; 
                        ifind_all(x, str, *i);

                        for(Results::const_iterator i = x.begin(); i != x.end(); ++i )
                        {
                            i_begin[std::distance(str.begin(), (*i).begin())] ++;
                            i_end[std::distance(str.begin(), (*i).end())] ++;
                        }
                    }

                    if( ! i_begin.empty() )
                    {
                            std::string output;
                            output.reserve(1024);

                            if( i_begin.size() == 1 && (*(i_begin.begin())).first == 0 && (*(i_end.begin())).first == str.size() )    
                            {
                                output += "<span color='#ff3030'>" + Glib::Markup::escape_text(str).raw() + "</span>";
                            }
                            else
                            {
                                    std::string chunk;
                                    chunk.reserve(1024);
                                    int c_open = 0;
                                    int c_close = 0;

                                    for(std::string::iterator i = str.begin(); i != str.end(); ++i)
                                    {
                                        std::string::size_type idx = std::distance(str.begin(), i);

                                        if( i_begin.count(idx) && i_end.count(idx) )
                                        {
                                            /* do nothing */
                                        }
                                        else
                                        if( i_begin.count(idx) )
                                        {
                                            int c_open_prev = c_open;
                                            c_open += (*(i_begin.find(idx))).second;
                                            if( !c_open_prev && c_open >= 1 )
                                            {
                                                output += Glib::Markup::escape_text(chunk).raw();
                                                chunk.clear();
                                                output += "<span color='#ff3030'>";
                                            }
                                        }
                                        else
                                        if( i_end.count(idx) )
                                        {
                                            c_close += (*(i_end.find(idx))).second;
                                            if( c_close == c_open )
                                            {
                                                output += Glib::Markup::escape_text(chunk).raw();
                                                chunk.clear();
                                                output += "</span>"; 
                                                c_close = 0;
                                                c_open  = 0;
                                            }
                                        }

                                        chunk += *i;
                                    }

                                    output += Glib::Markup::escape_text(chunk).raw();

                                    if( c_open )
                                    {
                                        output += "</span>";
                                    }
                            }

                            cell_p->property_markup() = output; 
                    }
                    else
                    {
                        cell_p->property_text() = str; 
                    }
        }

        void
        ViewBase::highlight_set (const std::string& highlight)
        {
            BaseData.Highlight = highlight;
        }
    
        void
        ViewBase::highlight_clear ()
        {
            BaseData.Highlight.reset();
        }

        ViewBase::ViewBase (BaseObjectType * obj,
                            Glib::RefPtr<Gnome::Glade::Xml> const& G_GNUC_UNUSED) 
        : TreeView (obj)
        {
            BaseData.Streams = ListStore::create (columns);
            BaseData.Filtered = TreeModelFilter::create (BaseData.Streams);
            BaseData.Filtered->set_visible_func (sigc::mem_fun (this, &MPX::RadioDirectory::ViewBase::visible_func));

            const char* column_headers[] =
            {
              N_("Stream Name"),
              N_("Bitrate"),
              N_("Stream Genre"),
              N_("Current Song"),
            };

            CellRendererText *cell;
            cell = manage (new CellRendererText());

            for (unsigned int n = 0; n < G_N_ELEMENTS (column_headers); n++)
            {
              append_column (_(column_headers[n]), *cell);
        
              switch( n ) 
              {
                    case 0:
                        get_column(n)->set_cell_data_func(
                            *cell,
                            sigc::mem_fun(
                                *this,
                                &ViewBase::cell_data_func_text1
                        ));
                        get_column (n)->set_resizable (true);
                        break;
        
                    case 1:
                        get_column (n)->add_attribute (*cell, "text", n);
                        get_column (n)->set_resizable (false);
                        break;

                    case 2:
                        get_column(n)->set_cell_data_func(
                            *cell,
                            sigc::mem_fun(
                                *this,
                                &ViewBase::cell_data_func_text2
                        ));
                        get_column (n)->set_resizable (true);
                        break;

                    case 3:
                        get_column(n)->set_cell_data_func(
                            *cell,
                            sigc::mem_fun(
                                *this,
                                &ViewBase::cell_data_func_text3
                        ));
                        get_column (n)->set_resizable (true);
                        break;
              }

              get_column (n)->signal_clicked ().connect(
                    sigc::bind(
                        sigc::mem_fun(
                            *this,
                            &MPX::RadioDirectory::ViewBase::column_clicked),
                        n
              ));
            }

            get_selection()->set_mode (SELECTION_SINGLE);
            set_headers_clickable (true);
            set_model (BaseData.Filtered);

            BaseData.MinimalBitrate = mcs->key_get<int>("radio","minimal-bitrate");
            mcs->subscribe(
                (boost::format ("RadioStreamsSource%d") % instance_counter++).str(),
                "radio",
                "minimal-bitrate",
                sigc::mem_fun(
                    *this,
                    &ViewBase::on_bitrate_changed
            ));
        }
    }
};
