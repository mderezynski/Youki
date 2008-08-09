
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

albumlist_actions_ui_string = """
    <ui>
        <menubar name='popup-albumlist-source-musiclib-treeview-albums'>
            <menu name='menu-albumlist-source-musiclib-treeview-albums' action='dummy'>
            <placeholder name='action-album-placeholder-source-musiclib-treeview-albums'>
                <menuitem action='albumlist-actions-action-add-to-collection'/>
                <menuitem action='albumlist-actions-action-add-to-new-collection'/>
            </placeholder>
            </menu>
        </menubar>
    </ui>
"""

class AlbumActions(mpx.Plugin):

    def __init__(self, id, player, mcs):

        self.id = id
        self.player = player
        self.mcs = mcs
        self.mlib = player.get_source("36068e19-dfb3-49cd-85b4-52cea16fe0fd");
        self.actiongroup = gtk.ActionGroup("AlbumActions-Actions") 

        self.action = []

        action = gtk.Action("albumlist-actions-action-add-to-collection", "Add to Collection", "", gtk.STOCK_ADD)
        action.connect("activate", self.on_action_add_cb)
        self.action.append(action)

        action = gtk.Action("albumlist-actions-action-add-to-new-collection", "Add to New Collection", "", gtk.STOCK_ADD)
        action.connect("activate", self.on_action_add_new_cb)
        self.action.append(action)

        self.actiongroup.add_action(self.action[0])
        self.actiongroup.add_action(self.action[1])

        self.ui = self.player.ui()
        self.ui.insert_action_group(action_group=self.actiongroup, pos=-1)
        self.merge_id = 0

    def on_action_add_cb(self, action):

        print "Action #1 CB"

    def on_action_add_new_cb(self, action):

        print "Action #2 CB"

    def activate(self):

        self.merge_id = self.ui.add_ui_from_string(albumlist_actions_ui_string)
        return True

    def deactivate(self):

        self.ui.remove_ui(self.merge_id)
        return True
