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
        
        self.lib = lib
        self.covers = covers
        self.mlib = mlib
        self.mlib.gobj().connect("playlist-tooltip", self.on_playlist_tooltip)

    def on_playlist_tooltip(self, object, treemodel, iter):

        mpxtrack_boxed = treemodel.get(iter, 9)[0]
        mpxtrack = mpx.unwrap_boxed_mpxtrack(mpxtrack_boxed)

        box = gtk.HBox()
        vbox = gtk.VBox()
        image = gtk.Image()
    
        box.pack_start(image)
        box.pack_start(vbox)
      
        labels = []
        for r in range(5):
            labels.append(gtk.Label())
            labels[r].set_property("xalign", 0.0)
 
        box.set_spacing(4)
        vbox.set_spacing(4)

        if mpxtrack[mpx.AttributeId.ARTIST].is_initialized():
            vbox.pack_start(labels[0], False, False)
            labels[0].set_markup("<b>Artist:</b>\t%s" % xml.sax.saxutils.escape(mpxtrack[mpx.AttributeId.ARTIST].val().get_string()))

        if mpxtrack[mpx.AttributeId.ALBUM].is_initialized():
            vbox.pack_start(labels[1], False, False)
            labels[1].set_markup("<b>Album:</b>\t%s" % xml.sax.saxutils.escape(mpxtrack[mpx.AttributeId.ALBUM].val().get_string()))

        if mpxtrack[mpx.AttributeId.TITLE].is_initialized():
            vbox.pack_start(labels[2], False, False)
            labels[2].set_markup("<b>Title:</b>\t%s" % xml.sax.saxutils.escape(mpxtrack[mpx.AttributeId.TITLE].val().get_string()))

        #if mpxtrack[mpx.AttributeId.BITRATE].is_initialized():
        #    vbox.pack_start(labels[3], False, False)
        #    labels[3].set_markup("<b>Bitrate:</b>\t%s" % str(mpxtrack[mpx.AttributeId.BITRATE].val().get_int()))

        #if mpxtrack[mpx.AttributeId.SAMPLERATE].is_initialized():
        #    vbox.pack_start(labels[4], False, False)
        #    labels[4].set_markup("<b>Samplerate:</b>\t%s" % str(mpxtrack[mpx.AttributeId.SAMPLERATE].val().get_int()))


        if mpxtrack[mpx.AttributeId.MB_ALBUM_ID].is_initialized():
            mbid = mpxtrack[mpx.AttributeId.MB_ALBUM_ID].val().get_string()
            cover = self.covers.fetch(mbid) 
            if cover:
                image.set_from_pixbuf(cover.scale_simple(64, 64, gtk.gdk.INTERP_BILINEAR))

        box.show_all()
        return box

    def run(self):
        print ">> Tooltip Running"
