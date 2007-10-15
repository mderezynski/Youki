INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)
noinst_LTLIBRARIES = libmpx_xiph_reader.la
libmpx_xiph_reader_la_LDFLAGS = -fPIC -module
libmpx_xiph_reader_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)
libmpx_xiph_reader_la_SOURCES =     \
      reader.cc											\
	  reader.hh

libmpx_xiph_readerdir = $(libdir)
