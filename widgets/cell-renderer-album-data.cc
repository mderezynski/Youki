#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glibmm.h>
#include <gtkmm.h>

#include <boost/format.hpp>

#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/widgets/cell-renderer-album-data.hh"

using namespace Gtk;

namespace
{
    const int XPAD = 3;
    const int YPAD = 2;
}

namespace MPX
{
    CellRendererAlbumData::CellRendererAlbumData ()
    : ObjectBase        (typeid(CellRendererAlbumData))
    , property_info_    (*this, "info", boost::shared_ptr<AlbumInfo>()) 
    {
    }

    CellRendererAlbumData::~CellRendererAlbumData ()
    {
    }

    void
    CellRendererAlbumData::get_size_vfunc  (Gtk::Widget & widget,
                                         const Gdk::Rectangle * cell_area,
                                         int *   x_offset,
                                         int *   y_offset,
                                         int *   width,
                                         int *   height) const 
    {
      if(x_offset)
        *x_offset = 0;

      if(y_offset)
        *y_offset = 0;

      int text_width, text_height;
      Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create( widget.get_pango_context() );
      layout->set_markup( property_info().get_value()->Name );
      layout->get_pixel_size( text_width, text_height );

      if(height)
        *height = 72; 

      if(width)
      {
        *width = text_width +  2 * XPAD;
      }
    }

    void
    CellRendererAlbumData::render_vfunc    (Glib::RefPtr<Gdk::Drawable> const& window,
                                        Gtk::Widget                      & widget,
                                        Gdk::Rectangle              const& background_area,
                                        Gdk::Rectangle              const& cell_area,
                                        Gdk::Rectangle              const& expose_area,
                                        Gtk::CellRendererState             state)
    {
        ::Cairo::RefPtr< ::Cairo::Context> cr = window->create_cairo_context (); 

        int xoff, yoff;
        xoff = cell_area.get_x();
        yoff = cell_area.get_y();

        Glib::RefPtr<Pango::Layout> layout[4];
        int text_width[4], text_height[4];

        // Name
        layout[0] = Pango::Layout::create( widget.get_pango_context() );
        layout[0]->set_text( property_info_.get_value()->Name );

        Pango::AttrList list;
        Pango::Attribute attr = Pango::Attribute::create_attr_scale(1.2);
        list.insert(attr);
        attr = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
        list.insert(attr);
        layout[0]->set_attributes(list);
        layout[0]->get_pixel_size( text_width[0], text_height[0] );

        cr->set_operator( ::Cairo::OPERATOR_SOURCE );
        RoundedRectangle( cr, xoff+1, yoff+5, cell_area.get_width()-2, text_height[0] + 4, 4. );

        if( state & Gtk::CELL_RENDERER_SELECTED )
        {
            Gdk::Cairo::set_source_color( cr, widget.get_style()->get_text(Gtk::STATE_SELECTED) );
            cr->fill ();
        }
        else
        {
            cr->set_source_rgba(.25, .25, .25, 1.);
            cr->fill ();
        }

        cr->move_to(
              xoff + 2 + XPAD
            , yoff + 5 + YPAD
        );

        if( state & Gtk::CELL_RENDERER_SELECTED )
            Gdk::Cairo::set_source_color( cr, widget.get_style()->get_text(Gtk::STATE_NORMAL) );
        else
            cr->set_source_rgba(1., 1., 1., 1.);

        pango_cairo_show_layout( cr->cobj(), layout[0]->gobj() );

        // Artist
        layout[1] = Pango::Layout::create( widget.get_pango_context() );
        layout[1]->set_text( property_info_.get_value()->Artist );

        list = Pango::AttrList();
        attr = Pango::Attribute::create_attr_scale(1.2);
        list.insert(attr);
        attr = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
        list.insert(attr);
        layout[1]->set_attributes(list);

        layout[1]->get_pixel_size( text_width[1], text_height[1] );

        if( state & Gtk::CELL_RENDERER_SELECTED )
            cr->set_source_rgba(1., 1., 1., 1.);
        else
            Gdk::Cairo::set_source_color( cr, widget.get_style()->get_text(Gtk::STATE_NORMAL) );

        cr->move_to(
              xoff + 2 + XPAD
            , yoff + text_height[0] + 4 + 6 
        );

        pango_cairo_show_layout( cr->cobj(), layout[1]->gobj() );

        // Release
        layout[2] = Pango::Layout::create( widget.get_pango_context() );
        layout[2]->set_text( property_info_.get_value()->Release );
        layout[2]->get_pixel_size( text_width[2], text_height[2] );

        cr->move_to(
              xoff + cell_area.get_width() - text_width[2] - 4 
            , yoff + 7 + YPAD
        );

        if( state & Gtk::CELL_RENDERER_SELECTED )
            Gdk::Cairo::set_source_color( cr, widget.get_style()->get_text(Gtk::STATE_NORMAL) );
        else
            cr->set_source_rgba(.9, .9, .9, 1.);

        pango_cairo_show_layout( cr->cobj(), layout[2]->gobj() );

        // Release Type

        AlbumInfo_pt info = property_info_.get_value();      

        std::string release_info;

        if( !info->Genre.empty() )
        {
            if( !info->Type.empty() )
                release_info = (boost::format("%s (%s)") % info->Type % info->Genre).str();
            else
                release_info = (boost::format("(%s)") % info->Genre).str();
        }
        else
            release_info = info->Type;
 
        layout[3] = Pango::Layout::create( widget.get_pango_context() );
        layout[3]->set_text( release_info ); 

        layout[3]->get_pixel_size( text_width[3], text_height[3] );

        if( state & Gtk::CELL_RENDERER_SELECTED )
            cr->set_source_rgba(1., 1., 1., .8);
        else
            Gdk::Cairo::set_source_color( cr, widget.get_style()->get_text(Gtk::STATE_NORMAL) );

        cr->move_to(
              xoff + 2 + XPAD
            , yoff + 72 - text_height[3]
        );

        pango_cairo_show_layout( cr->cobj(), layout[3]->gobj() );
    }

    ///////////////////////////////////////////////
    /// Object Properties
    ///////////////////////////////////////////////

    ProxyOf<PropAlbumInfo>::ReadOnly
    CellRendererAlbumData::property_info() const
    {
        return ProxyOf<PropAlbumInfo>::ReadOnly(
            this,
            "info"
        );
    }

    ProxyOf<PropAlbumInfo>::ReadWrite
    CellRendererAlbumData::property_info()
    {
        return ProxyOf<PropAlbumInfo>::ReadWrite(
            this,
            "info"
        );
    }
};
