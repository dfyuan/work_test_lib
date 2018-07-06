// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_BLS_H__
#define __CAMERIC_REG_DESCRIPTION_BLS_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_BLS     18
static RegDescription_t BlsRegisterMap[CAMERIC_NUMBER_REGISTER_BLS] = 
{
    { 0U, PERM_READ_WRITE, "ISP_BLS_CTRL"      , "Global control register"  , 0x00000000, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "ISP_BLS_SAMPLES"   , "Samples register"         , 0x00000000, 0x0000001F, 0x0000001F },
    { 0U, PERM_READ_WRITE, "ISP_BLS_H1_START"  , "Window 1 horizontal start", 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_H1_STOP"   , "Window 1 horizontal stop" , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_V1_START"  , "Window 1 vertical start"  , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_V1_STOP"   , "Window 1 vertical stop"   , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_H2_START"  , "Window 2 horizontal start", 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_H2_STOP"   , "Window 2 horizontal stop" , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_V2_START"  , "Window 2 vertical start"  , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_V2_STOP"   , "Window 2 vertical stop"   , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_A_FIXED"   , "Fixed black level A"      , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_B_FIXED"   , "Fixed black level B"      , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_C_FIXED"   , "Fixed black level C"      , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "ISP_BLS_D_FIXED"   , "Fixed black level D"      , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_ONLY , "ISP_BLS_A_MEASURED", "Measured black level A"   , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_ONLY , "ISP_BLS_B_MEASURED", "Measured black level B"   , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_ONLY , "ISP_BLS_C_MEASURED", "Measured black level C"   , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_ONLY , "ISP_BLS_D_MEASURED", "Measured black level D"   , 0x00000000, 0x00000FFF, 0x00000FFF }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_BLS_H__ */

