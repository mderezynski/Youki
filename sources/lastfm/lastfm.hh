//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifndef MPX_LAST_FM_HH
#define MPX_LAST_FM_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <exception>
#include <vector>
#include <queue>
#include <string>

#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <gtkmm.h>
#include <libglademm.h>
#include <boost/variant.hpp>

#include "mcs/mcs.h"
#include "mpx/minisoup.hh"
#include "xspf.hh"

using namespace Glib;

namespace MPX
{
namespace LastFMXMLRPC
{
    typedef std::vector <Glib::ustring> UStrV;
    typedef boost::variant <Glib::ustring, UStrV> xmlRpcVariant;
    typedef std::vector <xmlRpcVariant> xmlRpcVariantV;

    // User
    struct User
    { 
      ustring	username;
      ustring	url;
      ustring	image;
      double match;
    };
    typedef std::vector <User> UserV;

    // Tag
    struct Tag
    {
      ustring name;
      ustring url;
      guint64 count;
    };
    typedef std::vector <Tag> TagV;
    typedef std::pair   <Tag, double> TagRankP;
    typedef std::vector <TagRankP> RankedTagV;

    //bool operator< (TagRankP const& a, TagRankP const& b);
    //bool operator< (Tag const& a, Tag const& b);

    // LastFMArtist
    struct LastFMArtist
    {
      ustring name;
      ustring mbid;
      ustring url;
      ustring thumbnail;
      ustring image;
      guint64 count;
      guint64 rank;
      bool    streamable;

      LastFMArtist () : streamable(false) {}
    };
    typedef std::vector <LastFMArtist> LastFMArtistV;

    enum TagGlobalType
    {
      TAGS_GLOBAL_ARTIST,
      TAGS_GLOBAL_TRACK, 
    };

    enum TagUserType
    {
      TAGS_USER_TOPTAGS,
      TAGS_USER_ARTIST,
      TAGS_USER_ALBUM,
      TAGS_USER_TRACK, 
    };

    enum UserType
    {
      UT_N,
      UT_F, 
    };

    enum ArtistType
    {
      AT_USER_TOP,
      AT_USER_SYSTEMREC,
      AT_TAG_TOP,
      AT_SIMILAR
    };

    /////////////////////////////////////////////////
    ///// Requests
    /////////////////////////////////////////////////

    ustring formatXmlRpc (ustring const& method, xmlRpcVariantV const& argv);

    class XmlRpcCall; 
    typedef Glib::RefPtr<XmlRpcCall> XmlRpcCallRefPtr;

    class XmlRpcCall
      : public Glib::Object
    {
      public:
        typedef sigc::signal<void, std::string const&, guint> SignalReply;
      public:
        SignalReply & reply() { return s_; }
      protected:
        xmlRpcVariantV m_v;
        Soup::RequestRefP m_soup_request;
        SignalReply s_;
        XmlRpcCall ();
      public:
        virtual ~XmlRpcCall ();
        void cancel ();
        void setMethod (const ustring& method);
    };

    class XmlRpcCallSync
      : public Glib::Object
    {
      protected:
        xmlRpcVariantV m_v;
        Soup::RequestSyncRefP m_soup_request;
        XmlRpcCallSync ();
      public:
        virtual ~XmlRpcCallSync ();
        void setMethod (const ustring& method);
    };

    class ArtistMetadataRequest;
    typedef Glib::RefPtr<ArtistMetadataRequest> ArtistMetadataRequestRefPtr;

    class ArtistMetadataRequest
      : public XmlRpcCall
    {
        ustring m_artist;
        void reply_cb (char const* data, guint size, guint status_code);
      private: 
        ArtistMetadataRequest (ustring const& artist); 
      public:
        static ArtistMetadataRequestRefPtr create (ustring const& artist); 
        virtual ~ArtistMetadataRequest () {};
        void run ();
    };

    class RequestBase
      : public XmlRpcCallSync
    {
      public:

        RequestBase (ustring const& method);
        virtual ~RequestBase () {};

        void run ();

      protected:

        std::string   m_name;
        std::string   m_pass;
        std::string   m_user;
        std::string   m_time;
        std::string   m_key;
        std::string   m_kmd5;
    };

    class TrackAction
      : public RequestBase
    {
      public:

        TrackAction (ustring const& method, XSPF::Item const& item);
        virtual ~TrackAction () {}
    };

    class TagAction 
      : public RequestBase
    {
      public:

        TagAction (ustring const& method, XSPF::Item const& item, UStrV const& tags);
        virtual ~TagAction () {}
    };

    class RecommendAction
      : public RequestBase
    {
      public:

        enum RecommendItem
        {
          RECOMMEND_TRACK,
          RECOMMEND_ARTIST,
          RECOMMEND_ALBUM,
        };

        RecommendAction (RecommendItem item,
                    ustring const& artist, ustring const& album, ustring const& title, ustring const& user, ustring const& message); 
        virtual ~RecommendAction () {}
    };
}
}


namespace MPX
{
    typedef sigc::signal<void, XSPF::Playlist const&> SignalPlaylist;
    typedef sigc::signal<void, Glib::ustring const&>  SignalTuneError; 
    typedef sigc::signal<void> Signal;

    enum LFMErrorCode
    {
        LFM_ERROR_NOT_ENOUGH_CONTENT = 1, // There is not enough content to play this station.
        LFM_ERROR_FEWGROUPMEMBERS,        // This group does not have enough members for radio.
        LFM_ERROR_FEWFANS,                // This artist does not have enough fans for radio.
        LFM_ERROR_UNAVAILABLE,            // This item is not available for streaming.
        LFM_ERROR_SUBSCRIBE,              // This feature is only available to Subscribers.
        LFM_ERROR_FEWNEIGHBOURS,          // There are not enough neighbours for this radio.
        LFM_ERROR_OFFLINE                 // The streaming system is offline for maintenance, please try again later
    };

    /** Client code for Last.fm LastFMRadio
     *
     **/
    class LastFMRadio
    {
      public:

#include "mpx/exception.hh"

    EXCEPTION(LastFMStreamTuningError)
    EXCEPTION(LastFMNotConnectedError)

        struct Session
        {
          std::string SessionId;
          std::string BaseUrl;
          std::string BasePath;
          bool        Subscriber;

          Session ()
          : Subscriber (0)
          {}
        };

      public:

        LastFMRadio ();
        ~LastFMRadio ();

        Session const& session () const;

        bool  connected () const;
        void  handshake ();
        void  disconnect ();

        void  get_xspf_playlist ();
        void  get_xspf_playlist_cancel ();

        void  playurl (std::string const& url);
        bool  is_playlist (std::string const& url);
 
      private:

        struct SignalsT
        {
          Signal          Disconnected;
          Signal          Connected;
          Signal          NoPlaylist;
          SignalPlaylist  Playlist;
          SignalTuneError TuneError;
          Signal          Tuned;
        };

        SignalsT Signals;
    
      public:

        Signal&
        signal_tuned();

        SignalTuneError&
        signal_tune_error();

        Signal&
        signal_connected();

        Signal&
        signal_disconnected();

        SignalPlaylist&
        signal_playlist ();

        Signal&
        signal_no_playlist ();

      private:

        void  handshake_cb (char const * data, guint size, guint code);
        void  playlist_cb (char const * data, guint size, guint code);
        void  playurl_cb (char const * data, guint size, guint code, bool); 

        void  parse_playlist (char const * data, guint size);

        Soup::RequestRefP m_HandshakeRequest;
        Soup::RequestRefP m_PlaylistRequest;
        Soup::RequestRefP m_PlayURLRequest;

        Session m_Session;
        bool m_IsConnected;
    };
}

#endif // !MPX_LAST_FM_HH

