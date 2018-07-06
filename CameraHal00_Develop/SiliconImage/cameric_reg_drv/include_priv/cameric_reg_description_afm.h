// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_AFM_H__
#define __CAMERIC_REG_DESCRIPTION_AFM_H__

#include <ebase/types.h>

static RegDescription_t AfmRegisterMap[] =
{
    { 0U, PERM_READ_WRITE, "ISP_AFM_CTRL"     , "This is the control register for AF measurement unit", 0x00000000, 0x00000001, 0x00000001 },
    { 0U, PERM_READ_WRITE, "ISP_AFM_LT_A"     , "Top Left corner of measure window A"                 , 0x00000000, 0x0FFF07FF, 0x0FFF07FF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_RB_A"     , "Bottom right corner of measure window A"             , 0x00000000, 0x0FFF07FF, 0x0FFF07FF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_LT_B"     , "Top left corner of measure window B"                 , 0x00000000, 0x0FFF07FF, 0x0FFF07FF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_RB_B"     , "Bottom right corner of measure window B"             , 0x00000000, 0x0FFF07FF, 0x0FFF07FF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_LT_C"     , "Top left corner of measure window C"                 , 0x00000000, 0x0FFF07FF, 0x0FFF07FF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_RB_C"     , "Bottom right corner of measure window C"             , 0x00000000, 0x0FFF07FF, 0x0FFF07FF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_THRES"    , "Threshold register"                                  , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_AFM_VAR_SHIFT", "Variable shift register"                             , 0x00000000, 0x00070007, 0x00070007 },
    { 0U, PERM_READ_ONLY , "ISP_AFM_SUM_A"    , "Sharpness Value Status Register of Window A"         , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_AFM_SUM_B"    , "Sharpness Value Status Register of Window B"         , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_AFM_SUM_C"    , "Sharpness Value Status Register of Window C"         , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_AFM_LUM_A"    , "Luminance Value Status Register of Window A"         , 0x00000000, 0x00FFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_AFM_LUM_B"    , "Luminance Value Status Register of Window B"         , 0x00000000, 0x00FFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_AFM_LUM_C"    , "Luminance Value Status Register of Window C"         , 0x00000000, 0x00FFFFFF, 0x00000000 }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_AFM_H__ */

