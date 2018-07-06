// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_EXP_H__
#define __CAMERIC_REG_DESCRIPTION_EXP_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_EXP     30
static RegDescription_t ExpRegisterMap[CAMERIC_NUMBER_REGISTER_EXP] =
{
    { 0U, PERM_READ_WRITE, "ISP_EXP_CTRL"    , "Exposure control"                 , 0x00000000, 0x80000003, 0x80000003 },
    { 0U, PERM_READ_WRITE, "ISP_EXP_H_OFFSET", "Horizontal offset for first block", 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_EXP_V_OFFSET", "Vertical offset for first block"  , 0x00000000, 0x000007FF, 0x000007FF },
    { 0U, PERM_READ_WRITE, "ISP_EXP_H_SIZE"  , "Horizontal size of one block"     , 0x00000000, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_EXP_V_SIZE"  , "Vertical size of one block"       , 0x00000000, 0x000001FF, 0x000001FF },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_00" , "Mean luminance value of block 00" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_10" , "Mean luminance value of block 10" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_20" , "Mean luminance value of block 20" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_30" , "Mean luminance value of block 30" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_40" , "Mean luminance value of block 40" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_01" , "Mean luminance value of block 01" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_11" , "Mean luminance value of block 11" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_21" , "Mean luminance value of block 21" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_31" , "Mean luminance value of block 31" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_41" , "Mean luminance value of block 41" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_02" , "Mean luminance value of block 02" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_12" , "Mean luminance value of block 12" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_22" , "Mean luminance value of block 22" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_32" , "Mean luminance value of block 32" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_42" , "Mean luminance value of block 42" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_03" , "Mean luminance value of block 03" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_13" , "Mean luminance value of block 13" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_23" , "Mean luminance value of block 23" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_33" , "Mean luminance value of block 33" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_43" , "Mean luminance value of block 43" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_04" , "Mean luminance value of block 04" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_14" , "Mean luminance value of block 14" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_24" , "Mean luminance value of block 24" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_34" , "Mean luminance value of block 34" , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_EXP_MEAN_44" , "Mean luminance value of block 44" , 0x00000000, 0x000000FF, 0x00000000 }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_EXP_H__ */

