
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

class AudioRecord(mpx.Plugin):

    def __init__(self, id, player, mcs):

        self.id = id
        self.player = player
        self.mcs = mcs
        self.tap = player.get_play().tap()

    def activate(self):

        self.player_playstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)
        print "Tap: " + str(self.tap)

        return True

    def deactivate(self):

        self.player.gobj().disconnect(self.player_playstatus_changed_handler_id)

    def pstate_changed(self, blah, state):

        pass
