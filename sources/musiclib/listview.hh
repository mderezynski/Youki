#ifndef MPX_LISTVIEW_HH
#define MPX_LISTVIEW_HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include "mpx/aux/glibaddons.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/cairoextensions.hh"
#include "glib-marshalers.h"

typedef Glib::Property<Gtk::Adjustment*> PropAdj;

namespace MPX
{
        typedef boost::tuple<std::string, std::string, std::string, gint64> Row4;
        typedef std::vector<std::string>                                    Row1;
        typedef std::vector<Row4>                                           ModelT;
        typedef boost::shared_ptr<ModelT>                                   ModelP;
        typedef sigc::signal<void>                                          Signal0;

        struct DataModel
        {
                ModelP          m_realmodel;
                Signal0         m_changed;

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

                virtual Row4 const&
                row (int row)
                {
                    return (*m_realmodel)[row];
                }

                virtual void
                append_track(SQL::Row & r)
                {
                    using boost::get;

                    std::string artist, album, title;
                    gint64 id = 0;

                    if(r.count("id"))
                        id = get<gint64>(r["id"]); 
                    else
                        g_critical("%s: No id for track, extremeley suspicious", G_STRLOC);

                    if(r.count("artist"))
                        artist = get<std::string>(r["artist"]); 
                    if(r.count("album"))
                        album = get<std::string>(r["album"]); 
                    if(r.count("title"))
                        title = get<std::string>(r["title"]);

                    Row4 row (title, artist, album, id);
                    m_realmodel->push_back(row);
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

                    Row4 row (artist, album, title, id);
                    m_realmodel->push_back(row);
                }
        };

        typedef boost::shared_ptr<DataModel> DataModelP;

        struct DataModelFilter : public DataModel
        {
                typedef std::vector<ModelT::iterator> RowRowMapping;

                RowRowMapping   m_mapping;
                std::string     m_filter;

                DataModelFilter(DataModelP & model)
                : DataModel(model->m_realmodel)
                {
                    regen_mapping ();
                }

                virtual int 
                size ()
                {
                    return m_mapping.size();
                }

                virtual Row4 const&
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
                
                virtual void
                set_filter(std::string const& filter)
                {
                    if(!m_filter.empty() && filter.substr(0, filter.size()-1) == m_filter)
                    {
                        m_filter = filter; 
                        regen_mapping_iterative();
                    }
                    else
                    {
                        m_filter = filter; 
                        regen_mapping();
                    }
                }

                void
                regen_mapping ()
                {
                    using boost::get;

                    m_mapping.clear();

                    std::string filter = Glib::ustring(m_filter).lowercase().c_str();
                    
                    for(ModelT::iterator i = m_realmodel->begin(); i != m_realmodel->end(); ++i)
                    {
                        Row4 const& row = *i;

                        std::string compound_haystack = get<0>(row) + " " + get<1>(row) + " " + get<2>(row);
                    
                        if( m_filter.empty() || Util::match_keys( compound_haystack, m_filter )) 
                        {
                            m_mapping.push_back(i);
                        }
                    }

                    m_changed.emit();
                }

                void
                regen_mapping_iterative ()
                {
                    using boost::get;

                    RowRowMapping new_mapping;

                    std::string filter = Glib::ustring(m_filter).lowercase().c_str();

                    for(RowRowMapping::const_iterator i = m_mapping.begin(); i != m_mapping.end(); ++i)
                    {
                        Row4 const& row = *(*i);

                        std::string compound_haystack = get<0>(row) + " " + get<1>(row) + " " + get<2>(row);
                   
                        if( m_filter.empty() || Util::match_keys( compound_haystack, m_filter )) 
                        {
                            new_mapping.push_back(*i);
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

                    Gdk::Color c = widget.get_style()->get_text(Gtk::STATE_NORMAL);
                    cairo->set_source_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p(), 0.2);
                    //RoundedRectangle(cairo, x_pos+2, y_pos+2, m_width-4, rowheight-2, 4.);
                    cairo->rectangle(x_pos, y_pos, m_width, rowheight);
                    cairo->fill(); 

                    cairo->rectangle( x_pos + 6, y_pos + 6, m_width, rowheight);
                    cairo->clip();

                    cairo->move_to( x_pos + 6, y_pos + 6);
                    cairo->set_operator(Cairo::OPERATOR_SOURCE);
                    Gdk::Cairo::set_source_color(cairo, widget.get_style()->get_text(Gtk::STATE_NORMAL));
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
                    Row4 const&                     datarow,
                    std::string const&              filter,
                    Gtk::Widget&                    widget,
                    int                             row,
                    int                             x_pos,
                    int                             y_pos,
                    int                             rowheight,
                    bool                            highlight
                )
                {
                    using boost::get;

                    cairo->set_operator(Cairo::OPERATOR_SOURCE);
                    Gdk::Cairo::set_source_color(cairo, widget.get_style()->get_text(Gtk::STATE_NORMAL));

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
                            using namespace boost::algorithm;

                            typedef boost::iterator_range<std::string::iterator>    Range;
                            typedef std::vector< std::string >                      SplitVectorType;
                            typedef std::map<std::string::size_type, int>           IndexSet; 
                            typedef std::list<Range>                                Results;

                            SplitVectorType split_vec; // #2: Search for tokens
                            boost::algorithm::split( split_vec, filter, boost::algorithm::is_any_of(" ") );

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
                                        output += "<span color='#ffff80'>" + Glib::Markup::escape_text(str).raw() + "</span>";
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
                                                        output += "<span color='#ffff80'>";
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

                                    layout = widget.create_pango_layout("");
                                    layout->set_markup(output);
                            }
                            else
                            {
                                layout = widget.create_pango_layout(str);
                            }
                    }
                    else
                    {
                        layout = widget.create_pango_layout(str);
                    }

                    layout->set_ellipsize(Pango::ELLIPSIZE_END);
                    layout->set_width((m_width-8)*PANGO_SCALE);
                    pango_cairo_show_layout(cairo->cobj(), layout->gobj());

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
                        Row4 const& r = *(i->first);
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

                                Row4 const& r = *(m_selection.begin()->first);
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
                                        m_selection.insert(std::make_pair(i, row));

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
                                        m_selection.insert(std::make_pair(i, row));

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
                        return false;
                    }
        
                    m_sel_size_was = m_selection.size();

                    if(event->type == GDK_2BUTTON_PRESS)
                    {
                        if( !m_selection.empty() )
                        {
                                Row4 const& r = *(m_selection.begin()->first);
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
                
                    return false;
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

                    double columnwidth = double(event->width) / double(m_columns.size());

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
                    Gdk::Cairo::set_source_color(cairo, get_style()->get_base(Gtk::STATE_NORMAL));
                    cairo->paint();

                    Gtk::Allocation const& alloc = get_allocation();

                    int row = get_upper_row() ;
                    m_previousdrawnrow = row;

                    int y_pos       = m_row_height + 2;
                    int x_pos       = 0;
                    int col         = 0;
                    int cnt         = (m_visible_height - (m_row_height + 2)) / m_row_height;
                    
                    for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i, ++col)
                    {
                        (*i)->render_header(cairo, *this, x_pos, 0, m_row_height+2, col);
                        x_pos += (*i)->get_width() + 1;
                    }

                    while(m_model->is_set() && cnt && (row < m_model->m_mapping.size())) 
                    {
                        x_pos = 0;

                        if( row % 2 )
                        {
                          GdkRectangle background_area;
                          background_area.x = 0;
                          background_area.y = y_pos;
                          background_area.width = alloc.get_width();
                          background_area.height = m_row_height;

                          gtk_paint_flat_box(get_style()->gobj(),
                                  event->window,
                                  GTK_STATE_NORMAL,
                                  GTK_SHADOW_NONE,
                                  &event->area,
                                  m_treeview,
                                  "cell_odd_ruled",
                                  background_area.x,
                                  background_area.y,
                                  background_area.width,
                                  background_area.height);
                        }


                        ModelT::iterator selected = m_model->m_mapping[row];

                        if( !m_selection.empty() && m_selection.count(std::make_pair(selected, row))) 
                        {
                            cairo->set_operator(Cairo::OPERATOR_ATOP);
                            Gdk::Color c = get_style()->get_base(Gtk::STATE_SELECTED);
                            cairo->set_source_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p(), 0.8);
                            //RoundedRectangle(cairo, 2, y_pos+2, alloc.get_width()-4, m_row_height-4, 4.);
                            cairo->rectangle(0, y_pos+2, alloc.get_width(), m_row_height-4.);
                            cairo->fill_preserve(); 
                            cairo->set_source_rgb(c.get_red_p(), c.get_green_p(), c.get_blue_p());
                            cairo->stroke();
                        }

                        for(Columns::const_iterator i = m_columns.begin(); i != m_columns.end(); ++i)
                        {
                            (*i)->render(cairo, m_model->row(row), m_model->m_filter, *this, row, x_pos, y_pos, m_row_height, m_highlight);
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
                            get_allocation().get_x(), 
                            get_allocation().get_y(), 
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

                ListView ()
                : ObjectBase    ("MPXListView")
                , m_previousdrawnrow(0)
                , m_prop_vadj   (*this, "vadjustment", (Gtk::Adjustment*)(0))
                , m_prop_hadj   (*this, "hadjustment", (Gtk::Adjustment*)(0))
                , m_dnd(false)
                , m_click_row_1(0)
                , m_sel_size_was(0)
                , m_highlight(false)
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
                                      psm_glib_marshal_VOID__OBJECT_OBJECT, G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

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
