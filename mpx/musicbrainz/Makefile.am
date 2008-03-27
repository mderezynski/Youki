noinst_LTLIBRARIES = libmpxmusicbrainz.la

libmpxmusicbrainz_la_CXXFLAGS = \
	$(SOUP_CFLAGS) $(BOOST_FILESYSTEM_CFLAGS) $(GLIBMM_CFLAGS) $(XML_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/mpx

libmpxmusicbrainz_la_LDFLAGS =  $(BOOST_FILESYSTEM_LIBS) $(SOUP_LIBS)

libmpxmusicbrainz_la_SOURCES =  		\
		mb-libxml2-sax-release.cc		\
		mb-libxml2-sax-release.hh		\
		mb-libxml2-sax-release-list.cc	\
		mb-libxml2-sax-release-list.hh	\
		mb-libxml2-sax-tracks.cc		\
		mb-libxml2-sax-tracks.hh		\
		mb-utils.hh						\
		mb-utils.cc						\
		mbxml-v2.hh						\
		mbxml-v2.cc						\
		mbxml-types-v2.hh

