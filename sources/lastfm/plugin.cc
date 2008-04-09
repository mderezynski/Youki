#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <gtkmm.h>

#include "mpx/mpx-public.hh"
#include "mpx/i-playbacksource.hh"

// plugin-specific include
#include "source-lastfm.hh"

using namespace MPX;
using namespace Gtk;

extern "C" MPX::PlaybackSource*
get_instance (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player &player)
{
	return new MPX::Source::LastFM(ui_manager, player);
}

extern "C" void
del_instance (PlaybackSource * source)
{
	delete source;
}
