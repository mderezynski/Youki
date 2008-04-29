#
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Notification plugin
# (C) 2008 David Le Brun
#

import mpx
import dbus

class Pidgin(mpx.Plugin):

    """The Pidgin plugin updates your Pidgin status with the currently playing song from MPX"""

    def __init__(self,id,player,mcs):

        self.id = id
        self.player = player

    def activate(self):

        self.player_state_change_handler_id = self.player.gobj().connect("play-status-changed", self.on_state_change)
        self.player_new_track_handler_id = self.player.gobj().connect("new-track", self.now_playing)

        # Initiate a connection to the Session Bus
        bus = dbus.SessionBus()

        # Associate Pidgin's D-Bus interface with Python objects
        obj = bus.get_object(
            "im.pidgin.purple.PurpleService", "/im/pidgin/purple/PurpleObject")
        self.purple = dbus.Interface(obj, "im.pidgin.purple.PurpleInterface")

        status = self.player.get_status()
        if (status == mpx.PlayStatus.PLAYING) or (status == mpx.PlayStatus.PAUSED):
            self.now_playing(player.gobj())

        return True

    def deactivate(self):
        self.set_message("")
        self.player.gobj().disconnect(self.player_new_track_handler_id)
        self.player.gobj().disconnect(self.player_state_change_handler_id)
        self.purple = None

    def on_state_change(self, player, state):
        if state == mpx.PlayStatus.STOPPED:
            self.set_message("")

    def now_playing(self, blah):

        m = self.player.get_metadata()

        if m[mpx.AttributeId.ARTIST] and m[mpx.AttributeId.TITLE]: 
            self.set_message("Listening to %s - %s", (m[mpx.AttributeId.TITLE].get(), m[mpx.AttributeId.ARTIST].get()))

    def set_message(self, message):
        # Get current status type (Available/Away/etc.)
        current = self.purple.PurpleSavedstatusGetType(self.purple.PurpleSavedstatusGetCurrent())
        # Create new transient status and activate it
        status = self.purple.PurpleSavedstatusNew("", current)
        self.purple.PurpleSavedstatusSetMessage(status, message)
        self.purple.PurpleSavedstatusActivate(status) 

