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

#include "youki-theme-engine.hh"

namespace MPX
{
    YoukiThemeEngine::YoukiThemeEngine(
    )
    : Service::Base( "mpx-service-theme" )
    {
        // FIXME for now, we have one default theme engine

        ThemeColorMap_t colors ;

        colors[THEME_COLOR_BASE] = ThemeColor( 0.10, 0.10, 0.10, 1. ) ; 
        colors[THEME_COLOR_BASE_ALTERNATE] = ThemeColor( .2, .2, .2, 1. ) ;
        colors[THEME_COLOR_TEXT] = ThemeColor( 1., 1., 1., 1. ) ;
        colors[THEME_COLOR_SELECT] = ThemeColor( 1., 0.863, 0.102, 1. ) ;
        colors[THEME_COLOR_DRAWER] = ThemeColor( 0.65, 0.65, 0.65, .4 ) ;
        colors[THEME_COLOR_TITLEBAR_1] = ThemeColor( 1., 0.863, 0.102, 0.90 ) ;
        colors[THEME_COLOR_TITLEBAR_2] = ThemeColor( 0.953, 0.820, 0.094, 0.93 ) ;
        colors[THEME_COLOR_TITLEBAR_3] = ThemeColor( 0.886, 0.764, 0.098, 0.93 ) ;
        colors[THEME_COLOR_TITLEBAR_TOP] = ThemeColor( 0.95, 0.95, 0.95, 1. ) ; 
        colors[THEME_COLOR_WINDOW_BORDER] = ThemeColor( 0.35, 0.35, 0.35, 1. ) ; 
        colors[THEME_COLOR_ENTRY_OUTLINE] = ThemeColor( 0.65, 0.65, 0.65, .4 ) ;
        colors[THEME_COLOR_TREELINES] = ThemeColor( 1., 1., 1., .5 ) ;
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
