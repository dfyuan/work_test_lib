// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_BP_H__
#define __CAMERIC_REG_DESCRIPTION_BP_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_BP      15
static RegDescription_t BpRegisterMap[CAMERIC_NUMBER_REGISTER_BP] =
{
    { 0U, PERM_READ_WRITE, "ISP_BP_CTRL"       , "Control register for bad pixel detection and correction"       , 0x00000000, 0x00000005, 0x00000005 },
    { 0U, PERM_READ_WRITE, "ISP_BP_CFG1"       , "Thresholds for on-the-fly and table based detection"           , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_BP_CFG2"       , "Deviation thresholds for on-the-fly detection / correction"    , 0x00000000, 0x0FFF0FFF, 0x0FFF0FFF },
    { 0U, PERM_READ_WRITE, "ISP_BP_NUMBER"     , "Number of entries for bad pixel table (table based correction)", 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_BP_TABLE_ADDR" , "TABLE Address for table-based correction algorithm"            , 0x00000000, 0x0000007F, 0x0000007F },
    { 0U, PERM_READ_WRITE, "ISP_BP_TABLE_DATA" , "TABLE DATA register for read and write access of table RAM"    , 0x00000000, 0x87FF0FFF, 0x87FF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_NUMBER" , "Number of entries in new bad pixel table"                      , 0x00000000, 0x0000000F, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=0"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=1"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=2"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=3"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=4"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=5"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=6"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF },
    { 0U, PERM_READ_ONLY , "ISP_BP_NEW_TABLE"  , "New bad pixel table of table-based detection algorithm n=7"    , 0x00000000, 0xFFFF0FFF, 0xFFFF0FFF }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_BP_H__ */

