// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_IMG_EFFECTS_H__
#define __CAMERIC_REG_DESCRIPTION_IMG_EFFECTS_H__

#include <ebase/types.h>

static RegDescription_t ImgEffectsRegisterMap[] = 
{
    { 0U, PERM_READ_WRITE, "IMG_EFF_CTRL"     , "Global control register"                                                , 0x00000000, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_WRITE, "IMG_EFF_COLOR_SEL", "Color selection register (for color selection effect)"                  , 0x00000000, 0x0000FF03, 0x0000FF03 },
    { 0U, PERM_READ_WRITE, "IMG_EFF_MAT_1"    , "3x3 matrix coefficients for emboss effect (1)"                          , 0x00008089, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "IMG_EFF_MAT_2"    , "3x3 matrix coefficients for emboss effect (2)"                          , 0x0000C0C0, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "IMG_EFF_MAT_3"    , "3x3 matrix coefficients for emboss(3) effect / sketch/sharpen(1) effect", 0x0000CCC0, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "IMG_EFF_MAT_4"    , "3x3 matrix coefficients for sketch/sharpen effect (2)"                  , 0x0000CCBC, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "IMG_EFF_MAT_5"    , "3x3 matrix coefficients for sketch/sharpen effect (3)"                  , 0x000000CC, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "IMG_EFF_TINT"     , "Chrominance increment values of a tint (used for sepia effect)"         , 0x0000880C, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_ONLY , "IMG_EFF_CTRL_SHD" , "Shadow register for control register"                                   , 0x00000000, 0x0000000E, 0x0000000E },
    { 0U, PERM_READ_WRITE, "IMG_EFF_SHARPEN"  , "Factor and threshold for sharpen effect"                                , 0x00000000, 0x0000FFFF, 0x0000FFFF },
};

#endif /* __CAMERIC_REG_DESCRIPTION_IMG_EFFECTS_H__ */

