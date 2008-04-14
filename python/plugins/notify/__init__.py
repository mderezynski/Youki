#
# MPX Notification plugin
# (C) 2008 D. Le Brun
#

import mpx
import pynotify
#import mpx_playlist

class Dummy(mpx.Plugin):

    def activate(self,player,mcs):
        print ">> Notify Plugin activated"
        self.player = player
	self.new_track = self.player.gobj().connect("new-track", self.now_playing)
	pynotify.init("MPX")
        return True

    def deactivate(self):
        print ">> Notify Plugin deactivated"
        self.player = None

    def run(self):
        print ">> Dummy Plugin running"

    def now_playing(self, blah):

	m = self.player.get_metadata()

	if m.get(mpx.AttributeId.ARTIST).is_initialized() and m.get(mpx.AttributeId.TITLE).is_initialized():
		p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
		p_title = m.get(mpx.AttributeId.TITLE).val().get_string()

		message = "<b>" + p_artist + "</b> - " + p_title
		n = pynotify.Notification( "MPX", message)
		n.set_urgency(pynotify.URGENCY_NORMAL)
		n.show()

