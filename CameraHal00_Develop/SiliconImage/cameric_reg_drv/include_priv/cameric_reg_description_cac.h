// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_CAC_H__
#define __CAMERIC_REG_DESCRIPTION_CAC_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_CAC     7
static RegDescription_t CacRegisterMap[CAMERIC_NUMBER_REGISTER_CAC] =
{
    { 0U, PERM_READ_WRITE, "ISP_CAC_CTRL"       , "Control register for chromatic aberration correction"                                       , 0x00000000, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "ISP_CAC_COUNT_START", "Preload values for CAC pixel and line counter"                                              , 0x03CF0514, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_CAC_A"     		, "Linear Parameters for radial shift calculation"                                             , 0x00000000, 0x01FF01FF, 0x0FFF01FF },
    { 0U, PERM_READ_WRITE, "ISP_CAC_B"     		, "Square Parameters for radial shift calculation"                                             , 0x00000000, 0x01FF01FF, 0x0FFF01FF },
    { 0U, PERM_READ_WRITE, "ISP_CAC_C"     		, "Cubical Parameters for radial shift calculatio"                                             , 0x00000000, 0x01FF01FF, 0x0FFF01FF },
    { 0U, PERM_READ_WRITE, "ISP_CAC_X_NORM"     , "Normalization parameters for calculation of image coordinate x_d relative to optical center", 0x00060014, 0x0007001F, 0x0007001F },
    { 0U, PERM_READ_WRITE, "ISP_CAC_Y_NORM"     , "Normalization parameters for calculation of image coordinate y_d relative to optical center", 0x00060014, 0x0007001F, 0x0007001F }
   
};

#endif /* ___CAMERIC_REG_DESCRIPTION_CAC_H__ */

