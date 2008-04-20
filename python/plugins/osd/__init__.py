
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-
#
# MPX Trackinfo
# (C) 2008 M. Derezynski
#

import mpx
import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gobject
import cairo
import pango

class OSD(gtk.Window):

        def __init__(self):

            gtk.Window.__init__(self, type=gtk.WINDOW_POPUP)
            self.add_events(gtk.gdk.BUTTON_PRESS_MASK)
            self.INNER_PADDING = 8
            self.cover = None
            self.metadata = None
            self.m_artist = ""
            self.m_title = ""
            self.m_album = ""
            self.set_colormap(gtk.gdk.screen_get_default().get_rgba_colormap())
            self.resize(1,1)

        def move_center(self, width):

            self.resize(width, self.INNER_PADDING*2 + 128)
            screen = gtk.gdk.screen_get_default()
            self.move((screen.get_width() - width) / 2, (screen.get_height() - (self.INNER_PADDING*2+128))/2)

    
        def rounded_rect(self, cr, x, y, width, height, radius):

            edge_length_x = width - radius * 2;
            edge_length_y = height - radius * 2;

            cr.move_to (x + radius, y);
            cr.rel_line_to (edge_length_x, 0);
            cr.rel_curve_to (radius, 0, radius, radius, radius, radius);
            cr.rel_line_to (0, edge_length_y);
            cr.rel_curve_to (0, radius, -radius, radius, -radius, radius);
            cr.rel_line_to (-edge_length_x, 0);
            cr.rel_curve_to (-radius, 0, -radius, -radius, -radius, -radius);
            cr.rel_line_to (0, -edge_length_y);
            cr.rel_curve_to (0, -radius, radius, -radius, radius, -radius);

        def set_metadata(self, m):

            self.metadata = m

            if m[mpx.AttributeId.TITLE].is_initialized():
                self.m_title = m[mpx.AttributeId.TITLE].val().get_string()
            else:
                self.m_title = "(No Title)"

            if m[mpx.AttributeId.ARTIST].is_initialized():
                self.m_artist = m[mpx.AttributeId.ARTIST].val().get_string()
            else:
                self.m_artist = "(Unknown Artist)"

            if m[mpx.AttributeId.ALBUM].is_initialized():
                self.m_album = m[mpx.AttributeId.ALBUM].val().get_string()
            else:
                self.m_album = "(Unknown Album)"

            i = m.get_image()

            if(i):
                self.cover = i.scale_simple(128,128, gtk.gdk.INTERP_HYPER) 
            else:
                self.cover = None

            self.queue_draw()

        def clear(self):

            self.cover = None
            self.metadata = None
            self.queue_draw()

        def do_expose_event(self, event):

            x, y, width, height = self.allocation

            cr = self.window.cairo_create()
            cr.set_operator(cairo.OPERATOR_CLEAR)
            cr.rectangle(x, y, width, height) 
            cr.fill()

            cr.set_operator(cairo.OPERATOR_SOURCE)
            self.rounded_rect(cr, x+2, y+2, width-4, height-4, 10.)
            cr.set_source_rgba(1., 1., 1., 0.8)
            cr.fill_preserve()
            cr.set_source_rgba(0., 0., 0., 0.6)
            cr.stroke()

            layouts = [ self.create_pango_layout(self.m_artist),
                        self.create_pango_layout(self.m_album),
                        self.create_pango_layout(self.m_title)  ]

            layouts[0].set_font_description(pango.FontDescription("Sans Serif Bold 14"))
            layouts[1].set_font_description(pango.FontDescription("Sans Serif 14"))
            layouts[2].set_font_description(pango.FontDescription("Sans Serif 10"))

            max_layout_width = 0

            for layout in layouts:
                fontw, fonth = layout.get_pixel_size()
                if max_layout_width < fontw:
                    max_layout_width = fontw

            if max_layout_width < 300:
                max_layout_width = 300

            x_origin = self.INNER_PADDING

            if(self.cover):
            
                cr.set_source_pixbuf(self.cover, self.INNER_PADDING, self.INNER_PADDING)
                self.rounded_rect(cr, x + self.INNER_PADDING, y + self.INNER_PADDING, 128, 128, 8.)
                cr.fill()
                x_origin = 128 + (2*self.INNER_PADDING)
                self.move_center(max_layout_width + 128 + (3*self.INNER_PADDING))
            
            else:

                self.move_center(max_layout_width + (2*self.INNER_PADDING))

      

            positions = [ self.INNER_PADDING+28,
                          self.INNER_PADDING+24+28,
                          self.INNER_PADDING+48+28 ]

            cr.set_source_rgba(0., 0., 0., 1.)
            cr.set_operator(cairo.OPERATOR_SOURCE)
            for layout in layouts: 
                fontw, fonth = layout.get_pixel_size()
                position = positions[layouts.index(layout)]
                cr.move_to(x_origin + ((max_layout_width/2) - (fontw/2)), position)
                cr.update_layout(layout)
                cr.show_layout(layout)
    
            return True

class MPXOSD(mpx.Plugin):

    def __init__(self):
   
        gobject.type_register(OSD)
        self.osd = OSD()
        self.osd.connect("button-press-event", self.on_button_press)
        self.timeout_id = None

    def activate(self,player,mcs):

        self.player = player
        self.player_new_track_handler_id = self.player.gobj().connect("new-track", self.new_track)
        self.player_status_change_handler_id = self.player.gobj().connect("play-status-changed", self.pstatus_changed)

        status = self.player.get_status()

        if (status == mpx.PlayStatus.PLAYING) or (status == mpx.PlayStatus.PAUSED):
            
            self.new_track(player.gobj())

        return True

    def deactivate(self):
        self.player.gobj().disconnect(self.player_new_track_handler_id)
        self.player.gobj().disconnect(self.player_status_change_handler_id)
        self.player = None

    def on_button_press(self, widget, event):

        if(self.timeout_id): 
            gobject.source_remove(self.timeout_id)
        self.hide_osd()

    def hide_osd(self): 

        self.osd.hide()
        self.timeout_id = None
        return False

    def pstatus_changed(self, playergobj, status): 

        if status == mpx.PlayStatus.STOPPED:

            self.osd.clear()
            self.osd.hide()
            if(self.timeout_id):
                gobject.source_remove(self.timeout_id)

    def new_track(self, playergobj):
       
        if(self.timeout_id): 
            gobject.source_remove(self.timeout_id)

        m = self.player.get_metadata()
        self.osd.set_metadata(m)
        self.osd.show_all()
        self.timeout_id = gobject.timeout_add(5000, self.hide_osd) 
