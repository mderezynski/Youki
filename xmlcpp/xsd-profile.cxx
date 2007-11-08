// Copyright (C) 2005-2007 Code Synthesis Tools CC
//
// This program was generated by CodeSynthesis XSD, an XML Schema to
// C++ data binding compiler.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
//
// In addition, as a special exception, Code Synthesis Tools CC gives
// permission to link this program with the Xerces-C++ library (or with
// modified versions of Xerces-C++ that use the same license as Xerces-C++),
// and distribute linked combinations including the two. You must obey
// the GNU General Public License version 2 in all respects for all of
// the code used other than Xerces-C++. If you modify this copy of the
// program, you may extend this exception to your version of the program,
// but you are not obligated to do so. If you do not wish to do so, delete
// this exception statement from your version.
//
// Furthermore, Code Synthesis Tools CC makes a special exception for
// the Free/Libre and Open Source Software (FLOSS) which is described
// in the accompanying FLOSSE file.
//

// Begin prologue.
//
//
// End prologue.

#include <xsd/cxx/pre.hxx>

#include "xsd-profile.hxx"

namespace LastFM
{
  // profile
  // 

  const profile::url_type& profile::
  url () const
  {
    return this->url_.get ();
  }

  profile::url_type& profile::
  url ()
  {
    return this->url_.get ();
  }

  void profile::
  url (const url_type& url)
  {
    this->url_.set (url);
  }

  void profile::
  url (::std::auto_ptr< url_type > url)
  {
    this->url_.set (url);
  }

  const profile::realname_optional& profile::
  realname () const
  {
    return this->realname_;
  }

  profile::realname_optional& profile::
  realname ()
  {
    return this->realname_;
  }

  void profile::
  realname (const realname_type& realname)
  {
    this->realname_.set (realname);
  }

  void profile::
  realname (const realname_optional& realname)
  {
    this->realname_ = realname;
  }

  void profile::
  realname (::std::auto_ptr< realname_type > realname)
  {
    this->realname_.set (realname);
  }

  const profile::mbox_sha1sum_type& profile::
  mbox_sha1sum () const
  {
    return this->mbox_sha1sum_.get ();
  }

  profile::mbox_sha1sum_type& profile::
  mbox_sha1sum ()
  {
    return this->mbox_sha1sum_.get ();
  }

  void profile::
  mbox_sha1sum (const mbox_sha1sum_type& mbox_sha1sum)
  {
    this->mbox_sha1sum_.set (mbox_sha1sum);
  }

  void profile::
  mbox_sha1sum (::std::auto_ptr< mbox_sha1sum_type > mbox_sha1sum)
  {
    this->mbox_sha1sum_.set (mbox_sha1sum);
  }

  const profile::registered_type& profile::
  registered () const
  {
    return this->registered_.get ();
  }

  profile::registered_type& profile::
  registered ()
  {
    return this->registered_.get ();
  }

  void profile::
  registered (const registered_type& registered)
  {
    this->registered_.set (registered);
  }

  void profile::
  registered (::std::auto_ptr< registered_type > registered)
  {
    this->registered_.set (registered);
  }

  const profile::age_optional& profile::
  age () const
  {
    return this->age_;
  }

  profile::age_optional& profile::
  age ()
  {
    return this->age_;
  }

  void profile::
  age (const age_type& age)
  {
    this->age_.set (age);
  }

  void profile::
  age (const age_optional& age)
  {
    this->age_ = age;
  }

  const profile::gender_optional& profile::
  gender () const
  {
    return this->gender_;
  }

  profile::gender_optional& profile::
  gender ()
  {
    return this->gender_;
  }

  void profile::
  gender (const gender_type& gender)
  {
    this->gender_.set (gender);
  }

  void profile::
  gender (const gender_optional& gender)
  {
    this->gender_ = gender;
  }

  void profile::
  gender (::std::auto_ptr< gender_type > gender)
  {
    this->gender_.set (gender);
  }

  const profile::country_optional& profile::
  country () const
  {
    return this->country_;
  }

  profile::country_optional& profile::
  country ()
  {
    return this->country_;
  }

  void profile::
  country (const country_type& country)
  {
    this->country_.set (country);
  }

  void profile::
  country (const country_optional& country)
  {
    this->country_ = country;
  }

  void profile::
  country (::std::auto_ptr< country_type > country)
  {
    this->country_.set (country);
  }

  const profile::playcount_optional& profile::
  playcount () const
  {
    return this->playcount_;
  }

  profile::playcount_optional& profile::
  playcount ()
  {
    return this->playcount_;
  }

  void profile::
  playcount (const playcount_type& playcount)
  {
    this->playcount_.set (playcount);
  }

  void profile::
  playcount (const playcount_optional& playcount)
  {
    this->playcount_ = playcount;
  }

  const profile::avatar_optional& profile::
  avatar () const
  {
    return this->avatar_;
  }

  profile::avatar_optional& profile::
  avatar ()
  {
    return this->avatar_;
  }

  void profile::
  avatar (const avatar_type& avatar)
  {
    this->avatar_.set (avatar);
  }

  void profile::
  avatar (const avatar_optional& avatar)
  {
    this->avatar_ = avatar;
  }

  void profile::
  avatar (::std::auto_ptr< avatar_type > avatar)
  {
    this->avatar_.set (avatar);
  }

  const profile::icon_optional& profile::
  icon () const
  {
    return this->icon_;
  }

  profile::icon_optional& profile::
  icon ()
  {
    return this->icon_;
  }

  void profile::
  icon (const icon_type& icon)
  {
    this->icon_.set (icon);
  }

  void profile::
  icon (const icon_optional& icon)
  {
    this->icon_ = icon;
  }

  void profile::
  icon (::std::auto_ptr< icon_type > icon)
  {
    this->icon_.set (icon);
  }

  const profile::id_optional& profile::
  id () const
  {
    return this->id_;
  }

  profile::id_optional& profile::
  id ()
  {
    return this->id_;
  }

  void profile::
  id (const id_type& id)
  {
    this->id_.set (id);
  }

  void profile::
  id (const id_optional& id)
  {
    this->id_ = id;
  }

  const profile::cluster_optional& profile::
  cluster () const
  {
    return this->cluster_;
  }

  profile::cluster_optional& profile::
  cluster ()
  {
    return this->cluster_;
  }

  void profile::
  cluster (const cluster_type& cluster)
  {
    this->cluster_.set (cluster);
  }

  void profile::
  cluster (const cluster_optional& cluster)
  {
    this->cluster_ = cluster;
  }

  const profile::username_optional& profile::
  username () const
  {
    return this->username_;
  }

  profile::username_optional& profile::
  username ()
  {
    return this->username_;
  }

  void profile::
  username (const username_type& username)
  {
    this->username_.set (username);
  }

  void profile::
  username (const username_optional& username)
  {
    this->username_ = username;
  }

  void profile::
  username (::std::auto_ptr< username_type > username)
  {
    this->username_.set (username);
  }


  // registered
  // 

  const registered::unixtime_optional& registered::
  unixtime () const
  {
    return this->unixtime_;
  }

  registered::unixtime_optional& registered::
  unixtime ()
  {
    return this->unixtime_;
  }

  void registered::
  unixtime (const unixtime_type& unixtime)
  {
    this->unixtime_.set (unixtime);
  }

  void registered::
  unixtime (const unixtime_optional& unixtime)
  {
    this->unixtime_ = unixtime;
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace LastFM
{
  // profile
  //

  profile::
  profile (const url_type& url,
           const mbox_sha1sum_type& mbox_sha1sum,
           const registered_type& registered)
  : ::xml_schema::type (),
    url_ (url, ::xml_schema::flags (), this),
    realname_ (::xml_schema::flags (), this),
    mbox_sha1sum_ (mbox_sha1sum, ::xml_schema::flags (), this),
    registered_ (registered, ::xml_schema::flags (), this),
    age_ (::xml_schema::flags (), this),
    gender_ (::xml_schema::flags (), this),
    country_ (::xml_schema::flags (), this),
    playcount_ (::xml_schema::flags (), this),
    avatar_ (::xml_schema::flags (), this),
    icon_ (::xml_schema::flags (), this),
    id_ (::xml_schema::flags (), this),
    cluster_ (::xml_schema::flags (), this),
    username_ (::xml_schema::flags (), this)
  {
  }

  profile::
  profile (const profile& x,
           ::xml_schema::flags f,
           ::xml_schema::type* c)
  : ::xml_schema::type (x, f, c),
    url_ (x.url_, f, this),
    realname_ (x.realname_, f, this),
    mbox_sha1sum_ (x.mbox_sha1sum_, f, this),
    registered_ (x.registered_, f, this),
    age_ (x.age_, f, this),
    gender_ (x.gender_, f, this),
    country_ (x.country_, f, this),
    playcount_ (x.playcount_, f, this),
    avatar_ (x.avatar_, f, this),
    icon_ (x.icon_, f, this),
    id_ (x.id_, f, this),
    cluster_ (x.cluster_, f, this),
    username_ (x.username_, f, this)
  {
  }

  profile::
  profile (const ::xercesc::DOMElement& e,
           ::xml_schema::flags f,
           ::xml_schema::type* c)
  : ::xml_schema::type (e, f | ::xml_schema::flags::base, c),
    url_ (f, this),
    realname_ (f, this),
    mbox_sha1sum_ (f, this),
    registered_ (f, this),
    age_ (f, this),
    gender_ (f, this),
    country_ (f, this),
    playcount_ (f, this),
    avatar_ (f, this),
    icon_ (f, this),
    id_ (f, this),
    cluster_ (f, this),
    username_ (f, this)
  {
    if ((f & ::xml_schema::flags::base) == 0)
    {
      ::xsd::cxx::xml::dom::parser< char > p (e);
      this->parse (p, f);
    }
  }

  void profile::
  parse (::xsd::cxx::xml::dom::parser< char >& p,
         ::xml_schema::flags f)
  {
    for (; p.more_elements (); p.next_element ())
    {
      const ::xercesc::DOMElement& i (p.cur_element ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (i));

      // url
      //
      if (n.name () == "url" && n.namespace_ () == "")
      {
        ::std::auto_ptr< url_type > r (
          url_traits::create (i, f, this));

        if (!url_.present ())
        {
          this->url (r);
          continue;
        }
      }

      // realname
      //
      if (n.name () == "realname" && n.namespace_ () == "")
      {
        ::std::auto_ptr< realname_type > r (
          realname_traits::create (i, f, this));

        if (!this->realname ())
        {
          this->realname (r);
          continue;
        }
      }

      // mbox_sha1sum
      //
      if (n.name () == "mbox_sha1sum" && n.namespace_ () == "")
      {
        ::std::auto_ptr< mbox_sha1sum_type > r (
          mbox_sha1sum_traits::create (i, f, this));

        if (!mbox_sha1sum_.present ())
        {
          this->mbox_sha1sum (r);
          continue;
        }
      }

      // registered
      //
      if (n.name () == "registered" && n.namespace_ () == "")
      {
        ::std::auto_ptr< registered_type > r (
          registered_traits::create (i, f, this));

        if (!registered_.present ())
        {
          this->registered (r);
          continue;
        }
      }

      // age
      //
      if (n.name () == "age" && n.namespace_ () == "")
      {
        if (!this->age ())
        {
          this->age (age_traits::create (i, f, this));
          continue;
        }
      }

      // gender
      //
      if (n.name () == "gender" && n.namespace_ () == "")
      {
        ::std::auto_ptr< gender_type > r (
          gender_traits::create (i, f, this));

        if (!this->gender ())
        {
          this->gender (r);
          continue;
        }
      }

      // country
      //
      if (n.name () == "country" && n.namespace_ () == "")
      {
        ::std::auto_ptr< country_type > r (
          country_traits::create (i, f, this));

        if (!this->country ())
        {
          this->country (r);
          continue;
        }
      }

      // playcount
      //
      if (n.name () == "playcount" && n.namespace_ () == "")
      {
        if (!this->playcount ())
        {
          this->playcount (playcount_traits::create (i, f, this));
          continue;
        }
      }

      // avatar
      //
      if (n.name () == "avatar" && n.namespace_ () == "")
      {
        ::std::auto_ptr< avatar_type > r (
          avatar_traits::create (i, f, this));

        if (!this->avatar ())
        {
          this->avatar (r);
          continue;
        }
      }

      // icon
      //
      if (n.name () == "icon" && n.namespace_ () == "")
      {
        ::std::auto_ptr< icon_type > r (
          icon_traits::create (i, f, this));

        if (!this->icon ())
        {
          this->icon (r);
          continue;
        }
      }

      break;
    }

    if (!url_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "url",
        "");
    }

    if (!mbox_sha1sum_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "mbox_sha1sum",
        "");
    }

    if (!registered_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "registered",
        "");
    }

    while (p.more_attributes ())
    {
      const ::xercesc::DOMAttr& i (p.next_attribute ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (i));

      if (n.name () == "id" && n.namespace_ ().empty ())
      {
        this->id (id_traits::create (i, f, this));
        continue;
      }

      if (n.name () == "cluster" && n.namespace_ ().empty ())
      {
        this->cluster (cluster_traits::create (i, f, this));
        continue;
      }

      if (n.name () == "username" && n.namespace_ ().empty ())
      {
        ::std::auto_ptr< username_type > r (
          username_traits::create (i, f, this));

        this->username (r);
        continue;
      }
    }
  }

  profile* profile::
  _clone (::xml_schema::flags f,
          ::xml_schema::type* c) const
  {
    return new profile (*this, f, c);
  }

  // registered
  //

  registered::
  registered ()
  : ::xml_schema::type (),
    unixtime_ (::xml_schema::flags (), this)
  {
  }

  registered::
  registered (const registered& x,
              ::xml_schema::flags f,
              ::xml_schema::type* c)
  : ::xml_schema::type (x, f, c),
    unixtime_ (x.unixtime_, f, this)
  {
  }

  registered::
  registered (const ::xercesc::DOMElement& e,
              ::xml_schema::flags f,
              ::xml_schema::type* c)
  : ::xml_schema::type (e, f | ::xml_schema::flags::base, c),
    unixtime_ (f, this)
  {
    if ((f & ::xml_schema::flags::base) == 0)
    {
      ::xsd::cxx::xml::dom::parser< char > p (e);
      this->parse (p, f);
    }
  }

  void registered::
  parse (::xsd::cxx::xml::dom::parser< char >& p,
         ::xml_schema::flags f)
  {
    while (p.more_attributes ())
    {
      const ::xercesc::DOMAttr& i (p.next_attribute ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (i));

      if (n.name () == "unixtime" && n.namespace_ ().empty ())
      {
        this->unixtime (unixtime_traits::create (i, f, this));
        continue;
      }
    }
  }

  registered* registered::
  _clone (::xml_schema::flags f,
          ::xml_schema::type* c) const
  {
    return new registered (*this, f, c);
  }
}

#include <istream>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xsd/cxx/xml/sax/std-input-source.hxx>
#include <xsd/cxx/tree/error-handler.hxx>

namespace LastFM
{
  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::std::string& u,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::tree::error_handler< char > h;

    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (u, h, p, f));

    h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

    ::std::auto_ptr< ::LastFM::profile > r (
      ::LastFM::profile_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::std::string& u,
            ::xml_schema::error_handler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (u, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::profile > r (
      ::LastFM::profile_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::std::string& u,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (u, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::profile > r (
      ::LastFM::profile_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::std::istream& is,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::profile_ (wrap, f, p);
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::std::istream& is,
            ::xml_schema::error_handler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::profile_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::std::istream& is,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::sax::std_input_source isrc (is);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::profile_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::std::istream& is,
            const ::std::string& sid,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::profile_ (wrap, f, p);
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::std::istream& is,
            const ::std::string& sid,
            ::xml_schema::error_handler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::profile_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::std::istream& is,
            const ::std::string& sid,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::profile_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::xercesc::DOMInputSource& i,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::tree::error_handler< char > h;

    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (i, h, p, f));

    h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

    ::std::auto_ptr< ::LastFM::profile > r (
      ::LastFM::profile_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::xercesc::DOMInputSource& i,
            ::xml_schema::error_handler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (i, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::profile > r (
      ::LastFM::profile_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::xercesc::DOMInputSource& i,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (i, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::profile > r (
      ::LastFM::profile_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (const ::xercesc::DOMDocument& d,
            ::xml_schema::flags f,
            const ::xml_schema::properties& p)
  {
    if (f & ::xml_schema::flags::keep_dom)
    {
      ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > c (
        static_cast< ::xercesc::DOMDocument* > (d.cloneNode (true)));

      ::std::auto_ptr< ::LastFM::profile > r (
        ::LastFM::profile_ (
          c.get (), f | ::xml_schema::flags::own_dom, p));

      c.release ();
      return r;
    }

    const ::xercesc::DOMElement& e (*d.getDocumentElement ());
    const ::xsd::cxx::xml::qualified_name< char > n (
      ::xsd::cxx::xml::dom::name< char > (e));

    if (n.name () == "profile" &&
        n.namespace_ () == "")
    {
      ::std::auto_ptr< ::LastFM::profile > r (
        ::xsd::cxx::tree::traits< ::LastFM::profile, char >::create (
          e, f, 0));
      return r;
    }

    throw ::xsd::cxx::tree::unexpected_element < char > (
      n.name (),
      n.namespace_ (),
      "profile",
      "");
  }

  ::std::auto_ptr< ::LastFM::profile >
  profile_ (::xercesc::DOMDocument* d,
            ::xml_schema::flags f,
            const ::xml_schema::properties&)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > c (
      ((f & ::xml_schema::flags::keep_dom) &&
       !(f & ::xml_schema::flags::own_dom))
      ? static_cast< ::xercesc::DOMDocument* > (d->cloneNode (true))
      : 0);

    const ::xercesc::DOMElement& e (
      c.get ()
      ? *c->getDocumentElement ()
      : *d->getDocumentElement ());
    const ::xsd::cxx::xml::qualified_name< char > n (
      ::xsd::cxx::xml::dom::name< char > (e));

    if (n.name () == "profile" &&
        n.namespace_ () == "")
    {
      ::std::auto_ptr< ::LastFM::profile > r (
        ::xsd::cxx::tree::traits< ::LastFM::profile, char >::create (
          e, f, 0));
      c.release ();
      return r;
    }

    throw ::xsd::cxx::tree::unexpected_element < char > (
      n.name (),
      n.namespace_ (),
      "profile",
      "");
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

