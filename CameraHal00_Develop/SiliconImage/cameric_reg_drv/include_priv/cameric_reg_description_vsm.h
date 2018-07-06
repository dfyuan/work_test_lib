// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_VSM_H__
#define __CAMERIC_REG_DESCRIPTION_VSM_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_VSM     9
static RegDescription_t VsmRegisterMap[CAMERIC_NUMBER_REGISTER_VSM] = 
{
  { 0U, PERM_READ_WRITE, "ISP_VSM_MODE"         , "VS Measure Mode"                  , 0x00000000, 0x00000003, 0x00000003 },
  { 0U, PERM_READ_WRITE, "ISP_VSM_H_OFFS"       , "VSM window horizontal offset"     , 0x00000000, 0x00001FFF, 0x00001FFF },
  { 0U, PERM_READ_WRITE, "ISP_VSM_V_OFFS"       , "VSM window vertical offset"       , 0x00000000, 0x00001FFF, 0x00001FFF },
  { 0U, PERM_READ_WRITE, "ISP_VSM_H_SIZE"       , "Horizontal measure window size"   , 0x00000000, 0x000007FE, 0x000007FE },
  { 0U, PERM_READ_WRITE, "ISP_VSM_V_SIZE"       , "Vertical measure window size"     , 0x00000000, 0x000007FE, 0x000007FE },
  { 0U, PERM_READ_WRITE, "ISP_VSM_H_SEGMENTS"   , "Iteration 1 horizontal segments"  , 0x00000000, 0x000000FF, 0x000000FF },
  { 0U, PERM_READ_WRITE, "ISP_VSM_V_SEGMENTS"   , "Iteration 1 vertical segments"    , 0x00000000, 0x000000FF, 0x000000FF },
  { 0U, PERM_READ_ONLY , "ISP_VSM_DELTA_H"      , "estimated horizontal displacement", 0x00000000, 0x00000FFF, 0x00000000 },
  { 0U, PERM_READ_ONLY , "ISP_VSM_DELTA_V"      , "estimated vertical displacement"  , 0x00000000, 0x00000FFF, 0x00000000 }
};

#endif /* __CAMERIC_REG_DESCRIPTION_VSM_H__ */

