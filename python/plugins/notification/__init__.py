#
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Notification plugin
# (C) 2008 D. Le Brun
#

import mpx
import pynotify
import gtk

COVER_SIZE = 96

class Notification(mpx.Plugin):


    def activate(self,player,mcs):
        print ">> Notification Plugin activated"
        self.player = player
        self.new_track = self.player.gobj().connect("new-track", self.now_playing)
        pynotify.init("MPX")
        return True

    def deactivate(self):
        print ">> Notification Plugin deactivated"
        self.player = None

    def run(self):
        print ">> Notification Plugin running"

    def now_playing(self, blah):

        m = self.player.get_metadata()

        try:
            p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
            p_title = m.get(mpx.AttributeId.TITLE).val().get_string()
        except:
            p_artist = None
            p_title = None

        if p_artist != None and p_title != None: 
            message = "<big><b>" + p_artist + "</b>\n" + p_title + "</big>"
            n = pynotify.Notification( "MPX is now playing...", message)
            n.set_urgency(pynotify.URGENCY_NORMAL)
            image = m.get_image()
            if(image):
                image = image.scale_simple(COVER_SIZE, COVER_SIZE, gtk.gdk.INTERP_NEAREST)
                n.set_icon_from_pixbuf(image)
            n.show()

