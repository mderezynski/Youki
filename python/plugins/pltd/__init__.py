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
       
        label1 = gtk.Label() 
        label2 = gtk.Label() 
        label3 = gtk.Label() 

        label1.set_property("xalign", 0.0)
        label2.set_property("xalign", 0.0)
        label3.set_property("xalign", 0.0)

        vbox.pack_start(label1)
        vbox.pack_start(label2)
        vbox.pack_start(label3)

        box.set_spacing(4)
        vbox.set_spacing(4)

        if mpxtrack[mpx.AttributeId.ARTIST].is_initialized():
            label1.set_markup("<b>Artist:</b>\t%s" % mpxtrack[mpx.AttributeId.ARTIST].val().get_string())

        if mpxtrack[mpx.AttributeId.ALBUM].is_initialized():
            label2.set_markup("<b>Album:</b>\t%s" % mpxtrack[mpx.AttributeId.ALBUM].val().get_string())

        if mpxtrack[mpx.AttributeId.TITLE].is_initialized():
            label3.set_markup("<b>Title:</b>\t%s" % mpxtrack[mpx.AttributeId.TITLE].val().get_string())

        if mpxtrack[mpx.AttributeId.MB_ALBUM_ID].is_initialized():
            mbid = mpxtrack[mpx.AttributeId.MB_ALBUM_ID].val().get_string()
            cover = self.covers.fetch(mbid) 
            if cover:
                image.set_from_pixbuf(cover.scale_simple(64, 64, gtk.gdk.INTERP_BILINEAR))

        box.show_all()
        return box

    def run(self):
        print ">> Tooltip Running"
