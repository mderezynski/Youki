
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
import gtk.glade
import gobject
import pango
import urllib
import random
import math
import threading
import mutex
from mpxapi import lastfm

class TrackTagsDataAcquire(threading.Thread):

    def __init__(self, artist, title):

        threading.Thread.__init__(self)

        self.finished   = threading.Event()
        self.artist     = artist
        self.title      = title
        
    def is_finished(self):

        return self.finished.isSet()

    def stop(self):

        self.finished.set()
        self.join()

    def get_tags(self):

        return self.tags

    def run(self):

        self.tags = []

        try:
                t = lastfm.TrackTopTags(self.artist, self.title)
                tags = t.get()
                size = 1000 

                for tag in tags:

                        name = tag.getName()
                        count = tag.getCount()

                        if count == None:
                                count = size
            
                        try:
                            calc_size = math.sqrt(math.log10(((float(count) * 10.))+1))
                            if calc_size < 0.6:
                                calc_size = 0.6
                        except:
                            calc_size = 0.6

                        self.tags.append([name, calc_size]) 

                        size = size - 10
        except:
                pass
        
        random.seed(3.14159)
        random.shuffle(self.tags)
        self.finished.set()

class TrackTags(mpx.Plugin):

    """The TrackTags Plugin displays Last.fm tags for the currently played track, and allows you to start playing on Last.fm off of one of the tags!"""

    def __init__(self, id, player, mcs):

        self.id = id
        self.player = player
        self.lock = mutex.mutex()

    def activate(self):

        self.tagview = mpx.TagView()
        self.player.add_info_widget(self.tagview.get_widget(), "Last.fm Tags")
        self.player_tagview_tag_handler_id = self.tagview.get_widget().connect("tag-clicked", self.tag_clicked)
        self.player_metadata_updated_handler_id = self.player.gobj().connect("metadata-updated", self.metadata_updated)
        self.player_playtstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.status_changed)

        return True

    def deactivate(self):
        self.player.remove_info_widget(self.tagview.get_widget())
        self.player.gobj().disconnect(self.player_metadata_updated_handler_id)
        self.player.gobj().disconnect(self.player_playtstatus_changed_handler_id)
        self.tagview.get_widget().disconnect(self.player_tagview_tag_handler_id)

    def tag_clicked(self, blah, tag):
        
        if self.player and len(tag) > 0:
                print "Asking to play Tag '%s'" % tag
                self.player.play_uri("lastfm://globaltags/%s" % tag)
        else:
                print "No player obj or tag length is 0"

    def status_changed(self, blah, state):
    
        # Workaround for asynchronicity between signal emission and new-track
        if self.player.get_status() == mpx.PlayStatus.STOPPED:
            print "Clearing"
            self.tagview.clear()

    def metadata_updated(self, blah):

        self.tagview.clear()
        self.tagview.display(False)

        self.lock.lock(self.display_track_tags, blah)

    def display_track_tags(self, blah):

        m = self.player.get_metadata()

        if m[mpx.AttributeId.ARTIST] and m[mpx.AttributeId.TITLE]:

            self.player.info_set("Fetching Last.fm track tags")

            instance = TrackTagsDataAcquire(m[mpx.AttributeId.ARTIST].get(), m[mpx.AttributeId.TITLE].get())
            instance.start()

            while not instance.is_finished():
                    while gtk.events_pending():
                            gtk.main_iteration()

            tags = instance.get_tags()

            self.tagview.clear()
            self.tagview.display(False)

            for t in tags:
                self.tagview.add_tag(str(t[0]), float(t[1]))

            self.player.info_clear()

        self.tagview.display(True)

        self.lock.unlock()
