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

#include "xsd-mostknowntracks.hxx"

namespace LastFM
{
  // mostknowntracks
  // 

  const mostknowntracks::track_sequence& mostknowntracks::
  track () const
  {
    return this->track_;
  }

  mostknowntracks::track_sequence& mostknowntracks::
  track ()
  {
    return this->track_;
  }

  void mostknowntracks::
  track (const track_sequence& track)
  {
    this->track_ = track;
  }

  const mostknowntracks::artist_optional& mostknowntracks::
  artist () const
  {
    return this->artist_;
  }

  mostknowntracks::artist_optional& mostknowntracks::
  artist ()
  {
    return this->artist_;
  }

  void mostknowntracks::
  artist (const artist_type& artist)
  {
    this->artist_.set (artist);
  }

  void mostknowntracks::
  artist (const artist_optional& artist)
  {
    this->artist_ = artist;
  }

  void mostknowntracks::
  artist (::std::auto_ptr< artist_type > artist)
  {
    this->artist_.set (artist);
  }


  // track
  // 

  const track::name_type& track::
  name () const
  {
    return this->name_.get ();
  }

  track::name_type& track::
  name ()
  {
    return this->name_.get ();
  }

  void track::
  name (const name_type& name)
  {
    this->name_.set (name);
  }

  void track::
  name (::std::auto_ptr< name_type > name)
  {
    this->name_.set (name);
  }

  const track::mbid_type& track::
  mbid () const
  {
    return this->mbid_.get ();
  }

  track::mbid_type& track::
  mbid ()
  {
    return this->mbid_.get ();
  }

  void track::
  mbid (const mbid_type& mbid)
  {
    this->mbid_.set (mbid);
  }

  void track::
  mbid (::std::auto_ptr< mbid_type > mbid)
  {
    this->mbid_.set (mbid);
  }

  const track::reach_type& track::
  reach () const
  {
    return this->reach_.get ();
  }

  track::reach_type& track::
  reach ()
  {
    return this->reach_.get ();
  }

  void track::
  reach (const reach_type& reach)
  {
    this->reach_.set (reach);
  }

  const track::url_type& track::
  url () const
  {
    return this->url_.get ();
  }

  track::url_type& track::
  url ()
  {
    return this->url_.get ();
  }

  void track::
  url (const url_type& url)
  {
    this->url_.set (url);
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace LastFM
{
  // mostknowntracks
  //

  mostknowntracks::
  mostknowntracks ()
  : ::xml_schema::type (),
    track_ (::xml_schema::flags (), this),
    artist_ (::xml_schema::flags (), this)
  {
  }

  mostknowntracks::
  mostknowntracks (const mostknowntracks& x,
                   ::xml_schema::flags f,
                   ::xml_schema::type* c)
  : ::xml_schema::type (x, f, c),
    track_ (x.track_, f, this),
    artist_ (x.artist_, f, this)
  {
  }

  mostknowntracks::
  mostknowntracks (const ::xercesc::DOMElement& e,
                   ::xml_schema::flags f,
                   ::xml_schema::type* c)
  : ::xml_schema::type (e, f | ::xml_schema::flags::base, c),
    track_ (f, this),
    artist_ (f, this)
  {
    if ((f & ::xml_schema::flags::base) == 0)
    {
      ::xsd::cxx::xml::dom::parser< char > p (e);
      this->parse (p, f);
    }
  }

  void mostknowntracks::
  parse (::xsd::cxx::xml::dom::parser< char >& p,
         ::xml_schema::flags f)
  {
    for (; p.more_elements (); p.next_element ())
    {
      const ::xercesc::DOMElement& i (p.cur_element ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (i));

      // track
      //
      if (n.name () == "track" && n.namespace_ () == "LastFM")
      {
        ::std::auto_ptr< track_type > r (
          track_traits::create (i, f, this));

        this->track ().push_back (r);
        continue;
      }

      break;
    }

    while (p.more_attributes ())
    {
      const ::xercesc::DOMAttr& i (p.next_attribute ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (i));

      if (n.name () == "artist" && n.namespace_ ().empty ())
      {
        ::std::auto_ptr< artist_type > r (
          artist_traits::create (i, f, this));

        this->artist (r);
        continue;
      }
    }
  }

  mostknowntracks* mostknowntracks::
  _clone (::xml_schema::flags f,
          ::xml_schema::type* c) const
  {
    return new mostknowntracks (*this, f, c);
  }

  // track
  //

  track::
  track (const name_type& name,
         const mbid_type& mbid,
         const reach_type& reach,
         const url_type& url)
  : ::xml_schema::type (),
    name_ (name, ::xml_schema::flags (), this),
    mbid_ (mbid, ::xml_schema::flags (), this),
    reach_ (reach, ::xml_schema::flags (), this),
    url_ (url, ::xml_schema::flags (), this)
  {
  }

  track::
  track (const track& x,
         ::xml_schema::flags f,
         ::xml_schema::type* c)
  : ::xml_schema::type (x, f, c),
    name_ (x.name_, f, this),
    mbid_ (x.mbid_, f, this),
    reach_ (x.reach_, f, this),
    url_ (x.url_, f, this)
  {
  }

  track::
  track (const ::xercesc::DOMElement& e,
         ::xml_schema::flags f,
         ::xml_schema::type* c)
  : ::xml_schema::type (e, f | ::xml_schema::flags::base, c),
    name_ (f, this),
    mbid_ (f, this),
    reach_ (f, this),
    url_ (f, this)
  {
    if ((f & ::xml_schema::flags::base) == 0)
    {
      ::xsd::cxx::xml::dom::parser< char > p (e);
      this->parse (p, f);
    }
  }

  void track::
  parse (::xsd::cxx::xml::dom::parser< char >& p,
         ::xml_schema::flags f)
  {
    for (; p.more_elements (); p.next_element ())
    {
      const ::xercesc::DOMElement& i (p.cur_element ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (i));

      // name
      //
      if (n.name () == "name" && n.namespace_ () == "LastFM")
      {
        ::std::auto_ptr< name_type > r (
          name_traits::create (i, f, this));

        if (!name_.present ())
        {
          this->name (r);
          continue;
        }
      }

      // mbid
      //
      if (n.name () == "mbid" && n.namespace_ () == "LastFM")
      {
        ::std::auto_ptr< mbid_type > r (
          mbid_traits::create (i, f, this));

        if (!mbid_.present ())
        {
          this->mbid (r);
          continue;
        }
      }

      // reach
      //
      if (n.name () == "reach" && n.namespace_ () == "LastFM")
      {
        if (!reach_.present ())
        {
          this->reach (reach_traits::create (i, f, this));
          continue;
        }
      }

      // url
      //
      if (n.name () == "url" && n.namespace_ () == "LastFM")
      {
        if (!url_.present ())
        {
          this->url (url_traits::create (i, f, this));
          continue;
        }
      }

      break;
    }

    if (!name_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "name",
        "LastFM");
    }

    if (!mbid_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "mbid",
        "LastFM");
    }

    if (!reach_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "reach",
        "LastFM");
    }

    if (!url_.present ())
    {
      throw ::xsd::cxx::tree::expected_element< char > (
        "url",
        "LastFM");
    }
  }

  track* track::
  _clone (::xml_schema::flags f,
          ::xml_schema::type* c) const
  {
    return new track (*this, f, c);
  }
}

#include <istream>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xsd/cxx/xml/sax/std-input-source.hxx>
#include <xsd/cxx/tree/error-handler.hxx>

namespace LastFM
{
  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::std::string& u,
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

    ::std::auto_ptr< ::LastFM::mostknowntracks > r (
      ::LastFM::mostknowntracks_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::std::string& u,
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

    ::std::auto_ptr< ::LastFM::mostknowntracks > r (
      ::LastFM::mostknowntracks_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::std::string& u,
                    ::xercesc::DOMErrorHandler& h,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (u, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::mostknowntracks > r (
      ::LastFM::mostknowntracks_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::std::istream& is,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::mostknowntracks_ (wrap, f, p);
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::std::istream& is,
                    ::xml_schema::error_handler& h,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::mostknowntracks_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::std::istream& is,
                    ::xercesc::DOMErrorHandler& h,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::sax::std_input_source isrc (is);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::mostknowntracks_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::std::istream& is,
                    const ::std::string& sid,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::auto_initializer i (
      (f & ::xml_schema::flags::dont_initialize) == 0,
      (f & ::xml_schema::flags::keep_dom) == 0);

    ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::mostknowntracks_ (wrap, f, p);
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::std::istream& is,
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
    return ::LastFM::mostknowntracks_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::std::istream& is,
                    const ::std::string& sid,
                    ::xercesc::DOMErrorHandler& h,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
    ::xercesc::Wrapper4InputSource wrap (&isrc, false);
    return ::LastFM::mostknowntracks_ (wrap, h, f, p);
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::xercesc::DOMInputSource& i,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::tree::error_handler< char > h;

    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (i, h, p, f));

    h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

    ::std::auto_ptr< ::LastFM::mostknowntracks > r (
      ::LastFM::mostknowntracks_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::xercesc::DOMInputSource& i,
                    ::xml_schema::error_handler& h,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (i, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::mostknowntracks > r (
      ::LastFM::mostknowntracks_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::xercesc::DOMInputSource& i,
                    ::xercesc::DOMErrorHandler& h,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > d (
      ::xsd::cxx::xml::dom::parse< char > (i, h, p, f));

    if (!d)
      throw ::xsd::cxx::tree::parsing< char > ();

    ::std::auto_ptr< ::LastFM::mostknowntracks > r (
      ::LastFM::mostknowntracks_ (
        d.get (), f | ::xml_schema::flags::own_dom, p));

    if (f & ::xml_schema::flags::keep_dom)
      d.release ();

    return r;
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (const ::xercesc::DOMDocument& d,
                    ::xml_schema::flags f,
                    const ::xml_schema::properties& p)
  {
    if (f & ::xml_schema::flags::keep_dom)
    {
      ::xsd::cxx::xml::dom::auto_ptr< ::xercesc::DOMDocument > c (
        static_cast< ::xercesc::DOMDocument* > (d.cloneNode (true)));

      ::std::auto_ptr< ::LastFM::mostknowntracks > r (
        ::LastFM::mostknowntracks_ (
          c.get (), f | ::xml_schema::flags::own_dom, p));

      c.release ();
      return r;
    }

    const ::xercesc::DOMElement& e (*d.getDocumentElement ());
    const ::xsd::cxx::xml::qualified_name< char > n (
      ::xsd::cxx::xml::dom::name< char > (e));

    if (n.name () == "mostknowntracks" &&
        n.namespace_ () == "LastFM")
    {
      ::std::auto_ptr< ::LastFM::mostknowntracks > r (
        ::xsd::cxx::tree::traits< ::LastFM::mostknowntracks, char >::create (
          e, f, 0));
      return r;
    }

    throw ::xsd::cxx::tree::unexpected_element < char > (
      n.name (),
      n.namespace_ (),
      "mostknowntracks",
      "LastFM");
  }

  ::std::auto_ptr< ::LastFM::mostknowntracks >
  mostknowntracks_ (::xercesc::DOMDocument* d,
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

    if (n.name () == "mostknowntracks" &&
        n.namespace_ () == "LastFM")
    {
      ::std::auto_ptr< ::LastFM::mostknowntracks > r (
        ::xsd::cxx::tree::traits< ::LastFM::mostknowntracks, char >::create (
          e, f, 0));
      c.release ();
      return r;
    }

    throw ::xsd::cxx::tree::unexpected_element < char > (
      n.name (),
      n.namespace_ (),
      "mostknowntracks",
      "LastFM");
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

