#include <boost/python.hpp>
#include <boost/python/suite/indexing/indexing_suite.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <gdkmm/pixbuf.h>
#include <libglademm/xml.h>
#include <Python.h>
#include "mcs/mcs.h"

#include "mpx.hh"
#include "mpx/covers.hh"
#ifdef HAVE_HAL
#include "mpx/hal.hh"
#endif // HAVE_HAL
#include "mpx/library.hh"
#include "mpx/paccess.hh"
#include "mpx/types.hh"
#include "mpx/tagview.hh"

#include "audio-types.hh"
#include "lyrics-v2.hh"
#include "last-fm-xmlrpc.hh"
#include "playbacksource-py.hh"

#include "pysigc.hh"
#include "mpx-py.hh"

using namespace boost::python;

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
	
	void
	variant_setint(MPX::Variant  &self, gint64 value)
	{
		self = value;
	}

	gint64
	variant_getint(MPX::Variant &self)
	{
		gint64 i = boost::get<gint64>(self);
        return i;
	}

	std::string	
	variant_getstring(MPX::Variant &self)
	{
		std::string s = boost::get<std::string>(self);
        return s;
	}

	void
	variant_setstring(MPX::Variant  &self, std::string const& value)
	{
		self = value;
	}

	double
	variant_getdouble(MPX::Variant  &self)
	{
		double d = boost::get<double>(self);
        return d;
	}

	void
	variant_setdouble(MPX::Variant  &self, double const& value)
	{
		self = value;
	}

	std::string
	opt_repr(MPX::OVariant &self)
	{
		return variant_repr(self.get()); 
	}

	void
	opt_init(MPX::OVariant &self)
	{
		self = MPX::Variant();
	}

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
	PyObject*
	player_get_gobject (MPX::Player & obj)
	{
		return pygobject_new((GObject*)(obj.gobj()));
	}

	MPX::Library&
	player_get_library (MPX::Player & obj)
	{
		MPX::PAccess<MPX::Library> pa;	
		obj.get_object(pa);
		return pa.get();
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

	PyObject*
	tag_view_get_gobject (MPX::TagView & obj)
	{
		return pygobject_new((GObject*)(obj.gobj()));
	}
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

namespace MPX
{
	struct Plugin
	{
		bool
		activate (MPX::Player&, Mcs::Mcs&)
		{
			return false;
		}

		void
		deactivate ()
		{
		}
	};
}

namespace MPX
{
    namespace Soup
    {
        class PyRequest : public Request
        {
            public:

                PyRequest (std::string const& url, bool post = false)
                : Request(url, post)
                , m_signal0(new ::pysigc::sigc3<char const*, guint, guint>(Signals.Callback))
                {
                  g_object_ref(G_OBJECT(gobj()));
                }

                ::pysigc::sigc3<char const*, guint, guint> &
                pysignal_callback ()
                {
                    return *m_signal0;
                }

                virtual ~PyRequest ()
                {
                    delete m_signal0;
                }

            protected:

                ::pysigc::sigc3<char const*, guint, guint> * m_signal0; 
        };
    }
}

BOOST_PYTHON_MODULE(mpx)
{
	/*-------------------------------------------------------------------------------------*/

    class_< ::pysigc::sigc3<char const*, guint, guint>, boost::noncopyable>("Sigc3MiniSoupCallback", boost::python::no_init)
        .def("connect", &::pysigc::sigc3<char const*, guint, guint>::connect)
        .def("emit", &::pysigc::sigc3<char const*, guint, guint>::emit)
        .def("disconnect", &::pysigc::sigc3<char const*, guint, guint>::disconnect)
    ;

    class_<MPX::Soup::PyRequest, boost::noncopyable>("MPXSoupPyRequest", boost::python::init<std::string, bool>())
        .def("add_header", &MPX::Soup::PyRequest::add_header)
        .def("add_request", &MPX::Soup::PyRequest::add_request)
        .def("run", &MPX::Soup::PyRequest::run)
        .def("cancel", &MPX::Soup::PyRequest::cancel)
        .def("status", &MPX::Soup::PyRequest::status)
        .def("message_status", &MPX::Soup::PyRequest::message_status)
        .def("get_data_raw", &MPX::Soup::PyRequest::get_data_raw)
        .def("get_data_size", &MPX::Soup::PyRequest::get_data_size)
        .def("pysignal_callback", &MPX::Soup::PyRequest::pysignal_callback, return_internal_reference<>())
    ;
    
	/*-------------------------------------------------------------------------------------*/

	class_<MPX::OVariant>("Optional")
		.def("val", (MPX::Variant& (MPX::OVariant::*) ()) &MPX::OVariant::get, return_internal_reference<>() /*return_value_policy<return_by_value>()*/) 
		.def("is_initialized", (bool (MPX::OVariant::*) ()) &MPX::OVariant::is_initialized, return_value_policy<return_by_value>()) 
		.def("init", &mpxpy::opt_init)
		.def("__repr__", &mpxpy::opt_repr, return_value_policy<return_by_value>())
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Variant >("Variant")
		.def("get_int", &mpxpy::variant_getint, return_value_policy<return_by_value>()) 
		.def("set_int", &mpxpy::variant_setint)
		.def("get_string", &mpxpy::variant_getstring, return_value_policy<return_by_value>()) 
		.def("set_string", &mpxpy::variant_setstring)
		.def("get_double", &mpxpy::variant_getdouble, return_value_policy<return_by_value>()) 
		.def("set_double", &mpxpy::variant_setdouble)
		.def("__repr__", &mpxpy::variant_repr, return_value_policy<return_by_value>())
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Track >("Track")
		.def("__getitem__", &mpxpy::track_getitem, /*return_value_policy<return_by_value>()*/ return_internal_reference<>()) 
		.def("__len__", &mpxpy::track_len, return_value_policy<return_by_value>())
        .def("__repr__", &mpxpy::track_repr, return_value_policy<return_by_value>()) 
		.def("get", &mpxpy::track_getitem, return_value_policy<return_by_value>()) 
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Metadata >("Metadata")
		.def("__getitem__", &mpxpy::metadata_getitem, /*return_value_policy<return_by_value>()*/ return_internal_reference<>()) 
		.def("__len__", &mpxpy::metadata_len, return_value_policy<return_by_value>())
        .def("__repr__", &mpxpy::metadata_repr, return_value_policy<return_by_value>()) 
		.def("get", &mpxpy::metadata_getitem, /*return_value_policy<return_by_value>()*/ return_internal_reference<>()) 
		.def("get_image", &MPX::Metadata::get_image)
	;

	/*-------------------------------------------------------------------------------------*/

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
		.def("get_metadata", &MPX::Player::get_metadata, return_internal_reference<>())
		.def("gobj", &mpxpy::player_get_gobject)
		.def("play", &MPX::Player::play)
		.def("pause", &MPX::Player::pause_ext)
		.def("prev", &MPX::Player::prev)
		.def("next", &MPX::Player::next)
		.def("stop", &MPX::Player::stop)
        .def("play_uri", &MPX::Player::play_uri)
		.def("get_library", &mpxpy::player_get_library, return_internal_reference<>()) 
		.def("get_hal", &mpxpy::player_get_hal, return_internal_reference<>()) 
		.def("add_widget", &mpxpy::player_add_widget)
        .def("add_info_widget", &mpxpy::player_add_info_widget)
		.def("remove_widget", &mpxpy::player_remove_widget)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::LyricWiki::TextRequest, boost::noncopyable>("LyricWiki", boost::python::init<std::string, std::string>())	
		.def("run", &MPX::LyricWiki::TextRequest::run)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::LastFM::ArtistMetadataRequestSync, boost::noncopyable>("LastFMArtist", boost::python::init<std::string>())
		.def("run", &MPX::LastFM::ArtistMetadataRequestSync::run)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Plugin>("Plugin")	
		.def("activate", &MPX::Plugin::activate)
		.def("deactivate", &MPX::Plugin::deactivate)
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

	class_<MPX::Library, boost::noncopyable>("Library", boost::python::no_init)
		.def("getSQL", &MPX::Library::getSQL)
		.def("execSQL", &MPX::Library::execSQL)
		.def("getMetadata", &MPX::Library::getMetadata)
	;

	/*-------------------------------------------------------------------------------------*/

	enum_<MPX::PlaybackSource::Flags>("PlaybackSourceFlags")
		.value("F_NONE", MPX::PlaybackSource::F_NONE)	
		.value("F_ASYNC", MPX::PlaybackSource::F_ASYNC)	
		.value("F_HANDLE_STREAMINFO", MPX::PlaybackSource::F_HANDLE_STREAMINFO)	
		.value("F_PHONY_NEXT", MPX::PlaybackSource::F_PHONY_NEXT)	
		.value("F_PHONY_PREV", MPX::PlaybackSource::F_PHONY_PREV)	
		.value("F_HANDLE_LASTFM", MPX::PlaybackSource::F_HANDLE_LASTFM)	
		.value("F_HANDLE_LASTFM_ACTIONS", MPX::PlaybackSource::F_HANDLE_LASTFM_ACTIONS)	
		.value("F_USES_REPEAT", MPX::PlaybackSource::F_USES_REPEAT)	
		.value("F_USES_SHUFFLE", MPX::PlaybackSource::F_USES_SHUFFLE)	
	;

	enum_<MPX::PlaybackSource::Caps>("PlaybackSourceCaps")
		.value("C_NONE", MPX::PlaybackSource::C_NONE)	
		.value("C_CAN_GO_NEXT", MPX::PlaybackSource::C_CAN_GO_NEXT)	
		.value("C_CAN_GO_PREV", MPX::PlaybackSource::C_CAN_GO_PREV)	
		.value("C_CAN_PAUSE", MPX::PlaybackSource::C_CAN_PAUSE)	
		.value("C_CAN_PLAY", MPX::PlaybackSource::C_CAN_PLAY)	
		.value("C_CAN_SEEK", MPX::PlaybackSource::C_CAN_SEEK)	
		.value("C_CAN_RESTORE_CONTEXT", MPX::PlaybackSource::C_CAN_RESTORE_CONTEXT)	
		.value("C_CAN_PROVIDE_METADATA", MPX::PlaybackSource::C_CAN_PROVIDE_METADATA)	
		.value("C_CAN_BOOKMARK", MPX::PlaybackSource::C_CAN_BOOKMARK)	
		.value("C_PROVIDES_TIMING", MPX::PlaybackSource::C_PROVIDES_TIMING)	
	;

    enum_<MPX::MPXPlaystatus>("PlayStatus")
        .value("NONE", MPX::PLAYSTATUS_NONE)
        .value("STOPPED", MPX::PLAYSTATUS_STOPPED)
        .value("PLAYING", MPX::PLAYSTATUS_PLAYING)
        .value("PAUSED", MPX::PLAYSTATUS_PAUSED)
        .value("SEEKING", MPX::PLAYSTATUS_SEEKING)
        .value("WAITING", MPX::PLAYSTATUS_WAITING)
    ;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::PlaybackSourcePy, boost::noncopyable>("PlaybackSource", boost::python::no_init)
		.def("getUri", &MPX::PlaybackSourcePy::get_uri)
		.def("getType", &MPX::PlaybackSourcePy::get_type)
		.def("play", &MPX::PlaybackSourcePy::play)
		.def("goNext", &MPX::PlaybackSourcePy::go_next)
		.def("goPrev", &MPX::PlaybackSourcePy::go_prev)
		.def("stop", &MPX::PlaybackSourcePy::stop)
		.def("playAsync", &MPX::PlaybackSourcePy::play_async)
		.def("goNextAsync", &MPX::PlaybackSourcePy::go_next_async)
		.def("goPrevAsync", &MPX::PlaybackSourcePy::go_prev_async)
		.def("playPost", &MPX::PlaybackSourcePy::play_post)
		.def("nextPost", &MPX::PlaybackSourcePy::next_post)
		.def("prevPost", &MPX::PlaybackSourcePy::prev_post)
		.def("restoreContext", &MPX::PlaybackSourcePy::restore_context)
		.def("skipped", &MPX::PlaybackSourcePy::skipped)
		.def("segment", &MPX::PlaybackSourcePy::segment)
		.def("bufferingDone", &MPX::PlaybackSourcePy::buffering_done)
		.def("sendCaps", &MPX::PlaybackSourcePy::send_caps)
		.def("sendFlags", &MPX::PlaybackSourcePy::send_flags)
		.def("getName", &MPX::PlaybackSourcePy::get_name)
		.def("getUi", &MPX::PlaybackSourcePy::get_ui, return_internal_reference<>())
		.def("player", &MPX::PlaybackSourcePy::player, return_internal_reference<>())
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
        .def("get_widget", &mpxpy::tag_view_get_gobject)
    ;

	/*-------------------------------------------------------------------------------------*/
    /* HAL */
	/*-------------------------------------------------------------------------------------*/

    class_<MPX::HAL::Volume>("MPXHalVolume", boost::python::init<>())   
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
}

namespace MPX
{
	void
	mpx_py_init ()
	{
		initmpx();
	}
}
