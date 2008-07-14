
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
from Ft.Xml.XPath.Context import Context
from Ft.Xml.XPath import Evaluate
from Ft.Xml.Domlette import NonvalidatingReader
from Ft.Xml import EMPTY_NAMESPACE

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

        return self.l_tags

    def run(self):

        uri     = u"http://ws.audioscrobbler.com/1.0/track/%s/%s/toptags.xml" % (urllib.quote(self.artist), urllib.quote(self.title))
        lfmxml  = NonvalidatingReader.parseUri(uri)
        ctx     = Context(lfmxml)

        xml_names   = Evaluate("//name", context=ctx)
        xml_count   = Evaluate("//count", context=ctx)
        self.l_tags = []
        l_count     = []

        for count in xml_count:
            try:
                if count.firstChild:
                    l_count.append(float(math.log10((float(count.firstChild.data) * 5)+1)))
            except:
                pass

        for name in xml_names:
            if name.firstChild:
                    self.l_tags.append([str(name.firstChild.data), l_count[xml_names.index(name)]])

        random.shuffle(self.l_tags)

        self.finished.set()

class TrackTags(mpx.Plugin):

    """The TrackTags Plugin displays Last.fm tags for the currently played track, and allows you to start playing on Last.fm off of one of the tags!"""

    def __init__(self, id, player, mcs):

        self.id = id
        self.player = player

    def activate(self):

        self.tagview = mpx.TagView()
        self.player.add_info_widget(self.tagview.get_widget(), "Last.fm Tags")
        self.player_tagview_tag_handler_id = self.tagview.get_widget().connect("tag-clicked", self.tag_clicked)
        self.player_new_track_handler_id = self.player.gobj().connect("new-track", self.new_track)
        self.player_playtstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)

        return True

    def deactivate(self):
        self.player.remove_info_widget(self.tagview.get_widget())
        self.player.gobj().disconnect(self.player_new_track_handler_id)
        self.player.gobj().disconnect(self.player_playtstatus_changed_handler_id)
        self.tagview.get_widget().disconnect(self.player_tagview_tag_handler_id)

    def tag_clicked(self, blah, tag):
        
        if self.player and len(tag) > 0:
                print "Asking to play Tag '%s'" % tag
                self.player.play_uri("lastfm://globaltags/%s" % tag)
        else:
                print "No player obj or tag length is 0"

    def pstate_changed(self, blah, state):

        if state == mpx.PlayStatus.STOPPED:
            self.tagview.clear()

    def new_track(self, blah):

        self.tagview.clear()

        m = self.player.get_metadata()

        if m[mpx.AttributeId.ARTIST] and m[mpx.AttributeId.TITLE]:

            instance = TrackTagsDataAcquire(m[mpx.AttributeId.ARTIST].get(), m[mpx.AttributeId.TITLE].get())
            instance.start()

            while not instance.is_finished():
                while gtk.events_pending():
                    gtk.main_iteration()

            tags = instance.get_tags()

            for t in tags:
                self.tagview.add_tag(t[0], t[1])
