/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file front_input.c
 *     Front-end user keyboard and mouse input.
 * @par Purpose:
 *     Reacts on events by creating packets or directly modifying various parameters.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     20 Jan 2009 - 30 Jan 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "front_input.h"

#include "globals.h"
#include "bflib_basics.h"

#include "bflib_video.h"
#include "bflib_keybrd.h"
#include "bflib_mouse.h"
#include "bflib_sprfnt.h"
#include "bflib_datetm.h"
#include "bflib_fileio.h"
#include "bflib_memory.h"
#include "bflib_network.h"

#include "kjm_input.h"
#include "frontend.h"
#include "scrcapt.h"
#include "player_instances.h"
#include "gui_tooltips.h"
#include "keeperfx.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

unsigned short const zoom_key_room_order[] =
    {2, 3, 14, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 15, 0,};

/******************************************************************************/
DLLIMPORT void _DK_input(void);
DLLIMPORT char _DK_game_is_busy_doing_gui_string_input(void);
DLLIMPORT void _DK_get_packet_control_mouse_clicks(void);
DLLIMPORT int _DK_is_game_key_pressed(long, long *, unsigned int);
DLLIMPORT long _DK_get_inputs(void);
DLLIMPORT void _DK_get_isometric_view_nonaction_inputs(void);
DLLIMPORT void _DK_get_overhead_view_nonaction_inputs(void);
DLLIMPORT void _DK_get_front_view_nonaction_inputs(void);
DLLIMPORT long _DK_get_small_map_inputs(long x, long y, long zoom);
DLLIMPORT void _DK_get_level_lost_inputs(void);
DLLIMPORT long _DK_get_global_inputs(void);
DLLIMPORT long _DK_get_dungeon_control_action_inputs(void);
DLLIMPORT void _DK_get_dungeon_control_nonaction_inputs(void);
DLLIMPORT void _DK_get_creature_control_nonaction_inputs(void);
DLLIMPORT void _DK_get_map_nonaction_inputs(void);
DLLIMPORT long _DK_get_bookmark_inputs(void);
/******************************************************************************/
void get_dungeon_control_nonaction_inputs(void);
void get_creature_control_nonaction_inputs(void);
short zoom_shortcuts(void);
short get_bookmark_inputs(void);
/******************************************************************************/
short game_is_busy_doing_gui_string_input(void)
{
  //return _DK_game_is_busy_doing_gui_string_input();
  return (input_button != NULL);
}

short current_view_supports_status_menu()
{
  struct PlayerInfo *player;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  return (player->view_type != PVT_MapScreen);
}

int is_game_key_pressed(long key_id, long *val, short ignore_mods)
{
  //return _DK_is_game_key_pressed(key_id, val, ignore_mods);
  int result;
  int i;
  if ((key_id < 0) || (key_id >= GAME_KEYS_COUNT))
    return 0;
  if (val !=NULL)
  {
    *val = settings.kbkeys[key_id].code;
  }
  if ((key_id == 4) || (key_id == 5) || (key_id == 27) || (key_id == 28))
  {
    i = settings.kbkeys[key_id].code;
    switch (i)
    {
      case KC_LSHIFT:
      case KC_RSHIFT:
        result = key_modifiers & KM_SHIFT;
        break;
      case KC_LCONTROL:
      case KC_RCONTROL:
        result = key_modifiers & KM_CONTROL;
        break;
      case KC_LALT:
      case KC_RALT:
        result = key_modifiers & KM_ALT;
        break;
      default:
        result = lbKeyOn[i];
        break;
    }
  } else
  {
    if ((ignore_mods) || (key_modifiers == settings.kbkeys[key_id].mods))
      result = lbKeyOn[settings.kbkeys[key_id].code];
    else
      result = 0;
  }
  return result;
}

/*
 *  Reacts on a keystoke by sending text message packets.
 */
short get_players_message_inputs(void)
{
  struct PlayerInfo *player;
  int msg_width;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if (is_key_pressed(KC_RETURN,KM_NONE))
  {
      set_players_packet_action(player, PckA_PlyrMsgEnd, 0, 0, 0, 0);
      clear_key_pressed(KC_RETURN);
      return true;
  }
  lbFontPtr = winfont;
  msg_width = pixel_size * LbTextStringWidth(player->strfield_463);
  if ( (is_key_pressed(KC_BACK,KM_DONTCARE)) || (msg_width < 450) )
  {
      set_players_packet_action(player,PckA_PlyrMsgChar,lbInkey,key_modifiers,0,0);
      clear_key_pressed(lbInkey);
      return true;
  }
  return false;
}

/*
 * Captures the screen to make a gameplay movie or screenshot image.
 * @return Returns true if packet was created, false otherwise.
 */
short get_screen_capture_inputs(void)
{
  if (is_key_pressed(KC_M,KM_SHIFT))
  {
    if ((game.numfield_A & 0x08) != 0)
      movie_record_stop();
    else
      movie_record_start();
    clear_key_pressed(KC_M);
  }
  if (is_key_pressed(KC_C,KM_SHIFT))
  {
    game.numfield_A |= 0x10;
    clear_key_pressed(KC_C);
  }
  return false;
}

void clip_frame_skip(void)
{
  if (game.frame_skip > 512)
    game.frame_skip = 512;
  if (game.frame_skip < 0)
    game.frame_skip = 0;
}

/*
 * Handles game speed control inputs.
 * @return Returns true if packet was created, false otherwise.
 */
short get_speed_control_inputs(void)
{
  if (is_key_pressed(KC_ADD,KM_CONTROL))
  {
      if (game.frame_skip < 2)
        game.frame_skip ++;
      else
      if (game.frame_skip < 16)
        game.frame_skip += 2;
      else
        game.frame_skip += (game.frame_skip/3);
      clip_frame_skip();
      show_onscreen_msg(game.num_fps+game.frame_skip, "Frame skip %d",game.frame_skip);
      clear_key_pressed(KC_ADD);
  }
  if (is_key_pressed(KC_SUBTRACT,KM_CONTROL))
  {
      if (game.frame_skip <= 2)
        game.frame_skip --;
      else
      if (game.frame_skip <= 16)
        game.frame_skip -= 2;
      else
        game.frame_skip -= (game.frame_skip/4);
      clip_frame_skip();
      show_onscreen_msg(game.num_fps+game.frame_skip, "Frame skip %d",game.frame_skip);
      clear_key_pressed(KC_SUBTRACT);
  }
  return false;
}

/*
 * Handles control inputs in PacketLoad mode.
 */
short get_packet_load_game_control_inputs(void)
{
  if (lbKeyOn[KC_LALT] && lbKeyOn[KC_X])
  {
    clear_key_pressed(KC_X);
    if ((game.numfield_A & 0x01) != 0)
      LbNetwork_Stop();
    quit_game = 1;
    exit_keeper = 1;
    return true;
  }
  if (is_key_pressed(KC_T,KM_ALT))
  {
    clear_key_pressed(KC_T);
    close_packet_file();
    game.packet_load_enable = false;
    game.packet_save_enable = false;
    show_onscreen_msg(2*game.num_fps, "Packet mode disabled");
    set_gui_visible(true);
    return true;
  }
  return false;
}

long get_small_map_inputs(long x, long y, long zoom)
{
  static const char *func_name="get_small_map_inputs";
#if (BFDEBUG_LEVEL > 7)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  //return _DK_get_small_map_inputs(x, y, zoom);
  long curr_mx,curr_my;
  short result;
  result = 0;
  curr_mx = GetMouseX();
  curr_my = GetMouseY();
  dummy_x = curr_mx;
  dummy_y = curr_my;
  dummy = 1;
  if (!grabbed_small_map)
    game.small_map_state = 0;
  if (((game.numfield_C & 0x20) != 0) && (mouse_is_over_small_map(x,y) || grabbed_small_map))
  {
    if (left_button_clicked)
    {
      clicked_on_small_map = 1;
      left_button_clicked = 0;
    }
    if ( do_left_map_click(x, y, curr_mx, curr_my, zoom)
      || do_right_map_click(x, y, curr_mx, curr_my, zoom)
      || do_left_map_drag(x, y, curr_mx, curr_my, zoom) )
      result = 1;
  } else
  {
    clicked_on_small_map = 0;
  }
  if (grabbed_small_map)
  {
    LbMouseSetPosition((MyScreenWidth/pixel_size) >> 1, (MyScreenHeight/pixel_size) >> 1);
  }
  old_mx = curr_mx;
  old_my = curr_my;
  if (grabbed_small_map)
    game.small_map_state = 2;
#if (BFDEBUG_LEVEL > 8)
    LbSyncLog("%s: Finished\n",func_name);
#endif
  return result;
}

short get_bookmark_inputs(void)
{
  //return _DK_get_bookmark_inputs();
  struct Bookmark *bmark;
  struct PlayerInfo *player;
  int kcode;
  int i;
  player=&(game.players[my_player_number%PLAYERS_COUNT]);
  for (i=0; i < BOOKMARKS_COUNT; i++)
  {
    bmark = &game.bookmark[i];
    kcode = KC_1+i;
    // Store bookmark check
    if (is_key_pressed(kcode, KM_CONTROL))
    {
      clear_key_pressed(kcode);
      if (player->acamera != NULL)
      {
        bmark->x = player->acamera->mappos.x.stl.num;
        bmark->y = player->acamera->mappos.y.stl.num;
        bmark->flags |= 0x01;
        show_onscreen_msg(game.num_fps, "Bookmark %d stored",i+1);
      }
      return true;
    }
    // Load bookmark check
    if (is_key_pressed(kcode, KM_SHIFT))
    {
      clear_key_pressed(kcode);
      if ((bmark->flags & 0x01) != 0)
      {
        set_players_packet_action(player, PckA_BookmarkLoad, bmark->x, bmark->y, 0, 0);
        return true;
      }
    }
  }
  return false;
}

short zoom_shortcuts(void)
{
  long val;
  int i;
  for (i=0; i <= ZOOM_KEY_ROOMS_COUNT; i++)
  {
    if (is_game_key_pressed(i+10, &val, 0))
    {
      clear_key_pressed(val);
      go_to_my_next_room_of_type(zoom_key_room_order[i]);
      return true;
    }
  }
  return false;
}

/*
 * Handles minimap control inputs.
 * @return Returns true if packet was created, false otherwise.
 */
short get_minimap_control_inputs(void)
{
  struct PlayerInfo *player;
  short packet_made;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  packet_made = false;
  if (is_key_pressed(KC_SUBTRACT,KM_NONE))
  {
      if ( player->minimap_zoom < 0x0800 )
      {
        set_players_packet_action(player, PckA_SetMinimapConf, 2 * player->minimap_zoom, 0, 0, 0);
        packet_made = true;
      }
      clear_key_pressed(KC_SUBTRACT);
      if (packet_made) return true;
  }
  if (is_key_pressed(KC_ADD,KM_NONE))
  {
      if ( player->minimap_zoom > 0x0080 )
      {
        set_players_packet_action(player, PckA_SetMinimapConf, player->minimap_zoom >> 1, 0, 0, 0);
        packet_made = true;
      }
      clear_key_pressed(KC_ADD);
      if (packet_made) return true;
  }
  return false;
}

/*
 * Handles screen control inputs.
 * @return Returns true if packet was created, false otherwise.
 */
short get_screen_control_inputs(void)
{
  struct PlayerInfo *player;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  short packet_made;
  packet_made = false;
  if (is_key_pressed(KC_R,KM_ALT))
  {
      set_players_packet_action(player, PckA_SwitchScrnRes, 0, 0, 0, 0);
      packet_made = true;
      clear_key_pressed(KC_R);
      if (packet_made) return true;
  }
  return false;
}

short get_global_inputs(void)
{
  struct PlayerInfo *player;
  if (game_is_busy_doing_gui_string_input())
    return false;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  long keycode;
  if ((player->field_0 & 0x04) != 0)
  {
    get_players_message_inputs();
    return true;
  }
  if ((player->view_type == PVT_DungeonTop) && ((game.numfield_A & 0x01) != 0))
  {
      if (is_key_pressed(KC_RETURN,KM_NONE))
      {
        set_players_packet_action(player, PckA_PlyrMsgBegin, 0, 0, 0, 0);
        clear_key_pressed(KC_RETURN);
        return true;
      }
  }
  // Code for debugging purposes
  if ( is_key_pressed(KC_D,KM_ALT) )
  {
    LbJustLog("REPORT for gameturn %d\n",game.play_gameturn);
    // Timing report
    LbJustLog("Now time is %d, last loop time was %d, clock is %d, requested fps is %d\n",LbTimerClock(),last_loop_time,clock(),game.num_fps);
    test_variable = !test_variable;
  }

  int idx;
  for (idx=KC_F1;idx<=KC_F8;idx++)
  {
      if ( is_key_pressed(idx,KM_ALT) )
      {
        set_players_packet_action(player, PckA_PlyrFastMsg, idx-KC_F1, 0, 0, 0);
        clear_key_pressed(idx);
        return true;
      }
  }
  if ((player->instance_num != 14) && (player->instance_num != 15) && (!game_is_busy_doing_gui_string_input()))
  {
      if ( is_game_key_pressed(30, &keycode, 0) )
      {
        set_packet_pause_toggle();
        clear_key_pressed(keycode);
        return true;
      }
  }
  if ((game.numfield_C & 0x01) != 0)
      return true;
  if (get_speed_control_inputs())
      return true;
  if (get_minimap_control_inputs())
      return true;
  if (get_screen_control_inputs())
      return true;
  if (get_screen_capture_inputs())
      return true;
  if (is_key_pressed(KC_SPACE,KM_NONE))
  {
      if (player->victory_state != VicS_Undecided)
      {
        set_players_packet_action(player, PckA_FinishGame, 0, 0, 0, 0);
        clear_key_pressed(KC_SPACE);
        return true;
      }
  }
  if ( is_game_key_pressed(29, &keycode, 0) )
  {
      set_players_packet_action(player, 111, 0, 0, 0, 0);
      clear_key_pressed(keycode);
  }
  return false;
}

short get_level_lost_inputs(void)
{
  static const char *func_name="get_level_lost_inputs";
//  _DK_get_level_lost_inputs();
  struct PlayerInfo *player;
  long keycode;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  player=&(game.players[my_player_number%PLAYERS_COUNT]);
  if (player->field_0 & 0x04)
  {
    get_players_message_inputs();
    return true;
  }
  if ((game.numfield_A & 0x01) != 0)
  {
    if (is_key_pressed(KC_RETURN,KM_NONE))
    {
      set_players_packet_action(player, 13, 0,0,0,0);
      clear_key_pressed(KC_RETURN);
      return true;
    }
  }
  if (get_speed_control_inputs())
      return true;
  if (get_minimap_control_inputs())
      return true;
  if (get_screen_control_inputs())
      return true;
  if (get_screen_capture_inputs())
      return true;
  if (is_key_pressed(KC_SPACE,KM_NONE))
  {
    set_players_packet_action(player, PckA_FinishGame, 0,0,0,0);
    clear_key_pressed(KC_SPACE);
  }
  if (player->view_type == 4)
  {
    int screen_x = GetMouseX() - 150;
    int screen_y = GetMouseY() - 56;
    if ( is_game_key_pressed(31, &keycode, 0) )
    {
      lbKeyOn[keycode] = 0;
      zoom_from_map();
    } else
    if ( right_button_released )
    {
      right_button_released = 0;
      zoom_from_map();
    } else
    if ( left_button_released )
    {
        int actn_x = 3*screen_x/4 + 1;
        int actn_y = 3*screen_y/4 + 1;
        if  ((actn_x >= 0) && (actn_x < map_subtiles_x) && (actn_y >= 0) && (actn_y < map_subtiles_y))
        {
          set_players_packet_action(player, 81, actn_x,actn_y,0,0);
          left_button_released = 0;
        }
    }
  } else
  if (player->view_type == PVT_DungeonTop)
  {
    if (is_key_pressed(KC_TAB,KM_DONTCARE))
    {
        if ((player->field_37 == 2) || (player->field_37 == 5))
        {
          clear_key_pressed(KC_TAB);
          toggle_gui();
        }
    } else
    if ( is_game_key_pressed(31, &keycode, 0) )
    {
      lbKeyOn[keycode] = 0;
      if (player->field_37 != 7)
      {
        turn_off_all_window_menus();
        game.numfield_C = (game.numfield_C ^ (unsigned __int8)(2 * game.numfield_C)) & 0x40 ^ game.numfield_C;
        if ((game.numfield_A & 0x01) || (lbDisplay.PhysicalScreenWidth > 320))
        {
              if (toggle_status_menu(0))
                set_flag_byte(&game.numfield_C,0x40,true);
              else
                set_flag_byte(&game.numfield_C,0x40,false);
              set_players_packet_action(player, 119, 4,0,0,0);
        } else
        {
              set_players_packet_action(player, 80, 5,0,0,0);
        }
        turn_off_roaming_menus();
      }
    }
  }
  if (is_key_pressed(KC_ESCAPE,KM_DONTCARE))
  {
    clear_key_pressed(KC_ESCAPE);
    if ( a_menu_window_is_active() )
      turn_off_all_window_menus();
    else
      turn_on_menu(GMnu_OPTIONS);
  }
  struct Thing *thing;
  short inp_done=false;
  switch (player->view_type)
  {
    case PVT_DungeonTop:
      inp_done = menu_is_active(GMnu_SPELL_LOST);
      if ( !inp_done )
      {
        if ((game.numfield_C & 0x20) != 0)
        {
          initialise_tab_tags_and_menu(3);
          turn_off_all_panel_menus();
          turn_on_menu(38);
        }
      }
      inp_done = get_gui_inputs(GMnu_MAIN);
      if ( !inp_done )
      {
        if (player->work_state == 15)
        {
          set_player_instance(player, 10, 0);
        } else
        {
          inp_done=get_small_map_inputs(player->mouse_x, player->mouse_y, player->minimap_zoom / (3-pixel_size));
          if ( !inp_done )
            get_bookmark_inputs();
          get_dungeon_control_nonaction_inputs();
        }
      }
      break;
    case PVT_CreatureContrl:
      thing = thing_get(player->field_2F);
      if (thing->class_id == 5)
      {
        struct CreatureControl *cctrl;
        cctrl = creature_control_get_from_thing(thing);
        if ((cctrl->field_2 & 0x02) == 0)
          set_players_packet_action(player, 33, player->field_2F,0,0,0);
      } else
      {
        set_players_packet_action(player, 33, player->field_2F,0,0,0);
      }
      break;
    case PVT_CreaturePasngr:
      set_players_packet_action(player, 32, player->field_2F,0,0,0);
      break;
    case PVT_MapScreen:
      if ( menu_is_active(GMnu_SPELL_LOST) )
      {
        if ((game.numfield_C & 0x20) != 0)
          turn_off_menu(GMnu_SPELL_LOST);
      }
      break;
    default:
      return false;
  }
}

short get_status_panel_keyboard_action_inputs(void)
{
  if (is_key_pressed(KC_1, KM_NONE))
  {
    clear_key_pressed(KC_1);
    fake_button_click(1);
  }
  if (is_key_pressed(KC_2, KM_NONE))
  {
    clear_key_pressed(KC_2);
    fake_button_click(2);
  }
  if (is_key_pressed(KC_3, KM_NONE))
  {
    clear_key_pressed(KC_3);
    fake_button_click(3);
  }
  if (is_key_pressed(KC_4, KM_NONE))
  {
    clear_key_pressed(KC_4);
    fake_button_click(4);
  }
  if (is_key_pressed(KC_5, KM_NONE))
  {
    clear_key_pressed(KC_5);
    fake_button_click(5);
  }
  return false;
}

long get_dungeon_control_action_inputs(void)
{
  //return _DK_get_dungeon_control_action_inputs();
  struct PlayerInfo *player;
  long val;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if (get_players_packet_action(player) != PckA_None)
    return 1;
  if (pixel_size < 3)
      val = (player->minimap_zoom) / (3-pixel_size);
  else
      val = player->minimap_zoom;
  if (get_small_map_inputs(player->mouse_x, player->mouse_y, val))
    return 1;

  if (get_bookmark_inputs())
    return 1;

  if (is_key_pressed(KC_F8, KM_DONTCARE))
  {
    clear_key_pressed(KC_F8);
    toggle_tooltips();
  }
  if (is_key_pressed(KC_NUMPADENTER,KM_NONE))
  {
    if (toggle_main_cheat_menu())
      clear_key_pressed(KC_NUMPADENTER);
  }
  if (is_key_pressed(KC_TAB, KM_DONTCARE))
  {
    if ((player->field_37 == 2) || (player->field_37 == 5))
    {
      clear_key_pressed(KC_TAB);
      toggle_gui();
    }
  }

  if (is_game_key_pressed(25, &val, 0))
  {
    clear_key_pressed(val);
    zoom_to_fight(player->field_2B);
    return 1;
  }
  if ( is_game_key_pressed(26, &val, 0) )
  {
    clear_key_pressed(val);
    zoom_to_next_annoyed_creature();
    return 1;
  }
  if (zoom_shortcuts())
    return 1;
  if (is_game_key_pressed(31, &val, 0))
  {
    clear_key_pressed(val);
    if ((player->field_37 != 7) && (game.small_map_state != 2))
    {
      turn_off_all_window_menus();
      zoom_to_map();
    }
    return 1;
  }
  if (is_key_pressed(KC_F, KM_ALT))
  {
    clear_key_pressed(KC_F);
    toggle_hero_health_flowers();
  }
  get_status_panel_keyboard_action_inputs();
  return 0;
}

short get_creature_passenger_action_inputs(void)
{
  struct PlayerInfo *player;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if (get_players_packet_action(player) != PckA_None)
    return 1;
  if ( ((game.numfield_C & 0x01) == 0) || ((game.numfield_C & 0x80) != 0))
      get_gui_inputs(1);
  if ( !player->field_2F )
    return false;
  if (right_button_released)
  {
    set_players_packet_action(player, PckA_PasngrCtrlExit, player->field_2F,0,0,0);
    return true;
  }
  struct Thing *thing;
  thing = thing_get(player->field_2F);
  if ((player->field_31 != thing->field_9) || ((thing->field_0 & 1)==0) )
  {
    set_players_packet_action(player, PckA_PasngrCtrlExit, player->field_2F,0,0,0);
    return true;
  }
  if (is_key_pressed(KC_TAB,KM_NONE))
  {
    clear_key_pressed(KC_TAB);
    toggle_gui_overlay_map();
  }
  return false;
}

short get_creature_control_action_inputs(void)
{
  static const char *func_name="get_creature_control_action_inputs";
  struct PlayerInfo *player;
  long keycode;
#if (BFDEBUG_LEVEL > 6)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if (get_players_packet_action(player) != PckA_None)
    return 1;
  if ( ((game.numfield_C & 0x01)==0) || (game.numfield_C & 0x80) )
    get_gui_inputs(1);
  if (is_key_pressed(KC_NUMPADENTER,KM_DONTCARE))
  {
      if (toggle_instance_cheat_menu())
        clear_key_pressed(KC_NUMPADENTER);
  }
  if (is_key_pressed(KC_F12,KM_DONTCARE))
  {
      if (toggle_creature_cheat_menu())
        clear_key_pressed(KC_F12);
  }

  if ( player->field_2F )
  {
    short make_packet = right_button_released;
    if (!make_packet)
    {
      struct Thing *thing;
      thing = thing_get(player->field_2F);
      if ( (player->field_31 != thing->field_9) || ((thing->field_0 & 0x01) == 0)
         || (thing->field_7 == 67) )
        make_packet = true;
    }
    if (make_packet)
    {
      right_button_released = 0;
      set_players_packet_action(player, 33, player->field_2F,0,0,0);
    }
  }
  if ( is_key_pressed(KC_TAB,KM_NONE) )
  {
    clear_key_pressed(KC_TAB);
    toggle_gui();
  }
  int numkey;
  numkey = -1;
  {
    for (keycode=KC_1;keycode<=KC_0;keycode++)
    {
      if ( is_key_pressed(keycode,KM_NONE) )
      {
        clear_key_pressed(keycode);
        numkey = keycode-2;
        break;
      }
    }
  }
  if (numkey != -1)
  {
    int idx;
    int instnce;
    int num_avail;
    num_avail = 0;
    for (idx=0; idx < 10; idx++)
    {
      struct Thing *thing;
      thing = thing_get(player->field_2F);
      instnce = game.creature_stats[thing->model%CREATURE_TYPES_COUNT].instance_spell[idx];
      if ( creature_instance_is_available(thing,instnce) )
      {
        if ( numkey == num_avail )
        {
          set_players_packet_action(player, 39, instnce,0,0,0);
          break;
        }
        num_avail++;
      }
    }
  }
  return false;
}

void get_packet_control_mouse_clicks(void)
{
  static const char *func_name="get_packet_control_mouse_clicks";
  struct PlayerInfo *player;
#if (BFDEBUG_LEVEL > 8)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  //_DK_get_packet_control_mouse_clicks(); return;
  if ((game.numfield_C & 0x01) == 0)
  {
    player=&(game.players[my_player_number%PLAYERS_COUNT]);
    if (left_button_held)
    {
      set_players_packet_control(player, PCtr_LBtnHeld);
    }
    if ( right_button_held )
    {
      set_players_packet_control(player, PCtr_RBtnHeld);
    }
    if ( left_button_clicked )
    {
      set_players_packet_control(player, PCtr_LBtnClick);
    }
    if ( right_button_clicked )
    {
      set_players_packet_control(player, PCtr_RBtnClick);
    }
    if ( left_button_released )
    {
      set_players_packet_control(player, PCtr_LBtnRelease);
    }
    if ( right_button_released )
    {
      set_players_packet_control(player, PCtr_RBtnRelease);
    }
  }
}

short get_map_action_inputs(void)
{
  struct PlayerInfo *player=&(game.players[my_player_number%PLAYERS_COUNT]);
  long keycode;
  long mouse_y,mouse_x;
  int mappos_y,mappos_x;
  mouse_x = GetMouseX();
  mouse_y = GetMouseY();
  mappos_x = 3 * (mouse_x - 150) / 4 + 1;
  mappos_y = 3 * (mouse_y - 56) / 4 + 1;
  if ((mappos_x >= 0) && (mappos_x < map_subtiles_x) && (mappos_y >= 0) && (mappos_y < map_subtiles_y) )
  {
    if ( left_button_clicked )
    {
      left_button_clicked = 0;
    }
    if ( left_button_released )
    {
      left_button_released = 0;
      set_players_packet_action(player, 81,mappos_x,mappos_y,0,0);
      return true;
    }
  }

  if ( right_button_clicked )
    right_button_clicked = 0;
  if ( right_button_released )
  {
    right_button_released = 0;
    zoom_from_map();
    return true;
  } else
  {
    if (get_players_packet_action(player) != PckA_None)
      return true;
    if (is_key_pressed(KC_F8,KM_NONE))
    {
      clear_key_pressed(KC_F8);
      toggle_tooltips();
    }
    if (is_key_pressed(KC_NUMPADENTER,KM_NONE))
    {
      if (toggle_main_cheat_menu())
        clear_key_pressed(KC_NUMPADENTER);
    }
    if ( is_game_key_pressed(31, &keycode, 0) )
    {
      clear_key_pressed(keycode);
      turn_off_all_window_menus();
      zoom_from_map();
      return true;
    }
    return false;
  }
}

void get_isometric_view_nonaction_inputs(void)
{
  _DK_get_isometric_view_nonaction_inputs();
}

void get_overhead_view_nonaction_inputs(void)
{
  _DK_get_overhead_view_nonaction_inputs();
}

void get_front_view_nonaction_inputs(void)
{
  _DK_get_front_view_nonaction_inputs();
}

short slab_type_is_door(unsigned short slab_type)
{
  return ((slab_type >= 42) && (slab_type <= 49));
}

/*
 * Updates given position and context variables.
 * Makes no changes to the Game or Packets.
 */
unsigned char get_player_coords_and_context(struct Coord3d *pos, unsigned char *context)
{
  struct PlayerInfo *player;
  struct Dungeon *dungeon;
  struct SlabMap *slb;
  struct SlabAttr *slbattr;
  struct Thing *thing;
  struct Map *map;
  unsigned long x,y;
  unsigned int slab_x,slab_y;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if ((pointer_x < 0) || (pointer_y < 0)
    || (pointer_x >= player->engine_window_width/pixel_size)
    || (pointer_y >= player->engine_window_height/pixel_size))
      return false;
  if (top_pointed_at_x <= map_subtiles_x)
    x = top_pointed_at_x;
  else
    x = map_subtiles_x;
  if (top_pointed_at_y <= map_subtiles_y)
    y = top_pointed_at_y;
  else
    y = map_subtiles_y;
  slab_x = map_to_slab[x];
  slab_y = map_to_slab[y];
  slb = get_slabmap_block(slab_x, slab_y);
  dungeon = &(game.dungeon[player->field_2B%DUNGEONS_COUNT]);
  slbattr = &slab_attrs[slb->slab%SLAB_TYPES_COUNT];
  map = get_map_block(x, y);
  if (slab_type_is_door(slb->slab) && ((slb->field_5 & 7) == player->field_2B))
  {
    *context = 2;
    pos->x.val = (x<<8) + top_pointed_at_frac_x;
    pos->y.val = (y<<8) + top_pointed_at_frac_y;
  } else
  if (dungeon->things_in_hand[0])
  {
    *context = 3;
    pos->x.val = (x<<8) + top_pointed_at_frac_x;
    pos->y.val = (y<<8) + top_pointed_at_frac_y;
  } else
  if (((1 << player->field_2B) & (map->data >> 28)) == 0)
  {
    *context = 1;
    pos->x.val = (x<<8) + top_pointed_at_frac_x;
    pos->y.val = (y<<8) + top_pointed_at_frac_y;
  } else
  if ((slab_x >= map_tiles_x) || (slab_y >= map_tiles_y))
  {
    *context = 0;
    pos->x.val = (block_pointed_at_x<<8) + pointed_at_frac_x;
    pos->y.val = (block_pointed_at_y<<8) + pointed_at_frac_y;
  } else
  if ((slbattr->field_6 & 0x29) != 0)
  {
    *context = 1;
    pos->x.val = (x<<8) + top_pointed_at_frac_x;
    pos->y.val = (y<<8) + top_pointed_at_frac_y;
  } else
  {
    pos->x.val = (block_pointed_at_x<<8) + pointed_at_frac_x;
    pos->y.val = (block_pointed_at_y<<8) + pointed_at_frac_y;
    thing = get_nearest_thing_for_hand_or_slap(player->field_2B, pos->x.val, pos->y.val);
    if (!thing_is_invalid(thing))
      *context = 3;
    else
      *context = 0;
  }
  if (pos->x.val > (map_subtiles_x << 8))
    pos->x.val = (map_subtiles_x << 8);
  if (pos->y.val > (map_subtiles_y << 8))
    pos->y.val = (map_subtiles_y << 8);
  return true;
}

void get_dungeon_control_nonaction_inputs(void)
{
  //_DK_get_dungeon_control_nonaction_inputs(); return;
  unsigned char context;
  struct Coord3d pos;
  struct PlayerInfo *player;
  struct Packet *pckt;
  my_mouse_x = GetMouseX();
  my_mouse_y = GetMouseY();
  player=&(game.players[my_player_number%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  unset_packet_control(pckt, PCtr_MapCoordsValid);
  if (player->work_state == 1)
  {
    if (get_player_coords_and_context(&pos, &context) )
    {
      set_players_packet_position(player,pos.x.val,pos.y.val);
      set_packet_control(pckt, PCtr_MapCoordsValid);
      pckt->field_10 ^= (pckt->field_10 ^ (context<<1)) & 0x1E;
    }
  } else
  if (screen_to_map(player->acamera, my_mouse_x, my_mouse_y, &pos))
  {
      set_players_packet_position(player,pos.x.val,pos.y.val);
      set_packet_control(pckt, PCtr_MapCoordsValid);
      pckt->field_10 &= 0xE1u;
  }
  if (lbKeyOn[KC_LALT] && lbKeyOn[KC_X])
  {
    clear_key_pressed(KC_X);
    turn_on_menu(10);
  }
  switch (player->field_37)
  {
  case 2:
      get_isometric_view_nonaction_inputs();
      break;
  case 3:
      get_overhead_view_nonaction_inputs();
      break;
  case 5:
      get_front_view_nonaction_inputs();
      break;
  }
}

void get_map_nonaction_inputs(void)
{
  struct Coord3d pos;
  struct PlayerInfo *player;
  struct Packet *pckt;
  //_DK_get_map_nonaction_inputs();
  player=&(game.players[my_player_number%PLAYERS_COUNT]);
  pckt = &game.packets[player->packet_num%PACKETS_COUNT];
  set_players_packet_position(player,GetMouseX(),GetMouseY());
  unset_packet_control(pckt, PCtr_MapCoordsValid);
  if (screen_to_map(player->acamera, pckt->pos_x, pckt->pos_y, &pos))
    set_packet_control(pckt, PCtr_MapCoordsValid);
  if (((game.numfield_C & 0x01) == 0) && (player->field_37 == 3))
  {
    get_overhead_view_nonaction_inputs();
  }
}

short get_packet_load_game_inputs(void)
{
  static const char *func_name="get_packet_load_game_inputs";
  struct PlayerInfo *player;
  load_packets_for_turn(game.pckt_gameturn);
  game.pckt_gameturn++;
  get_packet_load_game_control_inputs();
  if (get_speed_control_inputs())
      return false;
  if (get_screen_control_inputs())
      return false;
  if (get_screen_capture_inputs())
      return false;
  return false;
}

/*
 * Inputs for demo mode. In this mode, the only control keys
 * should take the game back into main menu.
 */
short get_packet_load_demo_inputs(void)
{
  if (is_key_pressed(KC_SPACE,KM_DONTCARE) ||
      is_key_pressed(KC_ESCAPE,KM_DONTCARE) ||
      is_key_pressed(KC_RETURN,KM_DONTCARE) ||
      (lbKeyOn[KC_LALT] && lbKeyOn[KC_X]) ||
      left_button_clicked)
  {
      clear_key_pressed(KC_SPACE);
      clear_key_pressed(KC_ESCAPE);
      clear_key_pressed(KC_RETURN);
      lbKeyOn[KC_X] = 0;
      left_button_clicked = 0;
      quit_game = 1;
  }
}

void get_creature_control_nonaction_inputs(void)
{
  static const char *func_name="get_creature_control_nonaction_inputs";
  _DK_get_creature_control_nonaction_inputs(); return;
}

short get_inputs(void)
{
  static const char *func_name="get_inputs";
  //return _DK_get_inputs();
  struct PlayerInfo *player;
  long keycode;
  if ((game.flags_cd & MFlg_IsDemoMode) != 0)
  {
  #if (BFDEBUG_LEVEL > 5)
    LbSyncLog("%s: Starting for demo mode\n",func_name);
  #endif
    load_packets_for_turn(game.pckt_gameturn);
    game.pckt_gameturn++;
    get_packet_load_demo_inputs();
    return false;
  }
  if (game.packet_load_enable)
  {
  #if (BFDEBUG_LEVEL > 5)
    LbSyncLog("%s: Loading packet inputs\n",func_name);
  #endif
    return get_packet_load_game_inputs();
  }
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if ((player->field_0 & 0x80) != 0)
  {
    struct Packet *pckt;
  #if (BFDEBUG_LEVEL > 5)
    LbSyncLog("%s: Starting for creature fade\n",func_name);
  #endif
    set_players_packet_position(player,127,127);
    if ((!game_is_busy_doing_gui_string_input()) && (game.numfield_C & 0x01))
    {
      if ( is_game_key_pressed(30, &keycode, 0) )
      {
        lbKeyOn[keycode] = 0;
        set_packet_pause_toggle();
      }
    }
    return false;
  }
#if (BFDEBUG_LEVEL > 5)
  LbSyncLog("%s: Starting\n",func_name);
#endif
  if (gui_process_inputs())
  {
    return true;
  }
  if (player->victory_state == VicS_LostLevel)
  {
    if (player->field_2C != 1)
    {
      get_level_lost_inputs();
      return true;
    }
    struct Thing *thing;
    struct CreatureControl *cctrl;
    thing = thing_get(player->field_2F);
    if (!thing_is_creature(thing))
    {
      get_level_lost_inputs();
      return true;
    }
    cctrl = creature_control_get_from_thing(thing);
    if ((cctrl->field_2 & 0x02) == 0)
    {
      get_level_lost_inputs();
      return true;
    }
  }
  short inp_handled = false;
  if (((game.numfield_C & 0x01) == 0) || ((game.numfield_C & 0x80) != 0))
    inp_handled = get_gui_inputs(1);
  if (!inp_handled)
    inp_handled = get_global_inputs();
  if (game_is_busy_doing_gui_string_input())
    return false;
  switch (player->view_type)
  {
  case PVT_DungeonTop:
      if (!inp_handled)
        inp_handled = get_dungeon_control_action_inputs();
      get_dungeon_control_nonaction_inputs();
      get_player_gui_clicks();
      get_packet_control_mouse_clicks();
      return inp_handled;
  case PVT_CreatureContrl:
      if (!inp_handled)
        inp_handled = get_creature_control_action_inputs();
      get_creature_control_nonaction_inputs();
      get_player_gui_clicks();
      get_packet_control_mouse_clicks();
      return inp_handled;
  case PVT_CreaturePasngr:
      if (inp_handled)
      {
        get_player_gui_clicks();
        get_packet_control_mouse_clicks();
        return true;
      } else
      if ( get_creature_passenger_action_inputs() )
      {
        get_packet_control_mouse_clicks();
        return true;
      } else
      {
        get_player_gui_clicks();
        get_packet_control_mouse_clicks();
        return false;
      }
  case PVT_MapScreen:
      if (!inp_handled)
        inp_handled = get_map_action_inputs();
      get_map_nonaction_inputs();
      get_player_gui_clicks();
      get_packet_control_mouse_clicks();
      return inp_handled;
  case 5:
      if (player->field_37 != 6)
      {
        if ((game.numfield_A & 0x01) == 0)
          game.numfield_C &= 0xFE;
        if (toggle_status_menu(false))
          player->field_1 |= 0x01;
        else
          player->field_1 &= 0xFE;
        set_players_packet_action(player, 80, 4,0,0,0);
      }
      return false;
  case 6:
      if (player->field_37 != 7)
      {
        set_players_packet_action(player, 80, 1,0,0,0);
      }
      return false;
  default:
#if (BFDEBUG_LEVEL > 7)
      LbSyncLog("%s: Default exit\n", func_name);
#endif
      return false;
  }
}

void input(void)
{
  static const char *func_name="input";
#if (BFDEBUG_LEVEL > 4)
    LbSyncLog("%s: Starting\n",func_name);
#endif
  //_DK_input();return;
  update_key_modifiers();
  if ((game_is_busy_doing_gui_string_input()) && (lbInkey>0))
  {
    lbKeyOn[lbInkey] = 0;
  }
  struct PlayerInfo *player;
  struct Packet *pckt;
  player=&(game.players[my_player_number%PLAYERS_COUNT]);
  pckt=&game.packets[player->packet_num%PACKETS_COUNT];
  if (is_game_key_pressed(27, 0, 0) != 0)
    pckt->field_10 |= 0x20;
  else
    pckt->field_10 &= 0xDFu;
  if (is_game_key_pressed(28, 0, 0) != 0)
    pckt->field_10 |= 0x40;
  else
    pckt->field_10 &= 0xBFu;

  get_inputs();
#if (BFDEBUG_LEVEL > 7)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}


/******************************************************************************/
#ifdef __cplusplus
}
#endif