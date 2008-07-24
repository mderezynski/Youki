#
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx
import time
import urllib
import urllib2
import xml.dom.minidom
from mpxapi.lastfm.model import TrackTopTag

class TrackTopTags():

    """This plugin serves as a visualizer of Last.fm data belonging to library items (tracks, albums)."""

    def __init__(self, artist, title):

        self.artist = artist
        self.title  = title
        self.parse_current_elmt = None 
        self.parse_data = {}
        self.parse_tags = []

    def document(self, dom):

        for node in dom.childNodes:

            if node.nodeType == node.ELEMENT_NODE:

                if node.nodeName == "tag":

                    if "name" in self.parse_data:
        
                        model = TrackTopTag()

                        model.setName(self.parse_data["name"])
                        model.setCount(self.parse_data["count"])
                        model.setUrl(self.parse_data["url"])

                        self.parse_tags.append(model)

                        self.parse_data = {} 

                self.parse_current_elmt = node.nodeName
                self.parse_data[node.nodeName] = ""

            elif node.nodeType == node.TEXT_NODE:
                    self.parse_data[self.parse_current_elmt] = self.parse_data[self.parse_current_elmt] + node.nodeValue.strip()

            self.document(node)
 
    def get(self):

        req = "http://ws.audioscrobbler.com/1.0/track/%s/%s/toptags.xml" % (urllib.quote(self.artist), urllib.quote(self.title))
        handle = urllib2.urlopen(req)
        dom = xml.dom.minidom.parse(handle)
        handle.close()
        self.document(dom)
        return self.parse_tags
