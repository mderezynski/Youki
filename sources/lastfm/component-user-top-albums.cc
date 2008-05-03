#include "config.h"
#include "mpx/util-graphics.hh"
#include "component-user-top-albums.hh"

using namespace Glib;
using namespace Gtk;

MPX::ComponentUserTopAlbums::ComponentUserTopAlbums ()
: ComponentBase()
{
    m_XML = Gnome::Glade::Xml::create(
            DATA_DIR
            G_DIR_SEPARATOR_S
            "glade"
            G_DIR_SEPARATOR_S
            "lastfm-component-user-top-albums.glade"
    );

    m_UI  = m_XML->get_widget("lastfm_com_user_top_albums");
    m_XML->get_widget("entry", m_Entry);
    m_XML->get_widget("view", m_View);
    
    setup_view ();

    m_Entry->signal_activate().connect(
        sigc::mem_fun( *this, &ComponentUserTopAlbums::on_entry_activated
    ));

    m_Thread = new UserTopAlbumsFetchThread;
    m_Thread->run();
    m_Thread->SignalAlbum().connect(
        sigc::mem_fun(*this, &ComponentUserTopAlbums::on_new_album
    ));
}

MPX::ComponentUserTopAlbums::~ComponentUserTopAlbums ()
{
}

void
MPX::ComponentUserTopAlbums::setup_view ()
{
    CellRendererPixbuf* cell1 = manage( new CellRendererPixbuf );
    CellRendererText* cell2 = manage( new CellRendererText );
    TreeViewColumn* column = manage( new TreeViewColumn ); 

    cell1->property_xpad() = 4;
    cell1->property_ypad() = 4;

    cell2->property_yalign() = 0.0;
    cell2->property_weight() = Pango::WEIGHT_BOLD;
    cell2->property_size() = 12 * PANGO_SCALE;

    column->pack_start(*cell1, false );
    column->pack_start(*cell2, true );
    column->add_attribute(*cell1, "pixbuf", Columns.Pixbuf); 
    column->add_attribute(*cell2, "text", Columns.Text);

    m_View->append_column(*column);

    m_Model = Gtk::ListStore::create(Columns);
    m_View->set_model(m_Model);
}

void
MPX::ComponentUserTopAlbums::on_entry_activated ()
{
    m_Thread->RequestStop();
    m_Model->clear();
    m_Thread->RequestLoad(m_Entry->get_text());
}

void
MPX::ComponentUserTopAlbums::on_new_album(Glib::RefPtr<Gdk::Pixbuf> icon, std::string const& album)
{
    if(icon)
    {
        TreeIter iter = m_Model->append();

        Cairo::RefPtr<Cairo::ImageSurface> srfc =
            Util::cairo_image_surface_from_pixbuf(
                icon->scale_simple(64,64,Gdk::INTERP_BILINEAR
            ));

        srfc = Util::cairo_image_surface_round(srfc, 6.);
        Util::cairo_image_surface_rounded_border(srfc, 1., 6.);

        (*iter)[Columns.Pixbuf] = Util::cairo_image_surface_to_pixbuf(srfc);
        (*iter)[Columns.Text] = album;
    }
}
