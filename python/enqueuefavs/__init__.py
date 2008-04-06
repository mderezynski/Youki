#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx
import mpx_playlist
#import pygtk
#pygtk.require('2.0')
#import gtk
#import gtk.glade
#import pango
#import musicbrainz2
#import musicbrainz2.webservice as ws

class EnqueueFavs(mpx_playlist.PlaylistPlugin):
	def __init__(self):
		print ">> EnqueueFavs Plugin initialized (C) 2008 M. Derezynski"

	def prepare(self,lib):
		print ">> EnqueueFavs Plugin preparing"

	def run(self,lib,v):
		print ">> EnqueueFavs Running"
		rv = mpx.SQLRowV()
		lib.getSQL(rv, "SELECT id FROM track_view WHERE abs_rating(rating,album_rating) > 25");
		for val in rv:
			v.append(val["id"].get_int())


