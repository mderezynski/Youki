#ifndef MPX_LISTVIEW_HH
#define MPX_LISTVIEW_HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/format.hpp>
#include "mpx/aux/glibaddons.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/algorithm/aque.hh"

#include "glib-marshalers.h"

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace MPX
{
        typedef boost::tuple<std::string, std::string, std::string, gint64, MPX::Track> Row5;
        typedef std::vector<Row5>                                                       ModelT;
        typedef boost::shared_ptr<ModelT>                                               ModelP;
        typedef sigc::signal<void>                                                      Signal0;
        typedef std::map<gint64, ModelT::iterator>                                      IdIterMap;

        struct DataModel
        {
                ModelP          m_realmodel;
                Signal0         m_changed;
                IdIterMap       m_iter_map;

                DataModel()
                {
                    m_realmodel = ModelP(new ModelT); 
                }

                DataModel(ModelP model)
                {
                    m_realmodel = model; 
                }


                virtual Signal0&
                signal_changed ()
                {
                    return m_changed;
                }

                virtual bool
                is_set ()
                {
                    return bool(m_realmodel);
                }

                virtual int
                size ()
                {
                    return m_realmodel->size();
                }

                virtual Row5&
                row (int row)
                {
                    return (*m_realmodel)[row];
                }

                virtual void
                append_track(SQL::Row & r, const MPX::Track& track)
                {
                    using boost::get;

                    std::string artist, album, title;
                    gint64 id = 0;

                    if(r.count("id"))
                    { 
                        id = get<gint64>(r["id"]); 
                    }
                    else
                        g_critical("%s: No id for track, extremely suspicious", G_STRLOC);

                    if(r.count("artist"))
                        artist = get<std::string>(r["artist"]); 
                    if(r.count("album"))
                        album = get<std::string>(r["album"]); 
                    if(r.count("title"))
                        title = get<std::string>(r["title"]);

                    Row5 row (title, artist, album, id, track);
                    m_realmodel->push_back(row);

                    ModelT::iterator i = m_realmodel->end();
                    std::advance( i, -1 );
                    m_iter_map.insert(std::make_pair(id, i)); 
                }

                virtual void
                append_track(MPX::Track & track)
                {
                    using boost::get;

                    std::string artist, album, title;
                    gint64 id = 0;

                    if(track[ATTRIBUTE_MPX_TRACK_ID])
                        id = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()); 
                    else
                        g_critical("Warning, no id given for track; this is totally wrong and should never happen.");


                    if(track[ATTRIBUTE_ARTIST])
                        artist = get<std::string>(track[ATTRIBUTE_ARTIST].get()); 

                    if(track[ATTRIBUTE_ALBUM])
                        album = get<std::string>(track[ATTRIBUTE_ALBUM].get()); 

                    if(track[ATTRIBUTE_TITLE])
                        title = get<std::string>(track[ATTRIBUTE_TITLE].get()); 

                    Row5 row (artist, album, title, id, track);
                    m_realmodel->push_back(row);

                    ModelT::iterator i = m_realmodel->end();
                    std::advance( i, -1 );
                    m_iter_map.insert(std::make_pair(id, i)); 
                }

                void
                erase_track(gint64 id)
                {
#if 0
                    IdIterMap::iterator i = m_iter_map.find(id);
                    if( i != m_iter_map.end() )
                    {
                        m_realmodel->erase( i->second );
                        m_iter_map.erase( i );
                    }
#endif
                }
        };

        typedef boost::shared_ptr<DataModel> DataModelP;

        struct DataModelFilter : public DataModel
        {
                typedef std::vector<ModelT::iterator> RowRowMapping;

                RowRowMapping           m_mapping;
                std::string             m_filter_full;
                std::string             m_filter_effective;
                AQE::Constraints_t      m_constraints;
                bool                    m_advanced;

                DataModelFilter(DataModelP & model)
                : DataModel(model->m_realmodel)
                , m_advanced(false)
                {
                    regen_mapping ();
                }

                virtual int 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row5&
                row (int row)
                {
                    return *(m_mapping[row]);
                }

                virtual void
                append_track(MPX::Track& track)
                {
                    DataModel::append_track(track);
                    regen_mapping();
                }
                
                void
                erase_track(gint64 id)
                {
                    DataModel::erase_track(id);
                    regen_mapping();
                }

                virtual void
                set_filter(std::string const& filter)
                { 
                    if(!m_filter_full.empty() && (filter.substr(0, filter.size()-1) == m_filter_full) && !m_advanced)
                    {
                        m_filter_full = filter; 
                        m_filter_effective = filter;
                        regen_mapping_iterative();
                    }
                    else
                    {
                        m_filter_full = filter; 

                        if( m_advanced )
                        {
                            m_constraints.clear();
                            m_filter_effective = AQE::parse_advanced_query(m_constraints, filter);
                        }
                        else
                        {
                            m_filter_effective = m_filter_full;
                        }

                        regen_mapping();
                    }
                }

                virtual void
                set_advanced (bool advanced)
                {
                    m_advanced = advanced;
                    set_filter(m_filter_full);
                }

                void
                regen_mapping ()
                {
                    using boost::get;

                    m_mapping.clear();
                    
                    for(ModelT::iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i)
                    {
                        Row5 const& row = *i;

                        if( m_filter_effective.empty() && m_constraints.empty() ) 
                        {
                            m_mapping.push_back(i);
                        }
                        else
                        {
                            std::string compound_haystack = get<0>(row) + " " + get<1>(row) + " " + get<2>(row);
                            bool match = Util::match_keys( compound_haystack, m_filter_effective ); 
                            MPX::Track track = get<4>(row);
                            bool truth = AQE::match_track(m_constraints, track);

                            if( match && truth )
                            {
                                m_mapping.push_back(i);
                            }
                        }
                    }

                    m_changed.emit();
                }

                void
                regen_mapping_iterative ()
                {
                    using boost::get;

                    std::string filter = Glib::ustring(m_filter_effective).lowercase().c_str();

                    RowRowMapping new_mapping;
                    for(RowRowMapping::const_iterator i = m_mapping.begin(); i != m_mapping.end(); ++i)
                    {
                        Row5 const& row = *(*i);

                        if( m_filter_effective.empty() && m_constraints.empty() ) 
                        {
                            new_mapping.push_back(*i);
                        }
                        else
                        {
                            std::string compound_haystack = get<0>(row) + " " + get<1>(row) + " " + get<2>(row);
                            bool match = Util::match_keys( compound_haystack, m_filter_effective ); 
                            MPX::Track track = get<4>(row);
                            bool truth = AQE::match_track(m_constraints, track);

                            if( (match || m_filter_effective.empty()) && truth )
                            {
                                new_mapping.push_back(*i);
                            }
                        }
                    }

                    std::swap(m_mapping, new_mapping);
                    m_changed.emit();
                }

        };

        typedef boost::shared_ptr<DataModelFilter> DataModelFilterP;

        class Column
        {
                int         m_width;
                int         m_column;
                std::string m_title;

            public:

                Column (std::string const& title)
                : m_width   (0)
                , m_column  (0)
                , m_title   (title)
                {
                }

                ~Column ()
                {
                }

                void
                set_width (int width)
                {
                    m_width = width; 
                }

                int
                get_width ()
                {
                    return m_width;
                }

                void
                set_column (int column)
                {
                    m_column = column;
                }

                int
                get_column ()
                {
                    return m_column;
                }

                void
                set_title (std::string const& title)
                {
                    m_title = title;
                }

                std::string const&
                get_title ()
                {
                    return m_title;
                }


                void
                render_header(Cairo::RefPtr<Cairo::Context> cairo, Gtk::Widget& widget, int x_pos, int y_pos, int rowheight, int column)
                {
                    using boost::get;

                    /*
                    cairo->set_operator(Cairo::OPERATOR_ATOP);
                    Gdk::Color c = widget.get_style()->get_text(Gtk::STATE_NORMAL);
                    cairo->set_source_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p(), 0.2);
                    cairo->rectangle(x_pos, y_pos, m_width, rowheight);
                    cairo->fill(); 
                    */

                    cairo->rectangle( x_pos + 5, y_pos + 6, m_width+1, rowheight);
                    cairo->clip();

                    cairo->move_to( x_pos + 5, y_pos + 6);
                    cairo->set_operator(Cairo::OPERATOR_ATOP);
                    //Gdk::Cairo::set_source_color(cairo, widget.get_style()->get_text(Gtk::STATE_NORMAL));
                    cairo->set_source_rgba( 1., 1., 1., 1. ) ;
                    Glib::RefPtr<Pango::Layout> layout = widget.create_pango_layout(""); 
                    layout->set_markup((boost::format("<b>%s</b>") % Glib::Markup::escape_text(m_title).c_str()).str());
                    layout->set_ellipsize(Pango::ELLIPSIZE_END);
                    layout->set_width((m_width-10)*PANGO_SCALE);
                    pango_cairo_show_layout(cairo->cobj(), layout->gobj());

                    cairo->reset_clip();
                }

                void
                render(
                    Cairo::RefPtr<Cairo::Context>   cairo,
                    Row5 const&                     datarow,
                    std::string const&              filter,
                    Gtk::Widget&                    widget,
                    int                             row,
                    int                             x_pos,
                    int                             y_pos,
                    int                             rowheight,
                    bool                            selected,
                    bool                            highlight = true
                )
                {
                    using boost::get;

                    cairo->set_operator(Cairo::OPERATOR_ATOP);
                    //Gdk::Cairo::set_source_color(cairo, widget.get_style()->get_text(selected ? Gtk::STATE_SELECTED : Gtk::STATE_NORMAL ));
                    cairo->set_source_rgba( 1., 1., 1., 1. ) ;

                    cairo->rectangle( x_pos, y_pos, m_width, rowheight);
                    cairo->clip();
                    cairo->move_to( x_pos + 6, y_pos + 4);

                    std::string str;
                    switch( m_column )
                    {
                        case 0:
                            str = get<0>(datarow);
                            break;
                        case 1:
                            str = get<1>(datarow);
                            break;
                        case 2:
                            str = get<2>(datarow);
                            break;
                    }

                    Glib::RefPtr<Pango::Layout> layout; 

                    if( highlight )
                    {
                        layout = widget.create_pango_layout("");
                        layout->set_markup( Util::text_match_highlight( str, filter, "#ff3030") );
                    }
                    else
                    {
                        layout = widget.create_pango_layout( str );
                    }

                    layout->set_ellipsize( Pango::ELLIPSIZE_END );
                    layout->set_width( (m_width-8)*PANGO_SCALE );
                    pango_cairo_show_layout( cairo->cobj(), layout->gobj() );

                    cairo->reset_clip();
                }
        };

        typedef boost::shared_ptr<Column> ColumnP;
        typedef std::vector<ColumnP> Columns;
        typedef std::set<std::pair<ModelT::iterator, int> > Selection;

        typedef sigc::signal<void, gint64, bool> SignalTrackActivated;

        class ListView : public Gtk::DrawingArea
        {
                int                                 m_row_height;
                int                                 m_visible_height;
                int                                 m_previousdrawnrow;
                DataModelFilterP                    m_model;
                Columns                             m_columns;
                PropAdj                             m_prop_vadj;
                PropAdj                             m_prop_hadj;
                guint                               m_signal0; 
                Selection                           m_selection;
                boost::optional<ModelT::iterator>   m_selected;
                GtkWidget                         * m_treeview;
                IdV                                 m_dnd_idv;
                bool                                m_dnd;
                int                                 m_click_row_1;
                int                                 m_sel_size_was;
                bool                                m_highlight;

                SignalTrackActivated                m_trackactivated;

                void
                initialize_metrics ()
                {
                    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (gobj()));

                    PangoFontMetrics *metrics = pango_context_get_metrics (context,
                                                                            GTK_WIDGET (gobj())->style->font_desc, 
                                                                            pango_context_get_language (context));

                    m_row_height = (pango_font_metrics_get_ascent (metrics)/PANGO_SCALE) + 
                                   (pango_font_metrics_get_descent (metrics)/PANGO_SCALE) + 8;
                }

                void
                on_vadj_value_changed ()
                {
                    int row = (double(m_prop_vadj.get_value()->get_value()-m_row_height) / double(m_row_height));
                    if( m_previousdrawnrow != row )
                    {
                        queue_draw ();
                    }
                }

                int
                get_upper_row ()
                {
                    int row_upper = (m_prop_vadj.get_value()->get_value() / m_row_height); 
                    return row_upper;
                }

                bool
                get_row_is_visible (int row)
                {
                    int row_upper = (m_prop_vadj.get_value()->get_value() / m_row_height); 
                    int row_lower = row_upper + m_visible_height/m_row_height;
                    return ( row >= row_upper && row <= row_lower);
                }

            protected:

                virtual void 
                on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context)
                {
                    if(m_selection.empty())
                    {
                        m_selection.insert(std::make_pair(m_model->m_mapping[m_click_row_1], m_click_row_1));
                        queue_draw ();
                    }

                    m_dnd = true;
                }

                virtual void
                on_drag_end (const Glib::RefPtr<Gdk::DragContext>& context)
                {
                    m_dnd = false;
                } 

                virtual void
                on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time)
                {
                    using boost::get;

                    if(m_selection.empty())
                        return;

                    m_dnd_idv.clear();

                    for(Selection::const_iterator i = m_selection.begin(); i != m_selection.end(); ++i)
                    {
                        Row5 const& r = *(i->first);
                        m_dnd_idv.push_back(get<3>(r));
                    }

                    selection_data.set("mpx-idvec", 8, reinterpret_cast<const guint8*>(&m_dnd_idv), 8);

                    m_dnd = false;
                }

                virtual bool
                on_focus (Gtk::DirectionType direction)
                { 
                    grab_focus();
                    return true;
                }

                virtual bool
                on_key_press_event (GdkEventKey * event)
                {
                    int step; 

                    switch( event->keyval )
                    {
                        case GDK_Return:
                        case GDK_KP_Enter:
                        case GDK_ISO_Enter:
                        case GDK_3270_Enter:
                            if( m_selection.size() == 1 )
                            {
                                using boost::get;

                                Row5 const& r = *(m_selection.begin()->first);
                                gint64 id = get<3>(r);
                                bool play = (event->state & GDK_CONTROL_MASK);
                                m_trackactivated.emit(id, !play);
                            }
                            return true;

                        case GDK_Up:
                        case GDK_KP_Up:
                        case GDK_Page_Up:

                            if( event->keyval == GDK_Page_Up )
                            {
                                step = - (m_visible_height / m_row_height) + 1;
                            }
                            else
                            {
                                step = -1;
                            }

                            if( m_selection.size() > 1 || m_selection.size() == 0 )
                            {
                                mark_first_row_up:
    
                                int row = get_upper_row();
                                m_selection.clear();
                                m_selection.insert(std::make_pair(m_model->m_mapping[row], row));
                            }
                            else
                            {
                                if( get_row_is_visible( (*(m_selection.begin())).second ))
                                {
                                    ModelT::iterator i = (*(m_selection.begin())).first;
                                    int row = (*(m_selection.begin())).second;

                                    std::advance(i, step);
                                    row += step;

                                    if( row >= 0 )
                                    {
                                        m_selection.clear();
                                        m_selection.insert(std::make_pair(m_model->m_mapping[row], row));

                                        if( row < get_upper_row()) 
                                        {
                                            double value = m_prop_vadj.get_value()->get_value();
                                            value += step*m_row_height;
                                            m_prop_vadj.get_value()->set_value( value );
                                        }
                                    }
                                }
                                else
                                {
                                    goto mark_first_row_up;
                                }
                            }
                            queue_draw();
                            return true;

                        case GDK_Down:
                        case GDK_KP_Down:
                        case GDK_Page_Down:

                            if( event->keyval == GDK_Page_Down )
                            {
                                step = (m_visible_height / m_row_height) - 1;
                            }
                            else
                            {
                                step = 1;
                            }

                            if( m_selection.size() > 1 || m_selection.size() == 0 )
                            {
                                mark_first_row_down:

                                int row = get_upper_row();
                                m_selection.clear();
                                m_selection.insert(std::make_pair(m_model->m_mapping[row], row));
                            }
                            else
                            {
                                if( get_row_is_visible( (*(m_selection.begin())).second ))
                                {
                                    ModelT::iterator i = (*(m_selection.begin())).first;
                                    int row = (*(m_selection.begin())).second;
    
                                    std::advance(i, step);
                                    row += step;

                                    if( row < m_model->m_mapping.size() )
                                    {
                                        m_selection.clear();
                                        m_selection.insert(std::make_pair(m_model->m_mapping[row], row));

                                        if( row >= (get_upper_row() + ((m_visible_height-m_row_height)/m_row_height)))
                                        {
                                            double value = m_prop_vadj.get_value()->get_value();
                                            value += step*m_row_height;
                                            m_prop_vadj.get_value()->set_value( value );
                                        }
                                    }
                                }
                                else
                                {
                                    goto mark_first_row_down;
                                }
                            }
                            queue_draw();
                            return true;
                    }

                    return false;
                }

                bool
                on_button_press_event (GdkEventButton * event)
                {
                    using boost::get;

                    if( event->y < (m_row_height+4))
                    {
                        return true;
                    }
        
                    m_sel_size_was = m_selection.size();

                    if(event->type == GDK_2BUTTON_PRESS)
                    {
                        if( !m_selection.empty() )
                        {
                                Row5 const& r = *(m_selection.begin()->first);
                                gint64 id = get<3>(r);
                                bool play = (event->state & GDK_CONTROL_MASK);
                                m_trackactivated.emit(id, !play);
                        }
                    }
                    else
                    if(event->type == GDK_BUTTON_PRESS)
                    {
                        if(event->state & GDK_SHIFT_MASK)
                        {
                            Selection::iterator i_sel = m_selection.end();
                            i_sel--;
                            int row_p = i_sel->second; 
                            int row_c = get_upper_row() + ((int(event->y)-(m_row_height+2)) / m_row_height);
                            if( row_c < m_model->m_mapping.size() )
                            {
                                    for(int i = row_p+1; i <= row_c; ++i)
                                    {
                                        m_selection.insert(std::make_pair(m_model->m_mapping[i], i));
                                        queue_draw();
                                    }
                            }
                        }
                        else
                        {
                            int row = get_upper_row() + ((int(event->y)-(m_row_height+2)) / m_row_height);
                            if( row < m_model->m_mapping.size() )
                            {
                                    m_click_row_1 = row;

                                    if( event->state & GDK_CONTROL_MASK)
                                    {
                                        m_sel_size_was = 1; // hack
                                        m_selection.insert(std::make_pair(m_model->m_mapping[m_click_row_1], m_click_row_1));
                                        queue_draw();
                                    }
                                    else
                                    if( m_selection.size() <= 1)
                                    {
                                        m_selection.clear();
                                        m_selection.insert(std::make_pair(m_model->m_mapping[m_click_row_1], m_click_row_1));
                                        queue_draw();
                                    }
                            }
                        }
                    }
                
                    return true;
                }

                bool
                on_button_release_event (GdkEventButton * event)
                {
                    using boost::get;

                    if( event->y < (m_row_height+4))
                    {
                        return false;
                    }
        
                    if(event->type == GDK_BUTTON_RELEASE) 
                    {
                        if( m_dnd )
                        {
                            return false;
                        }

                        if( m_sel_size_was > 1 )
                        {
                            if( event->state & GDK_CONTROL_MASK )
                            {
                                m_selection.insert(std::make_pair(m_model->m_mapping[m_click_row_1], m_click_row_1));
                                queue_draw ();
                            }
                            if(event->state & GDK_SHIFT_MASK)
                            {
                                Selection::iterator i_sel = m_selection.end();
                                i_sel--;
                                int row_p = i_sel->second; 
                                int row_c = get_upper_row() + ((int(event->y)-(m_row_height+2)) / m_row_height);
                                if( row_c < m_model->m_mapping.size() )
                                {
                                        for(int i = row_p+1; i <= row_c; ++i)
                                        {
                                            m_selection.insert(std::make_pair(m_model->m_mapping[i], i));
                                            queue_draw();
                                        }
                                }
                            }
                            else
                            {
                                m_selection.clear();
                                m_selection.insert(std::make_pair(m_model->m_mapping[m_click_row_1], m_click_row_1));
                                queue_draw ();
                            }
                        }
                    }
                
                    return false;
                }


                bool
                on_configure_event (GdkEventConfigure * event)        
                {
                    m_prop_vadj.get_value()->set_page_size(event->height); 
                    m_prop_vadj.get_value()->set_upper((m_model->size()) * m_row_height);

                    m_visible_height = event->height;

                    double columnwidth = (double(event->width) - 16) / double(m_columns.size());

                    for(int n = 0; n < m_columns.size(); ++n)
                    {
                        m_columns[n]->set_width(columnwidth);
                    }
                    return false;
                }

                bool
                on_expose_event (GdkEventExpose *event)
                {
                    Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context(); 

                    cairo->set_operator(Cairo::OPERATOR_SOURCE);
                    //Gdk::Cairo::set_source_color(cairo, get_style()->get_base(Gtk::STATE_NORMAL));
                    cairo->set_source_rgba( 0.12, 0.12, 0.12, 1. ) ;
                    cairo->paint();

                    Gtk::Allocation const& alloc = get_allocation();

                    int row = get_upper_row() ;
                    m_previousdrawnrow = row;

                    int y_pos       = m_row_height + 2;
                    int x_pos       = 16;
                    int col         = 0;
                    int cnt         = (m_visible_height - (m_row_height + 2)) / m_row_height;
                    
                    for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i, ++col)
                    {
                        (*i)->render_header(cairo, *this, x_pos, 0, m_row_height+2, col);
                        x_pos += (*i)->get_width() + 1;
                    }

                    while(m_model->is_set() && cnt && (row < m_model->m_mapping.size())) 
                    {
                        x_pos = 16;

                        if( !(row%2) ) 
                        {
                            GdkRectangle background_area;
                            background_area.x = 0;
                            background_area.y = y_pos;
                            background_area.width = alloc.get_width();
                            background_area.height = m_row_height;

                            cairo->set_operator(Cairo::OPERATOR_ATOP);
                            cairo->rectangle( background_area.x, background_area.y, background_area.width, background_area.height ) ;
                            cairo->set_source_rgba( 0.2, 0.2, 0.2, 1. ) ;
                            cairo->fill() ;
                        }


                        ModelT::iterator selected = m_model->m_mapping[row];
                        bool iter_is_selected = false;

                        if( !m_selection.empty() && m_selection.count(std::make_pair(selected, row))) 
                        {
                            cairo->set_operator(Cairo::OPERATOR_ATOP);
                            Gdk::Color c = get_style()->get_base(Gtk::STATE_SELECTED);
                            cairo->set_source_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p(), 0.8);
                            RoundedRectangle (cairo, 1, y_pos+1, alloc.get_width()-2, m_row_height-2., 1.);
                            cairo->fill_preserve(); 
                            cairo->set_source_rgb(c.get_red_p(), c.get_green_p(), c.get_blue_p());
                            cairo->stroke();
                            iter_is_selected = true;
                        }

                        for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
                        {
                            (*i)->render(cairo, m_model->row(row), m_model->m_filter_effective, *this, row, x_pos, y_pos, m_row_height, iter_is_selected, m_highlight);
                            x_pos += (*i)->get_width();
                        }

                        y_pos += m_row_height;
                        row ++;
                        cnt --;
                    }

                    if( has_focus() )
                    {
                        get_style()->paint_focus(
                            get_window(),
                            Gtk::STATE_NORMAL,
                            get_allocation(),
                            *this,
                            "treeview",
                            0, 
                            0, 
                            get_allocation().get_width(), 
                            get_allocation().get_height()
                        );
                    }

                    return true;
                }

                void
                on_model_changed()
                {
                    int view_count = m_visible_height / m_row_height;
                    if( m_model->size() < view_count )
                    {
                        m_prop_vadj.get_value()->set_value(0.);
                    } 
                    m_selection.clear();
                    m_prop_vadj.get_value()->set_upper((m_model->size()) * m_row_height);
                    m_prop_vadj.get_value()->set_value(0.);
                    queue_draw();
                }

                static gboolean
                list_view_set_adjustments(
                    GtkWidget*obj,
                    GtkAdjustment*hadj,
                    GtkAdjustment*vadj, 
                    gpointer data
                )
                {
                    g_object_set(G_OBJECT(obj), "vadjustment", vadj, NULL); 
                    g_object_set(G_OBJECT(obj), "hadjustment", hadj, NULL);
                    g_object_set(G_OBJECT(vadj), "page-size", 0.05, "upper", 1.0, NULL);

                    ListView & view = *(reinterpret_cast<ListView*>(data));

                    view.m_prop_vadj.get_value()->signal_value_changed().connect(
                        sigc::mem_fun(
                            view,
                            &ListView::on_vadj_value_changed
                    ));

                    return TRUE;
                }

            public:
    
                void
                set_highlight(bool highlight)
                {
                    m_highlight = highlight;
                    queue_draw ();
                }

                void
                set_model(DataModelFilterP model)
                {
                    m_model = model;
                    set_size_request(200, 8 * m_row_height);
                    m_model->signal_changed().connect(
                        sigc::mem_fun(
                            *this,
                            &ListView::on_model_changed
                    ));
                }

                void
                append_column (ColumnP column)
                {
                    m_columns.push_back(column);
                }

                SignalTrackActivated&
                signal_track_activated()
                {
                    return m_trackactivated;
                }

                void
                set_advanced (bool advanced)
                {
                    m_model->set_advanced(advanced);
                }

                ListView ()
                : ObjectBase    ("MPXListView")
                , m_previousdrawnrow(0)
                , m_prop_vadj   (*this, "vadjustment", (Gtk::Adjustment*)(0))
                , m_prop_hadj   (*this, "hadjustment", (Gtk::Adjustment*)(0))
                , m_dnd(false)
                , m_click_row_1(0)
                , m_sel_size_was(0)
                , m_highlight(true)
                {
                    m_treeview = gtk_tree_view_new();
                    gtk_widget_realize(GTK_WIDGET(m_treeview));

                    set_flags(Gtk::CAN_FOCUS);
                    add_events(Gdk::EventMask(GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK));

                    ((GtkWidgetClass*)(G_OBJECT_GET_CLASS(G_OBJECT(gobj()))))->set_scroll_adjustments_signal = 
                            g_signal_new ("set_scroll_adjustments",
                                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                      GSignalFlags (G_SIGNAL_RUN_FIRST),
                                      0,
                                      NULL, NULL,
                                      g_cclosure_user_marshal_VOID__OBJECT_OBJECT, G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

                    g_signal_connect(G_OBJECT(gobj()), "set_scroll_adjustments", G_CALLBACK(list_view_set_adjustments), this);

                    gtk_widget_realize(GTK_WIDGET(gobj()));
                    initialize_metrics();

                    std::vector<Gtk::TargetEntry> Entries;
                    Entries.push_back(Gtk::TargetEntry("mpx-track", Gtk::TARGET_SAME_APP, 0x81));
                    Entries.push_back(Gtk::TargetEntry("mpx-idvec", Gtk::TARGET_SAME_APP, 0x82));
                    drag_source_set(Entries); 
                }

                ~ListView ()
                {
                }
        };
}

#endif
