noinst_LTLIBRARIES = libmpxparser.la

libmpxparser_la_CXXFLAGS = \
	$(BOOST_FILESYSTEM_CFLAGS) $(GLIBMM_CFLAGS) $(XML_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/src

libmpxparser_la_LDFLAGS = $(BOOST_FILESYSTEM_LIBS)

libmpxparser_la_SOURCES =  \
		libxml2-sax-base.cc			\
		libxml2-sax-base.hh			\
		xpath.hh					\
		xpath.cc
