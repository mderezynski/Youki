#
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Trackinfo
# (C) 2008 M. Derezynski


import mpx
import mpx_playlist
import gtkmm
import time
import pygtk
pygtk.require('2.0')
import gtk
import gobject
import gtk.glade
import urllib
import random
import xml
from Ft.Xml.XPath.Context import Context
from Ft.Xml.XPath import Evaluate
from Ft.Xml.Domlette import NonvalidatingReader
from Ft.Xml import EMPTY_NAMESPACE
#import musicbrainz2
#import musicbrainz2.webservice as ws

MBID_NODE_XPATH = u'//mbid'
MATCH_NODE_XPATH = u'//match'

class PlaylistDefaultTooltip(mpx_playlist.PlaylistPlugin):

    def __init__(self,lib,covers,mlib):
        
        self.xml = gtk.glade.XML("/usr/share/mpx" + "/playlist-python-plugins/pltd/pltd.glade")
        self.widget = self.xml.get_widget("widget")
        self.widget.unparent()
        self.widget.show_all()
        self.image = self.xml.get_widget("image")
        self.labels = []

        for l in ["artist", "album", "title", "bitrate", "samplerate"]:
            self.labels.append(self.xml.get_widget("label-%s" % l))

        self.lib = lib
        self.covers = covers
        self.mlib = mlib
        self.mlib.gobj().connect("playlist-tooltip", self.on_playlist_tooltip)

    def on_playlist_tooltip(self, object, treemodel, iter):

        mpxtrack_boxed = treemodel.get(iter, 9)[0]
        mpxtrack = mpx.unwrap_boxed_mpxtrack(mpxtrack_boxed)

        if mpxtrack[mpx.AttributeId.ARTIST].is_initialized():
            self.labels[0].set_text(mpxtrack[mpx.AttributeId.ARTIST].val().get_string())

        if mpxtrack[mpx.AttributeId.ALBUM].is_initialized():
            self.labels[1].set_text(mpxtrack[mpx.AttributeId.ALBUM].val().get_string())

        if mpxtrack[mpx.AttributeId.TITLE].is_initialized():
            self.labels[2].set_text(mpxtrack[mpx.AttributeId.TITLE].val().get_string())

        if mpxtrack[mpx.AttributeId.BITRATE].is_initialized():
            self.labels[3].set_text(str(mpxtrack[mpx.AttributeId.BITRATE].val().get_int())+ " kbit/s")

        if mpxtrack[mpx.AttributeId.SAMPLERATE].is_initialized():
            self.labels[4].set_text(str(mpxtrack[mpx.AttributeId.SAMPLERATE].val().get_int()/1000.)+ " KHz")

        self.image.hide()
        if mpxtrack[mpx.AttributeId.MB_ALBUM_ID].is_initialized():
            mbid = mpxtrack[mpx.AttributeId.MB_ALBUM_ID].val().get_string()
            cover = self.covers.fetch(mbid) 
            if cover:
                self.image.set_from_pixbuf(cover.scale_simple(96, 96, gtk.gdk.INTERP_BILINEAR))
                self.image.show()

        self.widget.show_all()
        return self.widget

    def run(self):
        print ">> Tooltip Running"
