#ifndef KOBO_ALLTRACKS_HH                
#define KOBO_ALLTRACKS_HH

#include <gtkmm.h>
#include <glibmm/i18n.h>
#include "mpx/mpx-main.hh"
#include "mpx/mpx-library.hh"
#include "mpx/com/view-tracklist.hh"

namespace MPX
{
        class AllTracksView
        {
                public:

                        Gtk::Widget*
                        get_widget()
                        {
                            return m_VBox ;
                        }

                        ListView*
                        get_alltracks()
                        {
                            return m_ListView ;
                        }

                        ListView*
                        get_alltracks()
                        {
                            return m_ListView ;
                        }

                        DataModelFilterP
                        get_model ()
                        {
                            return m_FilterModel ;
                        }

                        AllTracksView()
                        {
                        }

                        void
                                on_entry_changed (DataModelFilterP model, Gtk::Entry* entry)
                                {
                                        model->set_filter(entry->get_text());
                                }

    } ;
}

#endif // KOBO_ALLTRACKS_HH
