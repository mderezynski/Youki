#ifndef MPX_TIMED_CONFIRMATION_HH
#define MPX_TIMED_CONFIRMATION_HH

#include <gtkmm.h>
#include "mpx/widgets/widgetloader.hh"

using namespace Gnome::Glade;
namespace MPX
{
class TimedConfirmation : public WidgetLoader<Gtk::Dialog>
{
    public:

        TimedConfirmation (Glib::ustring const&, int);
        virtual ~TimedConfirmation ();

        int run (Glib::ustring const&);

    private:

        void
        button_clicked (int);
        
        bool
        handler ();

        int                          m_Response;
        int                          m_Seconds;
        Gtk::Button                * m_Button_OK;
        Gtk::Button                * m_Button_Cancel;
        sigc::connection             m_TimeoutConnection;
        Glib::RefPtr<Glib::MainLoop> m_MainLoop;
};
}

#endif
