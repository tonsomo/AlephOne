#ifndef __SCREEN_DRAWING_H
#define __SCREEN_DRAWING_H

/*
	SCREEN_DRAWING.H

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

	Monday, August 15, 1994 3:11:38 PM
	
Apr 21, 2000 (Loren Petrich): Added XML parser object for the rectangle set,
	and exporting color data for game_window color parser

Jul 2, 2000 (Loren Petrich):
	The HUD is now always buffered
*/

#include "XML_ElementParser.h"
#include	"shape_descriptors.h"

/* Rectangles for the interface, etc.. */
/* rectangle id's */
enum {
	/* game window rectangles */
	_player_name_rect= 0,
	_oxygen_rect,
	_shield_rect,
	_motion_sensor_rect,
	_microphone_rect,
	_inventory_rect,
	_weapon_display_rect,
	
	/* interface rectangles */
	_new_game_button_rect,
	_load_game_button_rect,
	_gather_button_rect,
	_join_button_rect,
	_prefs_button_rect,
	_replay_last_button_rect,
	_save_last_button_rect,
	_replace_saved_button_rect,
	_credits_button_rect,
	_quit_button_rect,
	_center_button_rect,
	_singleton_game_button_rect,
	NUMBER_OF_INTERFACE_RECTANGLES
};

/* Colors for drawing.. */
enum {
	_energy_weapon_full_color,
	_energy_weapon_empty_color,
	_black_color,
	_inventory_text_color,
	_inventory_header_background_color,
	_inventory_background_color,
	PLAYER_COLOR_BASE_INDEX,
	
	_white_color= 14,
	_invalid_weapon_color,
	_computer_border_background_text_color,
	_computer_border_text_color,
	_computer_interface_text_color,
	_computer_interface_color_purple,
	_computer_interface_color_red,
	_computer_interface_color_pink,
	_computer_interface_color_aqua,
	_computer_interface_color_yellow,
	_computer_interface_color_brown,
	_computer_interface_color_blue
};

enum { /* justification flags for _draw_screen_text */
	_no_flags,
	_center_horizontal= 0x01,
	_center_vertical= 0x02,
	_right_justified= 0x04,
	_top_justified= 0x08,
	_bottom_justified= 0x10,
	_wrap_text= 0x20
};

enum { /* Fonts for the interface et al.. */
	_interface_font,
	_weapon_name_font,
	_player_name_font,
	_interface_item_count_font,
	_computer_interface_font,
	_computer_interface_title_font,
	_net_stats_font,
	NUMBER_OF_INTERFACE_FONTS
};

/* Structure for portable rectangles.  notice it is exactly same as Rect */
struct screen_rectangle {
	short top, left;
	short bottom, right;
};
typedef struct screen_rectangle screen_rectangle;

/* ------- Prototypes */
void initialize_screen_drawing(void);

void _set_port_to_screen_window(void);
void _set_port_to_gworld(void);
void _restore_port(void);

/* If source==NULL, source= the shapes bounding rectangle */
void _draw_screen_shape(shape_descriptor shape_id, screen_rectangle *destination, 
	screen_rectangle *source);
void _draw_screen_shape_at_x_y(shape_descriptor shape, short x, short y);
void _draw_screen_shape_centered(shape_descriptor shape, screen_rectangle *rectangle,
	short flags);
void _draw_screen_text(const char *text, screen_rectangle *destination, 
	short flags, short font_id, short text_color);
short _text_width(const char *buffer, short font_id);
short _text_width(const char *buffer, int start, int length, short font_id);

void _erase_screen(short color_index);

void _scroll_window(short dy, short rectangle_id, short background_color_index);

void _fill_screen_rectangle(screen_rectangle *rectangle, short color_index);

screen_rectangle *get_interface_rectangle(short index);
const rgb_color &get_interface_color(short index);
class FontSpecifier;
FontSpecifier &get_interface_font(short index);

short _get_font_line_height(short font_index);

void _fill_rect(screen_rectangle *rectangle, short color_index);

void _frame_rect(screen_rectangle *rectangle, short color_index);

// LP addition: stuff to use a buffer for the Heads-Up Display
void _set_port_to_HUD();

// LP addition: get the parser for the interface rectangles
XML_ElementParser *InterfaceRectangles_GetParser();

// Makes the color and font parsers use screen-drawing stuff
void SetColorFontParserToScreenDrawing();

#ifdef SDL
class sdl_font_info;
struct world_point2d;
extern int draw_text(SDL_Surface *s, const char *text, size_t length, int x, int y, uint32 pixel, const sdl_font_info *font, uint16 style);
extern int8 char_width(uint8 c, const sdl_font_info *font, uint16 style);
extern uint16 text_width(const char *text, const sdl_font_info *font, uint16 style);
extern uint16 text_width(const char *text, size_t length, const sdl_font_info *font, uint16 style);
extern int trunc_text(const char *text, int max_width, const sdl_font_info *font, uint16 style);
extern void draw_polygon(SDL_Surface *s, const world_point2d *vertex_array, int vertex_count, uint32 pixel);
extern void draw_line(SDL_Surface *s, const world_point2d *v1, const world_point2d *v2, uint32 pixel, int pen_size);
extern void draw_rectangle(SDL_Surface *s, const SDL_Rect *r, uint32 pixel);

static inline int draw_text(SDL_Surface *s, const char *text, int x, int y, uint32 pixel, const sdl_font_info *font, uint16 style)
{
	return draw_text(s, text, strlen(text), x, y, pixel, font, style);
}
#endif

#endif
