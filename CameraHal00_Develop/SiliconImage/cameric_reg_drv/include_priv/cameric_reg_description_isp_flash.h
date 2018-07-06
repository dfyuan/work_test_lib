// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_ISP_FLASH_H__
#define __CAMERIC_REG_DESCRIPTION_ISP_FLASH_H__

#include <ebase/types.h>

static RegDescription_t IspFlashRegisterMap[] = 
{
    { 0U, PERM_WRITE_ONLY, "ISP_FLASH_CMD"   , "Flash command"                      , 0x00000000, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "ISP_FLASH_CONFIG", "Flash config"                       , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_FLASH_PREDIV", "Flash Counter Pre-Divider"          , 0x00000000, 0x000001FF, 0x000001FF },
    { 0U, PERM_READ_WRITE, "ISP_FLASH_DELAY" , "Flash DelayY"                       , 0x00000000, 0x0003FFFF, 0x0003FFFF },
    { 0U, PERM_READ_WRITE, "ISP_FLASH_TIME"  , "Flash time"                         , 0x00000000, 0x0003FFFF, 0x0003FFFF },
    { 0U, PERM_READ_WRITE, "ISP_FLASH_MAXP"  , "Maximum value for flash or preflash", 0x00000000, 0x0000FFFF, 0x0000FFFF },
};

#endif /* __CAMERIC_REG_DESCRIPTION_ISP_FLASH_H__ */

