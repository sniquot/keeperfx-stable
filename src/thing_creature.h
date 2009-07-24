/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file thing_creature.h
 *     Header file for thing_creature.c.
 * @par Purpose:
 *     Creatures related functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     17 Mar 2009 - 10 May 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_THINGCREATR_H
#define DK_THINGCREATR_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Thing;

#define CREATURE_TYPES_COUNT  32

/******************************************************************************/
typedef long (*Thing_Class_Func)(struct Thing *);
typedef long (*Creature_Instf_Func)(struct Thing *, long *);
typedef long (*Thing_Filter)(struct Thing *, long);
/******************************************************************************/
extern unsigned short creature_graphics[][22];
extern int creature_swap_idx[CREATURE_TYPES_COUNT];
/******************************************************************************/
unsigned long control_creature_as_controller(struct PlayerInfo *player, struct Thing *thing);
unsigned long control_creature_as_passenger(struct PlayerInfo *player, struct Thing *thing);
void load_swipe_graphic_for_creature(struct Thing *thing);
void leave_creature_as_controller(struct PlayerInfo *player, struct Thing *thing);
long creature_available_for_combat_this_turn(struct Thing *thing);
long creature_look_for_combat(struct Thing *thing);
struct Thing *get_enemy_dungeon_heart_creature_can_see(struct Thing *thing);
long set_creature_object_combat(struct Thing *crthing, struct Thing *obthing);
void set_creature_door_combat(struct Thing *crthing, struct Thing *obthing);
void food_eaten_by_creature(struct Thing *crthing, struct Thing *obthing);
short creature_take_wage_from_gold_pile(struct Thing *crthing,struct Thing *obthing);
struct Thing *get_creature_near(unsigned short pos_x, unsigned short pos_y);
struct Thing *get_creature_near_with_filter(unsigned short pos_x, unsigned short pos_y, Thing_Filter filter, long a4);
void anger_apply_anger_to_creature(struct Thing *thing, long anger, long a2, long a3);
long move_creature(struct Thing *thing);
short kill_creature(struct Thing *thing, struct Thing *tngrp, char a1, unsigned char a2, unsigned char a3, unsigned char a4);
void process_creature_instance(struct Thing *thing);
void update_creature_count(struct Thing *thing);
long process_creature_state(struct Thing *thing);
void process_creature_standing_on_corpses_at(struct Thing *thing, struct Coord3d *pos);
void creature_fire_shot(struct Thing *firing,struct  Thing *target, unsigned short a1, char a2, unsigned char a3);
void set_creature_level(struct Thing *thing, long nlvl);
void init_creature_level(struct Thing *thing, long nlev);
long creature_instance_has_reset(struct Thing *thing, long a2);
long get_human_controlled_creature_target(struct Thing *thing, long a2);
void set_creature_instance(struct Thing *thing, long a1, long a2, long a3, struct Coord3d *pos);
unsigned short find_next_annoyed_creature(unsigned char a1, unsigned short a2);
void draw_creature_view(struct Thing *thing);
struct Thing *get_creature_near_for_controlling(unsigned char a1, long a2, long a3);
long remove_creature_from_group(struct Thing *thing);
long add_creature_to_group_as_leader(struct Thing *thing1, struct Thing *thing2);
void set_first_creature(struct Thing *thing);
void remove_first_creature(struct Thing *thing);

/******************************************************************************/
TbBool thing_is_creature(const struct Thing *thing);
TbBool thing_is_creature_special_worker(const struct Thing *thing);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif