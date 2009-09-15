/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file engine_camera.h
 *     Header file for engine_camera.c.
 * @par Purpose:
 *     Camera move, maintain and support functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     20 Mar 2009 - 30 Mar 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/

#ifndef DK_ENGNCAM_H
#define DK_ENGNCAM_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
#pragma pack(1)

struct EngineCoord;
struct M33;
struct EngineCol;

struct MinMax { // sizeof = 8
    long min;
    long max;
};

//I'm not sure about size of this structure
struct Camera {
    struct Coord3d mappos;
    unsigned char field_6;
    int orient_a;
    int orient_b;
    int orient_c;
    int field_13;
    int field_17;
    int field_1B;
    unsigned char field_1F[9];
    short field_28;
};

/******************************************************************************/
DLLIMPORT extern struct M33 _DK_camera_matrix;
#define camera_matrix _DK_camera_matrix
DLLIMPORT extern struct EngineCoord _DK_object_origin;
#define object_origin _DK_object_origin
DLLIMPORT extern struct EngineCol _DK_ecs1[];
#define ecs1 _DK_ecs1
DLLIMPORT extern struct EngineCol _DK_ecs2[];
#define ecs2 _DK_ecs2
DLLIMPORT extern struct EngineCol *_DK_front_ec;
#define front_ec _DK_front_ec
DLLIMPORT extern struct EngineCol *_DK_back_ec;
#define back_ec _DK_back_ec
DLLIMPORT extern struct MinMax _DK_minmaxs[];
#define minmaxs _DK_minmaxs

#pragma pack()
/******************************************************************************/
long get_3d_box_distance(struct Coord3d *pos1, struct Coord3d *pos2);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif