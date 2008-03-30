#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx_boost
from mpx_boost import *
import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import pango
import musicbrainz2
import musicbrainz2.webservice as ws


class TrackInfo:
	def __init__(self):
		self.q = ws.Query() 
		self.tinc = musicbrainz2.webservice.TrackIncludes(trackRelations=True,artist=True,puids=True,releases=True,artistRelations=True,releaseRelations=True,urlRelations=True)

		self.xml = gtk.glade.XML("/usr/local/share/mpx/glade/trackinfo.glade")
		self.window = self.xml.get_widget("window1") 
		self.textview = self.xml.get_widget("textview")
		self.image = self.xml.get_widget("image")
		self.close = self.xml.get_widget("close")
		#self.close.connect("clicked",self.close_clicked)
		#self.window.connect("delete-event",self.delete_event)

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

		self.textTagSlant = gtk.TextTag()
		self.textTagSlant.set_property("style", pango.STYLE_ITALIC) 
		self.buf.get_tag_table().add(self.textTagSlant)


	def show(self, track, image):
		self.window.show()

		if image: 
			self.image.set_from_pixbuf(image.scale_simple(192,192,gtk.gdk.INTERP_BILINEAR))
		self.buf.delete(self.buf.get_start_iter(), self.buf.get_end_iter())

		if track.get(AttributeId.attr_title).is_initialized() and track.get(AttributeId.attr_artist).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_title).val().get_string(), self.textTagCenter, self.textTagLarge)
			self.buf.insert_with_tags(self.buf.get_end_iter(), "\nby\n", self.textTagCenter) 
			self.buf.insert_with_tags(self.buf.get_end_iter(), track.get(AttributeId.attr_artist).val().get_string(), self.textTagCenter, self.textTagLarge)

		if track.get(AttributeId.attr_album_artist).is_initialized() and track.get(AttributeId.attr_album).is_initialized():
			self.buf.insert_with_tags(self.buf.get_end_iter(), "\n(from \"" + track.get(AttributeId.attr_album).val().get_string()+"\"", self.textTagCenter)
			self.buf.insert_with_tags(self.buf.get_end_iter(), " by " + track.get(AttributeId.attr_album_artist).val().get_string()+")", self.textTagCenter)

		self.buf.insert(self.buf.get_end_iter(), "\n\n")

		if track.get(AttributeId.attr_mb_track_id).is_initialized():
			trackId = track.get(AttributeId.attr_mb_track_id).val().get_string()
			mbtrack = self.q.getTrackById(trackId, self.tinc) 
			rels = mbtrack.getRelations()

			relobj = []
			for i in range(7):
				relobj.append([])

			# Check if it's a cover
			for rel in rels:
				if rel.getDirection() != "backward":
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#Cover":
						relobj[0].append(rel)	
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#Composer":
						relobj[1].append(rel)	
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#Instrument":
						relobj[2].append(rel)	
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#SamplesMaterial":
						relobj[3].append(rel)	
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#Remixer":
						relobj[4].append(rel)	
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#Lyricist":
						relobj[5].append(rel)	
					if rel.getType() == "http://musicbrainz.org/ns/rel-1.0#Producer":
						relobj[6].append(rel)	

			funcs = []
			funcs.append(self.printCover)
			funcs.append(self.printComposer)
			funcs.append(self.printInstrument)
			funcs.append(self.printSamplesMaterial)
			funcs.append(self.printRemixer)
			funcs.append(self.printLyricist)
			funcs.append(self.printProducer)

			for i in range(7):
				if len(relobj[i]) > 0:
					funcs[i](track, mbtrack, relobj[i])

		self.buf.insert(self.buf.get_end_iter(), "\n\n")

		if track.get(AttributeId.attr_artist).is_initialized() and track.get(AttributeId.attr_title).is_initialized():
			try:
				lyricwiki = mpx_boost.LyricWiki(track.get(AttributeId.attr_artist).val().get_string(), track.get(AttributeId.attr_title).val().get_string())
				lyrics = lyricwiki.run()
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

	def printCover(self, track, mbtrack, rels):

		for rel in rels:

			# Workaround for buggy pymusicbrainzl; getTarget() on this rel returns None
			url = rel.getTargetId()
			id = url[url.rindex("/")+1:]

			cover = self.q.getTrackById(id, self.tinc)	
			coverartist = cover.getArtist()

			self.buf.insert_with_tags(self.buf.get_end_iter(), "...is a cover of ")
			self.buf.insert_with_tags(self.buf.get_end_iter(), cover.getTitle(), self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), " by ")
			self.buf.insert_with_tags(self.buf.get_end_iter(), coverartist.getName(), self.textTagBold)
			self.buf.insert_with_tags(self.buf.get_end_iter(), "")
			self.buf.insert(self.buf.get_end_iter(), "\n") 

		return True

	def printComposer(self, track, mbtrack, rels):

		self.buf.insert_with_tags(self.buf.get_end_iter(), "...was composed by ")

		composers = []

		for rel in rels:
			composer = rel.getTarget()
			composers.append(composer.getName())

		
		self.buf.insert_with_tags(self.buf.get_end_iter(), ", ".join(composers), self.textTagBold) 
		self.buf.insert(self.buf.get_end_iter(), "\n") 
		return True

	def printSamplesMaterial(self, track, mbtrack, rels):

		sampled = []

		for rel in rels:
			url = rel.getTargetId()
			id = url[url.rindex("/")+1:]
			sampled = self.q.getTrackById(id, self.tinc)
			sampledartist = sampled.getArtist()
			sampled.append(sampled.getTitle() + " by " + sampledartist.getName())

		self.buf.insert_with_tags(self.buf.get_end_iter(), "...contains samples from ")
		self.buf.insert_with_tags(self.buf.get_end_iter(), ", ".join(sampled), self.textTagBold) 
		self.buf.insert(self.buf.get_end_iter(), "\n") 
		return True

	def printRemixer(self, track, mbtrack, rels):

		remixers = []

		for rel in rels:
			url = rel.getTargetId()
			id = url[url.rindex("/")+1:]
			remixer = self.q.getArtistById(id, self.tinc)
			remixers.append(remixer.getName())

		self.buf.insert_with_tags(self.buf.get_end_iter(), "...was remixed by ")
		self.buf.insert_with_tags(self.buf.get_end_iter(), ", ".join(remixers), self.textTagBold) 
		self.buf.insert(self.buf.get_end_iter(), "\n") 
		return True

	def printProducer(self, track, mbtrack, rels):

		producers = []

		for rel in rels:
			url = rel.getTargetId()
			id = url[url.rindex("/")+1:]
			producer = self.q.getArtistById(id, self.tinc)
			producers.append(producer.getName())
			
		self.buf.insert_with_tags(self.buf.get_end_iter(), "...was produced by ")
		self.buf.insert_with_tags(self.buf.get_end_iter(), ", ".join(producers), self.textTagBold) 
		self.buf.insert(self.buf.get_end_iter(), "\n") 
		return True

	def printLyricist(self, track, mbtrack, rels):

		lyricists = []

		for rel in rels:
			lyricist = rel.getTarget() 
			lyricists.append(lyricist.getName())

		self.buf.insert_with_tags(self.buf.get_end_iter(), "...has lyrics by ")
		self.buf.insert_with_tags(self.buf.get_end_iter(), ", ".join(lyricists), self.textTagBold) 
		self.buf.insert(self.buf.get_end_iter(), "\n") 
		return True

	def printProducer(self, track, mbtrack, rels):

		producers = []

		for rel in rels:
			producer = rel.getTarget() 
			producers.append(producer.getName())

		self.buf.insert_with_tags(self.buf.get_end_iter(), "...was produced by ")
		self.buf.insert_with_tags(self.buf.get_end_iter(), ", ".join(producers), self.textTagBold) 
		self.buf.insert(self.buf.get_end_iter(), "\n") 
		return True

	def printInstrument(self, track, mbtrack, rels):
		return True

info = TrackInfo()
