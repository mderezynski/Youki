#
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Notification plugin
# (C) 2008 Jacek Wolszczak
# (C) 2008 David Le Brun
#

import mpx
import pynotify
import gtk

COVER_SIZE = 96

class Notification(mpx.Plugin):

    def __init__(self,id):

        self.id = id

    def activate(self,player,mcs):
        self.player = player

        self.next = gtk.StatusIcon()
        self.next.set_from_stock(gtk.STOCK_MEDIA_NEXT)
        self.next.connect('activate', self.next_cb)
        self.next.set_visible(True)

        self.playpause = gtk.StatusIcon()
        self.playpause.set_from_stock(gtk.STOCK_MEDIA_PLAY)
        self.playpause.connect('activate', self.playpause_cb)
        self.player_state_change_handler_id = self.player.gobj().connect("play-status-changed", self.on_state_change)
        self.playpause.set_visible(True)

        self.previous = gtk.StatusIcon()
        self.previous.set_from_stock(gtk.STOCK_MEDIA_PREVIOUS)
        self.previous.connect('activate', self.previous_cb)
        self.previous.set_visible(True)

        self.player_new_track_handler_id = self.player.gobj().connect("new-track", self.now_playing)
        self.previous_message = u''
        pynotify.init("MPX")

        return True

    def deactivate(self):
        self.next.set_visible(False)
        self.previous.set_visible(False)
        self.playpause.set_visible(False)
        self.player.gobj().disconnect(self.player_new_track_handler_id)
        self.player.gobj().disconnect(self.player_state_change_handler_id)
        self.player = None

    def on_state_change(self, player, state):
        if state == mpx.PlayStatus.PLAYING:
            self.playpause.set_from_stock(gtk.STOCK_MEDIA_PAUSE)
        else:
            self.playpause.set_from_stock(gtk.STOCK_MEDIA_PLAY)

    def next_cb(self,widget, data = None):
        self.player.next()

    def previous_cb(self,widget, data = None):
        self.player.prev()

    def playpause_cb(self,widget, data = None):
        self.player.pause()

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

            if message != self.previous_message:
                n = pynotify.Notification( "MPX is now playing...", message)
                n.set_urgency(pynotify.URGENCY_LOW)
                image = m.get_image()
                if(image):
                    image = image.scale_simple(COVER_SIZE, COVER_SIZE, gtk.gdk.INTERP_NEAREST)
                    n.set_icon_from_pixbuf(image)

                n.show()
                self.previous_message = message

