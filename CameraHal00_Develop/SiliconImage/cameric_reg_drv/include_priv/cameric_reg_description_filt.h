// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_FILT_H__
#define __CAMERIC_REG_DESCRIPTION_FILT_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_FILT     11
static RegDescription_t FiltRegisterMap[CAMERIC_NUMBER_REGISTER_FILT] =
{
    { 0U, PERM_READ_WRITE, "ISP_FILT_MODE"      , "Mode control register for the filter block", 0x000004F2, 0x00000FF3, 0x00000FF3 },
    { 0U, PERM_READ_WRITE, "ISP_FILT_THRESH_BL0", "Blurring threshold 0"                      , 0x0000000D, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_FILT_THRESH_BL1", "Blurring threshold 1"                      , 0x00000005, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_FILT_THRESH_SH0", "Sharpening threshold 0"                    , 0x0000001A, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_FILT_THRESH_SH1", "Sharpening threshold 1"                    , 0x0000002C, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_FILT_LUM_WEIGHT", "Parameters for luminance weight function"  , 0x00022040, 0x0007FFFF, 0x0007FFFF },
    { 0U, PERM_READ_WRITE, "ISP_FILT_FAC_SH1"   , "Filter factor sharp1"                      , 0x00000010, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_WRITE, "ISP_FILT_FAC_SH0"   , "Filter factor sharp0"                      , 0x0000000C, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_WRITE, "ISP_FILT_FAC_MID"   , "Filter factor middle"                      , 0x0000000A, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_WRITE, "ISP_FILT_FAC_BL0"   , "Parameter for blur 0 filter"               , 0x00000006, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_WRITE, "ISP_FILT_FAC_BL1"   , "Parameter for blur 1 filter"               , 0x00000002, 0x0000003F, 0x0000003F }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_FILT_H__ */

