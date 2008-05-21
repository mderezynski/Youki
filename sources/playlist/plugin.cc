#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "mpx/mpx-public.hh"
#include "mpx/i-playbacksource.hh"

// plugin-specific include
#include "source-playlist.hh"

using namespace MPX;
using namespace Gtk;

extern "C" MPX::PlaybackSource*
get_instance (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player &player)
{
	return new MPX::Source::PlaybackSourcePlaylist(ui_manager, player, _("Playlist"));
}

extern "C" void
del_instance (PlaybackSource * source)
{
	delete source;
}
