#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx_boost
import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade


class TrackInfo:
	def __init__(self):
		self.xml = gtk.glade.XML("/usr/local/share/mpx/glade/trackinfo.glade")
		self.window = self.xml.get_widget("window1") 
		self.textview = self.xml.get_widget("textview")

	def show(self, track):
		self.window.show()
		self.textview.get_buffer().set_text(track)


info = TrackInfo()
