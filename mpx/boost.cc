#include <boost/python.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <gdkmm/pixbuf.h>
#include <Python.h>
#include "boost.hh"
#include "mpx/playbacksource.hh"
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

namespace bpy
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

BOOST_PYTHON_MODULE(mpx_boost)
{
	class_<MPX::OVariant>("optional")
		.def("val", (MPX::Variant& (MPX::OVariant::*) ()) &MPX::OVariant::get, return_internal_reference<>() /*return_value_policy<return_by_value>()*/) 
		.def("is_initialized", (bool (MPX::OVariant::*) ()) &MPX::OVariant::is_initialized, return_value_policy<return_by_value>()) 
		.def("init", &bpy::opt_init)
		.def("__repr__", &bpy::opt_repr, return_value_policy<return_by_value>())
	;

	class_<MPX::Variant >("variant")
		.def("get_int", &bpy::variant_getint, return_value_policy<return_by_value>()) 
		.def("set_int", &bpy::variant_setint)
		.def("get_string", &bpy::variant_getstring, return_value_policy<return_by_value>()) 
		.def("set_string", &bpy::variant_setstring)
		.def("get_double", &bpy::variant_getdouble, return_value_policy<return_by_value>()) 
		.def("set_double", &bpy::variant_setdouble)
		.def("__repr__", &bpy::variant_repr, return_value_policy<return_by_value>())
	;

	class_<MPX::Track >("track")
		.def("__getitem__", &bpy::track_getitem, return_value_policy<return_by_value>()) 
		.def("__len__", &bpy::track_len, return_value_policy<return_by_value>())
		.def("get", &bpy::track_getitem, return_value_policy<return_by_value>()) 
	;

	class_<MPX::Metadata >("metadata")
		.def("__getitem__", &bpy::metadata_getitem, return_value_policy<return_by_value>()) 
		.def("__len__", &bpy::metadata_len, return_value_policy<return_by_value>())
		.def("get", &bpy::metadata_getitem, return_value_policy<return_by_value>()) 
	;

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

	// Musicbrainz

	// LyricWiki	
	class_<MPX::LyricWiki::TextRequest, boost::noncopyable>("LyricWiki", init<std::string, std::string>())	
		.def("run", &MPX::LyricWiki::TextRequest::run)
	;

	// Last.fm
	class_<MPX::LastFM::XMLRPC::ArtistMetadataRequestSync, boost::noncopyable>("LastFMArtist", init<std::string>())
		.def("run", &MPX::LastFM::XMLRPC::ArtistMetadataRequestSync::run)
	;
}

namespace MPX
{
	void
	mpx_py_init ()
	{
		initmpx_boost();
	}
}
