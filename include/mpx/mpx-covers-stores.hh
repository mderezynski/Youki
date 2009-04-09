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

#ifndef MPX_COVERSTORES_HH
#define MPX_COVERSTORES_HH

#include <string>

#include "mpx/mpx-covers.hh"
#include "mpx/mpx-minisoup.hh"

namespace MPX
{
    class Covers;
    struct CoverFetchContext;

    enum FetchState
    {
          FETCH_STATE_NOT_FETCHED
        , FETCH_STATE_COVER_SAVED
    } ;


    class CoverStore
    {
    protected:

        Covers & covers;

        FetchState m_fetch_state ;

    public:

        CoverStore(Covers& c) : covers(c), m_fetch_state(FETCH_STATE_NOT_FETCHED)
        {}

        virtual void
        load_artwork(CoverFetchContext* /*cover_data*/) = 0;

        FetchState
        get_state()
        {
            return m_fetch_state ;
        }
    };

    class RemoteStore : public CoverStore
    {
    public:
        RemoteStore(Covers& c) : CoverStore(c) { }

        virtual void
        fetch_image(const std::string&, CoverFetchContext*);

        virtual void
        load_artwork(CoverFetchContext*);

        virtual std::string
        get_url(CoverFetchContext*) = 0; 

        virtual bool
        can_load_artwork(CoverFetchContext*);

        void
        save_image(
              char const*
            , guint
            , CoverFetchContext*
        );

        virtual void
        request_failed( CoverFetchContext* );

        Soup::RequestSyncRefP request;
    };

    class AmazonCovers : public RemoteStore
    {
    public:
        AmazonCovers(Covers& c) : RemoteStore(c), n(0)
        {}

        bool
        can_load_artwork(CoverFetchContext*);

        std::string
        get_url(CoverFetchContext*);

        virtual void
        request_failed( CoverFetchContext* );

    private:

        unsigned int n;
    };

    class AmapiCovers : public RemoteStore
    {
    public:
        AmapiCovers(Covers& c) : RemoteStore(c)
        { }
        
        void
        load_artwork(CoverFetchContext*);

    protected:        

        virtual std::string
        get_url(CoverFetchContext*);
    };

    class MusicBrainzCovers : public RemoteStore
    {
    public:
        MusicBrainzCovers(Covers& c) : RemoteStore(c)
        { }

        void
        load_artwork(CoverFetchContext*);

    protected:

        virtual std::string
        get_url(CoverFetchContext*); 

    private:
        void
        reply_cb(char const*, guint, guint, CoverFetchContext*);
    };

    class LocalCovers
    : public CoverStore
    {
    public:
        LocalCovers(Covers& c) : CoverStore(c)
        { }

        void
        load_artwork(CoverFetchContext*);
    };

    class InlineCovers
    : public CoverStore
    {
    public:
       InlineCovers(Covers& c) : CoverStore(c)
        { }

        void
        load_artwork(CoverFetchContext*);
    };

}

#endif
