//  BMPx - The Dumb Music IPlayer
//  Copyright (C) 2005-2007 BMPx development team.
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
//  The BMPx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#include "config.h"

#include <map>
#include <vector>
#include <string>
#include <gtkmm/treeview.h>
#include "mpx/util-graphics.hh"

#include "youki-theme-engine.hh"

namespace MPX
{
    YoukiThemeEngine::YoukiThemeEngine(
    )
    : Service::Base( "mpx-service-theme" )
    {
        reload() ;
    }

    void
    YoukiThemeEngine::reload(
    )
    {
        // FIXME for now, we have one default theme engine

        double h, s, b ;

        Gtk::Window tv ;
        gtk_widget_realize(GTK_WIDGET(tv.gobj())) ;

        const Gdk::Color& csel = tv.get_style()->get_base( Gtk::STATE_SELECTED ) ;
       // csel.set_rgb_p( .976, .455, .071 ) ;
        
        ThemeColorMap_t colors ;

        colors[THEME_COLOR_SELECT] = ThemeColor( csel.get_red_p(), csel.get_green_p(), csel.get_blue_p(), 1. ) ;

        Gdk::Color ctit ;
        ctit.set_rgb_p( .4, .4, .4 ) ;

        colors[THEME_COLOR_TITLEBAR_1] = ThemeColor( ctit.get_red_p(), ctit.get_green_p(), ctit.get_blue_p(), 0.90 ) ;

        Util::color_to_hsb( ctit, h, s, b ) ;
        b = std::max( 0., b-0.10 ) ;
        s = std::max( 0., s-0.14 ) ;
        Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_2] = ThemeColor( c1.get_red_p(), c1.get_green_p(), c1.get_blue_p(), 0.90 ) ;

        Util::color_to_hsb( c1, h, s, b ) ;
        b = std::max( 0., b-0.15 ) ;
        s = std::max( 0., s-0.20 ) ;
        Gdk::Color c2 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_3] = ThemeColor( c2.get_red_p(), c2.get_green_p(), c2.get_blue_p(), 0.90 ) ;

        Util::color_to_hsb( ctit, h, s, b ) ;
        b = std::min( 1., b+0.10 ) ;
        s = std::min( 1., s+0.05 ) ;
        Gdk::Color c3 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_TOP] = ThemeColor( c3.get_red_p(), c3.get_green_p(), c3.get_blue_p(), 0.90 ) ; 

        colors[THEME_COLOR_BASE] = ThemeColor( 0.10, 0.10, 0.10, 1. ) ; 
        colors[THEME_COLOR_BASE_ALTERNATE] = ThemeColor( .2, .2, .2, 1. ) ;
        colors[THEME_COLOR_TEXT] = ThemeColor( 1., 1., 1., 1. ) ;
        colors[THEME_COLOR_DRAWER] = ThemeColor( 0.65, 0.65, 0.65, .4 ) ;
        colors[THEME_COLOR_WINDOW_BORDER] = ThemeColor( 0.35, 0.35, 0.35, 1. ) ; 
        colors[THEME_COLOR_ENTRY_OUTLINE] = ThemeColor( 0.65, 0.65, 0.65, .4 ) ;
        colors[THEME_COLOR_TREELINES] = ThemeColor( .5, .5, .5, 1. ) ; 
        colors[THEME_COLOR_INFO_AREA] = ThemeColor( .8, .8, .8, .08 ) ;
        colors[THEME_COLOR_VOLUME] = ThemeColor( .7, .7, .7, 1. ) ;
        colors[THEME_COLOR_RESIZE_GRIP] = ThemeColor( 1., 1., 1., .20 ) ; 

        Theme theme (
              "Youki-Default"
            , "Milosz Derezynski"
            , "(C) 2009 Youki Project"
            , "This is Youki's default theme"
            , "http://youki.mp"
            , colors 
        ) ;

        m_Themes.clear() ;
        m_Themes["default"] = theme ;
        m_CurrentTheme = m_Themes.begin() ;
    }
    
    YoukiThemeEngine::~YoukiThemeEngine(
    )
    {
    }

    std::vector<std::string>
    YoukiThemeEngine::list_themes(
    )
    {
        return std::vector<std::string>( 1, "default" ) ;
    }

    void
    YoukiThemeEngine::load_theme(
          const std::string&    theme
    )
    {
    }

    const ThemeColor& 
    YoukiThemeEngine::get_color(
          ThemeColorID          color
    ) 
    {
        return m_CurrentTheme->second.Colors[color] ;
    }
}
