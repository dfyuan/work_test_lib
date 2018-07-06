// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_IS_H__
#define __CAMERIC_REG_DESCRIPTION_IS_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_IS     13
static RegDescription_t IsRegisterMap[CAMERIC_NUMBER_REGISTER_IS] =
{
    { 0U, PERM_READ_WRITE, "ISP_IS_CTRL"      , "Image Stabilization Control Register"                        , 0x00000000, 0x00000001, 0x00000001 },
    { 0U, PERM_READ_WRITE, "ISP_IS_RECENTER"  , "Recenter register"                                           , 0x00000000, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "ISP_IS_H_OFFS"    , "Horizontal offset of output window"                          , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "ISP_IS_V_OFFS"    , "Vertical offset of output window"                            , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_IS_H_SIZE"    , "Output horizontal picture size"                              , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "ISP_IS_V_SIZE"    , "Output vertical picture size"                                , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_IS_MAX_DX"    , "Maximum Horizontal Displacement"                             , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_IS_MAX_DY"    , "Maximum Vertical Displacement"                               , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_IS_DISPLACE"  , "Camera displacement"                                         , 0x00000000, 0x0FFF0FFF, 0x0FFF0000 },
    { 0U, PERM_READ_ONLY , "ISP_IS_H_OFFS_SHD", "current horizontal offset of output window (shadow register)", 0x00000000, 0x00001FFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_IS_V_OFFS_SHD", "current vertical offset of output window (shadow register)"  , 0x00000000, 0x00000FFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_IS_H_SIZE_SHD", "current output horizontal picture size (shadow register)"    , 0x00000000, 0x00001FFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_IS_V_SIZE_SHD", "current output vertical picture size (shadow register)"      , 0x00000000, 0x00000FFF, 0x00000000 }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_IS_H__ */

