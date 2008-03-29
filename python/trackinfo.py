#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx_boost
import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango
from mpx_boost import *


class TrackInfo:
	def __init__(self):
		self.xml = gtk.glade.XML("/usr/local/share/mpx/glade/trackinfo.glade")
		self.window = self.xml.get_widget("window1") 
		self.textview = self.xml.get_widget("textview")
		self.image = self.xml.get_widget("image")
		self.close = self.xml.get_widget("close")
		self.close.connect("clicked",self.close_clicked)
		self.window.connect("delete-event",self.delete_event)

		self.buf = self.textview.get_buffer()

		self.textTagBold = gtk.TextTag()
		self.textTagBold.set_property("weight", pango.WEIGHT_BOLD)
		self.buf.get_tag_table().add(self.textTagBold)
		
		self.textTagCenter = gtk.TextTag()
		self.textTagCenter.set_property("justification", gtk.JUSTIFY_CENTER) 
		self.buf.get_tag_table().add(self.textTagCenter)

		self.textTagLarge = gtk.TextTag()
		self.textTagLarge.set_property("size-points", 14) 
		self.buf.get_tag_table().add(self.textTagLarge)


	def show(self, track, image):
		self.window.show()

		if image: 
			self.image.set_from_pixbuf(image.scale_simple(192,192,gtk.gdk.INTERP_BILINEAR))
		self.buf.delete(self.buf.get_start_iter(), self.buf.get_end_iter())

		if track.get(AttributeId.attr_title).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_title).val().get_string(), self.textTagCenter, self.textTagLarge)

		if track.get(AttributeId.attr_artist).is_initialized() and track.get(AttributeId.attr_album).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "\n(from \"" + track.get(AttributeId.attr_album).val().get_string()+"\"", self.textTagCenter)
			self.buf.insert_with_tags(self.buf.get_end_iter(), " by " + track.get(AttributeId.attr_artist).val().get_string()+")", self.textTagCenter)

		if track.get(AttributeId.attr_artist).is_initialized() and track.get(AttributeId.attr_title).is_initialized():
			try:
				lyricwiki = mpx_boost.LyricWiki(track.get(AttributeId.attr_artist).val().get_string(), track.get(AttributeId.attr_title).val().get_string())
				lyrics = lyricwiki.run()
				self.buf.insert(self.buf.get_end_iter(), "\n\n")
				self.buf.insert(self.buf.get_end_iter(), lyrics)
			except:
				print "Couldn't get lyrics for '" + track.get(AttributeId.attr_artist).val().get_string() + " / " +  track.get(AttributeId.attr_title).val().get_string()+ "'"
		
		if track.get(AttributeId.attr_artist).is_initialized():
			try:
				self.buf.insert(self.buf.get_end_iter(), "\n\n")
				self.buf.insert_with_tags(self.buf.get_end_iter(), "About " + track.get(AttributeId.attr_artist).val().get_string(), self.textTagCenter, self.textTagLarge)
				self.buf.insert(self.buf.get_end_iter(), "\n\n")
				lastfmartist = mpx_boost.LastFMArtist(track.get(AttributeId.attr_artist).val().get_string())
				artist = lastfmartist.run()
				self.buf.insert(self.buf.get_end_iter(), artist)
			except:
				print "Couldn't get Last.fm artist info for '" + track.get(AttributeId.attr_artist).val().get_string() 
				

	def close_clicked(self, button):
		self.window.hide()
		return True

	def delete_event(self, window, event):
		self.window.hide()
		return True


info = TrackInfo()
