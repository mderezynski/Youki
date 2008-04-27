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

        try:
            p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
            p_title = m.get(mpx.AttributeId.TITLE).val().get_string()
        except:
            p_artist = None
            p_title = None

        if p_artist != None and p_title != None: 
            self.set_message("Listening to " + p_title + " - " + p_artist)

    def set_message(self, message):
        # Get current status type (Available/Away/etc.)
        current = self.purple.PurpleSavedstatusGetType(self.purple.PurpleSavedstatusGetCurrent())
        # Create new transient status and activate it
        status = self.purple.PurpleSavedstatusNew("", current)
        self.purple.PurpleSavedstatusSetMessage(status, message)
        self.purple.PurpleSavedstatusActivate(status) 

