// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_LSC_H__
#define __CAMERIC_REG_DESCRIPTION_LSC_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_LSC     24
static RegDescription_t LscRegisterMap[CAMERIC_NUMBER_REGISTER_LSC] =
{
    { 0U, PERM_READ_WRITE, "ISP_LSC_CTRL"        , "Lens shade control"                   , 0x00000000, 0x00000000, 0x00000000 },
    { 0U, PERM_READ_WRITE, "ISP_LSC_R_TABLE_ADDR", "Table RAM Address for red component"  , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_G_TABLE_ADDR", "Table RAM Address for green component", 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_B_TABLE_ADDR", "Table RAM Address for blue component" , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_R_TABLE_DATA", "Sample table red"                     , 0x00000000, 0x00FFFFFF, 0x00FFFFFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_G_TABLE_DATA", "Sample table green"                   , 0x00000000, 0x00FFFFFF, 0x00FFFFFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_B_TABLE_DATA", "Sample table blue"                    , 0x00000000, 0x00FFFFFF, 0x00FFFFFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_STATUS"      , "Lens shade status"                    , 0x00000000, 0x00000000, 0x00000000 },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XGRAD_01"    , "Gradient table x"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XGRAD_23"    , "Gradient table x"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XGRAD_45"    , "Gradient table x"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XGRAD_67"    , "Gradient table x"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YGRAD_01"    , "Gradient table y"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YGRAD_23"    , "Gradient table y"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YGRAD_45"    , "Gradient table y"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YGRAD_67"    , "Gradient table y"                     , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XSIZE_01"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XSIZE_23"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XSIZE_45"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_XSIZE_67"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YSIZE_01"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YSIZE_23"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YSIZE_45"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF },
    { 0U, PERM_READ_WRITE, "ISP_LSC_YSIZE_67"    , "Size table"                           , 0x00000000, 0x03FF03FF, 0x03FF03FF }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_LSC_H__ */

