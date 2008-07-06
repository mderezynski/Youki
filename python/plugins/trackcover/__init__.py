
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx
import pygtk
pygtk.require('2.0')
import gtk
import gobject
import pango

class TrackCover(mpx.Plugin):

    def __init__(self, id, player, mcs):

        self.id = id
        self.player = player
        self.mcs = mcs
        self.image = gtk.Image()

    def activate(self):

        self.player.add_info_widget(self.image, "Album Cover")
        self.player_new_track_handler_id = self.player.gobj().connect("new-track", self.new_track)
        self.player_playstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)

        return True

    def deactivate(self):
        self.player.remove_info_widget(self.tagview.get_widget())
        self.player.gobj().disconnect(self.player_new_track_handler_id)
        self.player.gobj().disconnect(self.player_playstatus_changed_handler_id)

    def pstate_changed(self, blah, state):

        if state == mpx.PlayStatus.STOPPED:
            self.image.clear()

    def new_track(self, blah):

        self.image.clear()

        try:
            m = self.player.get_metadata()
            self.image.set_from_pixbuf(m.get_image())
        except:
            pass
