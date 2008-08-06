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

#include <sigx/sigx.h>

#include "mpx/mpx-covers.hh"

namespace MPX
{
    class Covers;
    struct CoverFetchData;

    class CoverStore
    {
    protected:
        Covers& covers;

        typedef sigc::signal<void, CoverFetchData*> SignalNotFoundT;

        struct SignalsT
        {
          // Fired if a store could not find artwork
          SignalNotFoundT NotFound;
        };

        SignalsT Signals;

    public:
        SignalNotFoundT&
        not_found_callback()
        {
            return Signals.NotFound;
        }

        CoverStore(Covers& c) : covers(c)
        { }

        virtual void
        load_artwork(CoverFetchData* /*cover_data*/) = 0;
    };

    class RemoteStore : public CoverStore
    {
    public:
        RemoteStore(Covers& c) : CoverStore(c) { }

        virtual void
        load_artwork(CoverFetchData*);

        virtual std::string
        get_url(CoverFetchData*) { return ""; }

        virtual bool
        can_load_artwork(CoverFetchData*);

        void
        save_image(
            char const*,
            guint,
            guint,
            CoverFetchData*
        );
    };

    class AmazonCovers : public RemoteStore
    {
    public:
        AmazonCovers(Covers& c) : RemoteStore(c)
        { }

        bool
        can_load_artwork(CoverFetchData*);

        std::string
        get_url(CoverFetchData*);
    };

    class AmapiCovers : public RemoteStore
    {
    public:
        AmapiCovers(Covers& c) : RemoteStore(c)
        { }
        
        void
        load_artwork(CoverFetchData*);
        
    private:
        void
        reply_cb(char const*, guint, guint, CoverFetchData*);
    };

    class MusicBrainzCovers : public RemoteStore
    {
    public:
        MusicBrainzCovers(Covers& c) : RemoteStore(c)
        { }

        void
        load_artwork(CoverFetchData*);

    private:
        void
        reply_cb(char const*, guint, guint, CoverFetchData*);
    };

    class LocalCovers : public CoverStore,
                         public sigx::glib_auto_dispatchable
    {
    public:
        LocalCovers(Covers& c) : CoverStore(c)
        { }

        void
        load_artwork(CoverFetchData*);
    };

    class InlineCovers : public CoverStore,
                         public sigx::glib_auto_dispatchable
    {
    public:
       InlineCovers(Covers& c) : CoverStore(c)
        { }

        void
        load_artwork(CoverFetchData*);
    };

}

#endif
