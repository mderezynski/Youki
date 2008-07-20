
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
        self.player_metadata_updated_handler_id = self.player.gobj().connect("metadata-updated", self.metadata_updated)
        self.player_playstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)

        return True

    def deactivate(self):
        self.player.remove_info_widget(self.tagview.get_widget())
        self.player.gobj().disconnect(self.player_metadata_updated_handler_id)
        self.player.gobj().disconnect(self.player_playstatus_changed_handler_id)

    def pstate_changed(self, blah, state):

        if state == mpx.PlayStatus.STOPPED:
            self.image.clear()

    def metadata_updated(self, blah):

        self.image.clear()

        try:
            image = self.player.get_metadata().get_image()

            if image:
                self.image.set_from_pixbuf(image.scale_simple(512,512,gtk.gdk.INTERP_HYPER))
        except:
            print "Error in TrackCover::metadata_updated"
