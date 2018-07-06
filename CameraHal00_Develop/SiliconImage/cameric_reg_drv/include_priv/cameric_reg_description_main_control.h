// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_MAIN_CONTROL_H__
#define __CAMERIC_REG_DESCRIPTION_MAIN_CONTROL_H__

#include <ebase/types.h>

static RegDescription_t MainControlRegisterMap[] =
{
    { 0U, PERM_READ_WRITE, "VI_CCL" , "Clock control register"          , 0x00000000, 0x00000006, 0x00000060 },
    { 0U, PERM_READ_ONLY , "VI_ID"  , "Revision identification register", 0x20353010, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "VI_ICCL", "Internal clock control register" , 0x00000F7B, 0x00000F7B, 0x00000F7B },
    { 0U, PERM_READ_WRITE, "VI_IRCL", "Internal reset control register" , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "VI_DPCL", "Data path control register"      , 0x00000000, 0x00000F7F, 0x00000F7F },
};

#endif /* __CAMERIC_REG_DESCRIPTION_MAIN_CONTROL_H__ */

