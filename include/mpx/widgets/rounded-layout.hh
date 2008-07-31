#ifndef MPX_ROUNDED_LAYOUT
#define MPX_ROUNDED_LAYOUT

#include <gtkmm.h>
#include "mpx/widgets/widgetloader.hh"

namespace MPX
{
    class RoundedLayout : public Gnome::Glade::WidgetLoader<Gtk::DrawingArea>
    {
        private:

                Glib::ustring       m_text;

        protected:

                void
                on_size_request (Gtk::Requisition*);

                virtual bool
                on_expose_event (GdkEventExpose*);

        public:

                RoundedLayout (Glib::RefPtr<Gnome::Glade::Xml> const &xml, std::string const &name);
                virtual ~RoundedLayout();

                void
                set_text(Glib::ustring const&);
    };
}


#endif
