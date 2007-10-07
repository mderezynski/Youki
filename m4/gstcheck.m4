dnl -*- Mode: Autoconf; -*-

dnl BMP_WRAPPED_GST_CHECK([BODY])
AC_DEFUN([BMP_WRAPPED_GST_CHECK],
    [BMP_WRAPPED_CHECK(
         [CXXFLAGS="$GST_CFLAGS $GST_PLUGINS_BASE_CFLAGS"
          LIBS="$GST_LIBS $GST_PLUGINS_BASE_LIBS -lgstvideo-0.10 -lgstinterfaces-0.10"
          $1
         ])
    ]
)

dnl BMP_CHECK_GST_ELEMENT([ELEMENT])
AC_DEFUN([BMP_CHECK_GST_ELEMENT],
           [BMP_WRAPPED_GST_CHECK([AC_MSG_CHECKING([for GStreamer element $1])
                                   AC_TRY_RUN([
                                    #include <gst/gst.h>
                                    #include <gst/interfaces/xoverlay.h>
                                    #include <gst/video/gstvideosink.h>
                                    #include <gst/video/video.h>
                                    #include <cstdlib>
                                    #include <cstring>
                                    #include <glib.h>
                                    int main (int n_args, char **args)
                                    {
                                      g_type_init ();
                                      gst_init (&n_args, &args);
                                      GstElement *elmt = NULL;
                                      elmt = gst_element_factory_make ("$1", "test");
                                      if (elmt) return 0; 
                                      return 255; 
                                    }],
                                    AC_MSG_RESULT([available]),
                                    AC_MSG_ERROR([GStreamer element '$1' not available]))])])

dnl BMP_CHECK_GST_FACTORY([FACTORY])
AC_DEFUN([BMP_CHECK_GST_FACTORY],
           [BMP_WRAPPED_GST_CHECK([AC_MSG_CHECKING([for GStreamer factory $1])
                                   AC_TRY_RUN([
                                    #include <gst/gst.h>
                                    #include <gst/interfaces/xoverlay.h>
                                    #include <gst/video/gstvideosink.h>
                                    #include <gst/video/video.h>
                                    #include <cstring>
                                    #include <cstdlib>
                                    #include <glib.h>
                                    int main (int n_args, char ** args)
                                    {
                                      g_type_init ();
                                      gst_init (&n_args, &args);
                                      GstPlugin *plugin = NULL;
                                      plugin = gst_default_registry_find_plugin ("$1");
                                      if (plugin) return 0; 
                                      return 255; 
                                    }],
                                    AC_MSG_RESULT([available]),
                                    AC_MSG_ERROR([GStreamer factory '$1' not available]))])])

