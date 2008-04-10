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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifndef MPX_LASTFM_EXTRA_WIDGETS_HH
#define MPX_LASTFM_EXTRA_WIDGETS_HH

#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm.h>
#include <gtk/gtktextview.h>

#include "lastfm.hh"

using namespace Gtk;
using namespace Glib;

namespace MPX
{
  class LastFMLinkTag
    : public TextTag
  {
    private:

        LastFMLinkTag (ustring const& url, guint64 rank, std::string const& fg)
        : ObjectBase (typeid(LastFMLinkTag))
        , TextTag ()
        , mTag (url)
        , mFg (fg)
        {
          property_foreground() = mFg; 
          property_scale() = (double (rank) / 50.) + 1;
          property_justification() = Gtk::JUSTIFY_FILL;
        }

    public:

        static RefPtr<LastFMLinkTag> create (ustring const& url, guint64 rank, std::string const& fg = "#4b72a9")
        {
          return RefPtr<LastFMLinkTag> (new LastFMLinkTag (url, rank, fg)); 
        }

        virtual ~LastFMLinkTag () {}

    public:

      typedef sigc::signal<void, ustring const&> SignalUrlActivated;
    
    private:

      struct SignalsT
      {
        SignalUrlActivated  UrlActivated;
      };

      SignalsT Signals;    

    public:

      SignalUrlActivated&
      signalUrlActivated ()
      {
        return Signals.UrlActivated;
      }

    protected:

      virtual bool
      on_event (RefPtr<Object> const& event_object, GdkEvent* event, TextIter const& iter) 
      {
        if (event->type == GDK_BUTTON_PRESS)
        {
              GdkEventButton * ev = (GdkEventButton*)(event);
              if (ev->button == 1)
              {
                    return true;
              }
        }
        if (event->type == GDK_BUTTON_RELEASE)
        {
              GdkEventButton * ev = (GdkEventButton*)(event);
              if (ev->button == 1)
              { 
                    Signals.UrlActivated.emit (mTag);
                    return true;
              }
        }
        return false;
      }

    private:
    
      ustring mName;
      ustring mTag;
      ustring mFg;
  };

  class LastFMTagView
    : public TextView
  {
      typedef RefPtr<TextTag>  RefTextTag;
      typedef std::vector<RefTextTag> TagV;
      RefTextTag m_current_tag;
      Gdk::Color mFg;

    public:

      LastFMTagView (BaseObjectType*                  obj,
                     RefPtr<Gnome::Glade::Xml> const& xml)
      : TextView (obj) 
      { 
        gtk_widget_add_events (GTK_WIDGET (gobj()), 
                               GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK);
        realize ();
        setCursor("left_ptr");
      }

      virtual ~LastFMTagView ()
      {
      }

      Glib::RefPtr<LastFMLinkTag>
      insert_tag( std::string const& text, std::string const& url, gint64 rank )
      {
        Glib::RefPtr<LastFMLinkTag> tag = LastFMLinkTag::create(url, rank); 
        get_buffer()->get_tag_table()->add(tag);
        TextBuffer::iterator iter = get_buffer()->end();
        iter = get_buffer()->insert( iter, "    ");
        iter = get_buffer()->insert_with_tag( iter, text, tag); 
        iter = get_buffer()->insert( iter, "    ");
        return tag;
      }

      void
      clear ()
      {
        get_buffer()->set_text ("");
        setCursor("left_ptr");
      }

    private:

      void
      setCursor (std::string const& type)
      {
        GdkWindow * window = GDK_WINDOW (gtk_text_view_get_window (GTK_TEXT_VIEW (gobj()), GTK_TEXT_WINDOW_TEXT));
        GdkCursor * cursor = gdk_cursor_new_from_name (gdk_display_get_default (), type.c_str()); 
        if (!cursor)
        {
             cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_XTERM);
        }
        gdk_window_set_cursor (window, cursor);
      }

    protected:

      virtual bool on_leave_notify_event (GdkEventCrossing * event)
      {
        if (m_current_tag && m_current_tag->gobj())
        {
          m_current_tag->property_foreground_gdk() = mFg; 
          m_current_tag = RefTextTag (0);
        }
        return false;
      }

      virtual bool on_motion_notify_event (GdkEventMotion * event)
      {
        int x, y;
        int x_orig, y_orig;
        GdkModifierType state;

        if (event->is_hint)
        {
            gdk_window_get_pointer (event->window, &x_orig, &y_orig, &state);
        }
        else
        {
            x_orig = int (event->x);
            y_orig = int (event->y);
            state = GdkModifierType (event->state);
        }

        window_to_buffer_coords (TEXT_WINDOW_WIDGET, x_orig, y_orig, x, y);

        RefPtr<TextBuffer> buf = get_buffer();
        TextBuffer::iterator iter;
        get_iter_at_location (iter, x, y);      
      
        TagV tags = iter.get_tags();
    
        if( !tags.empty() ) 
        {
          if ((m_current_tag && m_current_tag->gobj()) && (m_current_tag->gobj() != tags[0]->gobj()))
          {
            m_current_tag->property_foreground_gdk() = mFg; 
          }

          if (!m_current_tag || (m_current_tag->gobj() != tags[0]->gobj()))
          {
            setCursor("hand2");
            m_current_tag = tags[0];
            mFg = m_current_tag->property_foreground_gdk();
            m_current_tag->property_foreground() = "#ff5a00";
          }

          return true;
        }
 
        if (m_current_tag) 
        {
          setCursor("left_ptr");
          m_current_tag->property_foreground_gdk() = mFg; 
          m_current_tag = RefTextTag (0);
        }
    
        return true;
      }
  };
}

#endif // !MPX_LASTFM_EXTRA_WIDGETS_HH

