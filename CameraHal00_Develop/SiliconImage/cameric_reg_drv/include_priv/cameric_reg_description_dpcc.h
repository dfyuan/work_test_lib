// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_DPCC_H__
#define __CAMERIC_REG_DESCRIPTION_DPCC_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_DPCC     27
static RegDescription_t DpccRegisterMap[CAMERIC_NUMBER_REGISTER_DPCC] = 
{
    { 0U, PERM_READ_WRITE, "ISP_DPCC_MODE"          , "Mode control for DPCC detection unit"                          , 0x00000004, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_OUTPUT_MODE"   , "Interpolation mode for correction unit"                        , 0x00000000, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_SET_USE"       , "DPCC methods set usage for detection"                          , 0x00000001, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_METHODS_SET_1" , "Methods enable bits for SET_1"                                 , 0x00001D1D, 0x00001F1F, 0x00001F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_METHODS_SET_2" , "Methods enable bits for SET_2"                                 , 0x00000000, 0x00001F1F, 0x00001F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_METHODS_SET_3" , "Methods enable bits for SET_3"                                 , 0x00000000, 0x00001F1F, 0x00001F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_LINE_THRESH_1" , "Line threshold SET_1"                                          , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_LINE_MAD_FAC_1", "Mean Absolute Difference (MAD) factor for Line check set 1"    , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_PG_FAC_1"      , "Peak gradient factor for set 1"                                , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RND_THRESH_1"  , "Rank Neighbor Difference threshold for set 1"                  , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RG_FAC_1"      , "Rank gradient factor for set 1"                                , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_LINE_THRESH_2" , "Line threshold SET_2"                                          , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_LINE_MAD_FAC_2", "Mean Absolute Difference (MAD) factor for Line check set 2"    , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_PG_FAC_2"      , "Peak gradient factor for set 2"                                , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RND_THRESH_2"  , "Rank Neighbor Difference threshold for set 2"                  , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RG_FAC_2"      , "Rank gradient factor for set 2"                                , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_LINE_THRESH_3" , "Line threshold SET_3"                                          , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_LINE_MAD_FAC_3", "Mean Absolute Difference (MAD) factor for Line check set 3"    , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_PG_FAC_3"      , "Peak gradient factor for set 3"                                , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RND_THRESH_3"  , "Rank Neighbor Difference threshold for set 3"                  , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RG_FAC_3"      , "Rank gradient factor for set 3"                                , 0x00000000, 0x00003F3F, 0x00003F3F },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RO_LIMITS"     , "Rank Order Limits"                                             , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_RND_OFFS"      , "Differential Rank Offsets for Rank Neighbor Difference"        , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_BPT_CTRL"      , "Bad pixel table settings"                                      , 0x00000000, 0x00000FF3, 0x00000FF3 },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_BPT_NUMBER"    , "Number of entries for bad pixel table (table based correction)", 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_BPT_ADDR"      , "TABLE Start Address for table-based correction algorithm"      , 0x00000000, 0x000007FF, 0x000007FF },
    { 0U, PERM_READ_WRITE, "ISP_DPCC_BPT_DATA"      , "TABLE DATA register for read and write access of table RAM"    , 0x00000000, 0x0FFF1FFF, 0x0FFF1FFF }
};

#endif /* __CAMERIC_REG_DESCRIPTION_DPCC_H__ */

