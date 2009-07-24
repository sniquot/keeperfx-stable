/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file packets.c
 *     Packet processing routines.
 * @par Purpose:
 *     Functions for creating and executing packets.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     30 Jan 2009 - 11 Feb 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "packets.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_memory.h"
#include "bflib_video.h"
#include "bflib_sprite.h"
#include "bflib_keybrd.h"
#include "bflib_vidraw.h"
#include "bflib_fileio.h"
#include "bflib_dernc.h"
#include "bflib_network.h"
#include "bflib_sound.h"

#include "kjm_input.h"
#include "front_simple.h"
#include "front_landview.h"
#include "frontend.h"
#include "vidmode.h"
#include "config.h"
#include "player_instances.h"
#include "thing_doors.h"
#include "keeperfx.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT void _DK_process_pause_packet(long a1, long a2);
DLLIMPORT void _DK_process_map_packet_clicks(long idx);
DLLIMPORT void _DK_process_quit_packet(struct PlayerInfo *, int);
DLLIMPORT void _DK_process_dungeon_control_packet_clicks(long idx);
DLLIMPORT void _DK_process_players_dungeon_control_packet_control(long idx);
DLLIMPORT void _DK_process_packets(void);
DLLIMPORT char _DK_process_players_global_packet_action(long idx);
DLLIMPORT void _DK_process_players_dungeon_control_packet_action(long idx);
DLLIMPORT void _DK_process_players_creature_control_packet_control(long idx);
DLLIMPORT void _DK_process_players_creature_control_packet_action(long idx);
DLLIMPORT unsigned long _DK_get_packet_save_checksum(void);
DLLIMPORT void _DK_load_packets_for_turn(long gameturn);
DLLIMPORT void _DK_open_new_packet_file_for_save(void);
DLLIMPORT void _DK_open_packet_file_for_load(char *fname);
DLLIMPORT void _DK_set_packet_action(struct Packet *pckt,unsigned char,short,short,short,short);

/******************************************************************************/
#define PACKET_START_POS (sizeof(struct PacketSaveHead))
#define PACKET_TURN_SIZE (NET_PLAYERS_COUNT*sizeof(struct Packet) + sizeof(TbBigChecksum))

/******************************************************************************/
void set_packet_action(struct Packet *pckt, unsigned char pcktype, unsigned short par1, unsigned short par2, unsigned short par3, unsigned short par4)
{
  pckt->field_6 = par1;
  pckt->field_8 = par2;
  pckt->action = pcktype;
}

void set_players_packet_action(struct PlayerInfo *player, unsigned char pcktype, unsigned short par1, unsigned short par2, unsigned short par3, unsigned short par4)
{
  struct Packet *pckt;
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  pckt->field_6 = par1;
  pckt->field_8 = par2;
  pckt->action = pcktype;
}

unsigned char get_players_packet_action(struct PlayerInfo *player)
{
  struct Packet *pckt;
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  return pckt->action;
}

void set_packet_control(struct Packet *pckt, unsigned long flag)
{
  pckt->control_flags |= flag;
}

void set_players_packet_control(struct PlayerInfo *player, unsigned long flag)
{
  struct Packet *pckt;
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  pckt->control_flags |= flag;
}

void unset_packet_control(struct Packet *pckt, unsigned long flag)
{
  pckt->control_flags &= ~flag;
}

void unset_players_packet_control(struct PlayerInfo *player, unsigned long flag)
{
  struct Packet *pckt;
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  pckt->control_flags &= ~flag;
}

void set_players_packet_position(struct PlayerInfo *player, long x, long y)
{
  struct Packet *pckt;
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  pckt->pos_x = x;
  pckt->pos_y = y;
}

void clear_packets(void)
{
  int i;
  for (i=0; i < PACKETS_COUNT; i++)
    memset(&game.packets[i], 0, sizeof(struct Packet));
}

short set_packet_pause_toggle(void)
{
  struct PlayerInfo *player;
  struct Packet *pckt;
  if (my_player_number >= PLAYERS_COUNT)
    return false;
  player=&(game.players[my_player_number]);
  if (player->packet_num >= PACKETS_COUNT)
    return false;
  pckt = &game.packets[player->packet_num];
  set_packet_action(pckt, PckA_TogglePause, 0, 0, 0, 0);
  return true;
}

unsigned long compute_players_checksum(void)
{
  struct PlayerInfo *player;
  struct Coord3d *mappos;
  int i;
  unsigned long sum;
  sum = 0;
  for (i=0; i<PLAYERS_COUNT; i++)
  {
    player=&(game.players[i%PLAYERS_COUNT]);
    if (((player->field_0 & 0x01) != 0) && ((player->field_0 & 0x40) == 0) && (player->acamera != NULL))
    {
        mappos = &(player->acamera->mappos);
        sum += player->field_4B1 + player->instance_num
                   + mappos->x.val + mappos->z.val + mappos->y.val;
    }
  }
  return sum;
}

/*
 * Checks if all active players packets have same checksums.
 * @return Returns false if all checksums are same; true if there's mismatch.
 */
short checksums_different(void)
{
  struct PlayerInfo *player;
  struct Packet *pckt;
  TbChecksum checksum;
  unsigned short is_set;
  int i;
  is_set = false;
  for (i=0; i<PLAYERS_COUNT; i++)
  {
    player=&(game.players[i]);
    if (((player->field_0 & 0x01) != 0) && ((player->field_0 & 0x40) == 0))
    {
        pckt = &game.packets[player->packet_num];
        if (!is_set)
        {
          checksum = pckt->chksum;
          is_set = true;
        } else
        if (checksum != pckt->chksum)
        {
          return true;
        }
    }
  }
  return false;
}

void update_double_click_detection(long plyr_idx)
{
  struct PlayerInfo *player;
  struct Packet *pckt;
  unsigned short i;
  player = &(game.players[plyr_idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
  {
    if (packet_left_button_click_space_count[plyr_idx] < 5)
      packet_left_button_double_clicked[plyr_idx] = 1;
    packet_left_button_click_space_count[plyr_idx] = 0;
  }
  if ((pckt->control_flags & (PCtr_LBtnClick|PCtr_LBtnHeld)) == 0)
  {
    if (packet_left_button_click_space_count[plyr_idx] < LONG_MAX)
      packet_left_button_click_space_count[plyr_idx]++;
  }
}

void process_dungeon_control_packet_clicks(long plyr_idx)
{
  static const char *func_name="process_dungeon_control_packet_clicks";
  struct PlayerInfo *player;
  struct Dungeon *dungeon;
  struct Packet *pckt;
  struct CreatureControl *cctrl;
  struct Thing *thing;
  struct SlabMap *slb;
  struct Room *room;
  struct Map *map;
  MapSubtlCoord stl_x,stl_y;
  MapSubtlCoord cx,cy;
  short val172;
  struct Coord3d pos;
  MapCoord x,y;
  long i,k;

  //_DK_process_dungeon_control_packet_clicks(plyr_idx); return;

  player = &(game.players[plyr_idx%PLAYERS_COUNT]);
  dungeon = &(game.dungeon[player->field_2B%DUNGEONS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting for state %d\n",func_name,(int)player->work_state);
#endif
  player->field_4A4 = 1;
  packet_left_button_double_clicked[plyr_idx] = 0;
  if ((pckt->control_flags & 0x4000) != 0)
    return;

  if ((pckt->control_flags & PCtr_LBtnHeld) != 0)
  {
    switch (player->work_state)
    {
    case KSt_CallToArms:
        if (dungeon->field_884)
          player->field_4D2 = (dungeon->field_883 << 2);
        else
          update_spell_overcharge(player, 6);
        break;
    case KSt_CaveIn:
        update_spell_overcharge(player, 7);
        break;
    case KSt_SightOfEvil:
        update_spell_overcharge(player, 5);
        break;
    case KSt_Lightning:
        update_spell_overcharge(player, 10);
        break;
    case KSt_SpeedUp:
        update_spell_overcharge(player, 11);
        break;
    case KSt_Armour:
        update_spell_overcharge(player, 12);
        break;
    case KSt_Conceal:
        update_spell_overcharge(player, 13);
        break;
    case KSt_Heal:
        update_spell_overcharge(player, 8);
        break;
    default:
        player->field_4D2++;
        break;
    }
  } else
  if ((pckt->control_flags & PCtr_LBtnRelease) == 0)
  {
    player->field_4D2 = 0;
  }
  if ((pckt->control_flags & PCtr_RBtnHeld) != 0)
  {
    player->field_4D6++;
  } else
  if ((pckt->control_flags & PCtr_RBtnRelease) == 0)
  {
    player->field_4D6 = 0;
  }
  update_double_click_detection(plyr_idx);
  player->thing_under_hand = 0;
  x = ((unsigned short)pckt->pos_x);
  y = ((unsigned short)pckt->pos_y);
  stl_x = (x >> 8);
  stl_y = (y >> 8);
  val172 = false;

  switch (player->work_state)
  {
  case 1:
      val172 = 1;
      cx = slab_starting_subtile(stl_x);
      cy = slab_starting_subtile(stl_y);
      if ((pckt->control_flags & PCtr_LBtnAnyAction) == 0)
        player->field_455 = 0;
      player->field_454 = (unsigned short)(pckt->field_10 & 0x1E) >> 1;
      player->field_3 &= 0xFDu;
      if ((player->field_455 != 0) && (player->field_455 != 3))
      {
        if ((player->field_43A != 0) && (player->instance_num != 1))
        {
          thing = thing_get(player->field_43A);
          delete_thing_structure(thing, 0);
          player->field_43A = 0;
        }
      } else
      {
        thing = get_nearest_thing_for_hand_or_slap(plyr_idx, pckt->pos_x, pckt->pos_y);
        if (!thing_is_invalid(thing))
        {
          if (player->field_43A == 0)
            create_power_hand(player->field_2B);
          player->thing_under_hand = thing->field_1B;
        }
        thing = get_first_thing_in_power_hand(player);
        if (!thing_is_invalid(thing))
        {
          if (player->field_43A == 0)
            create_power_hand(player->field_2B);
          i = thing_is_creature_special_worker(thing);
          if (can_drop_thing_here(stl_x, stl_y, player->field_2B, i)
            || !can_dig_here(stl_x, stl_y, player->field_2B))
          {
            tag_cursor_blocks_thing_in_hand(player->field_2B, stl_x, stl_y, i, player->field_4A4);
          } else
          {
            player->field_3 |= 0x02u;
            tag_cursor_blocks_dig(player->field_2B, stl_x, stl_y, player->field_4A4);
          }
        }
        if (player->field_43A != 0)
        {
          if ((player->instance_num != 1) && (player->instance_num != 2) &&
              (player->instance_num != 3) && (player->instance_num != 4))
          {
            thing = get_first_thing_in_power_hand(player);
            if ((player->thing_under_hand != 0) || thing_is_invalid(thing))
            {
              set_power_hand_graphic(plyr_idx, 782, 256);
              if (!thing_is_invalid(thing))
                thing->field_4F |= 0x01;
            } else
            if ((thing->class_id == 1) && object_is_gold_pile(thing))
            {
              set_power_hand_graphic(plyr_idx, 781, 256);
              thing->field_4F &= 0xFEu;
            } else
            {
              set_power_hand_graphic(plyr_idx, 784, 256);
              thing->field_4F &= 0xFEu;
            }
          }
        }
      }

      if ((pckt->control_flags & PCtr_MapCoordsValid) != 0)
      {
        if (is_my_player(player) && !game_is_busy_doing_gui())
        {
          if (player->field_454 == 1)
            tag_cursor_blocks_dig(player->field_2B, stl_x, stl_y, player->field_4A4);
        }
        if ((pckt->control_flags & PCtr_LBtnClick) != 0)
        {
          player->field_4AB = stl_x;
          player->field_4AD = stl_y;
          player->field_4AF = 1;
          player->field_455 = player->field_454;
          switch (player->field_454)
          {
          case 1:
            i = get_subtile_number(slab_center_subtile(player->field_4AB),slab_center_subtile(player->field_4AD));
            set_flag_byte(&player->field_0, 0x20, find_from_task_list(plyr_idx,i) != -1);
            break;
          case 2:
            thing = get_door_for_position(player->field_4AB, player->field_4AD);
            if (!thing_is_invalid(thing))
            {
              if (thing->byte_17.h)
                unlock_door(thing);
              else
                lock_door(thing);
            }
            break;
          case 3:
            if (player->thing_under_hand == 0)
            {
              i = get_subtile_number(slab_center_subtile(player->field_4AB),slab_center_subtile(player->field_4AD));
              set_flag_byte(&player->field_0, 0x20, find_from_task_list(plyr_idx,i) != -1);
              player->field_3 |= 0x01;
            }
            break;
          }
          unset_packet_control(pckt, PCtr_LBtnClick);
        }
        if ((pckt->control_flags & PCtr_RBtnClick) != 0)
        {
          player->field_4AB = stl_x;
          player->field_4AD = stl_y;
          player->field_4AF = 1;
          unset_packet_control(pckt, PCtr_RBtnClick);
        }
      }
      if ((pckt->control_flags & PCtr_LBtnHeld) != 0)
      {
          if (player->field_455 == 0)
          {
            player->field_455 = player->field_454;
            if (player->field_454 == 1)
            {
              i = get_subtile_number(slab_center_subtile(stl_x),slab_center_subtile(stl_y));
              set_flag_byte(&player->field_0, 0x20, find_from_task_list(plyr_idx,i) != -1);
            }
          }
          if (player->field_4AF != 0)
          {
            if (player->field_454 == player->field_455)
            {
              if (player->field_455 == 1)
              {
                if ((player->field_0 & 0x20) != 0)
                {
                  untag_blocks_for_digging_in_rectangle_around(cx, cy, plyr_idx);
                } else
                if (dungeon->field_E8F < 300)
                {
                  tag_blocks_for_digging_in_rectangle_around(cx, cy, plyr_idx);
                } else
                if (is_my_player(player))
                {
                  output_message(101, 500, 1);
                }
              } else
              if ((player->field_455 == 3) && ((player->field_3 & 0x01) != 0))
              {
                if ((player->field_0 & 0x20) != 0)
                {
                  untag_blocks_for_digging_in_rectangle_around(cx, cy, plyr_idx);
                } else
                if (dungeon->field_E8F < 300)
                {
                  if (can_dig_here(stl_x, stl_y, player->field_2B))
                    tag_blocks_for_digging_in_rectangle_around(cx, cy, plyr_idx);
                } else
                if (is_my_player(player))
                {
                  output_message(101, 500, 1);
                }
              }
            }
            unset_packet_control(pckt, PCtr_LBtnRelease);
          }
      }
      if ((pckt->control_flags & PCtr_RBtnHeld) != 0)
      {
        if (player->field_4AF != 0)
          unset_packet_control(pckt, PCtr_RBtnRelease);
      }
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        if (player->field_455 == 0)
          player->field_455 = player->field_454;
        if (player->field_4AF != 0)
        {
          if ((player->thing_under_hand != 0) && (player->field_4 != 0)
            && (dungeon->things_in_hand[0] != player->thing_under_hand)
            && can_thing_be_possessed(thing_get(player->thing_under_hand), plyr_idx) )
          {
            player->field_43E = player->thing_under_hand;
            set_player_state(player, 11, 0);
            set_player_instance(player, 5, 0);
            unset_packet_control(pckt, PCtr_LBtnRelease);
          } else
          if ((player->thing_under_hand != 0) && (player->field_5 != 0)
            && (dungeon->things_in_hand[0] != player->thing_under_hand)
            && can_thing_be_queried(thing_get(player->thing_under_hand), plyr_idx) )
          {
            if (player->thing_under_hand != player->field_2F)
            {
              if (is_my_player(player))
              {
                turn_off_all_panel_menus();
                turn_on_menu(31);
              }
              player->field_43E = player->thing_under_hand;
              set_player_state(player, 12, 0);
              set_player_instance(player, 9, 0);
            }
            unset_packet_control(pckt, PCtr_LBtnRelease);
          } else
          if (player->field_455 == player->field_454)
          {
            if (player->field_454 == 1 )
            {
              if ((player->field_0 & 0x20) != 0)
              {
                untag_blocks_for_digging_in_rectangle_around(cx, cy, plyr_idx);
              } else
              if (300-dungeon->field_E8F >= 9)
              {
                tag_blocks_for_digging_in_rectangle_around(cx, cy, plyr_idx);
              } else
              if (is_my_player(player))
              {
                output_message(101, 500, 1);
              }
            } else
            if (player->field_454 == 3)
            {
              if (player->thing_under_hand != 0)
                magic_use_power_hand(plyr_idx, stl_x, stl_y, 0);
            }
          }
          player->field_4AF = 0;
          unset_packet_control(pckt, PCtr_LBtnRelease);
          player->field_455 = 0;
          player->field_3 &= 0xFEu;
        }
      }
      if ((pckt->control_flags & PCtr_RBtnRelease) != 0)
      {
        if (player->field_4AF != 0)
        {
          if (!power_hand_is_empty(player))
          {
            if (dump_held_things_on_map(player->field_2B, stl_x, stl_y, 1))
            {
              player->field_4AF = 0;
              unset_packet_control(pckt, PCtr_RBtnRelease);
            }
          } else
          {
            if (player->field_454 == 3)
              magic_use_power_slap(plyr_idx, stl_x, stl_y);
            player->field_4AF = 0;
            unset_packet_control(pckt, PCtr_RBtnRelease);
          }
        }
      }
      break;
  case 2:
      if ((pckt->control_flags & PCtr_MapCoordsValid) == 0)
      {
        if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && (player->field_4AF != 0))
        {
          player->field_4AF = 0;
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
        break;
      }
      player->field_4A4 = 1;
      if (is_my_player(player))
        gui_room_type_highlighted = player->field_4A3;
      i = tag_cursor_blocks_place_room(player->field_2B, stl_x, stl_y, player->field_4A4);
      if ((pckt->control_flags & PCtr_LBtnClick) == 0)
      {
        if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && (player->field_4AF != 0))
        {
          player->field_4AF = 0;
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
        break;
      }
      if (i == 0)
      {
        if (is_my_player(player))
        {
          play_non_3d_sample(119);
          unset_packet_control(pckt, PCtr_LBtnClick);
        }
        break;
      }
      k = game.room_stats[player->field_4A3].cost;
      if (i_can_allocate_free_room_structure())
      {
        if (take_money_from_dungeon(plyr_idx, k, 1) < 0)
        {
          if (is_my_player(player))
            output_message(87, 0, 1);
          break;
        }
      }
      x = ((player->field_4A4+1) / 2) + 3*map_to_slab[stl_x];
      y = ((player->field_4A4+1) / 2) + 3*map_to_slab[stl_y];
      if (place_room(plyr_idx, player->field_4A3, x, y) != NULL)
      {
        if (player->field_4A3 == 15)
          dungeon->lvstats.bridges_built++;
        dungeon->field_EA4 = 192;
        if (is_my_player(player))
          play_non_3d_sample(77);
        set_coords_to_slab_center(&pos,map_to_slab[stl_x],map_to_slab[stl_y]);
        thing = create_effect_element(&pos, 41, plyr_idx);
        if (!thing_is_invalid(thing))
          thing->long_13 = k;
      }
      unset_packet_control(pckt, PCtr_LBtnClick);
      break;
  case 3:
      if (((pckt->control_flags & PCtr_LBtnRelease) == 0) || ((pckt->control_flags & PCtr_MapCoordsValid) == 0))
        break;
      pos.x.val = x;
      pos.y.val = y;
      pos.z.val = 0;
      thing = create_creature(&pos, 8, plyr_idx);
      if (thing_is_invalid(thing))
      {
        unset_packet_control(pckt, PCtr_LBtnRelease);
        break;
      }
      pos.z.val = get_thing_height_at(thing, &pos);
      if (thing_in_wall_at(thing, &pos))
      {
        delete_thing_structure(thing, 0);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      } else
      {
        thing->mappos.x.val = pos.x.val;
        thing->mappos.y.val = pos.y.val;
        thing->mappos.z.val = pos.z.val;
        remove_first_creature(thing);
        set_first_creature(thing);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_PutHero:
      if (((pckt->control_flags & PCtr_LBtnRelease) == 0) || ((pckt->control_flags & PCtr_MapCoordsValid) == 0))
        break;
      create_random_hero_creature(x, y, game.field_14E496, CREATURE_MAX_LEVEL);
      unset_packet_control(pckt, PCtr_LBtnRelease);
      break;
  case KSt_CallToArms:
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_call_to_arms(plyr_idx, stl_x, stl_y, i, 0);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_CaveIn:
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_cave_in(plyr_idx, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_SightOfEvil:
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_sight(plyr_idx, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_Slap:
      val172 = 1;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
        player->thing_under_hand = 0;
      else
        player->thing_under_hand = thing->field_1B;
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        magic_use_power_slap(plyr_idx, stl_x, stl_y);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case 10:
      val172 = 1;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
        player->thing_under_hand = 0;
      else
        player->thing_under_hand = thing->field_1B;

      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        if (player->thing_under_hand > 0)
        {
          player->field_43E = player->thing_under_hand;
          set_player_instance(player, 6, 0);
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
      }
      if ((pckt->control_flags & PCtr_RBtnRelease) != 0)
      {
        if (player->instance_num != 6)
        {
          set_player_state(player, player->field_456, 0);
          unset_packet_control(pckt, PCtr_RBtnRelease);
        }
      }
      break;
  case 11:
      val172 = 1;
      thing = get_creature_near_for_controlling(plyr_idx, x, y);
      if (thing_is_invalid(thing))
        player->thing_under_hand = 0;
      else
        player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        if (player->thing_under_hand > 0)
        {
          player->field_43E = player->thing_under_hand;
          set_player_instance(player, 5, 0);
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
      }
      if ((pckt->control_flags & PCtr_RBtnRelease) != 0)
      {
        if (player->instance_num != 5)
        {
          set_player_state(player, player->field_456, 0);
          unset_packet_control(pckt, PCtr_RBtnRelease);
        }
      }
      break;
  case 12:
  case 15:
      val172 = 1;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
        player->thing_under_hand = 0;
      else
        player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        if (player->thing_under_hand > 0)
        {
          if (player->field_2F != player->thing_under_hand)
          {
            if (is_my_player(player))
            {
              turn_off_all_panel_menus();
              initialise_tab_tags_and_menu(31);
              turn_on_menu(31);
            }
            player->field_43E = player->thing_under_hand;
            set_player_instance(player, 9, 0);
          }
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
      }
      if (player->work_state == 15)
      {
        thing = thing_get(player->field_2F);
        if ((pckt->control_flags & PCtr_RBtnRelease) != 0)
        {
          if (is_my_player(player))
          {
            turn_off_query_menus();
            turn_on_main_panel_menu();
          }
          set_player_instance(player, 10, 0);
          unset_packet_control(pckt, PCtr_RBtnRelease);
        } else
        if (thing->health < 0)
        {
          set_player_instance(player, 10, 0);
          if (is_my_player(player))
          {
            turn_off_query_menus();
            turn_on_main_panel_menu();
          }
        }
      }
      break;
  case 13:
      val172 = 1;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
        player->thing_under_hand = 0;
      else
        player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        if ((player->field_2F > 0) && (player->field_2F < THINGS_COUNT))
        {
          if ((pckt->control_flags & PCtr_MapCoordsValid) != 0)
          {
            thing = thing_get(player->field_2F);
            setup_person_move_to_position(thing, stl_x, stl_y, 0);
            thing->field_8 = 122;
          }
        } else
        {
          thing = get_creature_near(x, y);
          if (!thing_is_invalid(thing))
          {
            player->field_2F = thing->field_1B;
            initialise_thing_state(thing, 122);
            cctrl = creature_control_get_from_thing(thing);
            i = cctrl->field_7A;// & 0xFFF;
            if (i > 0)
              make_group_member_leader(thing);
          }
        }
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      if ((pckt->control_flags & PCtr_RBtnRelease) != 0)
      {
        if ((player->field_2F > 0) && (player->field_2F < THINGS_COUNT))
        {
          thing = thing_get(player->field_2F);
          set_start_state(thing);
          player->field_2F = 0;
        }
        unset_packet_control(pckt, PCtr_RBtnRelease);
      }
      break;
  case KSt_PutCreature:
      if (((pckt->control_flags & PCtr_LBtnRelease) == 0) || ((pckt->control_flags & PCtr_MapCoordsValid) == 0))
        break;
      create_random_evil_creature(x, y, plyr_idx, CREATURE_MAX_LEVEL);
      unset_packet_control(pckt, PCtr_LBtnRelease);
      break;
  case 16:
      if ((pckt->control_flags & PCtr_MapCoordsValid) == 0)
      {
        if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && (player->field_4AF != 0))
        {
          player->field_4AF = 0;
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
        break;
      }
      set_coords_to_slab_center(&pos,map_to_slab[stl_x],map_to_slab[stl_y]);
      player->field_4A4 = 1;
      i = tag_cursor_blocks_place_trap(player->field_2B, stl_x, stl_y);
      if ((pckt->control_flags & PCtr_LBtnClick) == 0)
      {
        if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && (player->field_4AF != 0))
        {
          player->field_4AF = 0;
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
        break;
      }
      if (dungeon->trap_amount[player->field_4A5%TRAP_TYPES_COUNT] <= 0)
      {
        unset_packet_control(pckt, PCtr_LBtnClick);
        break;
      }
      if (i == 0)
      {
        if (is_my_player(player))
          play_non_3d_sample(119);
        unset_packet_control(pckt, PCtr_LBtnClick);
        break;
      }
      delete_room_slabbed_objects(map_to_slab[stl_x] + map_tiles_x * map_to_slab[stl_y]);
      thing = create_trap(&pos, player->field_4A5, plyr_idx);
      thing->mappos.z.val = get_thing_height_at(thing, &thing->mappos);
      thing->byte_17.h = 0;
      if (remove_workshop_item(plyr_idx, 8, player->field_4A5))
        dungeon->lvstats.traps_used++;
      dungeon->field_EA4 = 192;
      if (is_my_player(player))
      {
        play_non_3d_sample(117);
      }
      unset_packet_control(pckt, PCtr_LBtnClick);
      break;
  case KSt_Lightning:
      player->thing_under_hand = 0;
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_lightning(plyr_idx, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_PlaceDoor:
      if ((pckt->control_flags & PCtr_MapCoordsValid) != 0)
      {
        player->field_4A4 = 1;
        // Make the frame around active slab
        i = tag_cursor_blocks_place_door(player->field_2B, stl_x, stl_y);
        if ((pckt->control_flags & PCtr_LBtnClick) != 0)
        {
          k = map_tiles_x * map_to_slab[stl_y] + map_to_slab[stl_x];
          delete_room_slabbed_objects(k);
          packet_place_door(stl_x, stl_y, player->field_2B, player->field_4A6, i);
        }
        unset_packet_control(pckt, PCtr_LBtnClick);
      }
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        if (player->field_4AF != 0)
        {
          player->field_4AF = 0;
          unset_packet_control(pckt, PCtr_LBtnRelease);
        }
      }
      break;
  case KSt_SpeedUp:
      val172 = true;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
      {
        player->thing_under_hand = 0;
        break;
      }
      player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_speed(plyr_idx, thing, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_Armour:
      val172 = true;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
      {
        player->thing_under_hand = 0;
        break;
      }
      player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_armour(plyr_idx, thing, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_Conceal:
      val172 = true;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
      {
        player->thing_under_hand = 0;
        break;
      }
      player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_conceal(plyr_idx, thing, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_Heal:
      val172 = true;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_owned_by, plyr_idx);
      if (thing_is_invalid(thing))
      {
        player->thing_under_hand = 0;
        break;
      }
      player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_heal(plyr_idx, thing, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_Sell:
      if ((pckt->control_flags & PCtr_MapCoordsValid) == 0)
      {
        if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && (player->field_4AF != 0))
        {
          player->field_4AF = 0;
        unset_packet_control(pckt, PCtr_LBtnRelease);
        }
        break;
      }
      player->field_4A4 = 1;
      pos.x.val = x;
      pos.y.val = y;
      pos.z.val = 128;
      if (is_my_player(player))
      {
        if (!game_is_busy_doing_gui())
          tag_cursor_blocks_sell_area(player->field_2B, stl_x, stl_y, player->field_4A4);
      }
      if ((pckt->control_flags & PCtr_LBtnClick) == 0)
      {
        if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && (player->field_4AF != 0))
        {
          player->field_4AF = 0;
        unset_packet_control(pckt, PCtr_LBtnRelease);
        }
        break;
      }
      i = 0;
      slb = get_slabmap_for_subtile(stl_x, stl_y);
      map = get_map_block(stl_x,stl_y);
      if ((slb->field_5 & 7) == plyr_idx)
      {
        if (((map->flags & 0x02) != 0) && (slb->slab != SlbT_ENTRANCE) && (slb->slab != SlbT_DUNGHEART))
        {
          room = room_get(slb->room_index);
          if (room_is_invalid(room))
          {
            LbErrorLog("No room to delete at subtile (%d,%d)\n",stl_x,stl_y);
          } else
          {
            i = ((long)game.room_stats[room->kind].cost) * 50 / 100;
            dungeon = &(game.dungeon[room->owner%DUNGEONS_COUNT]);
            if (room->owner != game.field_14E497)
              dungeon->field_AD5++;
            delete_room_slab(map_to_slab[stl_x], map_to_slab[stl_y], 0);
            set_coords_to_slab_center(&pos,map_to_slab[stl_x],map_to_slab[stl_y]);
            dungeon->field_EA4 = 192;
            if (is_my_player(player))
              play_non_3d_sample(115);
          }
        } else
        {
          x = 3*map_to_slab[stl_x];
          y = 3*map_to_slab[stl_y];
          thing = get_door_for_position(x, y);
          if (!thing_is_invalid(thing))
          {
            dungeon->field_EA4 = 192;
            if (is_my_player(player))
              play_non_3d_sample(115);
            i = game.doors_config[thing->model].selling_value;
            destroy_door(thing);
          } else
          if (get_trap_for_slab_position(map_to_slab[stl_x], map_to_slab[stl_y]) != NULL)
          {
            i = 0;
            for (k=0; k < AROUND_TILES_COUNT; k++)
            {
              thing = get_trap_for_position(x+around[k].delta_x+1, y+around[k].delta_y+1);
              if (!thing_is_invalid(thing))
              {
                if (thing->byte_13.l == 0)
                  remove_workshop_object_from_player(thing->owner, trap_to_object[thing->model%TRAP_TYPES_COUNT]);
                i += game.traps_config[thing->model].selling_value;
                destroy_trap(thing);
              }
            }
            if (is_my_player(player))
              play_non_3d_sample(115);
            dungeon->field_EA4 = 192;
          }
        }
      } else
      {
        LbWarnLog("%s: Player %d can't sell item on unowned ground.\n",func_name,(int)plyr_idx);
      }
      if (i != 0)
      {
        thing = create_effect_element(&pos, 41, plyr_idx);
        if (!thing_is_invalid(thing))
          thing->long_13 = i;
        dungeon->field_AFD += i;
        dungeon->field_AF9 += i;
      }
      unset_packet_control(pckt, PCtr_LBtnClick);
      break;
  case KSt_CreateImp:
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        magic_use_power_imp(plyr_idx, stl_x, stl_y);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_DestroyWalls:
      if (((pckt->control_flags & PCtr_LBtnRelease) != 0) && ((pckt->control_flags & PCtr_MapCoordsValid) != 0))
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_destroy_walls(plyr_idx, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_Disease:
      thing = get_creature_near_with_filter(x, y, creature_near_filter_is_enemy_of_and_not_imp, plyr_idx);
      if (thing_is_invalid(thing))
      {
        player->thing_under_hand = 0;
        break;
      }
      player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_disease(plyr_idx, thing, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  case KSt_TurnChicken:
      val172 = true;
      thing = get_creature_near_with_filter(x, y, creature_near_filter_not_imp, plyr_idx);
      if (thing_is_invalid(thing))
      {
        player->thing_under_hand = 0;
        break;
      }
      player->thing_under_hand = thing->field_1B;
      if ((pckt->control_flags & PCtr_LBtnRelease) != 0)
      {
        i = get_spell_overcharge_level(player);
        magic_use_power_chicken(plyr_idx, thing, stl_x, stl_y, i);
        unset_packet_control(pckt, PCtr_LBtnRelease);
      }
      break;
  default:
      break;
  }
  // resetting position variables - they may have been changed
  x = ((unsigned short)pckt->pos_x);
  y = ((unsigned short)pckt->pos_y);
  stl_x = x/(map_subtiles_x+1);
  stl_y = y/(map_subtiles_y+1);
  if (player->thing_under_hand == 0)
  {
    if ((x != 0) && (y == 0))  // what this condition is supposed to mean!?
    {
      thing = get_queryable_object_near(x, y, plyr_idx);
      if (!thing_is_invalid(thing))
        player->thing_under_hand = thing->field_1B;
    }
  }
  if (((pckt->control_flags & PCtr_HeldAnyButton) != 0) && (val172))
  {
    if ((player->field_455 == 0) || (player->field_455 == 3))
      stop_creatures_around_hand(plyr_idx, stl_x, stl_y);
  }
}

TbBigChecksum get_packet_save_checksum(void)
{
  return _DK_get_packet_save_checksum();
}

void open_new_packet_file_for_save(void)
{
  static const char *func_name="open_new_packet_file_for_save";
  struct PlayerInfo *player;
  int i;
  //_DK_open_new_packet_file_for_save(); return;
  // Filling the header
  game.packet_save_head.field_0 = 0;
  game.packet_save_head.level_num = get_loaded_level_number();
  game.packet_save_head.field_8 = 0;
  game.packet_save_head.field_C = 0;
  game.packet_save_head.field_D = 0;
  game.packet_save_head.chksum = game.packet_checksum;
  for (i=0; i<PLAYERS_COUNT; i++)
  {
    player=&(game.players[i%PLAYERS_COUNT]);
    if (player->field_0 & 0x01)
    {
      game.packet_save_head.field_C |= (1 << i) & 0xff;
      if (player->field_0 & 0x40)
        game.packet_save_head.field_D |= (1 << i) & 0xff;
    }
  }
  LbFileDelete(game.packet_fname);
  game.packet_save_fp = LbFileOpen(game.packet_fname, Lb_FILE_MODE_NEW);
  if (game.packet_save_fp == -1)
  {
    error(func_name, 3774, "Cannot open keeper packet file for save");
    return;
  }
  game.packet_fopened = 1;
  LbFileWrite(game.packet_save_fp, &game.packet_save_head, sizeof(struct PacketSaveHead));
}

void load_packets_for_turn(long nturn)
{
  static const char *func_name="load_packets_for_turn";
  struct PlayerInfo *player;
  struct Packet *pckt;
  TbChecksum pckt_chksum;
  TbBigChecksum tot_chksum;
  long data_size;
  short done;
  char *text;
  long i;
  const int turn_data_size = PACKET_TURN_SIZE;
  unsigned char pckt_buf[PACKET_TURN_SIZE+4];
  player=&(game.players[my_player_number%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  pckt_chksum = pckt->chksum;
  if (nturn >= game.field_149F30)
  {
    error(func_name, 3973, "Out of turns to load from Packet File");
    return;
  }

  data_size = PACKET_START_POS + turn_data_size*nturn;
  if (data_size != game.packet_file_pos)
  {
    error(func_name, 3879, "Packet Loading Seek Offset is wrong");
    LbFileSeek(game.packet_save_fp, data_size, Lb_FILE_SEEK_BEGINNING);
    game.packet_file_pos = data_size;
  }
  if (LbFileRead(game.packet_save_fp, &pckt_buf, turn_data_size) == -1)
  {
    error(func_name, 3879, "Cannot read turn data from Packet File");
    return;
  }
  game.packet_file_pos += turn_data_size;
  for (i=0; i < NET_PLAYERS_COUNT; i++)
    LbMemoryCopy(&game.packets[i], &pckt_buf[i*sizeof(struct Packet)], sizeof(struct Packet));
  tot_chksum = llong(&pckt_buf[NET_PLAYERS_COUNT*sizeof(struct Packet)]);
  if (game.numfield_C & 0x01)
  {
      done = false;
      while (!done)
      {
        for (i=0; i<NET_PLAYERS_COUNT; i++)
        {
          pckt = &game.packets[i];
          if ((pckt->action != 0) || (pckt->control_flags != 0))
          {
            done = true;
            break;
          }
        }
        data_size += turn_data_size;
        game.pckt_gameturn++;
        if (data_size != game.packet_file_pos)
        {
          error(func_name, 3914, "Packet Saving Seek Offset is wrong");
          LbFileSeek(game.packet_save_fp, data_size, Lb_FILE_SEEK_BEGINNING);
          game.packet_file_pos = data_size;
        }
        if (LbFileRead(game.packet_save_fp, &pckt_buf, turn_data_size) == -1)
        {
          error(func_name, 3451, "Cannot read turn data from Packet File");
          return;
        }
        game.packet_file_pos += turn_data_size;
        for (i=0; i < NET_PLAYERS_COUNT; i++)
          LbMemoryCopy(&game.packets[i], &pckt_buf[i*sizeof(struct Packet)], sizeof(struct Packet));
        tot_chksum = llong(&pckt_buf[NET_PLAYERS_COUNT*sizeof(struct Packet)]);
      }
  }
  if (game.turns_fastforward > 0)
      game.turns_fastforward--;
  if (game.packet_checksum)
  {
      player=&(game.players[my_player_number%PLAYERS_COUNT]);
      pckt = &game.packets[player->packet_num%PACKETS_COUNT];
      if (get_packet_save_checksum() != tot_chksum)
      {
        text = buf_sprintf("PacketSave checksum - Out of sync (GameTurn %d)", game.play_gameturn);
        error(func_name, 3947, text);
        if (!is_onscreen_msg_visible())
          show_onscreen_msg(game.num_fps, "Out of sync");
      } else
      if (pckt->chksum != pckt_chksum)
      {
        text = buf_sprintf("Opps we are really Out Of Sync (GameTurn %d)", game.play_gameturn);
        error(func_name, 3955, text);
        if (!is_onscreen_msg_visible())
          show_onscreen_msg(game.num_fps, "Out of sync");
      }
  }
}

void process_pause_packet(long a1, long a2)
{
  _DK_process_pause_packet(a1, a2);
}

void process_players_dungeon_control_packet_control(long idx)
{
  static const char *func_name="process_players_dungeon_control_packet_control";
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  //_DK_process_players_dungeon_control_packet_control(idx); return;
  struct PlayerInfo *player;
  struct Packet *pckt;
  struct Camera *cam;
  unsigned long zoom_min,zoom_max;

  player = &(game.players[idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  cam = player->acamera;
  long inter_val;
  switch (cam->field_6)
  {
  case 2:
      inter_val = 2560000 / cam->field_17;
      break;
  case 5:
      inter_val = 12800000 / cam->field_17;
      break;
  default:
      inter_val = 256;
      break;
  }
  if (pckt->field_10 & 0x01)
    inter_val *= 3;

  if (pckt->control_flags & 0x04)
    view_set_camera_y_inertia(cam, -inter_val/4, -inter_val);
  if (pckt->control_flags & 0x08)
    view_set_camera_y_inertia(cam, inter_val/4, inter_val);
  if (pckt->control_flags & 0x10)
    view_set_camera_x_inertia(cam, -inter_val/4, -inter_val);
  if (pckt->control_flags & 0x20)
    view_set_camera_x_inertia(cam, inter_val/4, inter_val);
  if (pckt->control_flags & 0x02)
  {
    switch (cam->field_6)
    {
    case 2:
         view_set_camera_rotation_inertia(cam, 16, 64);
        break;
    case 5:
        cam->orient_a = (cam->orient_a + 512) & 0x7FF;
        break;
    }
  }
  if (pckt->control_flags & 0x01)
  {
    switch (cam->field_6)
    {
    case 2:
        view_set_camera_rotation_inertia(cam, -16, -64);
        break;
    case 5:
        cam->orient_a = (cam->orient_a - 512) & 0x7FF;
        break;
    }
  }
  zoom_min = scale_camera_zoom_to_screen(CAMERA_ZOOM_MIN);
  zoom_max = scale_camera_zoom_to_screen(CAMERA_ZOOM_MAX);
  if (pckt->control_flags & 0x40)
  {
    switch (cam->field_6)
    {
    case 2:
        view_zoom_camera_in(cam, zoom_max, zoom_min);
        update_camera_zoom_bounds(cam, zoom_max, zoom_min);
        break;
    default:
        view_zoom_camera_in(cam, zoom_max, zoom_min);
        break;
    }
  }
  if (pckt->control_flags & 0x80)
  {
    switch (cam->field_6)
    {
    case 2:
        view_zoom_camera_out(cam, zoom_max, zoom_min);
        update_camera_zoom_bounds(cam, zoom_max, zoom_min);
        break;
    default:
        view_zoom_camera_out(cam, zoom_max, zoom_min);
        break;
    }
  }
  process_dungeon_control_packet_clicks(idx);
  set_mouse_light(player);
}

void process_players_message_character(struct PlayerInfo *player)
{
  struct Packet *pcktd;
  struct PlayerInfo *playerd;
  char chr;
  int chpos;
  playerd = &(game.players[player->field_2B%PLAYERS_COUNT]);
  pcktd = &game.packets[playerd->packet_num%PACKETS_COUNT];
  if (pcktd->field_6 > 0)
  {
    chr = key_to_ascii(pcktd->field_6, pcktd->field_8);
    chpos = strlen(player->strfield_463);
    if (pcktd->field_6 == KC_BACK)
    {
      if (chpos>0)
        player->strfield_463[chpos-1] = '\0';
    } else
    if ((chr >= 'a') && (chr <= 'z') ||
        (chr >= 'A') && (chr <= 'Z') ||
        (chr >= '0') && (chr <= '9') || (chr == ' ')  || (chr == '!') ||
        (chr == '.'))
    {
      if (chpos < 63)
      {
        player->strfield_463[chpos] = toupper(chr);
        player->strfield_463[chpos+1] = '\0';
      }
    }
  }
}

void process_quit_packet(struct PlayerInfo *player, short complete_quit)
{
  //_DK_process_quit_packet(player,a2);
  struct PlayerInfo *swplyr;
  struct PlayerInfo *myplyr;
  short winning_quit;
  long plyr_count;
  int i;

  plyr_count = 0;
  myplyr = &game.players[my_player_number%PLAYERS_COUNT];
  if ((game.numfield_A & 0x01) != 0)
  {
    winning_quit = winning_player_quitting(player, &plyr_count);
    if (winning_quit)
    {
      // Set other players as losers
      for (i=0; i < PLAYERS_COUNT; i++)
      {
        swplyr = &(game.players[i]);
        if ((swplyr->field_0 & 0x01) != 0)
        {
          if (swplyr->field_2C == 1)
            if (swplyr->victory_state == VicS_Undecided)
              swplyr->victory_state = VicS_WonLevel;
        }
      }
    }

    if ((player == myplyr) || (frontend_should_all_players_quit()))
    {
      if ((!winning_quit) || (plyr_count <= 1))
        LbNetwork_Stop();
      else
        myplyr->field_3 |= 0x10u;
    } else
    {
      if (!winning_quit)
      {
        if (player->victory_state != VicS_Undecided)
        {
          player->field_0 &= 0xFE;
        } else
        {
          player->field_0 |= 0x40;
          toggle_computer_player(player->field_2B);
        }
        if (player == myplyr)
        {
          quit_game = 1;
          if (complete_quit)
            exit_keeper = 1;
        }
        return;
      } else
      if (plyr_count <= 1)
        LbNetwork_Stop();
      else
        myplyr->field_3 |= 0x10u;
    }
    quit_game = 1;
    if (complete_quit)
      exit_keeper = 1;
    if (frontend_should_all_players_quit())
    {
      for (i=0; i < PLAYERS_COUNT; i++)
      {
        swplyr = &(game.players[i]);
        if ((swplyr->field_0 & 0x01) != 0)
        {
          swplyr->field_0 &= 0xFEu;
          swplyr->field_6 |= 0x02u;
        }
      }
    } else
    {
      player->field_0 &= 0xFEu;
    }
    return;
  }
  player->field_0 &= 0xFEu;
  if (player == myplyr)
  {
    quit_game = 1;
    if (complete_quit)
      exit_keeper = 1;
  }
}

char process_players_global_packet_action(long plyridx)
{
  static const char *func_name="process_players_global_packet_action";
  //TODO: add commands from beta
  //return _DK_process_players_global_packet_action(plyridx);
  struct PlayerInfo *player;
  struct PlayerInfo *myplyr;
  struct Packet *pckt;
  struct Dungeon *dungeon;
  struct Thing *thing;
  struct Room *room;
  int i;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  player=&(game.players[plyridx%PLAYERS_COUNT]);
  pckt=&game.packets[player->packet_num%PACKETS_COUNT];
  switch (pckt->action)
  {
    case 1:
      myplyr=&(game.players[my_player_number%PLAYERS_COUNT]);
      if (my_player_number == plyridx)
      {
        turn_off_all_menus();
        frontend_save_continue_game(true);
        free_swipe_graphic();
      }
      player->field_6 |= 0x02;
      process_quit_packet(player, 0);
      return 1;
    case 3:
      myplyr=&(game.players[my_player_number%PLAYERS_COUNT]);
      if (my_player_number == plyridx)
      {
        turn_off_all_menus();
        frontend_save_continue_game(true);
      }
      player->field_6 |= 0x02;
      process_quit_packet(player, 1);
      return 1;
    case 4:
      return 1;
    case 5:
      if (my_player_number == plyridx)
      {
        turn_off_all_menus();
        free_swipe_graphic();
      }
      if ((game.numfield_A & 0x01) != 0)
      {
        process_quit_packet(player, 0);
        return 0;
      }
      switch (player->victory_state)
      {
      case VicS_WonLevel:
          complete_level(player);
          break;
      case VicS_LostLevel:
          lose_level(player);
          break;
      default:
          resign_level(player);
          break;
      }
      player->field_0 &= 0xFEu;
      if (my_player_number == plyridx)
      {
        frontend_save_continue_game(false);
      }
      return 0;
    case PckA_PlyrMsgBegin:
      player->field_0 |= 0x04;
      return 0;
    case PckA_PlyrMsgEnd:
      player->field_0 &= 0xFBu;
      if (player->strfield_463[0] != '\0')
        message_add(player->field_2B);
      memset(player->strfield_463, 0, 64);
      return 0;
    case PckA_ToggleLights:
      if (my_player_number == plyridx)
        light_set_lights_on(game.field_4614D == 0);
      return 1;
    case PckA_SwitchScrnRes:
      if (my_player_number == plyridx)
        switch_to_next_video_mode();
      return 1;
    case 22:
      process_pause_packet((game.numfield_C & 0x01) == 0, pckt->field_6);
      return 1;
    case 24:
      myplyr=&(game.players[my_player_number%PLAYERS_COUNT]);
      if (myplyr->field_2B == plyridx)
      {
        settings.video_cluedo_mode = pckt->field_6;
        save_settings();
      }
      player->field_4DA = pckt->field_6;
      return 0;
    case 25:
      myplyr=&(game.players[my_player_number%PLAYERS_COUNT]);
      if (myplyr->field_2B == player->field_2B)
      {
        change_engine_window_relative_size(pckt->field_6, pckt->field_8);
        centre_engine_window();
      }
      return 0;
    case 26:
      set_player_cameras_position(player, pckt->field_6 << 8, pckt->field_8 << 8);
      return 0;
    case PckA_SetGammaLevel:
      if (myplyr->field_2B == player->field_2B)
      {
        set_gamma(pckt->field_6, 1);
        save_settings();
      }
      return 0;
    case PckA_SetMinimapConf:
      player->minimap_zoom = pckt->field_6;
      return 0;
    case 29:
      player->cameras[2].orient_a = pckt->field_6;
      player->cameras[3].orient_a = pckt->field_6;
      player->cameras[0].orient_a = pckt->field_6;
      return 0;
    case PckA_SetPlyrState:
      set_player_state(player, pckt->field_6, pckt->field_8);
      return 0;
    case 37:
      set_engine_view(player, pckt->field_6);
      return 0;
    case 55:
      toggle_creature_tendencies(player, pckt->field_6);
      return 0;
    case 60:
//      game.???[my_player_number].cheat_mode = 1;
      show_onscreen_msg(2*game.num_fps, "Cheat mode activated by player %d", plyridx);
      return 1;
    case 61:
      //TODO: remake from beta
/*
      if (word_5E674A != 0)
        word_5E674A = 0;
      else
        word_5E674A = 15;
*/
      return 1;
    case 62:
      //TODO: remake from beta
      return 0;
    case PckA_CheatRevealMap:
      myplyr = &(game.players[my_player_number%PLAYERS_COUNT]);
      reveal_whole_map(myplyr);
      return 0;
    case 64:
      //TODO: remake from beta
      return 0;
    case 65:
      //TODO: remake from beta
      return 0;
    case 66:
      //TODO: remake from beta
      return 0;
    case 67:
      //TODO: remake from beta
      return 0;
    case 68:
      //TODO: remake from beta
      return 0;
    case 69:
      //TODO: remake from beta
      return 0;
    case 80:
      set_player_mode(player, pckt->field_6);
      return 0;
    case 81:
      myplyr=&(game.players[my_player_number%PLAYERS_COUNT]);
      set_player_cameras_position(player, pckt->field_6 << 8, pckt->field_8 << 8);
      player->cameras[2].orient_a = 0;
      player->cameras[3].orient_a = 0;
      player->cameras[0].orient_a = 0;
      if ((game.numfield_A & 0x01) || (lbDisplay.PhysicalScreenWidth > 320))
      {
        if (my_player_number == plyridx)
          toggle_status_menu((game.numfield_C & 0x40) != 0);
        set_player_mode(player, 1);
      } else
      {
        set_player_mode(player, 6);
      }
      return 0;
    case 82:
      process_pause_packet(pckt->field_6, pckt->field_8);
      return 1;
    case 83:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      event_move_player_towards_event(player, pckt->field_6);
      return 0;
    case 84:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      room = &game.rooms[pckt->field_6];
      player->field_E4 = room->field_8 << 8;
      player->field_E6 = room->field_9 << 8;
      set_player_instance(player, 16, 0);
      if (player->work_state == 2)
        set_player_state(player, 2, room->kind);
      return 0;
    case 85:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      thing = thing_get(pckt->field_6);
      player->field_E4 = thing->mappos.x.val;
      player->field_E6 = thing->mappos.y.val;
      set_player_instance(player, 16, 0);
      if ((player->work_state == 16) || (player->work_state == 18))
        set_player_state(player, 16, thing->model);
      return 0;
    case 86:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      thing = thing_get(pckt->field_6);
      player->field_E4 = thing->mappos.x.val;
      player->field_E6 = thing->mappos.y.val;
      set_player_instance(player, 16, 0);
      if ((player->work_state == 16) || (player->work_state == 18))
        set_player_state(player, 18, thing->model);
      return 0;
    case 87:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      player->field_E4 = pckt->field_6;
      player->field_E6 = pckt->field_8;
      set_player_instance(player, 16, 0);
      return 0;
    case 88:
      game.numfield_D ^= (game.numfield_D ^ (0x04 * ((game.numfield_D & 0x04) == 0))) & 0x04;
      return 0;
    case PckA_SpellCTADis:
      turn_off_call_to_arms(plyridx);
      return 0;
    case 90:
      if (game.dungeon[plyridx].field_63 < 8)
        place_thing_in_power_hand(thing_get(pckt->field_6), plyridx);
      return 0;
    case 91:
      dump_held_things_on_map(plyridx, pckt->field_6, pckt->field_8, 1);
      return 0;
    case 92:
      if (game.event[pckt->field_6].kind == 3)
      {
        turn_off_event_box_if_necessary(plyridx, pckt->field_6);
      } else
      {
        event_delete_event(plyridx, pckt->field_6);
      }
      return 0;
    case 97:
      magic_use_power_obey(plyridx);
      return 0;
    case 98:
      magic_use_power_armageddon(plyridx);
      return 0;
    case 99:
      turn_off_query(plyridx);
      return 0;
    case 104:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      battle_move_player_towards_battle(player, pckt->field_6);
      return 0;
    case 106:
      if (player->work_state == 15)
        turn_off_query(plyridx);
      dungeon = &(game.dungeon[plyridx]);
      switch (pckt->field_6)
      {
      case 5:
          if (dungeon->field_5D8)
          {
            struct Thing *thing;
            thing = thing_get(dungeon->field_5D8);
            player->field_E4 = thing->mappos.x.val;
            player->field_E6 = thing->mappos.y.val;
            set_player_instance(player, 16, 0);
          }
          break;
      case 6:
          if (dungeon->field_884)
          {
            player->field_E4 = ((unsigned long)dungeon->field_881) << 8;
            player->field_E6 = ((unsigned long)dungeon->field_882) << 8;
            set_player_instance(player, 16, 0);
          }
          break;
      }
      if ( spell_data[pckt->field_6].field_0 )
      {
        for (i=0; i<21; i++)
        {
          if (spell_data[i].field_4 == player->work_state)
          {
            set_player_state(player, spell_data[pckt->field_6].field_4, 0);
            break;
          }
        }
      }
      return 0;
    case PckA_PlyrFastMsg:
      //show_onscreen_msg(game.num_fps, "Message from player %d", plyridx);
      output_message(pckt->field_6+110, 0, 1);
      return 0;
    case 109:
      set_autopilot_type(plyridx, pckt->field_6);
      return 0;
    case 110:
      level_lost_go_first_person(plyridx);
      return 0;
    case 111:
      if (game.dungeon[plyridx].field_63)
      {
        thing = get_first_thing_in_power_hand(player);
        dump_held_things_on_map(plyridx, thing->mappos.x.stl.num, thing->mappos.y.stl.num, 1);
      }
      return 0;
    case PckA_SpellSOEDis:
      turn_off_sight_of_evil(plyridx);
      return 0;
    case 115:
      go_on_then_activate_the_event_box(plyridx, pckt->field_6);
      return 0;
    case 116:
      turn_off_event_box_if_necessary(plyridx, game.dungeon[plyridx].field_1173);
      game.dungeon[plyridx].field_1173 = 0;
      return 0;
    case 117:
      i = player->field_4D2 / 4;
      if (i > 8) i = 8;
      directly_cast_spell_on_thing(plyridx, pckt->field_6, pckt->field_8, i);
      return 0;
    case PckA_PlyrToggleAlly:
      toggle_ally_with_player(plyridx, pckt->field_6);
      return 0;
    case 119:
      if (player->acamera != NULL)
        player->field_4B5 = player->acamera->field_6;
      set_player_mode(player, pckt->field_6);
      return 0;
    case 120:
      set_player_mode(player, pckt->field_6);
      set_engine_view(player, player->field_4B5);
      return 0;
    default:
      return 0;
  }
}

void process_players_map_packet_control(long idx)
{
  static const char *func_name="process_players_map_packet_control";
  struct PlayerInfo *player;
  struct Packet *pckt;
  unsigned short x,y;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  player=&(game.players[idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  x = (3*pckt->pos_x - 450)/4 - 6;
  y = (3*pckt->pos_y - 168)/4 - 6;
  process_map_packet_clicks(idx);
  player->cameras[2].mappos.x.val = (x << 8) + 1920;
  player->cameras[2].mappos.y.val = (y << 8) + 1920;
  set_mouse_light(player);
#if (BFDEBUG_LEVEL > 8)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}

void process_map_packet_clicks(long idx)
{
  static const char *func_name="process_map_packet_clicks";
#if (BFDEBUG_LEVEL > 7)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  _DK_process_map_packet_clicks(idx);
#if (BFDEBUG_LEVEL > 8)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}

void process_players_packet(long idx)
{
  static const char *func_name="process_players_packet";
  struct PlayerInfo *player;
  struct Packet *pckt;
  player=&(game.players[idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Processing player %d packet of type %d.\n",func_name,idx,(int)pckt->action);
#endif
  player->field_4 = (pckt->field_10 & 0x20) >> 5;
  player->field_5 = (pckt->field_10 & 0x40) >> 6;
  if ( (player->field_0 & 0x04) && (pckt->action == PckA_PlyrMsgChar))
  {
     process_players_message_character(player);
  } else
  if ( !process_players_global_packet_action(idx) )
  {
      switch (player->view_type)
      {
      case PVT_DungeonTop:
        process_players_dungeon_control_packet_control(idx);
        process_players_dungeon_control_packet_action(idx);
        break;
      case PVT_CreatureContrl:
        process_players_creature_control_packet_control(idx);
        process_players_creature_control_packet_action(idx);
        break;
      case PVT_CreaturePasngr:
        process_players_creature_passenger_packet_action(idx);
        break;
      case PVT_MapScreen:
        process_players_map_packet_control(idx);
        break;
      default:
        break;
      }
  }
#if (BFDEBUG_LEVEL > 8)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}

void process_players_creature_passenger_packet_action(long idx)
{
  static const char *func_name="process_players_creature_passenger_packet_action";
  struct PlayerInfo *player;
  struct Packet *pckt;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  player=&(game.players[idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  if (pckt->action == 32)
  {
    player->field_43E = pckt->field_6;
    set_player_instance(player, 8, 0);
  }
#if (BFDEBUG_LEVEL > 8)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}

void process_players_dungeon_control_packet_action(long idx)
{
  static const char *func_name="process_players_dungeon_control_packet_action";
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  //_DK_process_players_dungeon_control_packet_action(idx); return;
  struct PlayerInfo *player;
  struct Packet *pckt;
  player = &(game.players[idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  switch (pckt->action)
  {
    case 41:
      magic_use_power_hold_audience(idx);
      break;
    case 93:
      activate_dungeon_special(thing_get(pckt->field_6), player);
      break;
    case 95:
      resurrect_creature(thing_get(pckt->field_6),
        (pckt->field_8) & 0x0F, (pckt->field_8 >> 4) & 0xFF, (pckt->field_8 >> 12) & 0x0F);
      break;
    case 96:
      transfer_creature(thing_get(pckt->field_6), thing_get(pckt->field_8), idx);
      break;
    case 107:
      toggle_computer_player(idx);
      break;
  }
}

void process_players_creature_control_packet_control(long idx)
{
  static const char *func_name="process_players_creature_control_packet_control";
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  _DK_process_players_creature_control_packet_control(idx);
}

void process_players_creature_control_packet_action(long idx)
{
  static const char *func_name="process_players_creature_control_packet_action";
  struct CreatureControl *cctrl;
  struct PlayerInfo *player;
  struct Thing *thing;
  struct Packet *pckt;
  long i,k;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  //_DK_process_players_creature_control_packet_action(idx); return;
  player = &(game.players[idx%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  switch (pckt->action)
  {
  case 33:
      player->field_43E = pckt->field_6;
      set_player_instance(player, 7, 0);
      break;
  case 39:
      thing = thing_get(player->field_2F);
      if (thing_is_invalid(thing))
        break;
      cctrl = creature_control_get_from_thing(thing);
      if ((cctrl == NULL) || (cctrl == game.persons.cctrl_lookup[0]))
        break;
      i = pckt->field_6;
      if (!instance_info[i].field_0)
      {
        cctrl->field_1E8 = i;
      } else
      if (cctrl->field_D2 == 0)
      {
        if (creature_instance_is_available(thing,i) && creature_instance_has_reset(thing, pckt->field_6))
        {
          i = pckt->field_6;
          k = get_human_controlled_creature_target(thing, instance_info[i].field_1D);
          set_creature_instance(thing, i, 1, k, 0);
          if (idx == my_player_number)
            instant_instance_selected(i);
        }
      }
      break;
  }
}

void open_packet_file_for_load(char *fname)
{
  static const char *func_name="open_packet_file_for_load";
  //_DK_open_packet_file_for_load(fname); return;
  strcpy(game.packet_fname, fname);
  game.packet_save_fp = LbFileOpen(game.packet_fname, Lb_FILE_MODE_READ_ONLY);
  if (game.packet_save_fp == -1)
  {
    error(func_name, 3835, "Cannot open keeper packet file for load");
    return;
  }
  LbFileRead(game.packet_save_fp, &game.packet_save_head, sizeof(struct PacketSaveHead));
  game.packet_file_pos = PACKET_START_POS;
  game.field_149F30 = (LbFileLengthRnc(fname) - PACKET_START_POS) / PACKET_TURN_SIZE;
  if ((game.packet_checksum) && (!game.packet_save_head.chksum))
  {
      LbWarnLog("PacketSave checksum not available, checking disabled.\n");
      game.packet_checksum = false;
  }
  if (game.numfield_149F3A == -1)
  {
    game.numfield_149F3A = 0;
    game.numfield_149F3E = game.field_149F30 + 1;
  }
  game.packet_fopened = 1;
}

void post_init_packets(void)
{
  if ((game.packet_load_enable) && (game.numfield_149F47))
  {
      open_packet_file_for_load(game.packet_fname);
      game.pckt_gameturn = 0;
  }
  clear_packets();
}

short save_packets(void)
{
  static const char *func_name="save_packets";
  const int turn_data_size = PACKET_TURN_SIZE;
  unsigned char pckt_buf[PACKET_TURN_SIZE+4];
  TbBigChecksum chksum;
  int i;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  if (game.packet_checksum)
    chksum = get_packet_save_checksum();
  else
    chksum = 0;
  LbFileSeek(game.packet_save_fp, 0, Lb_FILE_SEEK_END);
  // Prepare data in the buffer
  for (i=0; i<NET_PLAYERS_COUNT; i++)
    LbMemoryCopy(&pckt_buf[i*sizeof(struct Packet)], &game.packets[i], sizeof(struct Packet));
  LbMemoryCopy(&pckt_buf[NET_PLAYERS_COUNT*sizeof(struct Packet)], &chksum, sizeof(TbBigChecksum));
  // Write buffer into file
  if (LbFileWrite(game.packet_save_fp, &pckt_buf, turn_data_size) != turn_data_size)
  {
    error(func_name, 3818, "Packet file write error");
  }
  if ( !LbFileFlush(game.packet_save_fp) )
  {
    error(func_name, 3821, "Unable to flush PacketSave File");
    return false;
  }
  return true;
}

void close_packet_file(void)
{
  if ( game.packet_fopened )
  {
    LbFileClose(game.packet_save_fp);
    game.packet_fopened = 0;
    game.packet_save_fp = 0;
  }
}

void write_debug_packets(void)
{
  FILE *file;
  file = fopen("keeperd.pck", "w");
  fwrite(game.packets, 1, sizeof(struct Packet)*PACKETS_COUNT, file);
  fflush(file);
  fclose(file);
}

void process_packets(void)
{
  static const char *func_name="process_packets";
  //_DK_process_packets();return;

  int i,j,k;
  struct Packet *pckt;
  struct PlayerInfo *player;
#if (BFDEBUG_LEVEL > 5)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  // Do the network data exchange
  lbDisplay.DrawColour = colours[15][15][15];
  // Exchange packets with the network
  if (game.flagfield_14EA4A != 2)
  {
    player = &(game.players[my_player_number%PLAYERS_COUNT]);
    j=0;
    for (i=0; i<4; i++)
    {
      if (net_player_info[i].field_20 != 0)
        j++;
    }
    if ( !game.packet_load_enable || game.numfield_149F47 )
    {
      pckt = &game.packets[player->packet_num%PACKETS_COUNT];
      if (LbNetwork_Exchange(pckt) != 0)
      {
        error(func_name, 426, "LbNetwork_Exchange failed");
      }
    }
    k=0;
    for (i=0; i<4; i++)
    {
      if (net_player_info[i].field_20 != 0)
        k++;
    }
    if (j != k)
    {
      for (i=0; i<4; i++)
      {
        player=&(game.players[i]);
        if (net_player_info[player->packet_num%NET_PLAYERS_COUNT].field_20 == 0)
        {
          player->field_0 |= 0x40;
          toggle_computer_player(i);
        }
      }
    }
  }
  // Setting checksum problem flags
  switch (checksums_different())
  {
  case 1:
    game.numfield_A |= 0x02;
    game.numfield_A &= 0xFB;
    break;
  case 2:
    game.numfield_A |= 0x04;
    game.numfield_A &= 0xFD;
    break;
  case 3:
    game.numfield_A |= 0x04;
    game.numfield_A |= 0x02;
    break;
  default:
    game.numfield_A &= 0xFD;
    game.numfield_A &= 0xFB;
    break;
  }
  // Write packets into file, if requested
  if ((game.packet_save_enable) && (game.packet_fopened))
    save_packets();
//Debug code, to find packet errors
//write_debug_packets();
  // Process the packets
  for (i=0; i<PACKETS_COUNT; i++)
  {
    player=&(game.players[i]);
    if (((player->field_0 & 0x01) != 0) && ((player->field_0 & 0x40) == 0))
      process_players_packet(i);
  }
  // Clear all packets
  for (i=0; i<PACKETS_COUNT; i++)
    memset(&game.packets[i], 0, sizeof(struct Packet));
  if ((game.numfield_A & 0x02) || (game.numfield_A & 0x04))
  {
  #if (BFDEBUG_LEVEL > 0)
    LbSyncLog("%s: Resyncing.\n",func_name);
  #endif
    resync_game();
  }
#if (BFDEBUG_LEVEL > 7)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}

void process_frontend_packets(void)
{
  static const char *func_name="process_frontend_packets";
  struct ScreenPacket *nspckt;
  struct PlayerInfo *player;
  long i,k;
  unsigned short c;
  for (i=0; i < NET_PLAYERS_COUNT; i++)
  {
    net_screen_packet[i].field_4 &= 0xFE;
  }
  nspckt = &net_screen_packet[my_player_number];
  set_flag_byte(&nspckt->field_4, 0x01, true);
  nspckt->field_5 = frontend_alliances;
  set_flag_byte(&nspckt->field_4, 0x01, true);
  nspckt->field_4 ^= (nspckt->field_4 ^ (fe_computer_players << 1) & 0x06);
  nspckt->field_6 = VersionMajor;
  nspckt->field_8 = VersionMinor;
  if (LbNetwork_Exchange(nspckt))
    error(func_name, 2559, "LbNetwork_Exchange failed");
  if (frontend_should_all_players_quit())
  {
    i = frontnet_number_of_players_in_session();
    if (players_currently_in_session < i)
    {
      players_currently_in_session = i;
    }
    if (players_currently_in_session > i)
    {
      if (frontend_menu_state == 5)
      {
        if (LbNetwork_Stop())
        {
          error(func_name, 2584, "LbNetwork_Stop() failed");
          return;
        }
        frontend_set_state(1);
      } else
      if (frontend_menu_state == 6)
      {
        if (LbNetwork_Stop())
        {
          error(func_name, 2593, "LbNetwork_Stop() failed");
          return;
        }
        if (setup_network_service(net_service_index_selected))
        {
          frontend_set_state(5);
        } else
        {
          frontend_set_state(1);
        }
      }
    }
  }
  for (i=0; i < NET_PLAYERS_COUNT; i++)
  {
    nspckt = &net_screen_packet[i];
    player = &(game.players[i]);
    if ((nspckt->field_4 & 0x01) != 0)
    {
      switch (nspckt->field_4 >> 3)
      {
      case 2:
        add_message(i, (char *)&nspckt->field_A);
        break;
      case 3:
        if (!validate_versions())
        {
          versions_different_error();
          break;
        }
        fe_network_active = 1;
        frontend_set_state(24);
        break;
      case 4:
        frontend_set_alliance(nspckt->field_A, nspckt->field_B);
        break;
      case 7:
        fe_computer_players = nspckt->field_A;
        break;
      case 8:
        k = strlen(player->strfield_463);
        if (nspckt->field_A == KC_BACK)
        {
          if (k > 0)
          {
            k--;
            player->strfield_463[k] = '\0';
          }
        } else
        if (nspckt->field_A == KC_RETURN)
        {
          if (k > 0)
          {
            add_message(i, player->strfield_463);
            k = 0;
            player->strfield_463[k] = '\0';
          }
        } else
        {
          c = key_to_ascii(nspckt->field_A, nspckt->field_B);
          if ((c != 0) && (frontend_font_char_width(1,c) > 1) && (k < 62))
          {
            player->strfield_463[k] = c;
            k++;
            player->strfield_463[k] = '\0';
          }
        }
        if (frontend_font_string_width(1,player->strfield_463) >= 420)
        {
          if (k > 0)
          {
            k--;
            player->strfield_463[k] = '\0';
          }
        }
        break;
      default:
        break;
      }
      if (frontend_alliances == -1)
      {
        if (nspckt->field_5 != -1)
          frontend_alliances = nspckt->field_5;
      }
      if (fe_computer_players == 2)
      {
        k = ((nspckt->field_4 & 0x06) >> 1);
        if (k != 2)
          fe_computer_players = k;
      }
      player->field_4E7 = nspckt->field_8 + (nspckt->field_6 << 8);
    }
    nspckt->field_4 &= 0x07;
  }
  if (frontend_alliances == -1)
    frontend_alliances = 0;
  for (i=0; i < NET_PLAYERS_COUNT; i++)
  {
    nspckt = &net_screen_packet[i];
    if ((nspckt->field_4 & 0x01) == 0)
    {
      if (frontend_is_player_allied(my_player_number, i))
        frontend_set_alliance(my_player_number, i);
    }
  }
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif