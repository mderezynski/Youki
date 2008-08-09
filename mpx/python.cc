#include <boost/python.hpp>
#include <boost/python/suite/indexing/indexing_suite.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/bind.hpp>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <pycairo/pycairo.h>
#include <gdkmm/pixbuf.h>
#include <libglademm/xml.h>
#include <Python.h>
#include "mcs/mcs.h"

#include "mpx.hh"
#include "mpx/mpx-covers.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL
#include "mpx/mpx-library.hh"
#include "mpx/mpx-lyrics.hh"
#include "mpx/mpx-protected-access.hh"
#include "mpx/algorithm/random.hh"
#include "mpx/mpx-types.hh"
#include "mpx/com/tagview.hh"
#include "mpx/util-graphics.hh"

#include "audio-types.hh"
#include "play.hh"
#include "pysigc.hh"

#include "mpx/mpx-python.hh"
#include "gtkmmmodule.h"

using namespace boost::python;

namespace
{
    bool py_initialized = false;
}

namespace
{
    struct PyGILLock
    {
        PyGILState_STATE m_state;

        PyGILLock ()
        {
            m_state = (PyGILState_STATE)(pyg_gil_state_ensure ());
        }

        ~PyGILLock ()
        {
            pyg_gil_state_release(m_state);
        }
    };
}

namespace MPX
{
    struct Plugin
    {
    };

    struct Bindable
    {
    };

    enum AttributeId
    {
      MPX_ATTRIBUTE_LOCATION,
      MPX_ATTRIBUTE_TITLE,

      MPX_ATTRIBUTE_GENRE,
      MPX_ATTRIBUTE_COMMENT,

      MPX_ATTRIBUTE_MUSICIP_PUID,

      MPX_ATTRIBUTE_HASH,     
      MPX_ATTRIBUTE_MB_TRACK_ID, 

      MPX_ATTRIBUTE_ARTIST,
      MPX_ATTRIBUTE_ARTIST_SORTNAME,
      MPX_ATTRIBUTE_MB_ARTIST_ID,

      MPX_ATTRIBUTE_ALBUM,
      MPX_ATTRIBUTE_MB_ALBUM_ID,
      MPX_ATTRIBUTE_MB_RELEASE_DATE,
      MPX_ATTRIBUTE_MB_RELEASE_COUNTRY,
      MPX_ATTRIBUTE_MB_RELEASE_TYPE,
      MPX_ATTRIBUTE_ASIN,

      MPX_ATTRIBUTE_ALBUM_ARTIST,
      MPX_ATTRIBUTE_ALBUM_ARTIST_SORTNAME,
      MPX_ATTRIBUTE_MB_ALBUM_ARTIST_ID,

      // MIME type
      MPX_ATTRIBUTE_TYPE,

      // HAL
      MPX_ATTRIBUTE_HAL_VOLUME_UDI,
      MPX_ATTRIBUTE_HAL_DEVICE_UDI,
      MPX_ATTRIBUTE_VOLUME_RELATIVE_PATH,
        
      // SQL
      MPX_ATTRIBUTE_INSERT_PATH,
      MPX_ATTRIBUTE_LOCATION_NAME,

      MPX_ATTRIBUTE_TRACK, 
      MPX_ATTRIBUTE_TIME,
      MPX_ATTRIBUTE_RATING,
      MPX_ATTRIBUTE_DATE,
      MPX_ATTRIBUTE_MTIME,
      MPX_ATTRIBUTE_BITRATE,
      MPX_ATTRIBUTE_SAMPLERATE,
      MPX_ATTRIBUTE_COUNT,
      MPX_ATTRIBUTE_PLAYDATE,
      MPX_ATTRIBUTE_NEW_ITEM,  
      MPX_ATTRIBUTE_IS_MB_ALBUM_ARTIST,

      MPX_ATTRIBUTE_ACTIVE,
      MPX_ATTRIBUTE_MPX_TRACK_ID,

      N_MPX_ATTRIBUTES_INT
    };
}

namespace mpxpy
{
    PyObject*
    ovariant_get (MPX::OVariant &self)
    {
        if(!self.is_initialized())
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        PyObject * obj = 0;

		switch(self.get().which())
		{
			case 0:
				obj = PyLong_FromLongLong((boost::get<gint64>(self.get())));
                break;
			case 1:
				obj = PyFloat_FromDouble((boost::get<double>(self.get())));
                break;
			case 2:
				obj = PyString_FromString((boost::get<std::string>(self.get())).c_str());
                break;
		}

        return obj;
    }
}

namespace mpxpy
{
	std::string
	variant_repr(MPX::Variant &self)
	{
		switch(self.which())
		{
			case 0:
				return boost::lexical_cast<std::string>(boost::get<gint64>(self));
			case 1:
				return boost::lexical_cast<std::string>(boost::get<double>(self));
			case 2:
				return boost::get<std::string>(self); 
		}

		return std::string();
	}
	
	gint64
	variant_getint(MPX::Variant &self)
	{
		gint64 i = boost::get<gint64>(self);
        return i;
	}

	void
	variant_setint(MPX::Variant &self, gint64 value)
	{
		self = value;
	}


	std::string	
	variant_getstring(MPX::Variant &self)
	{
		std::string s = boost::get<std::string>(self);
        return s;
	}

	void
	variant_setstring(MPX::Variant &self, std::string const& value)
	{
		self = value;
	}

	double
	variant_getdouble(MPX::Variant &self)
	{
		double d = boost::get<double>(self);
        return d;
	}

	void
	variant_setdouble(MPX::Variant &self, double const& value)
	{
		self = value;
	}

	void
	ovariant_setint(MPX::OVariant & self, gint64 value)
	{
		self = value;
	}

	void
	ovariant_setstring(MPX::OVariant &self, std::string const& value)
	{
		self = value;
	}

	void
	ovariant_setdouble(MPX::OVariant &self, double const& value)
	{
		self = value;
	}
}

namespace mpxpy
{
	MPX::OVariant &
	track_getitem(MPX::Track &self, int id) 
	{
		return self[id];
	}

	MPX::OVariant &
	metadata_getitem(MPX::Metadata &self, int id) 
	{
		return self[id];
	}

	int
	track_len(MPX::Track &self)
	{
		return MPX::N_ATTRIBUTES_INT;
	}

	int
	metadata_len(MPX::Metadata &self)
	{
		return MPX::N_ATTRIBUTES_INT;
	}

    bool	
	track_contains(MPX::Track &self, MPX::AttributeId id)
	{
		return self.has(id);
	}

    bool	
	metadata_contains(MPX::Metadata &self, MPX::AttributeId id)
	{
		return self.has(id);
	}

    std::string
    track_repr (MPX::Track &self)
    {
        return "mpx.Track";
    }

    std::string
    metadata_repr(MPX::Metadata &self)
    {
        return "mpx.Metadata";
    }

}

namespace mpxpy
{
	MPX::Library&
	player_get_library (MPX::Player & obj)
	{
		MPX::PAccess<MPX::Library> pa;	
		obj.get_object(pa);
		return pa.get();
	}

	MPX::Covers&
	player_get_covers (MPX::Player & obj)
	{
		MPX::PAccess<MPX::Covers> pa;	
		obj.get_object(pa);
		return pa.get();
	}

	MPX::Play&
	player_get_play (MPX::Player & obj)
	{
		MPX::PAccess<MPX::Play> pa;	
		obj.get_object(pa);
		return pa.get();
	}

    PyObject*
	player_get_ui (MPX::Player & obj)
	{
		return pygobject_new((GObject*)(obj.ui()->gobj()));
	}

#ifdef HAVE_HAL
	MPX::HAL&
	player_get_hal (MPX::Player & obj)
	{
		MPX::PAccess<MPX::HAL> pa;	
		obj.get_object(pa);
		return pa.get();
	}
#endif // HAVE_HAL

	void
	player_add_widget (MPX::Player & obj, PyObject * pyobj)
	{
		obj.add_widget(Glib::wrap(((GtkWidget*)(((PyGObject*)(pyobj))->obj)), false));
	}

	void
	player_add_info_widget (MPX::Player & obj, PyObject * pyobj, PyObject * name_py)
	{
        const char* name = PyString_AsString (name_py);

        g_return_if_fail(name != NULL);

		obj.add_info_widget(Glib::wrap(((GtkWidget*)(((PyGObject*)(pyobj))->obj)), false), name);
	}

	void
	player_remove_widget (MPX::Player & obj, PyObject * pyobj)
	{
		obj.remove_widget(Glib::wrap(((GtkWidget*)(((PyGObject*)(pyobj))->obj)), false));
	}

	void
	player_remove_info_widget (MPX::Player & obj, PyObject * pyobj)
	{
		obj.remove_info_widget(Glib::wrap(((GtkWidget*)(((PyGObject*)(pyobj))->obj)), false));
	}
}

namespace mpxpy
{
    Glib::RefPtr<Gdk::Pixbuf>
	covers_fetch (MPX::Covers & obj, std::string const& mbid)
	{
        Glib::RefPtr<Gdk::Pixbuf> cover (0);
        obj.fetch(mbid, cover);
        return cover; // if it stays 0, the converter takes care of it
	}
}

namespace mpxpy
{
    struct Play
    {
            static PyObject*
            tap (MPX::Play & obj)
            {
                return pygobject_new((GObject*)(obj.tap()));
            }

            static PyObject*
            pipeline (MPX::Play & obj)
            {
                return pygobject_new((GObject*)(obj.pipeline()));
            }
    };
}

namespace mpxpy
{
	MPX::Variant&
	sql_row_getitem (MPX::SQL::Row & self, std::string const& key)
	{
		return self[key];
	}

	void
	sql_row_setitem (MPX::SQL::Row & self, std::string const& key, MPX::Variant const& value)
	{
		self[key] = value;
	}
	
	int
	sql_row_len (MPX::SQL::Row & self)
	{
		return self.size();
	}
}

namespace mpxpy
{
    template <typename T>
    T
    unwrap_boxed (PyObject * obj)
    {
        PyGBoxed * boxed = ((PyGBoxed*)(obj));
        return *(reinterpret_cast<T*>(boxed->boxed));
    }
}


namespace pysigc
{
    struct sigc0_to_pysigc 
    {
        static PyObject* 
        convert(sigc::signal<void> const& signal)
        {
            object obj ((pysigc::sigc0(signal)));
            Py_INCREF(obj.ptr());
            return obj.ptr();
        }
    };

    template <typename T1>
    struct sigc1_to_pysigc
    {
        static PyObject* 
        convert(sigc::signal<void, T1> const& signal)
        {
            object obj ((pysigc::sigc1<T1>(signal)));
            Py_INCREF(obj.ptr());
            return obj.ptr();
        }
    };

    template <typename T1, typename T2> 
    struct sigc2_to_pysigc
    {
        static PyObject* 
        convert(sigc::signal<void, T1, T2> const& signal)
        {
            object obj ((pysigc::sigc2<T1, T2>(signal)));
            Py_INCREF(obj.ptr());
            return obj.ptr();
        }
    };

    template <typename T1, typename T2, typename T3> 
    struct sigc3_to_pysigc
    {
        static PyObject* 
        convert(sigc::signal<void, T1, T2, T3> const& signal)
        {
            object obj ((pysigc::sigc3<T1, T2, T3>(signal)));
            Py_INCREF(obj.ptr());
            return obj.ptr();
        }
    };

    void wrap_signal0()
    {
        class_< pysigc::sigc0 >(typeid(pysigc::sigc0).name(), boost::python::no_init)
        .def("connect",     &pysigc::sigc0::connect)
        .def("disconnect",  &pysigc::sigc0::disconnect)
        ;

        to_python_converter<sigc::signal<void>, sigc0_to_pysigc
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
		, false
#endif
		>(); 
    }

    template <typename T1>
    void wrap_signal1()
    {
        typedef typename sigc::signal<void, T1>   signal_sigc;
        typedef typename pysigc::sigc1<T1>        signal_pysigc;

        class_< signal_pysigc >(typeid(signal_pysigc).name(), boost::python::no_init)
        .def("connect",     &signal_pysigc::connect)
        .def("disconnect",  &signal_pysigc::disconnect)
        ;

        to_python_converter<signal_sigc, sigc1_to_pysigc<T1>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
		, false
#endif
		>(); 
    }

    template <typename T1, typename T2> 
    void wrap_signal2()
    {
        typedef typename sigc::signal<void, T1, T2>   signal_sigc;
        typedef typename pysigc::sigc2<T1, T2>        signal_pysigc;

        class_< signal_pysigc >(typeid(signal_pysigc).name(), boost::python::no_init)
        .def("connect",     &signal_pysigc::connect)
        .def("disconnect",  &signal_pysigc::disconnect)
        ;

        to_python_converter<signal_sigc, sigc2_to_pysigc<T1, T2>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
		, false
#endif
		>(); 
    }

    template <typename T1, typename T2, typename T3> 
    void wrap_signal3()
    {
        typedef typename sigc::signal<void, T1, T2, T3>   signal_sigc;
        typedef typename pysigc::sigc3<T1, T2, T3>        signal_pysigc;

        class_< signal_pysigc >(typeid(signal_pysigc).name(), boost::python::no_init)
        .def("connect",     &signal_pysigc::connect)
        .def("disconnect",  &signal_pysigc::disconnect)
        ;

        to_python_converter<signal_sigc, sigc3_to_pysigc<T1, T2, T3>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
		, false
#endif
		>(); 
    }
}

namespace mpxpy
{
    std::string
    get_config_dir ()
    {
        return Glib::build_filename(Glib::get_user_config_dir(), "mpx");
    }
}

BOOST_PYTHON_MODULE(mpx)
{
    to_python_converter<Glib::RefPtr<Gdk::Pixbuf>, mpxpy::refptr_to_gobject<Gdk::Pixbuf> 
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
	    , true
#endif
		    >();

    to_python_converter<Glib::RefPtr<Gtk::ListStore>, mpxpy::refptr_to_gobject<Gtk::ListStore>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
	    , true
#endif
		    >();

    to_python_converter<Glib::RefPtr<Gtk::TreeStore>, mpxpy::refptr_to_gobject<Gtk::TreeStore>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
	    , true
#endif
		    >();

    to_python_converter<Glib::RefPtr<Gtk::ActionGroup>, mpxpy::refptr_to_gobject<Gtk::ActionGroup>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
	    , true
#endif
		    >();


    to_python_converter<Glib::RefPtr<Gtk::UIManager>, mpxpy::refptr_to_gobject<Gtk::UIManager>
#if defined BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
	    , true
#endif
		    >();


    def("unwrap_boxed_mpxtrack", &mpxpy::unwrap_boxed<MPX::Track>, return_value_policy<return_by_value>());

	class_<MPX::Plugin>("Plugin")	
	;

    class_<MPX::Bindable>("Bindable")
    ;

    def ("rand", &MPX::rand);
    def ("get_config_dir", &mpxpy::get_config_dir);

	/*-------------------------------------------------------------------------------------*/

    enum_<MPX::PlayStatus>("PlayStatus")
        .value("NONE", MPX::PLAYSTATUS_NONE)
        .value("STOPPED", MPX::PLAYSTATUS_STOPPED)
        .value("PLAYING", MPX::PLAYSTATUS_PLAYING)
        .value("PAUSED", MPX::PLAYSTATUS_PAUSED)
        .value("SEEKING", MPX::PLAYSTATUS_SEEKING)
        .value("WAITING", MPX::PLAYSTATUS_WAITING)
    ;

	/*-------------------------------------------------------------------------------------*/

	enum_<MPX::Flags>("PlaybackSourceFlags")
		.value("F_NONE", MPX::F_NONE)	
		.value("F_ASYNC", MPX::F_ASYNC)	
		.value("F_HANDLE_STREAMINFO", MPX::F_HANDLE_STREAMINFO)	
		.value("F_PHONY_NEXT", MPX::F_PHONY_NEXT)	
		.value("F_PHONY_PREV", MPX::F_PHONY_PREV)	
		.value("F_HANDLE_LASTFM", MPX::F_HANDLE_LASTFM)	
		.value("F_HANDLE_LASTFM_ACTIONS", MPX::F_HANDLE_LASTFM_ACTIONS)	
		.value("F_USES_REPEAT", MPX::F_USES_REPEAT)	
		.value("F_USES_SHUFFLE", MPX::F_USES_SHUFFLE)	
	;

	enum_<MPX::Caps>("PlaybackSourceCaps")
		.value("C_NONE", MPX::C_NONE)	
		.value("C_CAN_GO_NEXT", MPX::C_CAN_GO_NEXT)	
		.value("C_CAN_GO_PREV", MPX::C_CAN_GO_PREV)	
		.value("C_CAN_PAUSE", MPX::C_CAN_PAUSE)	
		.value("C_CAN_PLAY", MPX::C_CAN_PLAY)	
		.value("C_CAN_SEEK", MPX::C_CAN_SEEK)	
		.value("C_CAN_RESTORE_CONTEXT", MPX::C_CAN_RESTORE_CONTEXT)	
		.value("C_CAN_PROVIDE_METADATA", MPX::C_CAN_PROVIDE_METADATA)	
		.value("C_CAN_BOOKMARK", MPX::C_CAN_BOOKMARK)	
		.value("C_PROVIDES_TIMING", MPX::C_PROVIDES_TIMING)	
	;

	class_<MPX::IdV>("IdVector")
		.def(vector_indexing_suite<MPX::IdV>());
	;

    class_<MPX::PlaybackSource, boost::noncopyable>("PlaybackSourceAPI", boost::python::no_init)
            .def("get_guid", &MPX::PlaybackSource::get_guid)
            .def("get_class_guid", &MPX::PlaybackSource::get_class_guid)
    ;

 	/*-------------------------------------------------------------------------------------*/

#if 0
	enum_<MPX::SQLID>("SQLID")
      .value("LOCATION", "location") 
      .value("TITLE", "title") 
      .value("GENRE", "genre") 
      .value("COMMENT", "comment")
      .value("MUSICIP_PUID", "musicip_puid") 
      .value("HASH", "hash") 
      .value("MB_TRACK_ID", "mb_track_id") 
      .value("ARTIST", "artist") 
      .value("ARTIST_SORTNAME", "artist_sortname") 
      .value("MB_ARTIST_ID", "mb_artist_id") 
      .value("ALBUM", "album")
      .value("MB_ALBUM_ID", "mb_album_id")
      .value("MB_RELEASE_DATE", "mb_release_date") 
      .value("ASIN", "amazon_asin") 
      .value("ALBUM_ARTIST", "album_artist") 
      .value("ALBUM_ARTIST_SORTNAME", "album_artist_sortname") 
      .value("MB_ALBUM_ARTIST_ID", "mb_album_artist_id")
      .value("TYPE", "type")
      .value("HAL_VOLUME_UDI", "hal_volume_udi")
      .value("HAL_DEVICE_UDI", "hal_device_udi")
      .value("VOLUME_RELATIVE_PATH", "hal_vrp")
      .value("INSERT_PATH", "insert_path")
      .value("LOCATION_NAME", "location_name")
      .value("TRACK", "track")
      .value("TIME", "time")
      .value("RATING", "rating")
      .value("DATE", "date")
      .value("MTIME", "mtime")
      .value("BITRATE", "bitrate")
      .value("SAMPLERATE", "samplerate")
      .value("PLAYCOUNT", "pcount")
      .value("PLAYDATE", "pdate")
      .value("IS_MB_ALBUM_ARTIST", "is_mb_album_artist")
      .value("ACTIVE", "active")
      .value("MPX_ID", "id")
	;
#endif

	enum_<MPX::AttributeId>("AttributeId")
      .value("LOCATION", MPX::MPX_ATTRIBUTE_LOCATION)
      .value("TITLE", MPX::MPX_ATTRIBUTE_TITLE)
      .value("GENRE", MPX::MPX_ATTRIBUTE_GENRE)
      .value("COMMENT", MPX::MPX_ATTRIBUTE_COMMENT)
      .value("MUSICIP_PUID", MPX::MPX_ATTRIBUTE_MUSICIP_PUID)
      .value("HASH", MPX::MPX_ATTRIBUTE_HASH)     
      .value("MB_TRACK_ID", MPX::MPX_ATTRIBUTE_MB_TRACK_ID)
      .value("ARTIST", MPX::MPX_ATTRIBUTE_ARTIST)
      .value("ARTIST_SORTNAME", MPX::MPX_ATTRIBUTE_ARTIST_SORTNAME)
      .value("MB_ARTIST_ID", MPX::MPX_ATTRIBUTE_MB_ARTIST_ID)
      .value("ALBUM", MPX::MPX_ATTRIBUTE_ALBUM)
      .value("MB_ALBUM_ID", MPX::MPX_ATTRIBUTE_MB_ALBUM_ID)
      .value("MB_RELEASE_DATE", MPX::MPX_ATTRIBUTE_MB_RELEASE_DATE)
      .value("MB_RELEASE_COUNTRY", MPX::MPX_ATTRIBUTE_MB_RELEASE_COUNTRY)
      .value("MB_RELEASE_TYPE", MPX::MPX_ATTRIBUTE_MB_RELEASE_TYPE)
      .value("ASIN", MPX::MPX_ATTRIBUTE_ASIN)
      .value("ALBUM_ARTIST", MPX::MPX_ATTRIBUTE_ALBUM_ARTIST)
      .value("ALBUM_ARTIST_SORTNAME", MPX::MPX_ATTRIBUTE_ALBUM_ARTIST_SORTNAME)
      .value("MB_ALBUM_ARTIST_ID", MPX::MPX_ATTRIBUTE_MB_ALBUM_ARTIST_ID)
      .value("TYPE", MPX::MPX_ATTRIBUTE_TYPE)
      .value("HAL_VOLUME_UDI", MPX::MPX_ATTRIBUTE_HAL_VOLUME_UDI)
      .value("HAL_DEVICE_UDI", MPX::MPX_ATTRIBUTE_HAL_DEVICE_UDI)
      .value("VOLUME_RELATIVE_PATH", MPX::MPX_ATTRIBUTE_VOLUME_RELATIVE_PATH)
      .value("INSERT_PATH", MPX::MPX_ATTRIBUTE_INSERT_PATH)
      .value("LOCATION_NAME", MPX::MPX_ATTRIBUTE_LOCATION_NAME)
      .value("TRACK", MPX::MPX_ATTRIBUTE_TRACK)
      .value("TIME", MPX::MPX_ATTRIBUTE_TIME)
      .value("RATING", MPX::MPX_ATTRIBUTE_RATING)
      .value("DATE", MPX::MPX_ATTRIBUTE_DATE)
      .value("MTIME", MPX::MPX_ATTRIBUTE_MTIME)
      .value("BITRATE", MPX::MPX_ATTRIBUTE_BITRATE)
      .value("SAMPLERATE", MPX::MPX_ATTRIBUTE_SAMPLERATE)
      .value("PLAYCOUNT", MPX::MPX_ATTRIBUTE_COUNT)
      .value("PLAYDATE", MPX::MPX_ATTRIBUTE_PLAYDATE)
      .value("IS_MB_ALBUM_ARTIST", MPX::MPX_ATTRIBUTE_IS_MB_ALBUM_ARTIST)
      .value("ACTIVE", MPX::MPX_ATTRIBUTE_ACTIVE)
      .value("MPX_TRACK_ID", MPX::MPX_ATTRIBUTE_MPX_TRACK_ID)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Player, boost::noncopyable>("Player", boost::python::no_init)

		.def("gobj",                &mpxpy::get_gobject<MPX::Player>)

		.def("get_status",          &MPX::Player::get_status) 

        .def("get_source",          &MPX::Player::get_source)

        .def("get_sources_by_class",&MPX::Player::get_sources_by_class)

		.def("get_metadata",        &MPX::Player::get_metadata,
                                    return_internal_reference<>())

        // FIXME: Deprecate the object getters by providing an interface to Services

		.def("get_library",         &mpxpy::player_get_library,
                                    return_internal_reference<>()) 

		.def("get_hal",             &mpxpy::player_get_hal,
                                    return_internal_reference<>()) 

        .def("get_covers",          &mpxpy::player_get_covers,
                                    return_internal_reference<>())

        .def("get_play",            &mpxpy::player_get_play,
                                    return_internal_reference<>())

        .def("ui",                  &mpxpy::player_get_ui)

        .def("info_set",            &MPX::Player::info_set)

        .def("info_clear",          &MPX::Player::info_clear)

        .def("push_message",        &MPX::Player::push_message)

        .def("deactivate_plugin",   &MPX::Player::deactivate_plugin)

		.def("play",                &MPX::Player::play)
		.def("pause",               &MPX::Player::pause)
		.def("prev",                &MPX::Player::prev)
		.def("next",                &MPX::Player::next)
		.def("stop",                &MPX::Player::stop)
        .def("play_uri",            &MPX::Player::play_uri)

        .def("add_info_widget",     &mpxpy::player_add_info_widget)
        .def("remove_info_widget",  &mpxpy::player_remove_info_widget)

		.def("add_widget",          &mpxpy::player_add_widget)
		.def("remove_widget",       &mpxpy::player_remove_widget)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::OVariant>("Optional")

        .def("__nonzero__", (bool (MPX::OVariant::*) ()) &MPX::OVariant::is_initialized,
                            return_value_policy<return_by_value>()) 

        .def("get",         &mpxpy::ovariant_get,
                            return_value_policy<return_by_value>())

		.def("set_int",     &mpxpy::ovariant_setint)
		.def("set_string",  &mpxpy::ovariant_setstring)
		.def("set_double",  &mpxpy::ovariant_setdouble)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Variant >("Variant")

		.def("set_int",     &mpxpy::variant_setint)
		.def("set_string",  &mpxpy::variant_setstring)
		.def("set_double",  &mpxpy::variant_setdouble)

		.def("get_int",     &mpxpy::variant_getint,
                            return_value_policy<return_by_value>()) 

		.def("get_string",  &mpxpy::variant_getstring,
                            return_value_policy<return_by_value>()) 

		.def("get_double",  &mpxpy::variant_getdouble,
                            return_value_policy<return_by_value>()) 
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Track >("Track")
		.def("__getitem__", &mpxpy::track_getitem,  return_internal_reference<>()) 
		.def("__len__",     &mpxpy::track_len,      return_value_policy<return_by_value>())
        .def("__contains__",&mpxpy::track_contains, return_value_policy<return_by_value>())
		.def("get",         &mpxpy::track_getitem,  return_value_policy<return_by_value>()) 
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Metadata >("Metadata")
		.def("__getitem__", &mpxpy::metadata_getitem,   return_internal_reference<>()) 
		.def("__len__",     &mpxpy::metadata_len,       return_value_policy<return_by_value>())
        .def("__contains__",&mpxpy::metadata_contains,  return_value_policy<return_by_value>())
		.def("get",         &mpxpy::metadata_getitem,   return_internal_reference<>()) 
		.def("get_image",   &MPX::Metadata::get_image)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::SQL::Row>("SQLRow")
		.def(map_indexing_suite<MPX::SQL::Row>())
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::SQL::RowV>("SQLRowV")
		.def(vector_indexing_suite<MPX::SQL::RowV>())
	;

	/*-------------------------------------------------------------------------------------*/

    pysigc::wrap_signal1<gint64>();
    pysigc::wrap_signal3<MPX::Track,gint64,gint64>();

	class_<MPX::Library, boost::noncopyable>("Library", boost::python::no_init)

		.def("getSQL", &MPX::Library::getSQL)
		.def("execSQL", &MPX::Library::execSQL)

		.def("getMetadata", &MPX::Library::getMetadata)

        .def("sqlToTrack", &MPX::Library::sqlToTrack)

        .def("getTrackTags", &MPX::Library::getTrackTags)

        .def("trackRated", &MPX::Library::trackRated)
        .def("trackPlayed", &MPX::Library::trackPlayed) // can't see how plugins could possibly need this
        .def("trackTagged", &MPX::Library::trackTagged)

        .def("markovUpdate", &MPX::Library::markovUpdate)
        .def("markovGetRandomProbableTrack", &MPX::Library::markovGetRandomProbableTrack)

        .def("collectionCreate", &MPX::Library::collectionCreate) 
        .def("collectionAppend", &MPX::Library::collectionAppend) 

        .def("signal_new_album",        &MPX::Library::signal_new_album, return_value_policy<return_by_value>())
        .def("signal_new_artist",       &MPX::Library::signal_new_artist, return_value_policy<return_by_value>())
        .def("signal_new_track",        &MPX::Library::signal_new_track, return_value_policy<return_by_value>())
        .def("signal_track_updated",    &MPX::Library::signal_track_updated, return_value_policy<return_by_value>())
	;

    typedef std::vector<double> IEEEV;

	class_<IEEEV>("IEEEV")
		.def(vector_indexing_suite<IEEEV>());
	;

	/*-------------------------------------------------------------------------------------*/

	class_<Mcs::Mcs, boost::noncopyable>("MCS", boost::python::no_init)
		.def("domain_register", &Mcs::Mcs::domain_register)
		.def("key_register", &Mcs::Mcs::key_register)
		.def("domain_key_exist", &Mcs::Mcs::domain_key_exist)
		.def("key_set_bool", &Mcs::Mcs::key_set<bool>)
		.def("key_set_int", &Mcs::Mcs::key_set<int>)
		.def("key_set_double", &Mcs::Mcs::key_set<double>)
		.def("key_set_string", &Mcs::Mcs::key_set<std::string>)
		.def("key_get_bool", &Mcs::Mcs::key_get<bool>)
		.def("key_get_int", &Mcs::Mcs::key_get<int>)
		.def("key_get_double", &Mcs::Mcs::key_get<double>)
		.def("key_get_string", &Mcs::Mcs::key_get<std::string>)
	;	

	/*-------------------------------------------------------------------------------------*/

    class_<MPX::TagView, boost::noncopyable>("TagView")
        .def("get_active_tag", &MPX::TagView::get_active_tag, return_internal_reference<>())
        .def("clear", &MPX::TagView::clear)
        .def("add_tag", &MPX::TagView::add_tag)
        .def("get_widget", &mpxpy::get_gobject<MPX::TagView>)
        .def("display", &MPX::TagView::display)
    ;

	/*-------------------------------------------------------------------------------------*/
    /* HAL */
	/*-------------------------------------------------------------------------------------*/

    class_<MPX::HAL::Volume>("HalVolume", boost::python::init<>())   
        .def_readwrite("volume_udi", &MPX::HAL::Volume::volume_udi)
        .def_readwrite("device_udi", &MPX::HAL::Volume::device_udi)
        .def_readwrite("label", &MPX::HAL::Volume::label)
        .def_readwrite("size", &MPX::HAL::Volume::size)
        .def_readwrite("mount_point", &MPX::HAL::Volume::mount_point)
        .def_readwrite("mount_time", &MPX::HAL::Volume::mount_time)
        .def_readwrite("device_file", &MPX::HAL::Volume::device_file)
        .def_readwrite("drive_serial", &MPX::HAL::Volume::drive_serial)
        .def_readwrite("drive_bus", &MPX::HAL::Volume::drive_bus)
        .def_readwrite("drive_type", &MPX::HAL::Volume::drive_type)
        .def_readwrite("drive_size", &MPX::HAL::Volume::drive_size)
        .def_readwrite("disc", &MPX::HAL::Volume::disc)
    ;

    class_<MPX::HAL>("MPXHal", boost::python::no_init)
    ;

    /*-------------------------------------------------------------------------------------*/
    /* Covers */
    /*-------------------------------------------------------------------------------------*/

    class_<MPX::Covers, boost::noncopyable>("Covers", boost::python::no_init)
        .def("fetch", &mpxpy::covers_fetch)
    ;

    /*-------------------------------------------------------------------------------------*/
    /* Play */
    /*-------------------------------------------------------------------------------------*/

    class_<MPX::Play, boost::noncopyable>("Play", boost::python::no_init)
        .def("tap", &mpxpy::Play::tap)
        .def("pipeline", &mpxpy::Play::pipeline)
    ;
}

namespace
{
    gpointer
    boxed_copy(gpointer boxed)
    {
        Py_INCREF(static_cast<PyObject*>(boxed));
        return boxed;
    }

    void 
    boxed_free(gpointer boxed)
    {
        Py_DECREF(static_cast<PyObject*>(boxed));
    }
}

namespace MPX
{
    void
    mpx_py_init ()
    {
        if(!py_initialized)
        {
            try {
                PyImport_AppendInittab((char*)"gtkmm", initgtkmm);
                PyImport_AppendInittab((char*)"mpx", initmpx);
                Py_Initialize();
                init_pygobject();
                init_pygtk();
                //pyg_enable_threads ();
                py_initialized = true;
            } catch( error_already_set ) {
                g_warning("%s; Python Error:", G_STRFUNC);
                PyErr_Print();
            }
        }
        else
            g_warning("%s: mpx_py_init called, but is already initialized!", G_STRLOC);
    }
}
