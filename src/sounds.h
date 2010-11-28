/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file sounds.h
 *     Header file for sounds.c.
 * @par Purpose:
 *     Sound related functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   KeeperFX Team
 * @date     11 Jul 2010 - 05 Nov 2010
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_SOUNDS_H
#define DK_SOUNDS_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#pragma pack(1)

struct Thing;

struct SoundSettings {
  char *sound_data_path;
  char *music_data_path;
  char *dir3;
  unsigned short sound_type;
  unsigned short field_E;
  unsigned char max_number_of_samples;
  unsigned char stereo;
  unsigned char field_12;
  unsigned char danger_music;
  unsigned char no_load_sounds;
  unsigned char no_load_music;
  unsigned char field_16;
  unsigned char sound_system;
  unsigned char field_18;
  unsigned char redbook_enable;
};

struct SoundBankHead { // sizeof = 18
  unsigned char field_0[14];
  unsigned long field_E;
};

struct SoundBankSample { // sizeof = 32
  unsigned char field_0[18];
  unsigned long field_12;
  unsigned long field_16;
  unsigned long field_1A;
  unsigned char field_1E;
  unsigned char field_1F;
};

struct SoundBankEntry { // sizeof = 16
  unsigned long field_0;
  unsigned long field_4;
  unsigned long field_8;
  unsigned long field_C;
};

struct UnkSndSampleStr { // sizeof = 16
  unsigned long field_0;
  unsigned long field_4;
  unsigned long field_8;
  unsigned long field_C;
};

#pragma pack()
/******************************************************************************/
TbBool init_sound_heap_two_banks(unsigned char *heap_mem, long heap_size, char *snd_fname, char *spc_fname, long a5);
TbBool init_sound(void);

void update_player_sounds(void);
void process_3d_sounds(void);
void process_sound_heap(void);

void thing_play_sample(struct Thing *thing, short a2, unsigned short a3, char a4, unsigned char a5, unsigned char a6, long a7, long a8);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif