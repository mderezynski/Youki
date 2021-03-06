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

#ifndef XSD_ARTIST_INFO_2_0_HXX
#define XSD_ARTIST_INFO_2_0_HXX

// Begin prologue.
//
//
// End prologue.

#include <xsd/cxx/version.hxx>

#if (XSD_INT_VERSION != 3000000L)
#error XSD runtime version mismatch
#endif

#include <xsd/cxx/pre.hxx>

#ifndef XSD_USE_CHAR
#define XSD_USE_CHAR
#endif

#ifndef XSD_CXX_TREE_USE_CHAR
#define XSD_CXX_TREE_USE_CHAR
#endif

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/types.hxx>

#include <xsd/cxx/xml/error-handler.hxx>

#include <xsd/cxx/tree/parsing.hxx>

namespace xml_schema
{
  // anyType and anySimpleType.
  //
  typedef ::xsd::cxx::tree::type type;
  typedef ::xsd::cxx::tree::simple_type<type> simple_type;

  // 8-bit
  //
  typedef signed char byte;
  typedef unsigned char unsigned_byte;

  // 16-bit
  //
  typedef short short_;
  typedef unsigned short unsigned_short;

  // 32-bit
  //
  typedef int int_;
  typedef unsigned int unsigned_int;

  // 64-bit
  //
  typedef long long long_;
  typedef unsigned long long unsigned_long;

  // Supposed to be arbitrary-length integral types.
  //
  typedef long long integer;
  typedef integer non_positive_integer;
  typedef integer non_negative_integer;
  typedef integer positive_integer;
  typedef integer negative_integer;

  // Boolean.
  //
  typedef bool boolean;

  // Floating-point types.
  //
  typedef float float_;
  typedef double double_;
  typedef double decimal;

  // String types.
  //
  typedef ::xsd::cxx::tree::string< char, simple_type > string;
  typedef ::xsd::cxx::tree::normalized_string< char, string > normalized_string;
  typedef ::xsd::cxx::tree::token< char, normalized_string > token;
  typedef ::xsd::cxx::tree::name< char, token > name;
  typedef ::xsd::cxx::tree::nmtoken< char, token > nmtoken;
  typedef ::xsd::cxx::tree::nmtokens< char, simple_type, nmtoken> nmtokens;
  typedef ::xsd::cxx::tree::ncname< char, name > ncname;
  typedef ::xsd::cxx::tree::language< char, token > language;

  // ID/IDREF.
  //
  typedef ::xsd::cxx::tree::id< char, ncname > id;
  typedef ::xsd::cxx::tree::idref< type, char, ncname > idref;
  typedef ::xsd::cxx::tree::idrefs< char, simple_type, idref > idrefs;

  // URI.
  //
  typedef ::xsd::cxx::tree::uri< char, simple_type > uri;

  // Qualified name.
  //
  typedef ::xsd::cxx::tree::qname< char, simple_type, uri, ncname > qname;

  // Binary.
  //
  typedef ::xsd::cxx::tree::buffer< char > buffer;
  typedef ::xsd::cxx::tree::base64_binary< char, simple_type > base64_binary;
  typedef ::xsd::cxx::tree::hex_binary< char, simple_type > hex_binary;

  // Date/time.
  //
  typedef ::xsd::cxx::tree::date< char, simple_type > date;
  typedef ::xsd::cxx::tree::date_time< char, simple_type > date_time;
  typedef ::xsd::cxx::tree::duration< char, simple_type > duration;
  typedef ::xsd::cxx::tree::day< char, simple_type > day;
  typedef ::xsd::cxx::tree::month< char, simple_type > month;
  typedef ::xsd::cxx::tree::month_day< char, simple_type > month_day;
  typedef ::xsd::cxx::tree::year< char, simple_type > year;
  typedef ::xsd::cxx::tree::year_month< char, simple_type > year_month;
  typedef ::xsd::cxx::tree::time< char, simple_type > time;

  // Entity.
  //
  typedef ::xsd::cxx::tree::entity< char, ncname > entity;
  typedef ::xsd::cxx::tree::entities< char, simple_type, entity > entities;

  // Flags and properties.
  //
  typedef ::xsd::cxx::tree::flags flags;
  typedef ::xsd::cxx::tree::properties< char > properties;

  // DOM user data key for back pointers to tree nodes.
  //
#ifndef XSD_CXX_TREE_TREE_NODE_KEY_IN___XML_SCHEMA
#define XSD_CXX_TREE_TREE_NODE_KEY_IN___XML_SCHEMA

  const XMLCh* const tree_node_key = ::xsd::cxx::tree::user_data_keys::node;

#endif

  // Exceptions.
  //
  typedef ::xsd::cxx::tree::exception< char > exception;
  typedef ::xsd::cxx::tree::parsing< char > parsing;
  typedef ::xsd::cxx::tree::expected_element< char > expected_element;
  typedef ::xsd::cxx::tree::unexpected_element< char > unexpected_element;
  typedef ::xsd::cxx::tree::expected_attribute< char > expected_attribute;
  typedef ::xsd::cxx::tree::unexpected_enumerator< char > unexpected_enumerator;
  typedef ::xsd::cxx::tree::expected_text_content< char > expected_text_content;
  typedef ::xsd::cxx::tree::no_type_info< char > no_type_info;
  typedef ::xsd::cxx::tree::not_derived< char > not_derived;
  typedef ::xsd::cxx::tree::duplicate_id< char > duplicate_id;
  typedef ::xsd::cxx::tree::serialization< char > serialization;
  typedef ::xsd::cxx::tree::no_namespace_mapping< char > no_namespace_mapping;
  typedef ::xsd::cxx::tree::no_prefix_mapping< char > no_prefix_mapping;
  typedef ::xsd::cxx::tree::xsi_already_in_use< char > xsi_already_in_use;
  typedef ::xsd::cxx::tree::bounds< char > bounds;

  // Parsing/serialization diagnostics.
  //
  typedef ::xsd::cxx::tree::severity severity;
  typedef ::xsd::cxx::tree::error< char > error;
  typedef ::xsd::cxx::tree::diagnostics< char > diagnostics;

  // Error handler interface.
  //
  typedef ::xsd::cxx::xml::error_handler< char > error_handler;
}

// Forward declarations.
//
namespace LastFMArtistInfo
{
  class lfm;
  class artist;
  class bio;
  class image;
  class similar;
  class stats;
}


#include <memory>    // std::auto_ptr
#include <algorithm> // std::binary_search

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/containers.hxx>
#include <xsd/cxx/tree/list.hxx>

#include <xsd/cxx/xml/dom/parsing-header.hxx>

namespace LastFMArtistInfo
{
  class lfm: public ::xml_schema::type
  {
    public:
    // artist
    // 
    typedef ::LastFMArtistInfo::artist artist_type;
    typedef ::xsd::cxx::tree::traits< artist_type, char > artist_traits;

    const artist_type&
    artist () const;

    artist_type&
    artist ();

    void
    artist (const artist_type& x);

    void
    artist (::std::auto_ptr< artist_type > p);

    // status
    // 
    typedef ::xml_schema::ncname status_type;
    typedef ::xsd::cxx::tree::traits< status_type, char > status_traits;

    const status_type&
    status () const;

    status_type&
    status ();

    void
    status (const status_type& x);

    void
    status (::std::auto_ptr< status_type > p);

    // Constructors.
    //
    lfm (const artist_type&,
         const status_type&);

    lfm (const ::xercesc::DOMElement& e,
         ::xml_schema::flags f = 0,
         ::xml_schema::type* c = 0);

    lfm (const lfm& x,
         ::xml_schema::flags f = 0,
         ::xml_schema::type* c = 0);

    virtual lfm*
    _clone (::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0) const;

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::flags);

    private:
    ::xsd::cxx::tree::one< artist_type > artist_;
    ::xsd::cxx::tree::one< status_type > status_;
  };

  class artist: public ::xml_schema::type
  {
    public:
    // bio
    // 
    typedef ::LastFMArtistInfo::bio bio_type;
    typedef ::xsd::cxx::tree::optional< bio_type > bio_optional;
    typedef ::xsd::cxx::tree::traits< bio_type, char > bio_traits;

    const bio_optional&
    bio () const;

    bio_optional&
    bio ();

    void
    bio (const bio_type& x);

    void
    bio (const bio_optional& x);

    void
    bio (::std::auto_ptr< bio_type > p);

    // image
    // 
    typedef ::LastFMArtistInfo::image image_type;
    typedef ::xsd::cxx::tree::sequence< image_type > image_sequence;
    typedef image_sequence::iterator image_iterator;
    typedef image_sequence::const_iterator image_const_iterator;
    typedef ::xsd::cxx::tree::traits< image_type, char > image_traits;

    const image_sequence&
    image () const;

    image_sequence&
    image ();

    void
    image (const image_sequence& s);

    // mbid
    // 
    typedef ::xml_schema::string mbid_type;
    typedef ::xsd::cxx::tree::sequence< mbid_type > mbid_sequence;
    typedef mbid_sequence::iterator mbid_iterator;
    typedef mbid_sequence::const_iterator mbid_const_iterator;
    typedef ::xsd::cxx::tree::traits< mbid_type, char > mbid_traits;

    const mbid_sequence&
    mbid () const;

    mbid_sequence&
    mbid ();

    void
    mbid (const mbid_sequence& s);

    // name
    // 
    typedef ::xml_schema::string name_type;
    typedef ::xsd::cxx::tree::sequence< name_type > name_sequence;
    typedef name_sequence::iterator name_iterator;
    typedef name_sequence::const_iterator name_const_iterator;
    typedef ::xsd::cxx::tree::traits< name_type, char > name_traits;

    const name_sequence&
    name () const;

    name_sequence&
    name ();

    void
    name (const name_sequence& s);

    // similar
    // 
    typedef ::LastFMArtistInfo::similar similar_type;
    typedef ::xsd::cxx::tree::sequence< similar_type > similar_sequence;
    typedef similar_sequence::iterator similar_iterator;
    typedef similar_sequence::const_iterator similar_const_iterator;
    typedef ::xsd::cxx::tree::traits< similar_type, char > similar_traits;

    const similar_sequence&
    similar () const;

    similar_sequence&
    similar ();

    void
    similar (const similar_sequence& s);

    // stats
    // 
    typedef ::LastFMArtistInfo::stats stats_type;
    typedef ::xsd::cxx::tree::sequence< stats_type > stats_sequence;
    typedef stats_sequence::iterator stats_iterator;
    typedef stats_sequence::const_iterator stats_const_iterator;
    typedef ::xsd::cxx::tree::traits< stats_type, char > stats_traits;

    const stats_sequence&
    stats () const;

    stats_sequence&
    stats ();

    void
    stats (const stats_sequence& s);

    // streamable
    // 
    typedef ::xml_schema::integer streamable_type;
    typedef ::xsd::cxx::tree::sequence< streamable_type > streamable_sequence;
    typedef streamable_sequence::iterator streamable_iterator;
    typedef streamable_sequence::const_iterator streamable_const_iterator;
    typedef ::xsd::cxx::tree::traits< streamable_type, char > streamable_traits;

    const streamable_sequence&
    streamable () const;

    streamable_sequence&
    streamable ();

    void
    streamable (const streamable_sequence& s);

    // url
    // 
    typedef ::xml_schema::uri url_type;
    typedef ::xsd::cxx::tree::sequence< url_type > url_sequence;
    typedef url_sequence::iterator url_iterator;
    typedef url_sequence::const_iterator url_const_iterator;
    typedef ::xsd::cxx::tree::traits< url_type, char > url_traits;

    const url_sequence&
    url () const;

    url_sequence&
    url ();

    void
    url (const url_sequence& s);

    // Constructors.
    //
    artist ();

    artist (const ::xercesc::DOMElement& e,
            ::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0);

    artist (const artist& x,
            ::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0);

    virtual artist*
    _clone (::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0) const;

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::flags);

    private:
    bio_optional bio_;
    image_sequence image_;
    mbid_sequence mbid_;
    name_sequence name_;
    similar_sequence similar_;
    stats_sequence stats_;
    streamable_sequence streamable_;
    url_sequence url_;
  };

  class bio: public ::xml_schema::type
  {
    public:
    // published
    // 
    typedef ::xml_schema::string published_type;
    typedef ::xsd::cxx::tree::traits< published_type, char > published_traits;

    const published_type&
    published () const;

    published_type&
    published ();

    void
    published (const published_type& x);

    void
    published (::std::auto_ptr< published_type > p);

    // summary
    // 
    typedef ::xml_schema::string summary_type;
    typedef ::xsd::cxx::tree::traits< summary_type, char > summary_traits;

    const summary_type&
    summary () const;

    summary_type&
    summary ();

    void
    summary (const summary_type& x);

    void
    summary (::std::auto_ptr< summary_type > p);

    // content
    // 
    typedef ::xml_schema::string content_type;
    typedef ::xsd::cxx::tree::traits< content_type, char > content_traits;

    const content_type&
    content () const;

    content_type&
    content ();

    void
    content (const content_type& x);

    void
    content (::std::auto_ptr< content_type > p);

    // Constructors.
    //
    bio (const published_type&,
         const summary_type&,
         const content_type&);

    bio (const ::xercesc::DOMElement& e,
         ::xml_schema::flags f = 0,
         ::xml_schema::type* c = 0);

    bio (const bio& x,
         ::xml_schema::flags f = 0,
         ::xml_schema::type* c = 0);

    virtual bio*
    _clone (::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0) const;

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::flags);

    private:
    ::xsd::cxx::tree::one< published_type > published_;
    ::xsd::cxx::tree::one< summary_type > summary_;
    ::xsd::cxx::tree::one< content_type > content_;
  };

  class image: public ::xml_schema::uri
  {
    public:
    // size
    // 
    typedef ::xml_schema::ncname size_type;
    typedef ::xsd::cxx::tree::traits< size_type, char > size_traits;

    const size_type&
    size () const;

    size_type&
    size ();

    void
    size (const size_type& x);

    void
    size (::std::auto_ptr< size_type > p);

    // Constructors.
    //
    image (const ::xml_schema::uri&,
           const size_type&);

    image (const ::xercesc::DOMElement& e,
           ::xml_schema::flags f = 0,
           ::xml_schema::type* c = 0);

    image (const image& x,
           ::xml_schema::flags f = 0,
           ::xml_schema::type* c = 0);

    virtual image*
    _clone (::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0) const;

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::flags);

    private:
    ::xsd::cxx::tree::one< size_type > size_;
  };

  class similar: public ::xml_schema::type
  {
    public:
    // artist
    // 
    typedef ::LastFMArtistInfo::artist artist_type;
    typedef ::xsd::cxx::tree::sequence< artist_type > artist_sequence;
    typedef artist_sequence::iterator artist_iterator;
    typedef artist_sequence::const_iterator artist_const_iterator;
    typedef ::xsd::cxx::tree::traits< artist_type, char > artist_traits;

    const artist_sequence&
    artist () const;

    artist_sequence&
    artist ();

    void
    artist (const artist_sequence& s);

    // Constructors.
    //
    similar ();

    similar (const ::xercesc::DOMElement& e,
             ::xml_schema::flags f = 0,
             ::xml_schema::type* c = 0);

    similar (const similar& x,
             ::xml_schema::flags f = 0,
             ::xml_schema::type* c = 0);

    virtual similar*
    _clone (::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0) const;

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::flags);

    private:
    artist_sequence artist_;
  };

  class stats: public ::xml_schema::type
  {
    public:
    // listeners
    // 
    typedef ::xml_schema::integer listeners_type;
    typedef ::xsd::cxx::tree::traits< listeners_type, char > listeners_traits;

    const listeners_type&
    listeners () const;

    listeners_type&
    listeners ();

    void
    listeners (const listeners_type& x);

    // playcount
    // 
    typedef ::xml_schema::integer playcount_type;
    typedef ::xsd::cxx::tree::traits< playcount_type, char > playcount_traits;

    const playcount_type&
    playcount () const;

    playcount_type&
    playcount ();

    void
    playcount (const playcount_type& x);

    // Constructors.
    //
    stats (const listeners_type&,
           const playcount_type&);

    stats (const ::xercesc::DOMElement& e,
           ::xml_schema::flags f = 0,
           ::xml_schema::type* c = 0);

    stats (const stats& x,
           ::xml_schema::flags f = 0,
           ::xml_schema::type* c = 0);

    virtual stats*
    _clone (::xml_schema::flags f = 0,
            ::xml_schema::type* c = 0) const;

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::flags);

    private:
    ::xsd::cxx::tree::one< listeners_type > listeners_;
    ::xsd::cxx::tree::one< playcount_type > playcount_;
  };
}

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMInputSource.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

namespace LastFMArtistInfo
{
  // Parse a URI or a local file.
  //

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::std::string& uri,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::std::string& uri,
        ::xml_schema::error_handler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::std::string& uri,
        ::xercesc::DOMErrorHandler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  // Parse std::istream.
  //

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::std::istream& is,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::std::istream& is,
        ::xml_schema::error_handler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::std::istream& is,
        ::xercesc::DOMErrorHandler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::std::istream& is,
        const ::std::string& id,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::std::istream& is,
        const ::std::string& id,
        ::xml_schema::error_handler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::std::istream& is,
        const ::std::string& id,
        ::xercesc::DOMErrorHandler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  // Parse xercesc::DOMInputSource.
  //

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::xercesc::DOMInputSource& is,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::xercesc::DOMInputSource& is,
        ::xml_schema::error_handler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::xercesc::DOMInputSource& is,
        ::xercesc::DOMErrorHandler& eh,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  // Parse xercesc::DOMDocument.
  //

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (const ::xercesc::DOMDocument& d,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());

  ::std::auto_ptr< ::LastFMArtistInfo::lfm >
  lfm_ (::xercesc::DOMDocument* d,
        ::xml_schema::flags f = 0,
        const ::xml_schema::properties& p = ::xml_schema::properties ());
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // XSD_ARTIST_INFO_2_0_HXX
