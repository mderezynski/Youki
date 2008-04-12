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

#ifndef MPX_LAST_FM_XMLRPC_HH
#define MPX_LAST_FM_XMLRPC_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <exception>
#include <vector>
#include <queue>
#include <string>

#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <boost/variant.hpp>

#include "mcs/mcs.h"
#include "xspf.hh"
#include "mpx/minisoup.hh"
#include "mpx/xml.hh"

namespace MPX
{
  typedef std::vector <std::string> UStrV;
  typedef boost::variant <std::string, UStrV> xmlRpcVariant;
  typedef std::vector <xmlRpcVariant> xmlRpcVariantV;

  namespace LastFM
  {
      /////////////////////////////////////////////////
      ///// Requests
      /////////////////////////////////////////////////

      std::string formatXmlRpc (std::string const& /*method*/, xmlRpcVariantV const& /*argv*/);

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
          MPX::Soup::RequestRefP m_soup_request;
          SignalReply s_;
          XmlRpcCall ();
        public:
          virtual ~XmlRpcCall ();
          void cancel ();
          void setMethod (const std::string& method);
      };

      class XmlRpcCallSync
        : public Glib::Object
      {
        protected:
          xmlRpcVariantV m_v;
          MPX::Soup::RequestSyncRefP m_soup_request;
          XmlRpcCallSync ();
        public:
          virtual ~XmlRpcCallSync ();
          void setMethod (const std::string& /*method*/);
      };

      class RequestBase
        : public XmlRpcCallSync
      {
        public:
  
          RequestBase (std::string const& /*method*/, std::string const& /*user*/, std::string const& /*pass*/);
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

      class ArtistMetadataRequest;
      typedef Glib::RefPtr<ArtistMetadataRequest> ArtistMetadataRequestRefPtr;

      class ArtistMetadataRequest
        : public XmlRpcCall
      {
          std::string m_artist;
          void reply_cb (char const*, guint, guint);
        private: 
          ArtistMetadataRequest (std::string const& /*artist*/); 
        public:
          static ArtistMetadataRequestRefPtr create (std::string const& /*artist*/); 
          virtual ~ArtistMetadataRequest () {};
          void run ();
      };

      class ArtistMetadataRequestSync
        : public XmlRpcCallSync
      {
          std::string m_artist;
        public:
          ArtistMetadataRequestSync (std::string const& /*artist*/); 
          virtual ~ArtistMetadataRequestSync () {};
          std::string run ();
      };

      class TrackAction
        : public RequestBase
      {
        public:

          TrackAction (std::string const& /*method*/, XSPF::Item const& /*item*/, std::string const& /*user*/, std::string const& /*pass*/);
          virtual ~TrackAction () {}
      };
  }
}

#endif // !MPX_LAST_FM_HH

