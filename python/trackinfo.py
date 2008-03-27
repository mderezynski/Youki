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
		

	def show(self, track, image):
		self.window.show()
		self.image.set_from_pixbuf(image.scale_simple(192,192,gtk.gdk.INTERP_BILINEAR))

		if track.get(AttributeId.attr_artist).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "Artist: ", self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_artist).val().get_string())
			self.buf.insert(self.buf.get_end_iter(), "\n")

		if track.get(AttributeId.attr_album).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "Album: ", self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_album).val().get_string())
			self.buf.insert(self.buf.get_end_iter(), "\n")

		if track.get(AttributeId.attr_title).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "Title: ", self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_title).val().get_string())
			self.buf.insert(self.buf.get_end_iter(), "\n")

		if track.get(AttributeId.attr_bitrate).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "Bitrate: ", self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_bitrate).val().get_int())
			self.buf.insert(self.buf.get_end_iter(), "\n")

		if track.get(AttributeId.attr_mb_release_date).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "Release Date: ", self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_mb_release_date).val().get_string())
			self.buf.insert(self.buf.get_end_iter(), "\n")


	def close_clicked(self, button):
		self.window.hide()
		return True

	def delete_event(self, window, event):
		self.window.hide()
		return True


info = TrackInfo()
