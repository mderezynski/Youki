noinst_LTLIBRARIES = libmpx-json.la

libmpx_json_la_CFLAGS = 	\
	$(BMP_DEFINES) -I$(top_srcdir) 
libmpx_json_la_CXXFLAGS = \
	$(BMP_DEFINES) -I$(top_srcdir)

libmpx_json_la_SOURCES =  	        \
			json_reader.cpp			\
			json_value.cpp			\
			json_writer.cpp

noinst_HEADERS =					\
			forwards.h				\
			json_batchallocator.h	\
			json.h					\
			reader.h				\
			value.h					\
			writer.h

EXTRA_DIST = json_valueiterator.inl

