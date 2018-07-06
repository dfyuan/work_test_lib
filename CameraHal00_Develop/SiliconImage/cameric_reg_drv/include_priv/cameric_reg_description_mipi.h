// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_MIPI_H__
#define __CAMERIC_REG_DESCRIPTION_MIPI_H__

#include <ebase/types.h>

static RegDescription_t MipiRegisterMap[] =
{
    { 0U, PERM_READ_WRITE, "MIPI_CTRL"               , "global control register"                                      , 0x00063000, 0x00073F03, 0x00073F03 },
    { 0U, PERM_READ_ONLY , "MIPI_STATUS"             , "global status register"                                       , 0x00000000, 0x00007E01, 0x00000000 },
    { 0U, PERM_READ_WRITE, "MIPI_IMSC"               , "Interrupt mask"                                               , 0x00000000, 0x0FFFFFFF, 0x0FFFFFFF },
    { 0U, PERM_READ_ONLY , "MIPI_RIS"                , "Raw interrupt status"                                         , 0x00000000, 0x0FFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_MIS"                , "Masked interrupt status"                                      , 0x00000000, 0x0FFFFFFF, 0x00000000 },
    { 0U, PERM_WRITE_ONLY, "MIPI_ICR"                , "Interrupt clear register"                                     , 0x00000000, 0x00000000, 0x0FFFFFFF },
    { 0U, PERM_WRITE_ONLY, "MIPI_ISR"                , "Interrupt set register"                                       , 0x00000000, 0x00000000, 0x0FFFFFFF },
    { 0U, PERM_READ_ONLY , "MIPI_CUR_DATA_ID"        , "Current Data Identifier"                                      , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "MIPI_IMG_DATA_SEL"       , "Image Data Selector"                                          , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_1"     , "Additional Data Selector 1"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_2"     , "Additional Data Selector 2"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_3"     , "Additional Data Selector 3"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_4"     , "Additional Data Selector 4"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_ONLY , "MIPI_ADD_DATA_FIFO"      , "Additional Data Fifo"                                         , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_FILL_LEVEL", "Additional Data FIFO Fill Level"                              , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "MIPI_COMPRESSED_MODE"    , "controls processing of compressed raw data types"             , 0x00000000, 0x00000171, 0x00000171 },
    { 0U, PERM_READ_ONLY , "MIPI_FRAME"              , "frame number from frame start and frame end short packets"    , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_DT"       , "data type flags for received generic short packets"           , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_8_9"      , "data field for generic short packets of data type 0x8 and 0x9", 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_A_B"      , "data field for generic short packets of data type 0xA and 0xB", 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_C_D"      , "data field for generic short packets of data type 0xC and 0xD", 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_E_F"      , "data field for generic short packets of data type 0xE and 0xF", 0x00000000, 0xFFFFFFFF, 0x00000000 }
};

static RegDescription_t MipiRegisterMap2[] =
{
    { 0U, PERM_READ_WRITE, "MIPI_CTRL"               , "global control register"                                      , 0x00063000, 0x00073F03, 0x00073F03 },
    { 0U, PERM_READ_ONLY , "MIPI_STATUS"             , "global status register"                                       , 0x00000000, 0x00007E01, 0x00000000 },
    { 0U, PERM_READ_WRITE, "MIPI_IMSC"               , "Interrupt mask"                                               , 0x00000000, 0x0FFFFFFF, 0x0FFFFFFF },
    { 0U, PERM_READ_ONLY , "MIPI_RIS"                , "Raw interrupt status"                                         , 0x00000000, 0x0FFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_MIS"                , "Masked interrupt status"                                      , 0x00000000, 0x0FFFFFFF, 0x00000000 },
    { 0U, PERM_WRITE_ONLY, "MIPI_ICR"                , "Interrupt clear register"                                     , 0x00000000, 0x00000000, 0x0FFFFFFF },
    { 0U, PERM_WRITE_ONLY, "MIPI_ISR"                , "Interrupt set register"                                       , 0x00000000, 0x00000000, 0x0FFFFFFF },
    { 0U, PERM_READ_ONLY , "MIPI_CUR_DATA_ID"        , "Current Data Identifier"                                      , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "MIPI_IMG_DATA_SEL"       , "Image Data Selector"                                          , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_1"     , "Additional Data Selector 1"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_2"     , "Additional Data Selector 2"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_3"     , "Additional Data Selector 3"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_SEL_4"     , "Additional Data Selector 4"                                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_ONLY , "MIPI_ADD_DATA_FIFO"      , "Additional Data Fifo"                                         , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "MIPI_ADD_DATA_FILL_LEVEL", "Additional Data FIFO Fill Level"                              , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "MIPI_COMPRESSED_MODE"    , "controls processing of compressed raw data types"             , 0x00000000, 0x00000171, 0x00000171 },
    { 0U, PERM_READ_ONLY , "MIPI_FRAME"              , "frame number from frame start and frame end short packets"    , 0x00000000, 0x000000FF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_DT"       , "data type flags for received generic short packets"           , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_8_9"      , "data field for generic short packets of data type 0x8 and 0x9", 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_A_B"      , "data field for generic short packets of data type 0xA and 0xB", 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_C_D"      , "data field for generic short packets of data type 0xC and 0xD", 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_ONLY , "MIPI_GEN_SHORT_E_F"      , "data field for generic short packets of data type 0xE and 0xF", 0x00000000, 0xFFFFFFFF, 0x00000000 }
};


#endif /* ___CAMERIC_REG_DESCRIPTION_MIPI_H__ */

