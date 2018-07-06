// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_ISP_SHUTTER_H__
#define __CAMERIC_REG_DESCRIPTION_ISP_SHUTTER_H__

#include <ebase/types.h>

static RegDescription_t IspShutterRegisterMap[] = 
{
    { 0U, PERM_READ_WRITE, "ISP_SH_CTRL"  , "Mechanical Shutter Control"            , 0x00000000, 0x0000001F, 0x0000001F },
    { 0U, PERM_READ_WRITE, "ISP_SH_PREDIV", "Mech. Shutter Counter Pre-DividerFlash", 0x00000000, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_SH_DELAY" , "Delay register"                        , 0x00000000, 0x0003FFFF, 0x0003FFFF },
    { 0U, PERM_READ_WRITE, "ISP_SH_TIME"  , "Time register"                         , 0x00000000, 0x0003FFFF, 0x0003FFFF },
};

#endif /* __CAMERIC_REG_DESCRIPTION_ISP_SHUTTER_H__ */

