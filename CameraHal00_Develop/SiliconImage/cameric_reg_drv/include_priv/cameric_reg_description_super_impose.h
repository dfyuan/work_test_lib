// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_SUPER_IMPOSE_H__
#define __CAMERIC_REG_DESCRIPTION_SUPER_IMPOSE_H__

#include <ebase/types.h>

static RegDescription_t SuperImposeRegisterMap[] = 
{
    { 0U, PERM_READ_WRITE, "SUPER_IMP_CTRL"    , "Global control register"               , 0x00000000, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "SUPER_IMP_OFFSET_X", "Offset x register"                    , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "SUPER_IMP_OFFSET_Y", "Offset y register"                    , 0x00000000, 0x000007FF, 0x000007FF },
    { 0U, PERM_READ_WRITE, "SUPER_IMP_COLOR_Y" , "Y component of transparent key color" , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "SUPER_IMP_COLOR_CB", "Cb component of transparent key color", 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "SUPER_IMP_COLOR_CR", "Cr component of transparent key color", 0x00000000, 0x000000FF, 0x000000FF },
};

#endif /* __CAMERIC_REG_DESCRIPTION_SUPER_IMPOSE_H__ */

