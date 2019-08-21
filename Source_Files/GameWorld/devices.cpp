/*
DEVICES.C

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

Sunday, December 5, 1993 2:48:44 PM

Tuesday, December 7, 1993 11:12:25 PM
	changed to be Jason compatible, open/close doors, and nixed gratuitous enum.
Tuesday, January 4, 1994 10:36:08 AM
	opening doors can wake monsters.
Sunday, September 18, 1994 6:23:04 PM  (alain)
	much of control panel code has been rewritten. no longer use composite sides,
	but a flag in the side data structure. some control panels work over time (refueling)
	and there are on/off textures associated with each control panel. and sounds.
Friday, June 9, 1995 11:43:37 AM  (Jason')
	destroy-able switches.
Wednesday, June 21, 1995 8:31:57 AM  (Jason)
	tag switches.

Jan 30, 2000 (Loren Petrich):
	Changed "class" to "_class" to make data structures more C++-friendly
	Removed some "static" declarations that conflict with "extern"

Feb 3, 2000 (Loren Petrich):
	Added Jjaro control panels; they appear to be a clone of the sewage ones

Feb. 4, 2000 (Loren Petrich):
	Changed halt() to assert(false) for better debugging

May 26, 2000 (Loren Petrich):
	Added XML shapes support; had recently added XML configuration in general

June 3, 2000 (Loren Petrich):
	Idiot-proofed the control-panels accessor; it now returns NULL if an index is out of range.

Aug 10, 2000 (Loren Petrich):
	Added Chris Pruett's Pfhortran changes
        
Feb 3, 2003 (Woody Zenfell):
        Support for network saved-games
*/

#include "cseries.h"

#include "map.h"
#include "monsters.h"
#include "interface.h"
#include "player.h"
#include "platforms.h"
#include "mysound.h"
#include "computer_interface.h"
//#include "music.h"
#include "lightsource.h"
#include "game_window.h"
#include "items.h"
//CP Addition: Scripting Hooks
#include "scripting.h"
#include "shell.h"	// screen_printf()
//MH: Lua scripting
#include "lua_script.h"

#include <string.h>
#include <limits.h>

#ifdef env68k
#pragma segment marathon
#endif

/* ---------- constants */

#define OXYGEN_RECHARGE_FREQUENCY 0
#define ENERGY_RECHARGE_FREQUENCY 0

#define MAXIMUM_ACTIVATION_RANGE (3*WORLD_ONE)
#define MAXIMUM_PLATFORM_ACTIVATION_RANGE (3*WORLD_ONE)
#define MAXIMUM_CONTROL_ACTIVATION_RANGE (WORLD_ONE+WORLD_ONE_HALF)
#define OBJECT_RADIUS 50

#define MINIMUM_RESAVE_TICKS (2*TICKS_PER_SECOND)

// For detecting double-save (overwrite) in a netgame
enum {
        kDoubleClickTicks = 10
};

enum
{
	_target_is_platform,
	_target_is_control_panel,
	_target_is_unrecognized
};

/* ---------- structures */

enum // control panel sounds
{
	_activating_sound,
	_deactivating_sound,
	_unusuable_sound,
	
	NUMBER_OF_CONTROL_PANEL_SOUNDS
};


struct control_panel_definition
{
	int16 _class;
	uint16 flags;
	
	int16 collection;
	int16 active_shape, inactive_shape;

	int16 sounds[NUMBER_OF_CONTROL_PANEL_SOUNDS];
	_fixed sound_frequency;
	
	int16 item;
};

/* ---------- globals */

#define NUMBER_OF_CONTROL_PANEL_DEFINITIONS (sizeof(control_panel_definitions)/sizeof(struct control_panel_definition))

static struct control_panel_definition control_panel_definitions[]=
{
	// _collection_walls1 -- LP: water
	{_panel_is_oxygen_refuel, 0, _collection_walls1, 2, 3, {_snd_oxygen_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_shield_refuel, 0, _collection_walls1, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_double_shield_refuel, 0, _collection_walls1, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/8, NONE},
	{_panel_is_tag_switch, 0, _collection_walls1, 0, 1, {_snd_chip_insertion, NONE, NONE}, FIXED_ONE, _i_uplink_chip},
	{_panel_is_light_switch, 0, _collection_walls1, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_platform_switch, 0, _collection_walls1, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls1, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_pattern_buffer, 0, _collection_walls1, 4, 4, {_snd_pattern_buffer, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_computer_terminal, 0, _collection_walls1, 4, 4, {NONE, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls1, 1, 0, {_snd_destroy_control_panel, NONE, NONE}, FIXED_ONE, NONE},

	// _collection_walls2 -- LP: lava
	{_panel_is_shield_refuel, 0, _collection_walls2, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_double_shield_refuel, 0, _collection_walls2, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/8, NONE},
	{_panel_is_triple_shield_refuel, 0, _collection_walls2, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/4, NONE},
	{_panel_is_light_switch, 0, _collection_walls2, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_platform_switch, 0, _collection_walls2, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls2, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_pattern_buffer, 0, _collection_walls2, 4, 4, {_snd_pattern_buffer, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_computer_terminal, 0, _collection_walls2, 4, 4, {NONE, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_oxygen_refuel, 0, _collection_walls2, 2, 3, {_snd_oxygen_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls2, 0, 1, {_snd_chip_insertion, NONE, NONE}, FIXED_ONE, _i_uplink_chip},
	{_panel_is_tag_switch, 0, _collection_walls2, 1, 0, {_snd_destroy_control_panel, NONE, NONE}, FIXED_ONE, NONE},

	// _collection_walls3 -- LP: sewage
	{_panel_is_shield_refuel, 0, _collection_walls3, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_double_shield_refuel, 0, _collection_walls3, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/8, NONE},
	{_panel_is_triple_shield_refuel, 0, _collection_walls3, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/4, NONE},
	{_panel_is_light_switch, 0, _collection_walls3, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_platform_switch, 0, _collection_walls3, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls3, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_pattern_buffer, 0, _collection_walls3, 4, 4, {_snd_pattern_buffer, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_computer_terminal, 0, _collection_walls3, 4, 4, {NONE, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_oxygen_refuel, 0, _collection_walls3, 2, 3, {_snd_oxygen_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls3, 0, 1, {_snd_chip_insertion, NONE, NONE}, FIXED_ONE, _i_uplink_chip},
	{_panel_is_tag_switch, 0, _collection_walls3, 1, 0, {_snd_destroy_control_panel, NONE, NONE}, FIXED_ONE, NONE},

	// _collection_walls4 -- LP: really _collection_walls5 -- pfhor
	{_panel_is_shield_refuel, 0, _collection_walls5, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_double_shield_refuel, 0, _collection_walls5, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/8, NONE},
	{_panel_is_triple_shield_refuel, 0, _collection_walls5, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/4, NONE},
	{_panel_is_light_switch, 0, _collection_walls5, 0, 1, {_snd_pfhor_switch_on, _snd_pfhor_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_platform_switch, 0, _collection_walls5, 0, 1, {_snd_pfhor_switch_on, _snd_pfhor_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls5, 0, 1, {_snd_pfhor_switch_on, _snd_pfhor_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_pattern_buffer, 0, _collection_walls5, 4, 4, {_snd_pattern_buffer, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_computer_terminal, 0, _collection_walls5, 4, 4, {NONE, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_oxygen_refuel, 0, _collection_walls5, 2, 3, {_snd_oxygen_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls5, 0, 1, {_snd_chip_insertion, NONE, NONE}, FIXED_ONE, _i_uplink_chip},
	{_panel_is_tag_switch, 0, _collection_walls5, 1, 0, {_snd_destroy_control_panel, NONE, NONE}, FIXED_ONE, NONE},

	// LP addition: _collection_walls4 -- jjaro
	{_panel_is_shield_refuel, 0, _collection_walls4, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_double_shield_refuel, 0, _collection_walls4, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/8, NONE},
	{_panel_is_triple_shield_refuel, 0, _collection_walls4, 2, 3, {_snd_energy_refuel, NONE, NONE}, FIXED_ONE+FIXED_ONE/4, NONE},
	{_panel_is_light_switch, 0, _collection_walls4, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_platform_switch, 0, _collection_walls4, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls4, 0, 1, {_snd_switch_on, _snd_switch_off, _snd_cant_toggle_switch}, FIXED_ONE, NONE},
	{_panel_is_pattern_buffer, 0, _collection_walls4, 4, 4, {_snd_pattern_buffer, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_computer_terminal, 0, _collection_walls4, 4, 4, {NONE, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_oxygen_refuel, 0, _collection_walls4, 2, 3, {_snd_oxygen_refuel, NONE, NONE}, FIXED_ONE, NONE},
	{_panel_is_tag_switch, 0, _collection_walls4, 0, 1, {_snd_chip_insertion, NONE, NONE}, FIXED_ONE, _i_uplink_chip},
	{_panel_is_tag_switch, 0, _collection_walls4, 1, 0, {_snd_destroy_control_panel, NONE, NONE}, FIXED_ONE, NONE},
};

struct control_panel_settings_definition {
	// How far can one reach to activate the controls?
	short ReachDistance;
	short ReachHorizontal;
	// For recharging
	short SingleEnergy;
	short SingleEnergyRate;
	short DoubleEnergy;
	short DoubleEnergyRate;
	short TripleEnergy;
	short TripleEnergyRate;
};

struct control_panel_settings_definition control_panel_settings = {
	MAXIMUM_CONTROL_ACTIVATION_RANGE, // ReachDistance
	2, // ReachHorizontal
	PLAYER_MAXIMUM_SUIT_ENERGY, // SingleEnergy
	1, // SingleEnergyRate
	2*PLAYER_MAXIMUM_SUIT_ENERGY, // DoubleEnergy
	2, // DoubleEnergyRate
	3*PLAYER_MAXIMUM_SUIT_ENERGY, // TripleEnergy
	3 // TripleEnergyRate
};

/* ------------ private prototypes */

control_panel_definition *get_control_panel_definition(
	const short control_panel_type);

static short find_action_key_target(short player_index, world_distance range, short *target_type);
//static bool line_side_has_control_panel(short line_index, short polygon_index, short *side_index_with_panel);
static void	somebody_save_full_auto(player_data* inWhoSaved, bool inOverwrite);
static void	change_panel_state(short player_index, short panel_side_index);
static void set_control_panel_texture(struct side_data *side);

static bool line_is_within_range(short monster_index, short line_index, world_distance range);

static bool switch_can_be_toggled(short line_index, bool player_hit);

static void play_control_panel_sound(short side_index, short sound_index);

static bool get_recharge_status(short side_index);

/* ---------- code */

/* set the initial states of all switches based on the objects they control */
void initialize_control_panels_for_level(
	void)
{
	short side_index;
	struct side_data *side;

	for (side_index= 0, side= map_sides; side_index<dynamic_world->side_count; ++side, ++side_index)
	{
		if (SIDE_IS_CONTROL_PANEL(side))
		{
			// LP change: modified previous fix so that it edits the side definition
			struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
			if (!definition)
			{
				SET_SIDE_CONTROL_PANEL(side,false);
				continue;
			}
			bool status= false;
			
			switch (definition->_class)
			{
				case _panel_is_tag_switch:
					status= GET_CONTROL_PANEL_STATUS(side);
					// use default position
					break;
				
				case _panel_is_light_switch:
					status= get_light_status(side->control_panel_permutation);
					break;
				
				case _panel_is_platform_switch:
					if (platform_is_on(get_polygon_data(side->control_panel_permutation)->permutation)) status= true;
					break;
			}
			
			SET_CONTROL_PANEL_STATUS(side, status);
			set_control_panel_texture(side);
		}
	}
}

void update_control_panels(
	void)
{
	short player_index;
	struct player_data *player;

	for (player_index= 0, player= players; player_index<dynamic_world->player_count; ++player_index, ++player)
	{
		short side_index;

		if ((side_index= player->control_panel_side_index)!=NONE)
		{
			struct side_data *side= get_side_data(player->control_panel_side_index);
			// LP change: idiot-proofing
			struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
			if (!definition) continue;

			if(definition->_class == _panel_is_pattern_buffer)
			{	// Player was working on a full-auto save
				if(dynamic_world->tick_count - player->ticks_at_last_successful_save > kDoubleClickTicks)
				{	// no double-click - need to safe save
					somebody_save_full_auto(player, false);
				}
			}
			else
			{
				bool still_in_use= false;

				if (player->variables.direction == player->variables.last_direction &&
					player->variables.last_position.x == player->variables.position.x &&
					player->variables.last_position.y == player->variables.position.y &&
					player->variables.last_position.z == player->variables.position.z)
				{
					switch (definition->_class)
					{
						case _panel_is_oxygen_refuel:
							if (!(dynamic_world->tick_count&OXYGEN_RECHARGE_FREQUENCY))
							{
								if (player->suit_oxygen<PLAYER_MAXIMUM_SUIT_OXYGEN)
								{
									player->suit_oxygen+= TICKS_PER_SECOND;
									mark_oxygen_display_as_dirty();
									still_in_use= true;
								}
							}
							break;
                                                
						case _panel_is_shield_refuel:
						case _panel_is_double_shield_refuel:
						case _panel_is_triple_shield_refuel:
							if (!(dynamic_world->tick_count&ENERGY_RECHARGE_FREQUENCY))
							{
								short maximum, rate;

								switch (definition->_class)
								{
									case _panel_is_shield_refuel:
										maximum= control_panel_settings.SingleEnergy;
										rate= control_panel_settings.SingleEnergyRate;
										break;
									case _panel_is_double_shield_refuel:
										maximum= control_panel_settings.DoubleEnergy;
										rate= control_panel_settings.DoubleEnergyRate;
										break;
									case _panel_is_triple_shield_refuel:
										maximum= control_panel_settings.TripleEnergy;
										rate= control_panel_settings.TripleEnergyRate;
										break;
									default:
										assert(false);
								}
								if (player->suit_energy<maximum)
								{
									player->suit_energy= CEILING(player->suit_energy+rate, maximum);
									mark_shield_display_as_dirty();
									still_in_use= true;
								}
							}
							break;

						default:
							assert(false);
					}
				}
			
				if (still_in_use)
				{
					set_control_panel_texture(side);
					play_control_panel_sound(side_index, _activating_sound);
				}
				else
				{
					change_panel_state(player_index, side_index);
					stop_sound(NONE, definition->sounds[_activating_sound]);
				}
			}
		}
	}
}

void update_action_key(
	short player_index,
	bool triggered)
{
	short               object_index;
	short               target_type;
	
	if(triggered) 
	{
		object_index= find_action_key_target(player_index, MAXIMUM_ACTIVATION_RANGE, &target_type);
		
		if(object_index != NONE)
		{
			switch(target_type) 
			{
				case _target_is_platform:
					player_touch_platform_state(player_index, object_index);
					break;
				case _target_is_control_panel:
					change_panel_state(player_index, object_index);
					break;

				case _target_is_unrecognized:
					break;
					
				default:
					vhalt(csprintf(temporary, "%d is not a valid target type", target_type));
					break;
			}
		}
	}
}

bool untoggled_repair_switches_on_level(
	void)
{
	short side_index;
	struct side_data *side;
	bool untoggled_switch= false;
	
	for (side_index= 0, side= map_sides; side_index<dynamic_world->side_count && !untoggled_switch; ++side_index, ++side)
	{
		if (SIDE_IS_CONTROL_PANEL(side) && SIDE_IS_REPAIR_SWITCH(side))
		{
			// LP change: idiot-proofing
			struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
			if (!definition) continue;
			
			switch (definition->_class)
			{
				case _panel_is_platform_switch:
					untoggled_switch= platform_is_at_initial_state(get_polygon_data(side->control_panel_permutation)->permutation) ? true : false;
					break;
				
				default:
					untoggled_switch= !GET_CONTROL_PANEL_STATUS(side);
					break;
			}
		}
	}
	
	return untoggled_switch;
}

void assume_correct_switch_position(
	short switch_type, /* platform or light */
	short permutation, /* platform or light index */
	bool new_state)
{
	short side_index;
	struct side_data *side;
	
	for (side_index= 0, side= map_sides; side_index<dynamic_world->side_count; ++side_index, ++side)
	{
		if (SIDE_IS_CONTROL_PANEL(side) && side->control_panel_permutation==permutation)
		{
			struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
			// LP change: idiot-proofing
			if (!definition) continue;
			
			if (switch_type==definition->_class)
			{
				play_control_panel_sound(side_index, new_state ? _activating_sound : _deactivating_sound);
				SET_CONTROL_PANEL_STATUS(side, new_state);
				set_control_panel_texture(side);
			}
		}
	}
}

void try_and_toggle_control_panel(
	short polygon_index,
	short line_index)
{
	short side_index= find_adjacent_side(polygon_index, line_index);
	
	if (side_index!=NONE)
	{
		struct side_data *side= get_side_data(side_index);

		if (SIDE_IS_CONTROL_PANEL(side))
		{
			if (switch_can_be_toggled(side_index, false))
			{
				bool make_sound = false, state= GET_CONTROL_PANEL_STATUS(side);
				struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
				// LP change: idiot-proofing
				if (!definition) return;
				
				switch (definition->_class)
				{
					case _panel_is_tag_switch:
						state= !state;
						make_sound= set_tagged_light_statuses(side->control_panel_permutation, state);
						if (try_and_change_tagged_platform_states(side->control_panel_permutation, state)) make_sound= true;
						if (!side->control_panel_permutation) make_sound= true;
						if (make_sound)
						{
							SET_CONTROL_PANEL_STATUS(side, state);
							set_control_panel_texture(side);
						}
						//CP Addition: Script Hook; LP: this and its successors made consistent with change_panel_state()
						activate_tag_switch_trap(side->control_panel_permutation);
                                                //MH: Lua script hook
                                                // L_Call_Tag_Switch(side->control_panel_permutation);
						
						break;
					case _panel_is_light_switch:
						state= !state;
						make_sound= set_light_status(side->control_panel_permutation, state);
						
						//CP Addition: Script Hook
						activate_light_switch_trap(side->control_panel_permutation);
                                                //MH: Lua script hook
                                               //  L_Call_Light_Switch(side->control_panel_permutation);
						
						break;
					case _panel_is_platform_switch:
						state= !state;
						make_sound= try_and_change_platform_state(get_polygon_data(side->control_panel_permutation)->permutation, state);
						
						//CP Addition: Script Hook
						activate_platform_switch_trap(side->control_panel_permutation);
                                                //MH: Lua script hook
                                                // L_Call_Platform_Switch(side->control_panel_permutation);
						
						break;
				}
				
				if (make_sound)
				{
					play_control_panel_sound(side_index, state ? _activating_sound : _deactivating_sound);
				}
			}
		}
	}
}

#if 0
/* ---------- these functions are all used in Vulcan */
static bool shape_is_control_panel(
	shape_descriptor texture)
{
	bool is_control_panel= false;

	for(unsigned index= 0; index<NUMBER_OF_CONTROL_PANEL_DEFINITIONS; ++index)
	{
		struct control_panel_definition *definition= get_control_panel_definition(index);

		if(texture==BUILD_DESCRIPTOR(definition->collection, definition->active_shape) ||
			texture==BUILD_DESCRIPTOR(definition->collection, definition->inactive_shape))
		{
			is_control_panel= true;
			break;
		}
	}
	
	return is_control_panel;
}

static short get_panel_class(
	short panel_type)
{
	struct control_panel_definition *definition= get_control_panel_definition(panel_type);
	
	return definition->_class;
}

//---------- changed 9.18.95
static bool control_panel_type_valid_for_texture(
	shape_descriptor shape,
	short control_panel_type)
{
	struct control_panel_definition *definition= get_control_panel_definition(control_panel_type);
	bool valid= false;
	
	if(GET_DESCRIPTOR_COLLECTION(shape)==definition->collection)
	{
		if((GET_DESCRIPTOR_SHAPE(shape)==definition->active_shape) ||
			(GET_DESCRIPTOR_SHAPE(shape)==definition->inactive_shape))
		{
			valid= true;
		}
	}
	
	return valid;
}
//------------------
#endif

/* ---------- private code */

control_panel_definition *get_control_panel_definition(
	const short control_panel_type)
{
	return GetMemberWithBounds(control_panel_definitions,control_panel_type,NUMBER_OF_CONTROL_PANEL_DEFINITIONS);
}

static short find_action_key_target(
	short player_index,
	world_distance range,
	short *target_type)
{
	struct player_data *player= get_player_data(player_index);
	short current_polygon= player->camera_polygon_index;
	world_point2d destination;
	bool done= false;
	short itemhit, line_index;
	struct polygon_data *polygon;
	
	// In case we don't hit anything
	*target_type = _target_is_unrecognized;

	/* Should we use this one, the physics one, or the object one? */
	ray_to_line_segment((world_point2d *) &player->location, &destination, player->facing, range);

//	dprintf("#%d(#%d,#%d) --> (#%d,#%d) (#%d along #%d)", current_polygon, player->location.x, player->location.y, destination.x, destination.y, range, player->facing);

	itemhit= NONE;
	while (!done)
	{
		line_index= find_line_crossed_leaving_polygon(current_polygon, (world_point2d *) &player->location, &destination);
		
		if (line_index==NONE)
		{
			done= true;
		} 
		else 
		{
			struct line_data *line;
			short original_polygon;

			line= get_line_data(line_index);
			original_polygon= current_polygon;
			current_polygon= find_adjacent_polygon(current_polygon, line_index);

//			dprintf("leaving polygon #%d through line #%d to polygon #%d", original_polygon, line_index, current_polygon);

			if (current_polygon!=NONE)
			{
				polygon= get_polygon_data(current_polygon);

				/* We hit a platform */				
				if (polygon->type==_polygon_is_platform && line_is_within_range(player->monster_index, line_index, MAXIMUM_PLATFORM_ACTIVATION_RANGE) &&
					platform_is_legal_player_target(polygon->permutation))
				{
//					dprintf("found platform #%d in %p", polygon->permutation, polygon);
					itemhit= polygon->permutation;
					*target_type= _target_is_platform;
					done= true;
				} 
			} 
			else 
			{
				done= true;
			}

			/* Slammed a wall */
			if (line_is_within_range(player->monster_index, line_index, control_panel_settings.ReachDistance))
			{
				if (line_side_has_control_panel(line_index, original_polygon, &itemhit))
				{
					if (switch_can_be_toggled(itemhit, true))
					{
						*target_type= _target_is_control_panel;
						done= true;
					}
					else
					{
						itemhit= NONE;
					}
				}
			}
		}
	}
	
	return itemhit;
}

static bool line_is_within_range(
	short monster_index,
	short line_index,
	world_distance range)
{
	world_point3d monster_origin= get_object_data(get_monster_data(monster_index)->object_index)->location;
	world_point3d line_origin;
	world_distance radius, height;
	world_distance dx, dy, dz;
	
	calculate_line_midpoint(line_index, &line_origin);
	get_monster_dimensions(monster_index, &radius, &height);
	monster_origin.z+= height>>1;
	
	dx= monster_origin.x-line_origin.x;
	dy= monster_origin.y-line_origin.y;
	dz= control_panel_settings.ReachHorizontal*(monster_origin.z-line_origin.z); /* dz is weighted */
	
	return isqrt(dx*dx + dy*dy + dz*dz)<range ? true : false;
}

//tiennou: removed static for lua access
bool line_side_has_control_panel(
	short line_index, 
	short polygon_index,
	short *side_index_with_panel)
{
	short             side_index = NONE;
	bool           has_panel = false;
	struct line_data  *line = get_line_data(line_index);
	struct side_data  *side = NULL;
	
	if (line->clockwise_polygon_owner==polygon_index)
	{
		side_index = line->clockwise_polygon_side_index;
		if (side_index != NONE)
		{
			side = get_side_data(side_index);
		}
	} 
	else
	{
		assert(line->counterclockwise_polygon_owner==polygon_index);
		side_index = line->counterclockwise_polygon_side_index;
		if (side_index != NONE)
		{
			side= get_side_data(side_index);
		}
	}

	if (side != NULL && SIDE_IS_CONTROL_PANEL(side))
	{
		*side_index_with_panel = side_index;
		has_panel = true;
	}
	
	return has_panel;
}

static void
somebody_save_full_auto(player_data* inWhoSaved, bool inOverwrite)
{
        play_control_panel_sound(inWhoSaved->control_panel_side_index, _activating_sound);

        // These need to happen before the save so that a freshly restored player
        // doesn't end up automatically saving the game :)
        inWhoSaved->control_panel_side_index = NONE;
        inWhoSaved->ticks_at_last_successful_save = dynamic_world->tick_count;

        if(inWhoSaved == local_player)
        {
                save_game_full_auto(inOverwrite);
        }
        else
        {
                screen_printf("%s has saved the game", inWhoSaved->name);
        }
}

static void	change_panel_state(
	short player_index,
	short panel_side_index)
{
	bool state, make_sound= false;
	struct side_data *side= get_side_data(panel_side_index);
	struct player_data *player= get_player_data(player_index);
	struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
	
	// LP change: idiot-proofing
	if (!definition) return;
	
	state= GET_CONTROL_PANEL_STATUS(side);
	
	/* Do the right thing, based on the panel type.. */
	switch (definition->_class)
	{
		case _panel_is_oxygen_refuel:
		case _panel_is_shield_refuel:
		case _panel_is_double_shield_refuel:
		case _panel_is_triple_shield_refuel:
			player->control_panel_side_index= player->control_panel_side_index==panel_side_index ? NONE : panel_side_index;
			state= get_recharge_status(panel_side_index);
			SET_CONTROL_PANEL_STATUS(side, state);
			if (!state) set_control_panel_texture(side);
                                // Lua script hook
                                if (player -> control_panel_side_index == panel_side_index)
                                    L_Call_Start_Refuel (definition->_class, player_index, panel_side_index);
                                else
                                    L_Call_End_Refuel (definition->_class, player_index, panel_side_index);
			break;
		case _panel_is_computer_terminal:
			if (get_game_state()==_game_in_progress && !PLAYER_HAS_CHEATED(player) && !PLAYER_HAS_MAP_OPEN(player))
			{
				//CP Addition: Script Hook
				activate_terminal_enter_trap(side->control_panel_permutation);
                                //MH: Lua script hook
                                L_Call_Terminal_Enter(side->control_panel_permutation,player_index);
				
				/* this will handle changing levels, if necessary (i.e., if we�re finished) */
				enter_computer_interface(player_index, side->control_panel_permutation, calculate_level_completion_state());
			}
			break;
		case _panel_is_tag_switch:
			if (definition->item==NONE || (!state && try_and_subtract_player_item(player_index, definition->item)))
			{
				state= !state;
				
				make_sound= set_tagged_light_statuses(side->control_panel_permutation, state);
				if (try_and_change_tagged_platform_states(side->control_panel_permutation, state)) make_sound= true;
				if (!side->control_panel_permutation) make_sound= true;
				if (make_sound)
				{
					SET_CONTROL_PANEL_STATUS(side, state);
					set_control_panel_texture(side);
				}
				//CP Addition: Script Hook
				activate_tag_switch_trap(side->control_panel_permutation);
                                //MH: Lua script hook
                                L_Call_Tag_Switch(side->control_panel_permutation,player_index);
			
			}
			break;
		case _panel_is_light_switch:
			state= !state;
			make_sound= set_light_status(side->control_panel_permutation, state);
			
			//CP Addition: Script Hook
			activate_light_switch_trap(side->control_panel_permutation);
                        //MH: Lua script hook
                        L_Call_Light_Switch(side->control_panel_permutation,player_index);

			break;
		case _panel_is_platform_switch:
			state= !state;
			make_sound= try_and_change_platform_state(get_polygon_data(side->control_panel_permutation)->permutation, state);
			
			//CP Addition: Script Hook
			activate_platform_switch_trap(side->control_panel_permutation);
                        //MH: Lua script hook
                        L_Call_Platform_Switch(side->control_panel_permutation,player_index);
			
			break;
		case _panel_is_pattern_buffer:
                        if (player_controlling_game() && !PLAYER_HAS_CHEATED(local_player))
                        {
                                if(game_is_networked)
                                {
                                        if(player->control_panel_side_index != panel_side_index)
                                        {
                                                if(dynamic_world->tick_count - player->ticks_at_last_successful_save > MINIMUM_RESAVE_TICKS)
                                                {	// User pressed "action" - we'll see if they're going to do it again.
                                                        player->ticks_at_last_successful_save = dynamic_world->tick_count;
                                                        player->control_panel_side_index = panel_side_index;
                                                }
                                        }
                                        else
                                        {	// Double-press - overwrite recent saved game
                                                somebody_save_full_auto(player, true);
                                        }
                                }
                                else
                                {	// game is not networked
                                        if(dynamic_world->tick_count-player->ticks_at_last_successful_save>MINIMUM_RESAVE_TICKS)
                                        {
                                                play_control_panel_sound(panel_side_index, _activating_sound);
                                                
                                                //CP Addition: Script Hook
                                                activate_pattern_buffer_trap(side->control_panel_permutation);
                                                //MH: Lua script hook
                                                L_Call_Pattern_Buffer(side->control_panel_permutation,player_index);
                                        
                //				fade_out_background_music(30);
                
                                                /* Assume a successful save- prevents vidding of the save game key.. */
                                                player->ticks_at_last_successful_save= dynamic_world->tick_count;
                                                if (!save_game()) 
                                                {
                                                        player->ticks_at_last_successful_save= 0;
                                                }
                //				fade_in_background_music(30);
                                        }
                                }
                        }
			break;
	}
	
	if (make_sound)
	{
		play_control_panel_sound(panel_side_index, state ? _activating_sound : _deactivating_sound);
	}
	
	return;	
}

static void set_control_panel_texture(
	struct side_data *side)
{
	struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
	// LP change: idiot-proofing
	if (!definition) return;
	
	side->primary_texture.texture= BUILD_DESCRIPTOR(definition->collection,
		GET_CONTROL_PANEL_STATUS(side) ? definition->active_shape : definition->inactive_shape);
}


static bool switch_can_be_toggled(
	short side_index,
	bool player_hit)
{
	bool valid_toggle= true;
	struct side_data *side= get_side_data(side_index);
	struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
	// LP change: idiot-proofing
	if (!definition) return false;
	
	if (side->flags&_side_is_lighted_switch)
	{
		valid_toggle= get_light_intensity(side->primary_lightsource_index)>(3*FIXED_ONE/4) ? true : false;
	}

	if (definition->item!=NONE && !player_hit) valid_toggle= false;
	if (player_hit && (side->flags&_side_switch_can_only_be_hit_by_projectiles)) valid_toggle= false;
	
	if (valid_toggle && (side->flags&_side_switch_can_be_destroyed))
	{
		// destroy switch
		SET_SIDE_CONTROL_PANEL(side, false);
	}
	
	if (!valid_toggle && player_hit) play_control_panel_sound(side_index, _unusuable_sound);
	
	return valid_toggle;
}

static void play_control_panel_sound(
	short side_index,
	short sound_index)
{
	struct side_data *side= get_side_data(side_index);
	struct control_panel_definition *definition= get_control_panel_definition(side->control_panel_type);
	
	// LP change: idiot-proofing
	if (!definition) return;

	if (!(sound_index>=0 && sound_index<NUMBER_OF_CONTROL_PANEL_SOUNDS)) return;
	
	_play_side_sound(side_index, definition->sounds[sound_index], definition->sound_frequency);
}

static bool get_recharge_status(
	short side_index)
{
	short player_index;
	bool status= false;
	
	for (player_index= 0; player_index<dynamic_world->player_count; ++player_index)
	{
		struct player_data *player= get_player_data(player_index);
		
		if (player->control_panel_side_index==side_index) status= true;
	}
	
	return status;
}


// Parses sound indices
// Note: This does not need a ResetValues() method because the SoundList is
// already part of a struct control_panel_definition that gets reset anyways.
class XML_CPSoundParser: public XML_ElementParser
{
	short Type;
	short Which;

	enum {NumberOfValues = 2};
	bool IsPresent[NumberOfValues];

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	
	short *SoundList;
	
	XML_CPSoundParser(): XML_ElementParser("sound") {}
};


bool XML_CPSoundParser::Start()
{
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	
	return true;
}

bool XML_CPSoundParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"type"))
	{
		if (ReadBoundedInt16Value(Value,Type,0,NUMBER_OF_CONTROL_PANEL_SOUNDS-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"which"))
	{
		if (ReadBoundedInt16Value(Value,Which,NONE,SHRT_MAX))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_CPSoundParser::AttributesDone()
{
	// Verify...
	bool AllPresent = true;
	for (int k=0; k<NumberOfValues; k++)
		AllPresent &= IsPresent[k];
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
	
	SoundList[Type] = Which;
			
	return true;
}

static XML_CPSoundParser CPSoundParser;


struct control_panel_definition *original_control_panel_definitions = NULL;

class XML_ControlPanelParser: public XML_ElementParser
{
	short Index;
	control_panel_definition Data;
	
	// What is present?
	bool IndexPresent;
	enum {NumberOfValues = 6};
	bool IsPresent[NumberOfValues];
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	XML_ControlPanelParser(): XML_ElementParser("panel") {}
};

bool XML_ControlPanelParser::Start()
{
	// back up old values first
	if (!original_control_panel_definitions) {
		original_control_panel_definitions = (struct control_panel_definition *) malloc(sizeof(struct control_panel_definition) * NUMBER_OF_CONTROL_PANEL_DEFINITIONS);
		assert(original_control_panel_definitions);
		for (unsigned i = 0; i < NUMBER_OF_CONTROL_PANEL_DEFINITIONS; i++)
			original_control_panel_definitions[i] = control_panel_definitions[i];
	}

	IndexPresent = false;
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	
	return true;
}

bool XML_ControlPanelParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		if (ReadBoundedInt16Value(Value,Index,0,NUMBER_OF_CONTROL_PANEL_DEFINITIONS-1))
		{
			IndexPresent = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"type"))
	{
		if (ReadBoundedInt16Value(Value,Data._class,0,NUMBER_OF_CONTROL_PANELS-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"coll"))
	{
		if (ReadBoundedInt16Value(Value,Data.collection,0,NUMBER_OF_COLLECTIONS-1))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"active_frame"))
	{
		if (ReadBoundedInt16Value(Value,Data.active_shape,0,MAXIMUM_SHAPES_PER_COLLECTION-1))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"inactive_frame"))
	{
		if (ReadBoundedInt16Value(Value,Data.inactive_shape,0,MAXIMUM_SHAPES_PER_COLLECTION-1))
		{
			IsPresent[3] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"pitch"))
	{
		float Pitch;
		if (ReadBoundedNumericalValue(Value,"%f",Pitch,float(0),float(SHRT_MAX+1)))
		{
			Data.sound_frequency = long(FIXED_ONE*Pitch+0.5);
			IsPresent[4] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"item"))
	{
		if (ReadBoundedInt16Value(Value,Data.item,NONE,NUMBER_OF_DEFINED_ITEMS-1))
		{
			IsPresent[5] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_ControlPanelParser::AttributesDone()
{
	// Verify...
	if (!IndexPresent)
	{
		AttribsMissing();
		return false;
	}
	control_panel_definition& OrigData = control_panel_definitions[Index];
	
	if (IsPresent[0]) OrigData._class = Data._class;
	if (IsPresent[1]) OrigData.collection = Data.collection;
	if (IsPresent[2]) OrigData.active_shape = Data.active_shape;
	if (IsPresent[3]) OrigData.inactive_shape = Data.inactive_shape;
	if (IsPresent[4]) OrigData.sound_frequency = Data.sound_frequency;
	if (IsPresent[5]) OrigData.item = Data.item;
	
	CPSoundParser.SoundList = OrigData.sounds;
	
	return true;
}

bool XML_ControlPanelParser::ResetValues()
{
	if (original_control_panel_definitions) {
		for (unsigned i = 0; i < NUMBER_OF_CONTROL_PANEL_DEFINITIONS; i++)
			control_panel_definitions[i] = original_control_panel_definitions[i];
		free(original_control_panel_definitions);
		original_control_panel_definitions = NULL;
	}
	return true;
}

static XML_ControlPanelParser ControlPanelParser;


struct control_panel_settings_definition *original_control_panel_settings = NULL;

class XML_ControlPanelsParser: public XML_ElementParser
{	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool ResetValues();

	XML_ControlPanelsParser(): XML_ElementParser("control_panels") {}
};

bool XML_ControlPanelsParser::Start()
{
	if (!original_control_panel_settings) {
		original_control_panel_settings = (struct control_panel_settings_definition *) malloc(sizeof(struct control_panel_settings_definition));
		*original_control_panel_settings = control_panel_settings;
	}
	return true;
}

bool XML_ControlPanelsParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"reach"))
	{
		float FVal;
		if (ReadFloatValue(Value,FVal))
		{
			control_panel_settings.ReachDistance = int(WORLD_ONE*FVal + 0.5);
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"horiz"))
	{
		return ReadInt16Value(Value,control_panel_settings.ReachHorizontal);
	}
	else if (StringsEqual(Tag,"single_energy"))
	{
		return ReadInt16Value(Value,control_panel_settings.SingleEnergy);
	}
	else if (StringsEqual(Tag,"single_energy_rate"))
	{
		return ReadInt16Value(Value,control_panel_settings.SingleEnergyRate);
	}
	else if (StringsEqual(Tag,"double_energy"))
	{
		return ReadInt16Value(Value,control_panel_settings.DoubleEnergy);
	}
	else if (StringsEqual(Tag,"double_energy_rate"))
	{
		return ReadInt16Value(Value,control_panel_settings.DoubleEnergyRate);
	}
	else if (StringsEqual(Tag,"triple_energy"))
	{
		return ReadInt16Value(Value,control_panel_settings.TripleEnergy);
	}
	else if (StringsEqual(Tag,"triple_energy_rate"))
	{
		return ReadInt16Value(Value,control_panel_settings.TripleEnergyRate);
	}
	UnrecognizedTag();
	return false;
}

bool XML_ControlPanelsParser::ResetValues()
{
	if (original_control_panel_settings) {
		control_panel_settings = *original_control_panel_settings;
		free(original_control_panel_settings);
		original_control_panel_settings = NULL;
	}
	return true;
}

static XML_ControlPanelsParser ControlPanelsParser;


// XML-parser support
XML_ElementParser *ControlPanels_GetParser()
{
	ControlPanelParser.AddChild(&CPSoundParser);
	ControlPanelsParser.AddChild(&ControlPanelParser);

	return &ControlPanelsParser;
}