#include "config.h"
#include <boost/format.hpp>
#include <glibmm/i18n.h>

#include "mpx/ld.hh"
#include "mpx/util-graphics.hh"
#include "mpx/sql.hh"

#include "component-top-albums.hh"

using namespace Glib;
using namespace Gtk;

namespace
{
    std::string
    push_artist(MPX::SQL::Row & row)
    {
        return boost::get<std::string>((*row.find("artist")).second);
    }
}

MPX::ComponentTopAlbums::ComponentTopAlbums (PAccess<MPX::Library> & obj_pa_library)
: ComponentBase()
, m_Library(obj_pa_library)
{
    SQL::RowV v; 
    m_Library.get().getSQL(v, "SELECT DISTINCT artist FROM artist");

    std::transform(v.begin(), v.end(), std::inserter(m_Artists, m_Artists.end()), push_artist); 

    m_XML = Gnome::Glade::Xml::create(
            DATA_DIR
            G_DIR_SEPARATOR_S
            "glade"
            G_DIR_SEPARATOR_S
            "lastfm-component-top-albums.glade"
    );

    m_UI  = m_XML->get_widget("lastfm_com_top_albums");
    m_XML->get_widget("entry", m_Entry);
    m_XML->get_widget("view", m_View);
    m_XML->get_widget("label", m_Label);
    m_XML->get_widget("label-suggest", m_LabelSuggest);
    
    setup_view ();

    m_Entry->signal_activate().connect(
        sigc::mem_fun( *this, &ComponentTopAlbums::on_entry_activated
    ));

    m_Entry->signal_changed().connect(
        sigc::mem_fun( *this, &ComponentTopAlbums::on_entry_changed
    ));

    m_Thread = new TopAlbumsFetchThread;
    m_Thread->run();

    m_Thread->SignalAlbum().connect(
        sigc::mem_fun(*this, &ComponentTopAlbums::on_new_album
    ));

    m_Thread->SignalStopped().connect(
        sigc::mem_fun(*this, &ComponentTopAlbums::on_stopped
    ));
}

MPX::ComponentTopAlbums::~ComponentTopAlbums ()
{
}

void
MPX::ComponentTopAlbums::setup_view ()
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

std::string
MPX::ComponentTopAlbums::find_nearest_artist(std::string const& input)
{
    Artists_T::const_iterator o = m_Artists.end();
    std::string::size_type    t = std::string::npos;

    for(Artists_T::const_iterator i = m_Artists.begin(); i != m_Artists.end(); ++i)
    {
        std::string::size_type match = MPX::ld_distance(input, *i);
        if( match < t )
        {
           o = i; 
           t = match;
        }
    }

    if(o == m_Artists.end())
    {
        return _("(No match)");
    }

    return *o;
}

void
MPX::ComponentTopAlbums::on_entry_changed ()
{
    m_Thread->RequestStop();
}

void
MPX::ComponentTopAlbums::on_entry_activated ()
{
    m_Thread->RequestStop();
    m_Model->clear();
    m_Label->set_markup((boost::format ("Showing '<b>%s</b>'") % Markup::escape_text(m_Entry->get_text())).str());
    std::string text = m_Entry->get_text();
    m_LabelSuggest->set_markup((boost::format ("'<b>%s</b>'") % find_nearest_artist(text)).str());
    m_Thread->RequestLoad(text);
}

void
MPX::ComponentTopAlbums::on_stopped ()
{
    m_Model->clear();
}

void
MPX::ComponentTopAlbums::on_new_album(Glib::RefPtr<Gdk::Pixbuf> icon, std::string const& album)
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
