// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_CPROC_H__
#define __CAMERIC_REG_DESCRIPTION_CPROC_H__

#include <ebase/types.h>

static RegDescription_t CprocRegisterMap[] = 
{
    { 0U, PERM_READ_WRITE, "CPROC_CTRL"      , "Global control register"             , 0x00000000, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "CPROC_CONTRAST"  , "Color Processing contrast register"  , 0x00000080, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "CPROC_BRIGHTNESS", "Color Processing brightness register", 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "CPROC_SATURATION", "Color Processing saturation register", 0x00000080, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "CPROC_HUE"       , "Color Processing hue register"       , 0x00000000, 0x000000FF, 0x000000FF },
};

#endif /* __CAMERIC_REG_DESCRIPTION_CPROC_H__ */

