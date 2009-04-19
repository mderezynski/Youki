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

#include <gtkmm.h>
#include <boost/algorithm/string.hpp>

#include <string>
#include <map>
#include <vector>

#include "json/json.h"
#include "mpx/util-graphics.hh"
#include "youki-theme-engine.hh"

namespace MPX
{
    YoukiThemeEngine::YoukiThemeEngine(
    )
    : Service::Base( "mpx-service-theme" )
    {
        reload() ;
        load_stored_themes() ;
    }

    void
    YoukiThemeEngine::load_theme(
          const std::string&    name
        , const std::string&    document
    )
    {
        Json::Value root ;
        Json::Reader reader ;

        reader.parse(
              document
            , root
            , false
        ) ;

        if( !root.isArray() )
        {
            g_warning("%s: Theme root node is not array! ('%s')", G_STRLOC, name.c_str() ) ;
            return ;
        }

        for( Json::Value::iterator i = root.begin() ; i != root.end() ; ++i )
        {
            Json::Value::Members m = (*i).getMemberNames() ;

            Json::Value   val_default ;

            Json::Value   val_name
                        , val_colr ;

            val_name = (*i).get( "name"  , val_default ) ;
            val_colr = (*i).get( "value" , val_default ) ;

            std::string name    = val_name.asString() ;
            std::string color   = val_colr.asString() ;
        }
    }

    void
    YoukiThemeEngine::load_stored_themes()
    {
        std::string path = Glib::build_filename( DATA_DIR, "themes" ) ;
        Glib::Dir d ( path ) ;
        std::vector<std::string> s (d.begin(), d.end()) ;
        d.close() ;

        for( std::vector<std::string>::const_iterator i = s.begin(); i != s.end(); ++i )
        {
            std::vector<std::string> subs;
            boost::split( subs, *i, boost::is_any_of (".") ) ;

            if( !subs.empty() )
            {
                std::string suffix = subs[subs.size()-1] ;
    
                if( suffix == "mpxtheme" )
                {
                    std::string document = Glib::file_get_contents( Glib::build_filename( path, *i )) ;
                    std::string name     = subs[0] ;

                    load_theme( name, document ) ;
                }
            }
        }
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
        
        ThemeColorMap_t colors ;

        colors[THEME_COLOR_SELECT] = ThemeColor( csel.get_red_p(), csel.get_green_p(), csel.get_blue_p(), 1. ) ;

        Util::color_to_hsb( csel, h, s, b ) ;
        b = std::max( 0.15, b-0.10 ) ;
        s = 0.20 ; 
        Gdk::Color c0 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_1] = ThemeColor( c0.get_red_p(), c0.get_green_p(), c0.get_blue_p(), 0.90 ) ;

        Util::color_to_hsb( csel, h, s, b ) ;
        b = std::max( 0.08, b-0.20 ) ;
        s = 0.10 ;
        Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_2] = ThemeColor( c1.get_red_p(), c1.get_green_p(), c1.get_blue_p(), 0.90 ) ;

        Util::color_to_hsb( csel, h, s, b ) ;
        b = std::max( 0.05, b-0.25 ) ;
        s = 0.05 ; 
        Gdk::Color c2 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_3] = ThemeColor( c2.get_red_p(), c2.get_green_p(), c2.get_blue_p(), 0.90 ) ;

        Util::color_to_hsb( csel, h, s, b ) ;
        b = std::max( 0., b+0.15 ) ;
        s = std::max( 0., s+0.05 ) ;
        Gdk::Color c3 = Util::color_from_hsb( h, s, b ) ;
        colors[THEME_COLOR_TITLEBAR_TOP] = ThemeColor( c3.get_red_p(), c3.get_green_p(), c3.get_blue_p(), 0.90 ) ; 

        colors[THEME_COLOR_BACKGROUND] = ThemeColor( 0.10, 0.10, 0.10, 1. ) ; 
        colors[THEME_COLOR_BASE] = ThemeColor( 0.10, 0.10, 0.10, 1. ) ; 
        colors[THEME_COLOR_BASE_ALTERNATE] = ThemeColor( .2, .2, .2, 1. ) ;
        colors[THEME_COLOR_TEXT] = ThemeColor( 1., 1., 1., 1. ) ;
        colors[THEME_COLOR_TEXT_SELECTED] = ThemeColor( 1., 1., 1., 1. ) ;
        colors[THEME_COLOR_DRAWER] = ThemeColor( 0.65, 0.65, 0.65, .4 ) ;
        colors[THEME_COLOR_WINDOW_BORDER] = ThemeColor( 0.25, 0.25, 0.25, 1. ) ; 
        colors[THEME_COLOR_ENTRY_OUTLINE] = ThemeColor( 0.65, 0.65, 0.65, .4 ) ;
        colors[THEME_COLOR_TREELINES] = ThemeColor( .5, .5, .5, 1. ) ; 
        colors[THEME_COLOR_INFO_AREA] = ThemeColor( .8, .8, .8, .08 ) ;
        colors[THEME_COLOR_VOLUME] = ThemeColor( .7, .7, .7, 1. ) ;
        colors[THEME_COLOR_RESIZE_GRIP] = ThemeColor( 1., 1., 1., .10 ) ; 

        Theme theme (
              "Youki-Default"
            , "Milosz Derezynski"
            , "(C) 2009 Youki Project"
            , "This is Youki's default theme"
            , "http://youki.mp"
            , colors 
        ) ;

        m_Themes.erase("default") ;
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
