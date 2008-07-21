
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
import gst
import pygst
import gobject
import pango

class AudioRecord(mpx.Plugin):

    def __init__(self, id, player, mcs):

        self.id = id
        self.player = player
        self.mcs = mcs
        self.tap = player.get_play().tap()
        self.pipeline = player.get_play().pipeline()

        self.queue    = gst.element_factory_make("queue", "queue-audiorecord")
        self.pipeline.add(self.queue)

        self.lame     = gst.element_factory_make("lame", "lame-audiorecord")
        self.pipeline.add(self.lame)

        self.shoutsink  = gst.element_factory_make("shout2send", "shout-audiorecord") 
        self.pipeline.add(self.shoutsink)
        self.shoutsink.set_property("protocol", 2)
        self.shoutsink.set_property("url","/")

        gst.element_link_many(self.queue, self.lame, self.shoutsink)

    def activate(self):

        self.player_playstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)
        gst.element_link_many(self.tap, self.queue)

        return True

    def deactivate(self):

        self.player.gobj().disconnect(self.player_playstatus_changed_handler_id)
        gst.element_unlink_many(self.tap, self.queue)

    def pstate_changed(self, blah, state):

        pass
