
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
        self.alignment = gtk.Alignment(xalign=0.5, yalign=0.5)
        self.box = gtk.HBox(False,16)
        self.box.show_all()
        self.alignment.add(self.box)
        self.alignment.show_all()

    def activate(self):

        self.player.add_info_widget(self.alignment, "Album Covers")
        self.player_metadata_updated_handler_id = self.player.gobj().connect("metadata-updated", self.metadata_updated)
        self.player_metadata_updated_handler_id = self.player.gobj().connect("new-coverart", self.metadata_updated)
        self.player_metadata_updated_handler_id = self.player.gobj().connect("new-track", self.new_track)
        self.player_playstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)

        self.metadata_updated(None)

        return True

    def deactivate(self):

        self.player.remove_info_widget(self.alignment)
        self.player.gobj().disconnect(self.player_metadata_updated_handler_id)
        self.player.gobj().disconnect(self.player_playstatus_changed_handler_id)

        return True

    def regen_box(self):

        self.box.destroy()
        self.box = gtk.HBox(False,16)
        self.box.show_all()
        self.alignment.add(self.box)
        self.alignment.show_all()

    def pstate_changed(self, blah, state):

        if state == mpx.PlayStatus.STOPPED:
                self.regen_box()

    def new_track(self, blah):

        self.regen_box()

    def metadata_updated(self, blah):

        self.regen_box()

        try:
            pixbuf = self.player.get_metadata().get_image()
            if pixbuf:
                image = gtk.Image()
                image.set_from_pixbuf(pixbuf.scale_simple( 384, 384, gtk.gdk.INTERP_HYPER ))
                self.box.pack_end(image, False, False)
                image.show_all()
        except:
            pass
