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
import libxml2

class TrackTopTags():

    """This plugin serves as a visualizer of Last.fm data belonging to library items (tracks, albums)."""

    def __init__(self, artist, title):

        self.artist = artist
        self.title  = title

    def get(self):

        req     = "http://ws.audioscrobbler.com/1.0/track/%s/%s/toptags.xml" % (urllib.quote(self.artist), urllib.quote(self.title))
        handle  = urllib2.urlopen(req)
        res     = "".join(handle.readlines())

        doc     = libxml2.parseMemory(response,len(response))
        root    = doc.children

        for child in root:        

            print child
