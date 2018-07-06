// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_HIST_H__
#define __CAMERIC_REG_DESCRIPTION_HIST_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_HIST    28
static RegDescription_t HistRegisterMap[CAMERIC_NUMBER_REGISTER_HIST] =
{
    { 0U, PERM_READ_WRITE, "ISP_HIST_PROP"         , "Histogram properties"                                                 , 0x00000000, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_HIST_H_OFFS"       , "Histogram window horizontal offset for first window of 25 sub-windows", 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "ISP_HIST_V_OFFS"       , "Histogram window vertical offset for first window of 25 sub-windows"  , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_HIST_H_SIZE"       , "Horizontal (sub-)window size"                                         , 0x00000000, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_HIST_V_SIZE"       , "Vertical (sub-)window size"                                           , 0x00000000, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN0"         , "Histogram measurement result bin 0"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN1"         , "Histogram measurement result bin 1"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN2"         , "Histogram measurement result bin 2"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN3"         , "Histogram measurement result bin 3"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN4"         , "Histogram measurement result bin 4"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN5"         , "Histogram measurement result bin 5"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN6"         , "Histogram measurement result bin 6"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN7"         , "Histogram measurement result bin 7"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN8"         , "Histogram measurement result bin 8"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN9"         , "Histogram measurement result bin 9"                                   , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN10"        , "Histogram measurement result bin 10"                                  , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN11"        , "Histogram measurement result bin 11"                                  , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN12"        , "Histogram measurement result bin 12"                                  , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN13"        , "Histogram measurement result bin 13"                                  , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN14"        , "Histogram measurement result bin 14"                                  , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "ISP_HIST_BIN15"        , "Histogram measurement result bin 15"                                  , 0x00000000, 0x000FFFFF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_00TO30", "Weighting factor for sub-windows"                                     , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_40TO21", "Weighting factor for sub-windows"                                     , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_31TO12", "Weighting factor for sub-windows"                                     , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_22TO03", "Weighting factor for sub-windows"                                     , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_13TO43", "Weighting factor for sub-windows"                                     , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_04TO34", "Weighting factor for sub-windows"                                     , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_HIST_WEIGHT_44"    , "Weighting factor for sub-windows"                                     , 0x00000010, 0x0000001F, 0x0000001F }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_HIST_H__ */

