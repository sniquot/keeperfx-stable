/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file player_computer.c
 *     Computer player definitions and activities.
 * @par Purpose:
 *     Defines a computer player control variables and events/checks/processes
 *      functions.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     10 Mar 2009 - 20 Mar 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "player_computer.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "bflib_dernc.h"
#include "bflib_memory.h"

#include "config.h"
#include "keeperfx.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
const char keeper_compplayer_file[]="keepcompp.cfg";

const struct NamedCommand compp_common_commands[] = {
  {"COMPUTERASSISTS", 1},
  {"PROCESSESCOUNT",  2},
  {"CHECKSCOUNT",     3},
  {"EVENTSCOUNT",     4},
  {"COMPUTERSCOUNT",  5},
  {NULL,              0},
  };

const struct NamedCommand compp_process_commands[] = {
  {"NAME",            1},
  {"VALUES",          2},
  {"FUNCTIONS",       3},
  {"PARAMS",          4},
  {"MNEMONIC",        5},
  {NULL,              0},
  };

const struct NamedCommand compp_check_commands[] = {
  {"NAME",            1},
  {"VALUES",          2},
  {"FUNCTIONS",       3},
  {"PARAMS",          4},
  {"MNEMONIC",        5},
  {NULL,              0},
  };

const struct NamedCommand compp_event_commands[] = {
  {"NAME",            1},
  {"VALUES",          2},
  {"FUNCTIONS",       3},
  {"PROCESS",         4},
  {"PARAMS",          5},
  {"MNEMONIC",        6},
  {NULL,              0},
  };

const struct NamedCommand compp_computer_commands[] = {
  {"NAME",            1},
  {"VALUES",          2},
  {"PROCESSES",       3},
  {"CHECKS",          4},
  {"EVENTS",          5},
  {NULL,              0},
  };

ComputerType computer_assist_types[] = { 6, 7, 8, 9 };
unsigned short computer_types[] = { 201, 201, 201, 201, 201, 201, 729, 730, 731, 732 };

/******************************************************************************/
DLLIMPORT void _DK_process_computer_player2(unsigned long plridx);
DLLIMPORT void _DK_setup_a_computer_player(unsigned long a1, long a2);
DLLIMPORT struct ComputerTask *_DK_computer_setup_build_room(struct Computer2 *comp, unsigned short a2, long a3, long a4, long a5);
DLLIMPORT void _DK_process_computer_players2(void);
DLLIMPORT long _DK_set_next_process(struct Computer2 *comp);
DLLIMPORT void _DK_computer_check_events(struct Computer2 *comp);
DLLIMPORT long _DK_process_checks(struct Computer2 *comp);
DLLIMPORT long _DK_process_tasks(struct Computer2 *comp);
DLLIMPORT long _DK_get_computer_money_less_cost(struct Computer2 *comp);

DLLIMPORT long _DK_computer_check_build_all_rooms(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_check_build_all_rooms _DK_computer_check_build_all_rooms
DLLIMPORT long _DK_computer_setup_any_room_continue(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_setup_any_room_continue _DK_computer_setup_any_room_continue
DLLIMPORT long _DK_computer_check_any_room(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_check_any_room _DK_computer_check_any_room
DLLIMPORT long _DK_computer_setup_any_room(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_setup_any_room _DK_computer_setup_any_room
DLLIMPORT long _DK_computer_check_dig_to_entrance(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_check_dig_to_entrance _DK_computer_check_dig_to_entrance
DLLIMPORT long _DK_computer_setup_dig_to_entrance(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_setup_dig_to_entrance _DK_computer_setup_dig_to_entrance
DLLIMPORT long _DK_computer_check_dig_to_gold(struct Computer2 *comp, struct ComputerProcess *process);
#define computer_check_dig_to_gold _DK_computer_check_dig_to_gold
DLLIMPORT long _DK_computer_setup_dig_to_gold(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_setup_dig_to_gold _DK_computer_setup_dig_to_gold
DLLIMPORT long _DK_computer_check_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_check_sight_of_evil _DK_computer_check_sight_of_evil
DLLIMPORT long _DK_computer_setup_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_setup_sight_of_evil _DK_computer_setup_sight_of_evil
DLLIMPORT long _DK_computer_process_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_process_sight_of_evil _DK_computer_process_sight_of_evil
DLLIMPORT long _DK_computer_check_attack1(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_check_attack1 _DK_computer_check_attack1
DLLIMPORT long _DK_computer_setup_attack1(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_setup_attack1 _DK_computer_setup_attack1
DLLIMPORT long _DK_computer_completed_attack1(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_completed_attack1 _DK_computer_completed_attack1
DLLIMPORT long _DK_computer_check_safe_attack(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_check_safe_attack _DK_computer_check_safe_attack
DLLIMPORT long _DK_computer_process_task(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_process_task _DK_computer_process_task
DLLIMPORT long _DK_computer_completed_build_a_room(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_completed_build_a_room _DK_computer_completed_build_a_room
DLLIMPORT long _DK_computer_paused_task(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_paused_task _DK_computer_paused_task
DLLIMPORT long _DK_computer_completed_task(struct Computer2 *comp, struct ComputerProcess *process);
//#define computer_completed_task _DK_computer_completed_task

DLLIMPORT long _DK_computer_checks_hates(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_move_creatures_to_best_room(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_move_creatures_to_room(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_no_imps(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_pretty(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_quick_attack(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_accelerate(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_slap_imps(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_enemy_entrances(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_place_door(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_neutral_places(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_place_trap(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_expand_room(struct Computer2 *comp, struct ComputerCheck * check);
DLLIMPORT long _DK_computer_check_for_money(struct Computer2 *comp, struct ComputerCheck * check);
//#define computer_checks_hates _DK_computer_checks_hates
//#define computer_check_move_creatures_to_best_room _DK_computer_check_move_creatures_to_best_room
//#define computer_check_move_creatures_to_room _DK_computer_check_move_creatures_to_room
//#define computer_check_no_imps _DK_computer_check_no_imps
//#define computer_check_for_pretty _DK_computer_check_for_pretty
//#define computer_check_for_quick_attack _DK_computer_check_for_quick_attack
//#define computer_check_for_accelerate _DK_computer_check_for_accelerate
//#define computer_check_slap_imps _DK_computer_check_slap_imps
//#define computer_check_enemy_entrances _DK_computer_check_enemy_entrances
//#define computer_check_for_place_door _DK_computer_check_for_place_door
//#define computer_check_neutral_places _DK_computer_check_neutral_places
//#define computer_check_for_place_trap _DK_computer_check_for_place_trap
//#define computer_check_for_expand_room _DK_computer_check_for_expand_room
//#define computer_check_for_money _DK_computer_check_for_money

DLLIMPORT long _DK_computer_event_battle(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event);
DLLIMPORT long _DK_computer_event_find_link(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event);
DLLIMPORT long _DK_computer_event_battle_test(struct Computer2 *comp, struct ComputerEvent *cevent);
DLLIMPORT long _DK_computer_event_check_fighters(struct Computer2 *comp, struct ComputerEvent *cevent);
DLLIMPORT long _DK_computer_event_attack_magic_foe(struct Computer2 *comp, struct ComputerEvent *cevent);
DLLIMPORT long _DK_computer_event_check_rooms_full(struct Computer2 *comp, struct ComputerEvent *cevent);
DLLIMPORT long _DK_computer_event_check_imps_in_danger(struct Computer2 *comp, struct ComputerEvent *cevent);
DLLIMPORT long _DK_computer_event_check_payday(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event);
//#define computer_event_battle _DK_computer_event_battle
//#define computer_event_find_link _DK_computer_event_find_link
//#define computer_event_battle_test _DK_computer_event_battle_test
//#define computer_event_check_fighters _DK_computer_event_check_fighters
//#define computer_event_attack_magic_foe _DK_computer_event_attack_magic_foe
//#define computer_event_check_rooms_full _DK_computer_event_check_rooms_full
//#define computer_event_check_imps_in_danger _DK_computer_event_check_imps_in_danger
//#define computer_event_check_payday _DK_computer_event_check_payday

DLLIMPORT extern struct ComputerProcess _DK_BuildAllRooms3x3;
#define BuildAllRooms3x3 _DK_BuildAllRooms3x3
DLLIMPORT extern struct ComputerProcess _DK_BuildAllRooms4x4;
#define BuildAllRooms4x4 _DK_BuildAllRooms4x4
DLLIMPORT extern struct ComputerProcess _DK_BuildPrisonRoom;
#define BuildPrisonRoom _DK_BuildPrisonRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildTortureRoom;
#define BuildTortureRoom _DK_BuildTortureRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildScavengerRoom;
#define BuildScavengerRoom _DK_BuildScavengerRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildTempleRoom;
#define BuildTempleRoom _DK_BuildTempleRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildGraveyardRoom;
#define BuildGraveyardRoom _DK_BuildGraveyardRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildBarrackRoom;
#define BuildBarrackRoom _DK_BuildBarrackRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildTreasureRoom;
#define BuildTreasureRoom _DK_BuildTreasureRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildResearchRoom;
#define BuildResearchRoom _DK_BuildResearchRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildHatcheryRoom;
#define BuildHatcheryRoom _DK_BuildHatcheryRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildLairRoom;
#define BuildLairRoom _DK_BuildLairRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildTrainingRoom;
#define BuildTrainingRoom _DK_BuildTrainingRoom
DLLIMPORT extern struct ComputerProcess _DK_BuildWorkshopRoom;
#define BuildWorkshopRoom _DK_BuildWorkshopRoom
DLLIMPORT extern struct ComputerProcess _DK_DigToEntrance;
#define DigToEntrance _DK_DigToEntrance
DLLIMPORT extern struct ComputerProcess _DK_DigToGoldForMoney;
#define DigToGoldForMoney _DK_DigToGoldForMoney
DLLIMPORT extern struct ComputerProcess _DK_BuildTreasureRoom4x4;
#define BuildTreasureRoom4x4 _DK_BuildTreasureRoom4x4
DLLIMPORT extern struct ComputerProcess _DK_BuildLairRoom4x4;
#define BuildLairRoom4x4 _DK_BuildLairRoom4x4
DLLIMPORT extern struct ComputerProcess _DK_DigToCloseGoldForMoney;
#define DigToCloseGoldForMoney _DK_DigToCloseGoldForMoney
DLLIMPORT extern struct ComputerProcess _DK_DigToGoldGreedy;
#define DigToGoldGreedy _DK_DigToGoldGreedy
DLLIMPORT extern struct ComputerProcess _DK_DigToGoldGreedy2;
#define DigToGoldGreedy2 _DK_DigToGoldGreedy2
DLLIMPORT extern struct ComputerProcess _DK_ComputerSightOfEvil;
#define ComputerSightOfEvil _DK_ComputerSightOfEvil
DLLIMPORT extern struct ComputerProcess _DK_ComputerSightOfEvilScare;
#define ComputerSightOfEvilScare _DK_ComputerSightOfEvilScare
DLLIMPORT extern struct ComputerProcess _DK_ComputerAttack1;
#define ComputerAttack1 _DK_ComputerAttack1
DLLIMPORT extern struct ComputerProcess _DK_ComputerSafeAttack;
#define ComputerSafeAttack _DK_ComputerSafeAttack

/******************************************************************************/
long computer_setup_any_room(struct Computer2 *comp, struct ComputerProcess *process);
long computer_setup_dig_to_entrance(struct Computer2 *comp, struct ComputerProcess *process);
long computer_setup_dig_to_gold(struct Computer2 *comp, struct ComputerProcess *process);
long computer_setup_any_room_continue(struct Computer2 *comp, struct ComputerProcess *process);
long computer_setup_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process);
long computer_setup_attack1(struct Computer2 *comp, struct ComputerProcess *process);
long computer_check_build_all_rooms(struct Computer2 *comp, struct ComputerProcess *process);
long computer_check_any_room(struct Computer2 *comp, struct ComputerProcess *process);
long computer_check_dig_to_entrance(struct Computer2 *comp, struct ComputerProcess *process);
//TODO: this function address is compared; rewrite the comparison!
//long computer_check_dig_to_gold(struct Computer2 *comp, struct ComputerProcess *process);
long computer_check_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process);
long computer_check_attack1(struct Computer2 *comp, struct ComputerProcess *process);
long computer_check_safe_attack(struct Computer2 *comp, struct ComputerProcess *process);
long computer_process_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process);
long computer_process_task(struct Computer2 *comp, struct ComputerProcess *process);
long computer_paused_task(struct Computer2 *comp, struct ComputerProcess *process);
long computer_completed_task(struct Computer2 *comp, struct ComputerProcess *process);
long computer_completed_attack1(struct Computer2 *comp, struct ComputerProcess *process);
long computer_completed_build_a_room(struct Computer2 *comp, struct ComputerProcess *process);

/*TODO!
struct ComputerProcess BuildAllRooms3x3 = {
  "BUILD ALL ROOM 3x3", 0, 3, 3, 0, -1, computer_check_build_all_rooms,
  computer_setup_any_room_continue, computer_process_task,
  computer_completed_build_a_room, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildAllRooms4x4 = {
  "BUILD ALL ROOM 4x4", 0, 4, 4, 0, -1, computer_check_build_all_rooms,
  computer_setup_any_room_continue, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildPrisonRoom = {
  "BUILD A PRISON ROOM", 0, 3, 4, 4, 14, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildTortureRoom = {
  "BUILD A TORTURE ROOM", 0, 3, 4, 5, 4, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildScavengerRoom = {
  "BUILD A SCAVENGER ROOM", 0, 3, 3, 9, 4, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildTempleRoom = {
  "BUILD A TEMPLE ROOM", 0, 3, 3, 10, 8, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildGraveyardRoom = {
  "BUILD A GRAVEYARD ROOM", 0, 4, 5, 11, 5, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildBarrackRoom = {
  "BUILD A BARRACK ROOM", 0, 3, 4, 12, 6, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildTreasureRoom = {
  "BUILD A TREASURE ROOM", 10, 5, 5, 2, 7, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildResearchRoom = {
  "BUILD A RESEARCH ROOM", 0, 5, 5, 3, 2, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildHatcheryRoom = {
  "BUILD A HATCHERY", 0, 6, 5, 13, 14, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildLairRoom = {
 "BUILD A LAIR ROOM", 0, 5, 5, 14, 7, computer_check_any_room,
 computer_setup_any_room, computer_process_task,
 computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildTrainingRoom = {
  "BUILD A TRAINING ROOM", 0, 4, 5, 6, 14, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildWorkshopRoom = {
 "BUILD A WORKSHOP ROOM", 0, 6, 6, 8, 3, computer_check_any_room,
 computer_setup_any_room, computer_process_task,
 computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess DigToEntrance = {
 "DIG TO AN ENTRANCE", 0, 1700, 0, 0, 0, computer_check_dig_to_entrance,
 computer_setup_dig_to_entrance, computer_process_task,
 computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess DigToGoldForMoney = {
 "DIG TO GOLD", 0, 10999, 150, 7, 0, computer_check_dig_to_gold,
 computer_setup_dig_to_gold, computer_process_task,
 computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildTreasureRoom4x4 = {
  "BUILD A TREASURE ROOM 4x4", 10, 4, 4, 2, 7, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess BuildLairRoom4x4 = {
  "BUILD A LAIR ROOM 4x4", 0, 4, 4, 14, 7, computer_check_any_room,
  computer_setup_any_room, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess DigToCloseGoldForMoney = {
 "DIG TO CLOSE GOLD", 0, 30999, 500, 5, 71, computer_check_dig_to_gold,
  computer_setup_dig_to_gold, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess DigToGoldGreedy = {
 "DIG TO GREEDY GOLD", 0, 40999, 400, 7, 900, computer_check_dig_to_gold,
  computer_setup_dig_to_gold, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess DigToGoldGreedy2 = {
  "DIG TO GREEDY GOLD2", 0, 40999, 50, 7, 900, computer_check_dig_to_gold,
  computer_setup_dig_to_gold, computer_process_task,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess ComputerSightOfEvil = {
  "SIGHT OF EVIL", 0, 8, 64, 1500, 0, computer_check_sight_of_evil,
  computer_setup_sight_of_evil, computer_process_sight_of_evil,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess ComputerSightOfEvilScare = {
  "SIGHT OF EVIL SCARE", 0, 8, 10, 5000, 0, computer_check_sight_of_evil,
  computer_setup_sight_of_evil, computer_process_sight_of_evil,
  computer_completed_task, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess ComputerAttack1 = {
  "ATTACK PLAN 1", 0, 55, 6, 80, 0, computer_check_attack1,
  computer_setup_attack1, computer_process_task,
  computer_completed_attack1, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};

struct ComputerProcess ComputerSafeAttack = {
  "ATTACK SAFE ATTACK", 0, 25, 4, 80, 0, computer_check_safe_attack,
  computer_setup_attack1, computer_process_task,
  computer_completed_attack1, computer_paused_task,
  0, 0, 0, 0, 0, 0, 0};
*/

struct ValidRooms valid_rooms_to_build[] = {
  {RoK_TREASURE,  &BuildTreasureRoom},
  {RoK_LAIR,      &BuildLairRoom},
  {RoK_GARDEN,    &BuildHatcheryRoom},
  {RoK_LIBRARY,   &BuildResearchRoom},
  {RoK_TRAINING,  &BuildTrainingRoom},
  {RoK_WORKSHOP,  &BuildWorkshopRoom},
  {RoK_SCAVENGER, &BuildScavengerRoom},
  {RoK_PRISON,    &BuildPrisonRoom},
  {RoK_TEMPLE,    &BuildTempleRoom},
  {RoK_TORTURE,   &BuildTortureRoom},
  {RoK_GRAVEYARD, &BuildGraveyardRoom},
  {RoK_BARRACKS,  &BuildBarrackRoom},
  {-1,            NULL},
};

char const event_pay_day_text[] = "EVENT PAY DAY";
char const event_save_imps_text[] = "EVENT SAVE IMPS";
char const event_check_room_text[] = "EVENT CHECK ROOMS FULL";
char const event_magic_foe_text[] = "EVENT MAGIC FOE";
char const event_check_fighters_text[] = "EVENT CHECK FIGHTERS";
char const event_fight_test_text[] = "EVENT FIGHT TEST";
char const event_fight_text[] = "EVENT FIGHT";
char const event_living_space_full_text[] = "EVENT LIVING SPACE FULL";
char const event_treasure_room_full_text[] = "EVENT TREASURE ROOM FULL";
char const event_heart_under_attack_text[] = "EVENT HEART UNDER ATTACK";
char const event_room_attack_text[] = "EVENT ROOM ATTACK";
char const event_dungeon_breach_text[] = "EVENT DUNGEON BREACH";

char const check_money_text[] = "CHECK MONEY";
char const check_expand_room_text[] = "CHECK EXPAND ROOM";
char const check_avail_trap_text[] = "CHECK AVAILIABLE TRAP";
char const check_neutral_places_text[] = "CHECK FOR NEUTRAL PLACES";
char const check_avail_door_text[] = "CHECK AVAILIABLE DOOR";
char const check_enemy_entrances_text[] = "CHECK FOR ENEMY ENTRANCES";
char const check_for_slap_imp_text[] = "CHECK FOR SLAP IMP";
char const check_for_speed_up_text[] = "CHECK FOR SPEED UP";
char const check_for_quick_attack_text[] = "CHECK FOR QUICK ATTACK";
char const check_to_pretty_text[] = "CHECK TO PRETTY";
char const check_enough_imps_text[] = "CHECK FOR ENOUGH IMPS";
char const move_creature_to_train_text[] = "MOVE CREATURE TO TRAINING";
char const move_creature_to_best_text[] = "MOVE CREATURE TO BEST ROOM";
char const computer_check_hates_text[] = "COMPUTER CHECK HATES";

long computer_checks_hates(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_move_creatures_to_best_room(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_move_creatures_to_room(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_no_imps(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_pretty(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_quick_attack(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_accelerate(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_slap_imps(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_enemy_entrances(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_place_door(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_neutral_places(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_place_trap(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_expand_room(struct Computer2 *comp, struct ComputerCheck * check);
long computer_check_for_money(struct Computer2 *comp, struct ComputerCheck * check);

long computer_event_battle(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event);
long computer_event_find_link(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event);
long computer_event_battle_test(struct Computer2 *comp, struct ComputerEvent *cevent);
long computer_event_check_fighters(struct Computer2 *comp, struct ComputerEvent *cevent);
long computer_event_attack_magic_foe(struct Computer2 *comp, struct ComputerEvent *cevent);
long computer_event_check_rooms_full(struct Computer2 *comp, struct ComputerEvent *cevent);
long computer_event_check_imps_in_danger(struct Computer2 *comp, struct ComputerEvent *cevent);
long computer_event_check_payday(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event);

struct ComputerProcessMnemonic computer_process_config_list[] = {
  {"Unused", NULL,},
  {"", &BuildAllRooms3x3,},
  {"", &BuildAllRooms4x4,},
  {"", &BuildPrisonRoom,},
  {"", &BuildTortureRoom,},
  {"", &BuildScavengerRoom,},
  {"", &BuildTempleRoom,},
  {"", &BuildGraveyardRoom,},
  {"", &BuildBarrackRoom,},
  {"", &BuildTreasureRoom,},
  {"", &BuildResearchRoom,},
  {"", &BuildHatcheryRoom,},
  {"", &BuildLairRoom,},
  {"", &BuildTrainingRoom,},
  {"", &BuildWorkshopRoom,},
  {"", &DigToEntrance,},
  {"", &DigToGoldForMoney,},
  {"", &BuildTreasureRoom4x4,},
  {"", &BuildLairRoom4x4,},
  {"", &DigToCloseGoldForMoney,},
  {"", &DigToGoldGreedy,},
  {"", &DigToGoldGreedy2,},
  {"", &ComputerSightOfEvil,},
  {"", &ComputerSightOfEvilScare,},
  {"", &ComputerAttack1,},
  {"", &ComputerSafeAttack,},
};

const struct NamedCommand computer_process_func_type[] = {
  {"check_build_all_rooms",   1,},
  {"setup_any_room_continue", 2,},
  {"check_any_room",          3,},
  {"setup_any_room",          4,},
  {"check_dig_to_entrance",   5,},
  {"setup_dig_to_entrance",   6,},
  {"check_dig_to_gold",       7,},
  {"setup_dig_to_gold",       8,},
  {"check_sight_of_evil",     9,},
  {"setup_sight_of_evil",    10,},
  {"process_sight_of_evil",  11,},
  {"check_attack1",          12,},
  {"setup_attack1",          13,},
  {"completed_attack1",      14,},
  {"check_safe_attack",      15,},
  {"process_task",           16,},
  {"completed_build_a_room", 17,},
  {"paused_task",            18,},
  {"completed_task",         19,},
  {"none",                   20,},
  {NULL,                      0,},
};

Comp_Process_Func computer_process_func_list[] = {
  NULL,
  computer_check_build_all_rooms,
  computer_setup_any_room_continue,
  computer_check_any_room,
  computer_setup_any_room,
  computer_check_dig_to_entrance,
  computer_setup_dig_to_entrance,
  computer_check_dig_to_gold,
  computer_setup_dig_to_gold,
  computer_check_sight_of_evil,
  computer_setup_sight_of_evil,
  computer_process_sight_of_evil,
  computer_check_attack1,
  computer_setup_attack1,
  computer_completed_attack1,
  computer_check_safe_attack,
  computer_process_task,
  computer_completed_build_a_room,
  computer_paused_task,
  computer_completed_task,
  NULL,
  NULL,
};

ComputerName computer_check_names[COMPUTER_CHECKS_TYPES_COUNT];
struct ComputerCheck computer_checks[COMPUTER_CHECKS_TYPES_COUNT];
struct ComputerCheckMnemonic computer_check_config_list[COMPUTER_CHECKS_TYPES_COUNT];

const struct NamedCommand computer_check_func_type[] = {
  {"checks_hates",            1,},
  {"check_move_to_best_room", 2,},
  {"check_move_to_room",      3,},
  {"check_no_imps",           4,},
  {"check_for_pretty",        5,},
  {"check_for_quick_attack",  6,},
  {"check_for_accelerate",    7,},
  {"check_slap_imps",         8,},
  {"check_enemy_entrances",   9,},
  {"check_for_place_door",   10,},
  {"check_neutral_places",   11,},
  {"check_for_place_trap",   12,},
  {"check_for_expand_room",  13,},
  {"check_for_money",        14,},
  {"none",                   15,},
  {NULL,                      0,},
};

Comp_Check_Func computer_check_func_list[] = {
  NULL,
  computer_checks_hates,
  computer_check_move_creatures_to_best_room,
  computer_check_move_creatures_to_room,
  computer_check_no_imps,
  computer_check_for_pretty,
  computer_check_for_quick_attack,
  computer_check_for_accelerate,
  computer_check_slap_imps,
  computer_check_enemy_entrances,
  computer_check_for_place_door,
  computer_check_neutral_places,
  computer_check_for_place_trap,
  computer_check_for_expand_room,
  computer_check_for_money,
  NULL,
  NULL,
};

ComputerName computer_event_names[COMPUTER_EVENTS_TYPES_COUNT];
struct ComputerEvent computer_events[COMPUTER_EVENTS_TYPES_COUNT];
struct ComputerEventMnemonic computer_event_config_list[COMPUTER_EVENTS_TYPES_COUNT];

const struct NamedCommand computer_event_test_func_type[] = {
  {"event_battle_test",       1,},
  {"event_check_fighters",    2,},
  {"event_attack_magic_foe",  3,},
  {"event_check_rooms_full",  4,},
  {"event_check_imps_danger", 5,},
  {"none",                    6,},
  {NULL,                      0,},
};

Comp_EvntTest_Func computer_event_test_func_list[] = {
  NULL,
  computer_event_battle_test,
  computer_event_check_fighters,
  computer_event_attack_magic_foe,
  computer_event_check_rooms_full,
  computer_event_check_imps_in_danger,
  NULL,
  NULL,
};

const struct NamedCommand computer_event_func_type[] = {
  {"event_battle",            1,},
  {"event_find_link",         2,},
  {"event_check_payday",      3,},
  {"none",                    4,},
  {NULL,                      0,},
};

Comp_Event_Func computer_event_func_list[] = {
  NULL,
  computer_event_battle,
  computer_event_find_link,
  computer_event_check_payday,
  NULL,
  NULL,
};

ComputerName ComputerProcessListsNames[COMPUTER_PROCESS_LISTS_COUNT];
struct ComputerProcessTypes ComputerProcessLists[COMPUTER_PROCESS_LISTS_COUNT];
/******************************************************************************/

int get_computer_process_config_list_index_prc(struct ComputerProcess *process)
{
  int i;
  const int arr_size = sizeof(computer_process_config_list)/sizeof(computer_process_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    if (computer_process_config_list[i].process == process)
      return i;
  }
  return 0;
}

int get_computer_process_config_list_index_mnem(const char *mnemonic)
{
  int i;
  const int arr_size = sizeof(computer_process_config_list)/sizeof(computer_process_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    if (stricmp(computer_process_config_list[i].name, mnemonic) == 0)
      return i;
  }
  return 0;
}

int get_computer_check_config_list_index_prc(struct ComputerCheck *check)
{
  struct ComputerCheck *cchk;
  int i;
  const int arr_size = sizeof(computer_check_config_list)/sizeof(computer_check_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    cchk = computer_check_config_list[i].check;
    if ((cchk == NULL) || (cchk->name == NULL))
      continue;
    if (check == NULL)
      continue;
/*
    if (stricmp(cchk->name, check->name) != 0)
      continue;
*/
    if (cchk->field_4 != check->field_4)
      continue;
    if (cchk->field_8 != check->field_8)
      continue;
    if (cchk->param1 != check->param1)
      continue;
    if (cchk->param2 != check->param2)
      continue;
    if (cchk->param3 != check->param3)
      continue;
    if (cchk->param4 != check->param4)
      continue;
//    if (cchk->func_check == check->func_check)
      return i;
  }
  return 0;
}

int get_computer_check_config_list_index_mnem(const char *mnemonic)
{
  int i;
  const int arr_size = sizeof(computer_check_config_list)/sizeof(computer_check_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    if (stricmp(computer_check_config_list[i].name, mnemonic) == 0)
      return i;
  }
  return 0;
}

int get_computer_event_config_list_index_prc(struct ComputerEvent *event)
{
  struct ComputerEvent *cevnt;
  int i;
  const int arr_size = sizeof(computer_event_config_list)/sizeof(computer_event_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    cevnt = computer_event_config_list[i].event;
    if ((cevnt == NULL) || (cevnt->name == NULL))
      continue;
    if ((cevnt == NULL) || (event->name == NULL))
      continue;
    if (stricmp(cevnt->name, event->name) != 0)
      continue;
    if (cevnt->field_4 != event->field_4)
      continue;
    if (cevnt->field_8 != event->field_8)
      continue;
    if (cevnt->field_14 != event->field_14)
      continue;
    if (cevnt->param1 != event->param1)
      continue;
    if (cevnt->param2 != event->param2)
      continue;
    if (cevnt->param3 != event->param3)
      continue;
    if (cevnt->param4 != event->param4)
      continue;
//    if (cevnt->func_check == check->func_check)
      return i;
  }
  return 0;
}

int get_computer_event_config_list_index_mnem(const char *mnemonic)
{
  int i;
  const int arr_size = sizeof(computer_event_config_list)/sizeof(computer_event_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    if (stricmp(computer_event_config_list[i].name, mnemonic) == 0)
      return i;
  }
  return 0;
}

short computer_type_clear_processes(struct ComputerProcessTypes *cpt)
{
  int i;
  for (i=0; i<COMPUTER_PROCESSES_COUNT; i++)
  {
    cpt->processes[i] = NULL;
  }
  return true;
}

int computer_type_add_process(struct ComputerProcessTypes *cpt, struct ComputerProcess *process)
{
  int i;
  for (i=0; i<COMPUTER_PROCESSES_COUNT; i++)
  {
      if (cpt->processes[i] == NULL)
      {
        cpt->processes[i] = process;
        return i;
      }
  }
  return -1;
}

short computer_type_clear_checks(struct ComputerProcessTypes *cpt)
{
  int i;
  for (i=0; i<COMPUTER_CHECKS_COUNT; i++)
  {
    LbMemorySet(&cpt->checks[i], 0, sizeof(struct ComputerCheck));
  }
  return true;
}

int computer_type_add_check(struct ComputerProcessTypes *cpt, struct ComputerCheck *check)
{
  int i;
  for (i=0; i<COMPUTER_CHECKS_COUNT; i++)
  {
      if (cpt->checks[i].name == NULL)
      {
        LbMemoryCopy(&cpt->checks[i], check, sizeof(struct ComputerCheck));
        return i;
      }
  }
  return -1;
}

short computer_type_clear_events(struct ComputerProcessTypes *cpt)
{
  int i;
  for (i=0; i<COMPUTER_EVENTS_COUNT; i++)
  {
    LbMemorySet(&cpt->events[i], 0, sizeof(struct ComputerEvent));
  }
  return true;
}

int computer_type_add_event(struct ComputerProcessTypes *cpt, struct ComputerEvent *event)
{
  int i;
  for (i=0; i<COMPUTER_EVENTS_COUNT; i++)
  {
      if (cpt->events[i].name == NULL)
      {
        LbMemoryCopy(&cpt->events[i], event, sizeof(struct ComputerEvent));
        return i;
      }
  }
  return -1;
}

short init_computer_process_lists(void)
{
  struct ComputerProcessTypes *cpt;
  int i;
  for (i=0; i<COMPUTER_PROCESS_LISTS_COUNT; i++)
  {
    cpt = &ComputerProcessLists[i];
    LbMemorySet(cpt, 0, sizeof(struct ComputerProcessTypes));
    LbMemorySet(ComputerProcessListsNames[i], 0, LINEMSG_SIZE);
  }
  for (i=0; i<COMPUTER_PROCESS_LISTS_COUNT-1; i++)
  {
    cpt = &ComputerProcessLists[i];
    cpt->name = ComputerProcessListsNames[i];
    computer_type_clear_processes(cpt);
    computer_type_clear_checks(cpt);
  }
  return true;
}

short parse_computer_player_common_blocks(char *buf,long len)
{
  long pos;
  int i,k,n;
  int cmd_num;
  // Block name and parameter word store variables
  char block_buf[32];
  char word_buf[32];
  // Find the block
  sprintf(block_buf,"common");
  pos = 0;
  k = find_conf_block(buf,&pos,len,block_buf);
  if (k < 0)
  {
    LbWarnLog("Block [%s] not found in Computer Player file.\n",block_buf);
    return 0;
  }
  while (pos<len)
  {
      // Finding command number in this line
      cmd_num = recognize_conf_command(buf,&pos,len,compp_common_commands);
      // Now store the config item in correct place
      if (cmd_num == -3) break; // if next block starts
      n = 0;
      switch (cmd_num)
      {
      case 1: // COMPUTERASSISTS
//TODO
          break;
      case 2: // PROCESSESCOUNT
//TODO
          break;
      case 3: // CHECKSCOUNT
//TODO
          break;
      case 4: // EVENTSCOUNT
//TODO
          break;
      case 5: // COMPUTERSCOUNT
//TODO
          break;
      case 0: // comment
          break;
      case -1: // end of buffer
          break;
      default:
          LbWarnLog("Unrecognized command in Computer Player file, starting on byte %d.\n",pos);
          break;
      }
      skip_conf_to_next_line(buf,&pos,len);
  }
  return 1;
}

short parse_computer_player_process_blocks(char *buf,long len)
{
  struct ComputerProcess *process;
  long pos;
  int i,k,n;
  int cmd_num;
  // Block name and parameter word store variable
  char block_buf[32];
  char word_buf[32];
  const int arr_size = sizeof(computer_process_config_list)/sizeof(computer_process_config_list[0]);
  for (i=1; i < arr_size; i++)
  {
    sprintf(block_buf,"process%d",i);
    pos = 0;
    k = find_conf_block(buf,&pos,len,block_buf);
    if (k < 0)
    {
      LbWarnLog("Block [%s] not found in Computer Player file.\n",block_buf);
      continue;
    }
    process = computer_process_config_list[i].process;
    process->parent = NULL;
    while (pos<len)
    {
      // Finding command number in this line
      cmd_num = recognize_conf_command(buf,&pos,len,compp_process_commands);
      // Now store the config item in correct place
      if (cmd_num == -3) break; // if next block starts
      n = 0;
      switch (cmd_num)
      {
      case 1: // NAME
          //For now, let's leave default names.
          break;
      case 2: // VALUES
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_4 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_8 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_C = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_10 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_14 = k;
            n++;
          }
          if (n < 5)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","VALUES",block_buf);
          }
          break;
      case 3: // FUNCTIONS
          k = recognize_conf_parameter(buf,&pos,len,computer_process_func_type);
          if (k > 0)
          {
            //TODO: hack to make it work, for now (don't set check_dig_to_gold)
            if (k != 7)
              process->func_check = computer_process_func_list[k];
              n++;
          }
          k = recognize_conf_parameter(buf,&pos,len,computer_process_func_type);
          if (k > 0)
          {
              process->func_setup = computer_process_func_list[k];
              n++;
          }
          k = recognize_conf_parameter(buf,&pos,len,computer_process_func_type);
          if (k > 0)
          {
              process->func_task = computer_process_func_list[k];
              n++;
          }
          k = recognize_conf_parameter(buf,&pos,len,computer_process_func_type);
          if (k > 0)
          {
              process->func_complete = computer_process_func_list[k];
              n++;
          }
          k = recognize_conf_parameter(buf,&pos,len,computer_process_func_type);
          if (k > 0)
          {
              process->func_pause = computer_process_func_list[k];
              n++;
          }
          if (n < 5)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","FUNCTIONS",block_buf);
          }
          break;
      case 4: // PARAMS
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_30 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_34 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_38 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_3C = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_40 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            process->field_44 = k;
            n++;
          }
          if (n < 6)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","PARAMS",block_buf);
          }
          break;
      case 5: // MNEMONIC
          if (get_conf_parameter_whole(buf,&pos,len,computer_process_config_list[i].name,sizeof(computer_process_config_list[i].name)) <= 0)
          {
            LbWarnLog("Couldn't read \"%s\" parameter in [%s] block of Computer file.\n","MNEMONIC",block_buf);
            break;
          }
          break;
      case 0: // comment
          break;
      case -1: // end of buffer
          break;
      default:
          LbWarnLog("Unrecognized command in Computer Player file, starting on byte %d.\n",pos);
          break;
      }
      skip_conf_to_next_line(buf,&pos,len);
    }
  }
  return 1;
}

short parse_computer_player_check_blocks(char *buf,long len)
{
  struct ComputerCheck *check;
  long pos;
  int i,k,n;
  int cmd_num;
  // Block name and parameter word store variables
  char block_buf[32];
  char word_buf[32];
  // Initialize the checks array
  const int arr_size = sizeof(computer_check_config_list)/sizeof(computer_check_config_list[0]);
  for (i=0; i < arr_size; i++)
  {
    check = &computer_checks[i];
    computer_check_config_list[i].name[0] = '\0';
    computer_check_config_list[i].check = check;
    check->name = computer_check_names[i];
    LbMemorySet(computer_check_names[i], 0, LINEMSG_SIZE);
  }
  strcpy(computer_check_names[0],"INCORRECT CHECK");
  // Load the file
  for (i=1; i < arr_size; i++)
  {
    sprintf(block_buf,"check%d",i);
    pos = 0;
    k = find_conf_block(buf,&pos,len,block_buf);
    if (k < 0)
    {
      LbWarnLog("Block [%s] not found in Computer Player file.\n",block_buf);
      continue;
    }
    check = computer_check_config_list[i].check;
    while (pos<len)
    {
      // Finding command number in this line
      cmd_num = recognize_conf_command(buf,&pos,len,compp_check_commands);
      // Now store the config item in correct place
      if (cmd_num == -3) break; // if next block starts
      n = 0;
      switch (cmd_num)
      {
      case 1: // NAME
          if (get_conf_parameter_whole(buf,&pos,len,check->name,LINEMSG_SIZE) <= 0)
          {
            LbWarnLog("Couldn't read \"%s\" parameter in [%s] block of Computer file.\n","NAME",block_buf);
            break;
          }
          break;
      case 2: // VALUES
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            check->field_4 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            check->field_8 = k;
            n++;
          }
          if (n < 2)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","VALUES",block_buf);
          }
          break;
      case 3: // FUNCTIONS
          k = recognize_conf_parameter(buf,&pos,len,computer_check_func_type);
          if (k > 0)
          {
              check->func_check = computer_check_func_list[k];
              n++;
          }
          if (n < 1)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","FUNCTIONS",block_buf);
          }
          break;
      case 4: // PARAMS
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            check->param1 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            check->param2 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            check->param3 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            check->param4 = k;
            n++;
          }
          if (n < 4)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","PARAMS",block_buf);
          }
          break;
      case 5: // MNEMONIC
          if (get_conf_parameter_whole(buf,&pos,len,computer_check_config_list[i].name,sizeof(computer_check_config_list[i].name)) <= 0)
          {
            LbWarnLog("Couldn't read \"%s\" parameter in [%s] block of Computer file.\n","MNEMONIC",block_buf);
            break;
          }
          break;
      case 0: // comment
          break;
      case -1: // end of buffer
          break;
      default:
          LbWarnLog("Unrecognized command in Computer Player file, starting on byte %d.\n",pos);
          break;
      }
      skip_conf_to_next_line(buf,&pos,len);
    }
  }
  return 1;
}

short parse_computer_player_event_blocks(char *buf,long len)
{
  struct ComputerEvent *event;
  long pos;
  int i,k,n;
  int cmd_num;
  // Block name and parameter word store variables
  char block_buf[32];
  char word_buf[32];
  // Initialize the events array
  const int arr_size = sizeof(computer_event_config_list)/sizeof(computer_event_config_list[0]);
  for (i=0; i < arr_size; i++)
  {
    event = &computer_events[i];
    computer_event_config_list[i].name[0] = '\0';
    computer_event_config_list[i].event = event;
    event->name = computer_event_names[i];
    LbMemorySet(computer_event_names[i], 0, LINEMSG_SIZE);
  }
  strcpy(computer_event_names[0],"INCORRECT EVENT");
  // Load the file
  for (i=1; i < arr_size; i++)
  {
    sprintf(block_buf,"event%d",i);
    pos = 0;
    k = find_conf_block(buf,&pos,len,block_buf);
    if (k < 0)
    {
      LbWarnLog("Block [%s] not found in Computer Player file.\n",block_buf);
      continue;
    }
    event = computer_event_config_list[i].event;
    while (pos<len)
    {
      // Finding command number in this line
      cmd_num = recognize_conf_command(buf,&pos,len,compp_event_commands);
      // Now store the config item in correct place
      if (cmd_num == -3) break; // if next block starts
      n = 0;
      switch (cmd_num)
      {
      case 1: // NAME
          if (get_conf_parameter_whole(buf,&pos,len,event->name,LINEMSG_SIZE) <= 0)
          {
            LbWarnLog("Couldn't read \"%s\" parameter in [%s] block of Computer file.\n","NAME",block_buf);
            break;
          }
          break;
      case 2: // VALUES
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->field_4 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->field_8 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->field_14 = k;
            n++;
          }
          if (n < 3)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","VALUES",block_buf);
          }
          break;
      case 3: // FUNCTIONS
          k = recognize_conf_parameter(buf,&pos,len,computer_event_func_type);
          if (k > 0)
          {
              event->func_event = computer_event_func_list[k];
              n++;
          }
          k = recognize_conf_parameter(buf,&pos,len,computer_event_test_func_type);
          if (k > 0)
          {
              event->func_test = computer_event_test_func_list[k];
              n++;
          }
          if (n < 2)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","FUNCTIONS",block_buf);
          }
          break;
      case 4: // PROCESS
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = get_computer_process_config_list_index_mnem(word_buf);
            if (k > 0)
            {
              event->process = computer_process_config_list[k].process;
            } else
            {
              LbWarnLog("Couldn't recognize \"%s\" parameter \"%s\" in Computer file.\n","PROCESS",word_buf);
            }
          }
          break;
      case 5: // PARAMS
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->param1 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->param2 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->param3 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            event->param4 = k;
            n++;
          }
          if (n < 4)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","PARAMS",block_buf);
          }
          break;
      case 6: // MNEMONIC
          if (get_conf_parameter_whole(buf,&pos,len,computer_event_config_list[i].name,sizeof(computer_event_config_list[i].name)) <= 0)
          {
            LbWarnLog("Couldn't read \"%s\" parameter in [%s] block of Computer file.\n","MNEMONIC",block_buf);
            break;
          }
          break;
      case 0: // comment
          break;
      case -1: // end of buffer
          break;
      default:
          LbWarnLog("Unrecognized command in Computer Player file, starting on byte %d.\n",pos);
          break;
      }
      skip_conf_to_next_line(buf,&pos,len);
    }
  }
  return 1;
}

short write_computer_player_check_to_log(struct ComputerCheck *check)
{
  LbJustLog("[checkXX]\n");
  LbJustLog("Name = %s\n",check->name);
  LbJustLog("Mnemonic = %s\n","XX");
  LbJustLog("Values = %d %d\n",check->field_4,check->field_8);
  LbJustLog("Functions = %x\n",check->func_check);
  LbJustLog("Params = %d %d %d %d\n",check->param1,check->param2,check->param3,check->param4);
  return true;
}

short write_computer_player_event_to_log(struct ComputerEvent *event)
{
  LbJustLog("[eventXX]\n");
  LbJustLog("Name = %s\n",event->name);
  LbJustLog("Mnemonic = %s\n","XX");
  LbJustLog("Values = %d %d %d\n",event->field_4,event->field_8,event->field_14);
  LbJustLog("Functions = %x %x\n",event->func_event,event->func_test);
  LbJustLog("Params = %d %d %d %d\n",event->param1,event->param2,event->param3,event->param4);
  return true;
}

/*
 * Writes AI config file to keeper log.
 */
short write_computer_player_file_to_log(void)
{
  int i,k,n;
  struct ComputerProcessTypes *cpt;
  i = 0;
  while (ComputerProcessLists[i].name != NULL)
  {
    cpt = &ComputerProcessLists[i];
    LbJustLog("[computer%d]\n",i);
    LbJustLog("Name = %s\n",cpt->name);
    LbJustLog("Values = %d %d %d %d %d %d %d\n",cpt->field_4,cpt->field_8,
        cpt->field_C,cpt->field_10,cpt->field_14,cpt->field_18,cpt->field_1C);
    LbJustLog("Processes = ");
    for (k=0; k<COMPUTER_PROCESSES_COUNT; k++)
    {
      n = get_computer_process_config_list_index_prc(cpt->processes[k]);
      LbJustLog("%s ",computer_process_config_list[n].name);
    }
    LbJustLog("\n");
    LbJustLog("Checks =");
    for (k=0; k<COMPUTER_CHECKS_COUNT; k++)
    {
      n = get_computer_check_config_list_index_prc(&cpt->checks[k]);
      if (n == 0)
      {
        LbJustLog("\n");
        write_computer_player_check_to_log(&cpt->checks[k]);
        break;
      }
      LbJustLog(" %s",computer_check_config_list[n].name);
    }
    LbJustLog("\n");

    LbJustLog("Events =");
    for (k=0; k<COMPUTER_EVENTS_COUNT; k++)
    {
      n = get_computer_event_config_list_index_prc(&cpt->events[k]);
      if (n == 0)
      {
        LbJustLog("\n");
        write_computer_player_event_to_log(&cpt->events[k]);
        break;
      }
      LbJustLog(" %s",computer_event_config_list[n].name);
    }
    LbJustLog("\n");

    LbJustLog("\n");
    i++;
  }
}

short parse_computer_player_computer_blocks(char *buf,long len)
{
  struct ComputerProcessTypes *cpt;
  long pos;
  int i,k,n;
  int cmd_num;
  // Block name and parameter word store variable
  char block_buf[32];
  char word_buf[32];
  const int arr_size = sizeof(ComputerProcessLists)/sizeof(ComputerProcessLists[0])-1;
  for (i=0; i < arr_size; i++)
  {
    sprintf(block_buf,"computer%d",i);
    pos = 0;
    k = find_conf_block(buf,&pos,len,block_buf);
    if (k < 0)
    {
      LbWarnLog("Block [%s] not found in Computer Player file.\n",block_buf);
      continue;
    }
    cpt = &ComputerProcessLists[i];
    while (pos<len)
    {
      // Finding command number in this line
      cmd_num = recognize_conf_command(buf,&pos,len,compp_computer_commands);
      // Now store the config item in correct place
      if (cmd_num == -3) break; // if next block starts
      n = 0;
      switch (cmd_num)
      {
      case 1: // NAME
          if (get_conf_parameter_whole(buf,&pos,len,cpt->name,LINEMSG_SIZE) <= 0)
          {
            LbWarnLog("Couldn't read \"%s\" parameter in [%s] block of Computer file.\n","NAME",block_buf);
            break;
          }
          break;
      case 2: // VALUES
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_4 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_8 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_C = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_10 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_14 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_18 = k;
            n++;
          }
          if (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = atoi(word_buf);
            cpt->field_1C = k;
            n++;
          }
          if (n < 7)
          {
            LbWarnLog("Couldn't recognize all \"%s\" parameters in [%s] block of Computer file.\n","VALUES",block_buf);
          }
          break;
      case 3: // PROCESSES
          computer_type_clear_processes(cpt);
          while (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = get_computer_process_config_list_index_mnem(word_buf);
            if (k <= 0)
            {
              LbWarnLog("Couldn't recognize \"%s\" parameter \"%s\" in Computer file.\n","PROCESSES",word_buf);
              continue;
            }
            n = computer_type_add_process(cpt, computer_process_config_list[k].process);
            if (n < 0)
              LbWarnLog("Couldn't add \"%s\" list element \"%s\" when reading Computer file.\n","PROCESSES",word_buf);
          }
          break;
      case 4: // CHECKS
          computer_type_clear_checks(cpt);
          while (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = get_computer_check_config_list_index_mnem(word_buf);
            if (k <= 0)
            {
              LbWarnLog("Couldn't recognize \"%s\" parameter \"%s\" in Computer file.\n","CHECKS",word_buf);
              continue;
            }
            n = computer_type_add_check(cpt, computer_check_config_list[k].check);
            if (n < 0)
              LbWarnLog("Couldn't add \"%s\" list element \"%s\" when reading Computer file.\n","CHECKS",word_buf);
          }
          break;
      case 5: // EVENTS
          computer_type_clear_events(cpt);
          while (get_conf_parameter_single(buf,&pos,len,word_buf,sizeof(word_buf)) > 0)
          {
            k = get_computer_event_config_list_index_mnem(word_buf);
            if (k <= 0)
            {
              LbWarnLog("Couldn't recognize \"%s\" parameter \"%s\" in Computer file.\n","EVENTS",word_buf);
              continue;
            }
            n = computer_type_add_event(cpt, computer_event_config_list[k].event);
            if (n < 0)
              LbWarnLog("Couldn't add \"%s\" list element \"%s\" when reading Computer file.\n","EVENTS",word_buf);
          }
          break;
      case 0: // comment
          break;
      case -1: // end of buffer
          break;
      default:
          LbWarnLog("Unrecognized command in Computer Player file, starting on byte %d.\n",pos);
          break;
      }
      skip_conf_to_next_line(buf,&pos,len);
    }
  }
  return 1;
}

short load_computer_player_config(void)
{
  static const char *func_name="load_computer_player_config";
  const char *fname;
  char *buf;
  long len;
  init_computer_process_lists();
  // Load the config file
  fname = prepare_file_path(FGrp_FxData,keeper_compplayer_file);
  len = LbFileLengthRnc(fname);
  if (len < 2)
  {
    LbErrorLog("Computer Player file \"%s\" doesn't exist or is too small.\n",keeper_compplayer_file);
    return false;
  }
  if (len > 65536)
  {
    LbErrorLog("Computer Player file \"%s\" is too large.\n",keeper_compplayer_file);
    return false;
  }
  buf = (char *)LbMemoryAlloc(len+256);
  if (buf == NULL)
    return false;
  // Loading file data
  len = LbFileLoadAt(fname, buf);
  if (len>0)
  {
    parse_computer_player_common_blocks(buf,len);
    parse_computer_player_process_blocks(buf,len);
    parse_computer_player_check_blocks(buf,len);
    parse_computer_player_event_blocks(buf,len);
//write_computer_player_file_to_log();
    parse_computer_player_computer_blocks(buf,len);
  }
  //Freeing and exiting
  LbMemoryFree(buf);
  // Hack to synchronize local structure with the one inside DLL.
  // Remove when it's not needed anymore.
  LbMemoryCopy(_DK_ComputerProcessLists,ComputerProcessLists,13*sizeof(struct ComputerProcessTypes));
  return 1;
}

void shut_down_process(struct Computer2 *comp, struct ComputerProcess *process)
{
  Comp_Process_Func callback;
  if (process != NULL)
  {
    set_flag_dword(&process->field_44, 0x0008, true);
    set_flag_dword(&process->field_44, 0x0020, false);
    process->field_34 = game.play_gameturn;
    callback = process->func_complete;
    if (callback != NULL)
      callback(comp, process);
  }
}

void reset_process(struct Computer2 *comp, struct ComputerProcess *process)
{
  if (process != NULL)
  {
    process->field_3C = 0;
    process->field_38 = 0;
    set_flag_dword(&process->field_44, 0x0020, false);
    process->field_34 = game.play_gameturn;
  }
}

long get_computer_money_less_cost(struct Computer2 *comp)
{
  return _DK_get_computer_money_less_cost(comp);
}

struct ComputerTask *computer_setup_build_room(struct Computer2 *comp, unsigned short rkind, long a3, long a4, long a5)
{
  return _DK_computer_setup_build_room(comp, rkind, a3, a4, a5);
}

long computer_setup_any_room(struct Computer2 *comp, struct ComputerProcess *process)
{
  //return _DK_computer_setup_any_room(comp, process);
  struct ComputerTask *task;
  long i;
  task = computer_setup_build_room(comp, process->field_10, process->field_8, process->field_C, process->field_14);
  if (task != NULL)
  {
    set_flag_dword(&process->field_44, 0x0020, true);
    i = (long)((char *)process - (char *)&comp->processes[0]) / sizeof(struct ComputerProcess);
    if ((i < 0) || (i > COMPUTER_PROCESSES_COUNT))
    {
      LbErrorLog("Process \"%s\" is outside of Computer Player.\n",process->name);
      i = COMPUTER_PROCESSES_COUNT;
    }
    task->field_8C = i;
    shut_down_process(comp, process);
    return 2;
  }
  if (process->field_8 > process->field_C)
  {
    if (process->field_8 <= 2)
      return 0;
    process->field_8--;
  } else
  {
    if (process->field_C <= 2)
      return 0;
    process->field_C--;
  }
  reset_process(comp, process);
  return 2;
}

long computer_setup_dig_to_entrance(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_setup_dig_to_entrance(comp, process);
}

long computer_setup_dig_to_gold(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_setup_dig_to_gold(comp, process);
}

long computer_setup_any_room_continue(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_setup_any_room_continue(comp, process);
}

long computer_setup_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_setup_sight_of_evil(comp, process);
}

long computer_setup_attack1(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_setup_attack1(comp, process);
}

long computer_check_build_all_rooms(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_build_all_rooms(comp, process);
}

long computer_check_any_room(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_any_room(comp, process);
}

long computer_check_dig_to_entrance(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_dig_to_entrance(comp, process);
}

/*TODO
long computer_check_dig_to_gold(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_dig_to_gold(comp, process);
}
*/

long computer_check_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_sight_of_evil(comp, process);
}

long computer_check_attack1(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_attack1(comp, process);
}

long computer_check_safe_attack(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_check_safe_attack(comp, process);
}

long computer_process_sight_of_evil(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_process_sight_of_evil(comp, process);
}

long computer_process_task(struct Computer2 *comp, struct ComputerProcess *process)
{
  return 0;
}

long computer_paused_task(struct Computer2 *comp, struct ComputerProcess *process)
{
  comp->field_0 = 2;
  return 0;
}

long computer_completed_task(struct Computer2 *comp, struct ComputerProcess *process)
{
  process->field_34 = game.play_gameturn;
  comp->field_0 = 2;
  return 0;
}

long computer_completed_attack1(struct Computer2 *comp, struct ComputerProcess *process)
{
  return _DK_computer_completed_attack1(comp, process);
}

long computer_completed_build_a_room(struct Computer2 *comp, struct ComputerProcess *process)
{
  process->field_44 &= 0xFFFFFFF7u;
  comp->field_0 = 2;
  return 0;
}

long computer_checks_hates(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_checks_hates(comp, check);
}

long computer_check_move_creatures_to_best_room(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_move_creatures_to_best_room(comp, check);
}

long computer_check_move_creatures_to_room(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_move_creatures_to_room(comp, check);
}

long computer_check_no_imps(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_no_imps(comp, check);
}

long computer_check_for_pretty(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_pretty(comp, check);
}

long computer_check_for_quick_attack(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_quick_attack(comp, check);
}

long computer_check_for_accelerate(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_accelerate(comp, check);
}

long computer_check_slap_imps(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_slap_imps(comp, check);
}

long computer_check_enemy_entrances(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_enemy_entrances(comp, check);
}

long computer_check_for_place_door(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_place_door(comp, check);
}

long computer_check_neutral_places(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_neutral_places(comp, check);
}

long computer_check_for_place_trap(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_place_trap(comp, check);
}

long computer_check_for_expand_room(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_expand_room(comp, check);
}

long computer_check_for_money(struct Computer2 *comp, struct ComputerCheck * check)
{
  return _DK_computer_check_for_money(comp, check);
}

long computer_event_battle(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event)
{
  return _DK_computer_event_battle(comp, cevent, event);
}

long computer_event_find_link(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event)
{
  return _DK_computer_event_find_link(comp, cevent, event);
}

long computer_event_battle_test(struct Computer2 *comp, struct ComputerEvent *cevent)
{
  return _DK_computer_event_battle_test(comp, cevent);
}

long computer_event_check_fighters(struct Computer2 *comp, struct ComputerEvent *cevent)
{
  return _DK_computer_event_check_fighters(comp, cevent);
}

long computer_event_attack_magic_foe(struct Computer2 *comp, struct ComputerEvent *cevent)
{
  return _DK_computer_event_attack_magic_foe(comp, cevent);
}

long computer_event_check_rooms_full(struct Computer2 *comp, struct ComputerEvent *cevent)
{
  return _DK_computer_event_check_rooms_full(comp, cevent);
}

long computer_event_check_imps_in_danger(struct Computer2 *comp, struct ComputerEvent *cevent)
{
  return _DK_computer_event_check_imps_in_danger(comp, cevent);
}

long computer_event_check_payday(struct Computer2 *comp, struct ComputerEvent *cevent,struct Event *event)
{
  return _DK_computer_event_check_payday(comp, cevent, event);
}

void setup_a_computer_player(unsigned short plyridx, long comp_model)
{
  struct ComputerProcessTypes *cproctype;
  struct ComputerProcess *process;
  struct ComputerProcess *newproc;
  struct ComputerCheck *check;
  struct ComputerCheck *newchk;
  struct ComputerEvent *event;
  struct ComputerEvent *newevnt;
  struct Comp2_UnkStr1 *unkptr;
  struct Computer2 *comp;
  long i;
  //_DK_setup_a_computer_player(plyridx, comp_model); return;
  comp = &game.computer[plyridx];
  LbMemorySet(comp, 0, sizeof(struct Computer2));
  cproctype = &ComputerProcessLists[comp_model];
  comp->field_24 = &game.dungeon[plyridx];
  comp->model = comp_model;
  comp->field_18 = cproctype->field_C;
  comp->field_14 = cproctype->field_8;
  comp->field_30 = cproctype->field_10;
  comp->field_2C = cproctype->field_14;
  comp->field_20 = cproctype->field_18;
  comp->field_C = 1;
  comp->field_0 = 2;

  for (i=0; i < 5; i++)
  {
    unkptr = &comp->unkarr_A10[i];
    if (i == plyridx)
      unkptr->field_6 = 0x80000000;
    else
      unkptr->field_6 = 0;
  }
  comp->field_1C = cproctype->field_4;

  for (i=0; i < COMPUTER_PROCESSES_COUNT; i++)
  {
    process = cproctype->processes[i];
    newproc = &comp->processes[i];
    if ((process == NULL) || (process->name == NULL))
    {
      newproc->name = NULL;
      break;
    }
    // Modifying original ComputerProcessTypes structure - I don't like it!
    if (process->func_setup == computer_setup_any_room)
    {
      if (process->field_14 >= 0)
        process->field_14 = get_room_look_through(process->field_14);
    }
    LbMemoryCopy(newproc, process, sizeof(struct ComputerProcess));
    newproc->parent = process;
  }
  newproc = &comp->processes[i];
  newproc->field_44 |= 0x02;

  for (i=0; i < COMPUTER_CHECKS_COUNT; i++)
  {
    check = &cproctype->checks[i];
    newchk = &comp->checks[i];
    if ((check == NULL) || (check->name == NULL))
    {
      newchk->name = NULL;
      break;
    }
    LbMemoryCopy(newchk, check, sizeof(struct ComputerCheck));
  }
  // Note that we don't have special, empty check at end of array
  //newchk = &comp->checks[i];
  newchk->field_4 |= 0x02;

  for (i=0; i < COMPUTER_EVENTS_COUNT; i++)
  {
    event = &cproctype->events[i];
    newevnt = &comp->events[i];
    if ((event == NULL) || (event->name == NULL))
    {
      newevnt->name = NULL;
      break;
    }
    LbMemoryCopy(newevnt, event, sizeof(struct ComputerEvent));
  }
}

long set_next_process(struct Computer2 *comp)
{
  return _DK_set_next_process(comp);
}

void computer_check_events(struct Computer2 *comp)
{
  _DK_computer_check_events(comp);
}

long process_checks(struct Computer2 *comp)
{
  return _DK_process_checks(comp);
}

long process_tasks(struct Computer2 *comp)
{
  return _DK_process_tasks(comp);
}

short process_processes_and_task(struct Computer2 *comp)
{
  static const char *func_name="process_processes_and_task";
  struct ComputerProcess *process;
  Comp_Process_Func callback;
  int i;
  for (i=comp->field_10; i > 0; i--)
  {
    if (comp->field_10 <= 0)
      return false;
    if ((game.play_gameturn % comp->field_18) == 0)
      process_tasks(comp);
    switch (comp->field_0)
    {
    case 1:
        comp->field_8--;
        if (comp->field_8 <= 0)
        {
          comp->field_8 = comp->field_4;
          set_next_process(comp);
        }
        break;
    case 2:
        set_next_process(comp);
        break;
    case 3:
        if (comp->field_14C4 > 0)
        {
          process = &comp->processes[comp->field_14C4];
          callback = process->func_task;
          if (callback != NULL)
            callback(comp,process);
        } else
        {
          error(func_name, 231, "No Process for a computer player");
          comp->field_0 = 1;
        }
        break;
    }
  }
  return true;
}

void process_computer_player2(unsigned long plridx)
{
  static const char *func_name="process_computer_player2";
  //_DK_process_computer_player2(plridx);
  struct Computer2 *comp;
  comp = &game.computer[plridx%PLAYERS_COUNT];
  if ((comp->field_14) && (comp->field_2C <= game.play_gameturn))
    comp->field_10 = 1;
  else
    comp->field_10 = 0;
  if (comp->field_10 <= 0)
    return;
  computer_check_events(comp);
  process_checks(comp);
  process_processes_and_task(comp);
  if ((comp->field_10 < 0) || (comp->field_10 > 1))
    error(func_name, 239, "Computer performed more than one task");
}

void process_computer_players2(void)
{
  //_DK_process_computer_players2();
  struct PlayerInfo *player;
  struct Dungeon *dungeon;
  struct Computer2 *comp;
  struct ComputerProcess *process;
  struct ComputerProcess *dig_process;
  short needs_gold_check;
  int i;
  static long turn_last_checked_for_gold;
  needs_gold_check = false;
  for (i=0; i < PLAYERS_COUNT; i++)
  {
    player = &(game.players[i]);
    comp = &(game.computer[i]);
    dungeon = &(game.dungeon[i%DUNGEONS_COUNT]);
    if ((player->field_0 & 0x01) == 0)
      continue;
    if ((player->field_0 & 0x40) || (dungeon->computer_enabled & 0x01))
      if (player->field_2C == 1)
      {
        process_computer_player2(i);
        dig_process = NULL;
        process = &comp->processes[0];
        while ((process->field_44 & 0x02) == 0)
        {
          if (process->func_setup == computer_setup_dig_to_gold)
          {
            if ((process->field_44 & 0x04) != 0)
            {
              dig_process = process;
              break;
            }
          }
          process++;
        }
        if (dig_process != NULL)
          if (turn_last_checked_for_gold+5000 < game.play_gameturn)
          {
            needs_gold_check = true;
            set_flag_dword(&dig_process->field_44, 0x0004, false);
          }
      }
  }
  if (needs_gold_check)
  {
    turn_last_checked_for_gold = game.play_gameturn;
    check_map_for_gold();
  }
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif