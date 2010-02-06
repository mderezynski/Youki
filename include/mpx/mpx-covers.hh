//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifndef MPX_COVERS_HH
#define MPX_COVERS_HH

#ifndef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mpx/mpx-minisoup.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-covers-stores.hh"
#include "mpx/mpx-network.hh"
#include "mpx/mpx-services.hh"

#include "mpx/metadatareader-taglib.hh"

#include <string>
#include <map>
#include <set>

#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h> // bleh!
#include <cairomm/cairomm.h>
#include <sigx/sigx.h>

#include <boost/shared_ptr.hpp>

namespace MPX
{
    enum CoverSize
    {
          COVER_SIZE_ALBUM        = 0,
          COVER_SIZE_DEFAULT      = 1,
          N_COVER_SIZES
    };

    typedef std::map <std::string, Glib::RefPtr<Gdk::Pixbuf> >            PixbufCache ;
    typedef boost::shared_ptr<CoverStore>                                 StorePtr ;
    typedef std::vector<StorePtr>                                         StoresVec ;
    typedef boost::shared_ptr<Glib::RecMutex>                             MutexPtr ;
    typedef std::map<std::string, MutexPtr>                               MutexMap ;

    struct RequestQualifier
    {
        std::string mbid ;
        std::string asin ; 
        std::string uri ;
        std::string artist ;
        std::string album ;
        gint64      id ;
    };

    struct CoverFetchContext
    {
        RequestQualifier      qualifier;
        StoresVec             stores;
        StoresVec::iterator   stores_i;

        CoverFetchContext(
                const RequestQualifier& qual
              , StoresVec               stores 
        )
        : qualifier(qual)
        , stores(stores) // we COPY intentionally
        , stores_i(stores.begin())
        {
        }
    };

    typedef sigc::signal<void, gint64>          SignalGotCover_t ;
    typedef sigx::signal_f<SignalGotCover_t>    SignalGotCover_xt ;

    class CoverStore ;
    class Library ;

    class Covers

        : public Service::Base
        , public sigx::glib_threadable

    {
        protected:

            struct ThreadData ; 
            Glib::Private<ThreadData>   m_ThreadData ;

            virtual void
            on_startup () ;

            virtual void
            on_cleanup () ;

        public:

            typedef bool (Covers::*FetchFunc) (const std::string&, Glib::RefPtr<Gdk::Pixbuf>&);

            Covers ();
            virtual ~Covers() {}


            sigx::request_f<const RequestQualifier&, bool> cache ;
            sigx::request_f<const ::MPX::Library*>         precache ;

            SignalGotCover_xt  signal_got_cover ;

            bool
            fetch(
                  const std::string&                      /*mbid*/
                , Glib::RefPtr<Gdk::Pixbuf>&              /*cover*/
                , int                                     /*size*/ = -1
            );

            bool
            fetch(
                  Glib::RefPtr<Gdk::Pixbuf>&              /*cover*/
                , int                                     /*size*/ = -1
            );

            bool
            fetch(
              const std::string&                      /*mbid*/,
              Cairo::RefPtr<Cairo::ImageSurface>&     /*cover*/,
              CoverSize                               /*size*/
            );

            void
            purge();

            void
            cache_artwork(
              const std::string& /* mbid */,
              Glib::RefPtr<Gdk::Pixbuf> /*cover*/
            );

            std::string
            get_thumb_path (std::string /*mbid*/);

        protected:

            void
            on_cache(
                const RequestQualifier&     rql
              , bool                        acquire
            ) ;

            void
            on_precache(
              const ::MPX::Library*
            );


            bool
            handle_cache(
                const RequestQualifier&     rql
              , bool                        acquire
            ) ;

            bool
            handle_precache(
              const ::MPX::Library*
            );

            bool
            fetch_back1(
                  const std::string&                      /*mbid*/
                , Glib::RefPtr<Gdk::Pixbuf>&              /*cover*/
                , int                                     /*size*/
            );

            bool
            fetch_back2(
                  const std::string&                      /*mbid*/
                , Cairo::RefPtr<Cairo::ImageSurface>&     /*cover*/
                , CoverSize                               /*size*/
            );

            void
            source_pref_changed_callback(
                  const std::string&
                , const std::string&
                , const Mcs::KeyVariant&
            ) ;

            void
            rebuild_stores(
            ) ;

            PixbufCache                 m_pixbuf_cache;
            StoresVec                   m_stores_all,
                                        m_stores_cur;
            int                         m_rebuild;
            int                         m_rebuilt;
            MutexMap                    m_mutexes;

            Glib::RefPtr<Gdk::Pixbuf>   m_default_cover ;
    };
}

#endif
