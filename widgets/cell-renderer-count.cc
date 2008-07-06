#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glibmm.h>
#include <gtkmm.h>

#include "mpx/widgets/cairoextensions.hh"
#include "mpx/widgets/cell-renderer-count.hh"

using namespace Gtk;

namespace
{
    const int XPAD = 3;
    const int YPAD = 2;
}

namespace MPX
{
    CellRendererCount::CellRendererCount ()
    : ObjectBase        (typeid(CellRendererCount))
    , property_text_    (*this, "text", "") 
    , property_box_     (*this, "box", false)
    {
    }

    CellRendererCount::~CellRendererCount ()
    {
    }

    void
    CellRendererCount::get_size_vfunc  (Gtk::Widget & widget,
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
      layout->set_markup( property_text().get_value() );
      layout->get_pixel_size( text_width, text_height );

      if(width)
        *width = text_width + 2 * XPAD; 

      if(height)
        *height = text_height + 2 * YPAD;
    }

    void
    CellRendererCount::render_vfunc    (Glib::RefPtr<Gdk::Drawable> const& window,
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

        int text_width, text_height;
        Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create( widget.get_pango_context() );
        layout->set_markup( property_text_.get_value() );
        Pango::AttrList list;
        Pango::Attribute attr = Pango::Attribute::create_attr_scale(0.8);
        list.insert(attr);
        layout->set_attributes(list);
        layout->get_pixel_size( text_width, text_height );

        cr->set_operator( ::Cairo::OPERATOR_SOURCE );

        RoundedRectangle( cr, xoff+1, yoff+1, cell_area.get_width()-2, cell_area.get_height()-2, 4 );

        if( state & Gtk::CELL_RENDERER_SELECTED )
        {
            Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_SELECTED));
            cr->fill ();
        }
        else
        {
            Cairo::RefPtr<Cairo::LinearGradient> gradient =
                Cairo::LinearGradient::create(
                    xoff,
                    yoff+cell_area.get_height()/2,
                    xoff+cell_area.get_width(),
                    yoff+cell_area.get_height()/2
                );
            gradient->add_color_stop_rgb(0., 0.00, 0.00, 0.00);
            gradient->add_color_stop_rgb(1., 0.35, 0.35, 0.35);
            cr->set_source(gradient);
            cr->fill_preserve();

            Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_NORMAL));
            cr->stroke();
        }

        cr->move_to (xoff + XPAD + 1 + ((cell_area.get_width()-2*XPAD) - text_width)/2, yoff + YPAD + ((cell_area.get_height()-2*YPAD) - text_height)/2);

        if( state & Gtk::CELL_RENDERER_SELECTED )
            Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_NORMAL));
        else
            Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_SELECTED));

        pango_cairo_show_layout (cr->cobj(), layout->gobj());
    }

    ///////////////////////////////////////////////
    /// Object Properties
    ///////////////////////////////////////////////

    ProxyOf<PropText>::ReadOnly
    CellRendererCount::property_text() const
    {
      return ProxyOf<PropText>::ReadOnly (this, "text");
    }

    ProxyOf<PropText>::ReadWrite
    CellRendererCount::property_text()
    {
      return ProxyOf<PropText>::ReadWrite (this, "text");
    }

    ProxyOf<PropInt>::ReadWrite
    CellRendererCount::property_box()
    {
      return ProxyOf<PropInt>::ReadWrite (this, "box");
    }

};
