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

#include "mpx.hh"
#include "mpxpy.hh"
#include "mpx/amazon.hh"
#include "mpx/library.hh"
#include "mpx/paccess.hh"
#include "mpx/types.hh"

#include "playbacksource-py.hh"

#include "lyrics-v2.hh"
#include "last-fm-xmlrpc.hh"

using namespace boost::python;

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
	variant_getint(MPX::Variant  &self)
	{
		return boost::get<gint64>(self);
	}

	std::string	
	variant_getstring(MPX::Variant  &self)
	{
		return boost::get<std::string>(self);
	}

	void
	variant_setstring(MPX::Variant  &self, std::string const& value)
	{
		self = value;
	}

	double
	variant_getdouble(MPX::Variant  &self)
	{
		return boost::get<double>(self);
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

	MPX::OVariant
	track_getitem(MPX::Track &self, int id) 
	{
		return self[id];
	}

	MPX::OVariant
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
	metadata_len(MPX::Track &self)
	{
		return MPX::N_ATTRIBUTES_INT;
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

	void
	player_add_widget (MPX::Player & obj, PyObject * pyobj)
	{
		obj.add_widget(Glib::wrap(((GtkWidget*)(((PyGObject*)(pyobj))->obj)), false));
	}

	void
	player_remove_widget (MPX::Player & obj, PyObject * pyobj)
	{
		obj.remove_widget(Glib::wrap(((GtkWidget*)(((PyGObject*)(pyobj))->obj)), false));
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
		void
		activate (MPX::Player & player)
		{
		}

		void
		deactivate ()
		{
		}
	};
}

BOOST_PYTHON_MODULE(mpx)
{
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
		.def("__getitem__", &mpxpy::track_getitem, return_value_policy<return_by_value>()) 
		.def("__len__", &mpxpy::track_len, return_value_policy<return_by_value>())
		.def("get", &mpxpy::track_getitem, return_value_policy<return_by_value>()) 
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Metadata >("Metadata")
		.def("__getitem__", &mpxpy::metadata_getitem, return_value_policy<return_by_value>()) 
		.def("__len__", &mpxpy::metadata_len, return_value_policy<return_by_value>())
		.def("get", &mpxpy::metadata_getitem, return_value_policy<return_by_value>()) 
		.def("get_image", &MPX::Metadata::get_image)
	;

	/*-------------------------------------------------------------------------------------*/

	enum_<MPX::AttributeId>("AttributeId")
      .value("attr_location", MPX::MPX_ATTRIBUTE_LOCATION)
      .value("attr_title", MPX::MPX_ATTRIBUTE_TITLE)
      .value("attr_genre", MPX::MPX_ATTRIBUTE_GENRE)
      .value("attr_comment", MPX::MPX_ATTRIBUTE_COMMENT)
      .value("attr_musicip_puid", MPX::MPX_ATTRIBUTE_MUSICIP_PUID)
      .value("attr_hash", MPX::MPX_ATTRIBUTE_HASH)     
      .value("attr_mb_track_id", MPX::MPX_ATTRIBUTE_MB_TRACK_ID)
      .value("attr_artist", MPX::MPX_ATTRIBUTE_ARTIST)
      .value("attr_artist_sortname", MPX::MPX_ATTRIBUTE_ARTIST_SORTNAME)
      .value("attr_mb_artist_id", MPX::MPX_ATTRIBUTE_MB_ARTIST_ID)
      .value("attr_album", MPX::MPX_ATTRIBUTE_ALBUM)
      .value("attr_mb_album_id", MPX::MPX_ATTRIBUTE_MB_ALBUM_ID)
      .value("attr_mb_release_date", MPX::MPX_ATTRIBUTE_MB_RELEASE_DATE)
      .value("attr_asin", MPX::MPX_ATTRIBUTE_ASIN)
      .value("attr_album_artist", MPX::MPX_ATTRIBUTE_ALBUM_ARTIST)
      .value("attr_album_artist_sortname", MPX::MPX_ATTRIBUTE_ALBUM_ARTIST_SORTNAME)
      .value("attr_mb_album_artist_id", MPX::MPX_ATTRIBUTE_MB_ALBUM_ARTIST_ID)
      .value("attr_type", MPX::MPX_ATTRIBUTE_TYPE)
      .value("attr_hal_volume_udi", MPX::MPX_ATTRIBUTE_HAL_VOLUME_UDI)
      .value("attr_hal_device_udi", MPX::MPX_ATTRIBUTE_HAL_DEVICE_UDI)
      .value("attr_volume_relative_path", MPX::MPX_ATTRIBUTE_VOLUME_RELATIVE_PATH)
      .value("attr_insert_path", MPX::MPX_ATTRIBUTE_INSERT_PATH)
      .value("attr_location_name", MPX::MPX_ATTRIBUTE_LOCATION_NAME)
      .value("attr_track", MPX::MPX_ATTRIBUTE_TRACK)
      .value("attr_time", MPX::MPX_ATTRIBUTE_TIME)
      .value("attr_rating", MPX::MPX_ATTRIBUTE_RATING)
      .value("attr_date", MPX::MPX_ATTRIBUTE_DATE)
      .value("attr_mtime", MPX::MPX_ATTRIBUTE_MTIME)
      .value("attr_bitrate", MPX::MPX_ATTRIBUTE_BITRATE)
      .value("attr_samplerate", MPX::MPX_ATTRIBUTE_SAMPLERATE)
      .value("attr_count", MPX::MPX_ATTRIBUTE_COUNT)
      .value("attr_playdate", MPX::MPX_ATTRIBUTE_PLAYDATE)
      .value("attr_new_item", MPX::MPX_ATTRIBUTE_NEW_ITEM)
      .value("attr_is_mb_album_artist", MPX::MPX_ATTRIBUTE_IS_MB_ALBUM_ARTIST)
      .value("attr_active", MPX::MPX_ATTRIBUTE_ACTIVE)
      .value("attr_mpx_track_id", MPX::MPX_ATTRIBUTE_MPX_TRACK_ID)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::Player, boost::noncopyable>("Player", boost::python::no_init)
		.def("get_metadata", &MPX::Player::get_metadata, return_internal_reference<>()) 
		.def("gobj", &mpxpy::player_get_gobject)
		.def("play", &MPX::Player::play)
		.def("pause", &MPX::Player::play)
		.def("prev", &MPX::Player::play)
		.def("next", &MPX::Player::play)
		.def("stop", &MPX::Player::play)
		.def("get_library", &mpxpy::player_get_library, return_internal_reference<>()) 
		.def("add_widget", &mpxpy::player_add_widget)
		.def("remove_widget", &mpxpy::player_remove_widget)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::LyricWiki::TextRequest, boost::noncopyable>("LyricWiki", boost::python::init<std::string, std::string>())	
		.def("run", &MPX::LyricWiki::TextRequest::run)
	;

	/*-------------------------------------------------------------------------------------*/

	class_<MPX::LastFM::XMLRPC::ArtistMetadataRequestSync, boost::noncopyable>("LastFMArtist", boost::python::init<std::string>())
		.def("run", &MPX::LastFM::XMLRPC::ArtistMetadataRequestSync::run)
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
}

namespace MPX
{
	void
	mpx_py_init ()
	{
		initmpx();
	}
}
