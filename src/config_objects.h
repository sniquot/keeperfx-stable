/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file config_objects.h
 *     Header file for config_objects.c.
 * @par Purpose:
 *     Object things configuration loading functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     11 Jun 2012 - 16 Aug 2012
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_CFGOBJECTS_H
#define DK_CFGOBJECTS_H

#include "globals.h"
#include "bflib_basics.h"

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

#define OBJECT_ITEMS_MAX 256

enum ObjectCategoryIndex {
    OCtg_Unknown = 0,
    OCtg_Decoration, //< Object has no strong function
    OCtg_Furniture,  //< Object is crucial part of a room
    OCtg_Valuable,   //< Object is gold in some form
    OCtg_Spellbook,  //< Object is a spellbook
    OCtg_WrkshpBox,  //< Object is a manufactured box for workshop
    OCtg_Food,       //< Object is food for creatures
    OCtg_Power,      //< Object is a keeper power effect, ie. hand of evil or keeper spell
    OCtg_Lair,       //< Object is a creature lair
    OCtg_Effect,     //< Object is some kind of effect which has influence on things or on terrain
};

/******************************************************************************/
#pragma pack(1)


#pragma pack()
/******************************************************************************/
struct ObjectConfigStats {
    char code_name[COMMAND_WORD_LEN];
    long gerne;
    long name_stridx;
};

struct ObjectsConfig {
    long object_types_count;
    struct ObjectConfigStats object_cfgstats[OBJECT_ITEMS_MAX];
};
/******************************************************************************/
extern const char keeper_objects_file[];
extern struct NamedCommand object_desc[OBJECT_ITEMS_MAX];
/******************************************************************************/
TbBool load_objects_config(const char *conf_fname,unsigned short flags);
struct ObjectConfigStats *get_object_model_stats(ThingModel tngmodel);
const char *object_code_name(ThingModel tngmodel);
ThingModel object_model_id(const char * code_name);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif