noinst_LTLIBRARIES = libjnetlib.la

libjnetlib_la_LDFLAGS = -pthread $(GLIB_LIBS) $(GLIBMM_LIBS) $(SIGC_LIBS) $(GTK_LIBS)
libjnetlib_la_CXXFLAGS =  -Wall -DTHREAD_SAFE -D_THREAD_SAFE -D_REENTRANT $(GLIB_CFLAGS) $(SIGC_CFLAGS) $(GLIBMM_CFLAGS) $(GTK_CFLAGS)

libjnetlib_la_SOURCES =  	\
			asyncdns.cpp				\
			connection.cpp			\
			httpget.cpp					\
			httppost.cpp				\
			listen.cpp					\
			util.cpp		

noinst_HEADERS = exception.hh	\
			asyncdns.h							\
			connection.h						\
			httpget.h								\
			httppost.h							\
			httpserv.h							\
			jnetlib.h								\
			listen.h								\
			netinc.h								\
			sslconnection.h					\
			ssllisten.h							\
			udpconnection.h					\
			util.h

