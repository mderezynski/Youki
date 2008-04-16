
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
from Ft.Xml.XPath.Context import Context
from Ft.Xml.XPath import Evaluate
from Ft.Xml.Domlette import NonvalidatingReader
from Ft.Xml import EMPTY_NAMESPACE

class TrackTags(mpx.Plugin):

    def activate(self,player,mcs):

        self.player = player
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL) # so reparent works, FIXME
        self.tagview = mpx.TagView()
        self.window.add(self.tagview.get_widget())
        self.player.add_info_widget(self.tagview.get_widget(), "Track Tags")
        self.tagview.get_widget().show_all()

        self.player_tagview_tag_handler_id = self.tagview.get_widget().connect("tag-clicked", self.tag_clicked)
        self.player_new_track_handler_id = self.player.gobj().connect("new-track", self.new_track)
        self.player_playtstatus_changed_handler_id = self.player.gobj().connect("play-status-changed", self.pstate_changed)

        return True

    def deactivate(self):
        self.player.gobj().disconnect(self.player_new_track_handler_id)
        self.player.gobj().disconnect(self.player_playtstatus_changed_handler_id)
        self.tagview.get_widget().disconnect(self.player_tagview_tag_handler_id)
        self.player = None

    def tag_clicked(self, blah, tag):
        
        if self.player and len(tag) > 0:
                self.player.play_uri("lastfm://globaltags/%s" % tag)

    def pstate_changed(self, blah, state):

        if state == mpx.PlayStatus.STOPPED:
            self.tagview.clear()

    def new_track(self, blah):

        self.tagview.clear()

        try:
            m = self.player.get_metadata()
            if m.get(mpx.AttributeId.ARTIST) and m.get(mpx.AttributeId.TITLE):
                uri = u"http://ws.audioscrobbler.com/1.0/track/%s/%s/toptags.xml" % (urllib.quote(m.get(mpx.AttributeId.ARTIST).val().get_string()), urllib.quote(m.get(mpx.AttributeId.TITLE).val().get_string()))
                lastfm = NonvalidatingReader.parseUri(uri)
                ctx = Context(lastfm)

                tags = []
                names_xml = Evaluate("//name", context=ctx)
                counts_xml = Evaluate("//count", context=ctx)

                counts = []
                for count in counts_xml:
                    try:
                        if count.firstChild:
                            counts.append(float(math.log10((float(count.firstChild.data) * 5)+1)))
                    except:
                        print "Range Exceeded"

                for name in names_xml:
                    if name.firstChild:
                        tags.append([str(name.firstChild.data), counts[names_xml.index(name)]])

                random.shuffle(tags)
                
                for lst in tags:
                    self.tagview.add_tag(lst[0], lst[1])
        
        except:
            print "Error displaying tags"
