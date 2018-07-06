// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_JPE_H__
#define __CAMERIC_REG_DESCRIPTION_JPE_H__

#include <ebase/types.h>

static RegDescription_t JpeRegisterMap[] =
{
    { 0U, PERM_WRITE_ONLY, "JPE_GEN_HEADER"      , "command to start stream header generation"        , 0x00000000, 0x00000000, 0x00000001 },
    { 0U, PERM_READ_WRITE, "JPE_ENCODE"          , "Start command to start JFIF stream encoding"      , 0x00000000, 0x00000001, 0x00000001 },
    { 0U, PERM_WRITE_ONLY, "JPE_INIT"            , "Automatic configuration update (INIT)"            , 0x00000000, 0x00000000, 0x00000001 },
    { 0U, PERM_READ_WRITE, "JPE_Y_SCALE_EN"      , "Y value scaling control register"                 , 0x00000000, 0x00000001, 0x00000001 },
    { 0U, PERM_READ_WRITE, "JPE_CBCR_SCALE_EN"   , "Cb/Cr value scaling control register"             , 0x00000000, 0x00000001, 0x00000001 },
    { 0U, PERM_READ_WRITE, "JPE_TABLE_FLUSH"     , "header generation debug register"                 , 0x00000000, 0x00000001, 0x00000001 },
    { 0U, PERM_READ_WRITE, "JPE_ENC_HSIZE"       , "JPEG codec horizontal image size for encoding"    , 0x00000000, 0x00001FFF, 0x00001FFF },
    { 0U, PERM_READ_WRITE, "JPE_ENC_VSIZE"       , "JPEG codec vertical image size for encoding"      , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "JPE_PIC_FORMAT"      , "JPEG picture encoding format"                     , 0x00000001, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "JPE_RESTART_INTERVAL", "restart marker insertion register"                , 0x00000000, 0x0000FFFF, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "JPE_TQ_Y_SELECT"     , "Q- table selector 0, quant. table for Y component", 0x00000000, 0x00000003, 0x00000003 },
    { 0U, PERM_READ_WRITE, "JPE_TQ_U_SELECT"     , "Q- table selector 1, quant. table for U component", 0x00000001, 0x00000003, 0x00000003 },
    { 0U, PERM_READ_WRITE, "JPE_TQ_V_SELECT"     , "Q- table selector 2, quant. table for V component", 0x00000001, 0x00000003, 0x00000003 },
    { 0U, PERM_READ_WRITE, "JPE_DC_TABLE_SELECT" , "Huffman table selector for DC values"             , 0x00000006, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "JPE_AC_TABLE_SELECT" , "Huffman table selector for AC values"             , 0x00000006, 0x00000007, 0x00000007 },
    { 0U, PERM_WRITE_ONLY, "JPE_TABLE_DATA"      , "table programming register"                       , 0x00000000, 0x00000000, 0x0000FFFF },
    { 0U, PERM_READ_WRITE, "JPE_TABLE_ID"        , "table programming select register"                , 0x00000000, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "JPE_TAC0_LEN"        , "Huffman AC table 0 length"                        , 0x000000B2, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "JPE_TDC0_LEN"        , "Huffman DC table 0 length"                        , 0x0000001C, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "JPE_TAC1_LEN"        , "Huffman AC table 1 length"                        , 0x000000B2, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "JPE_TDC1_LEN"        , "Huffman DC table 1 length"                        , 0x0000001C, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_ONLY,  "JPE_ENCODER_BUSY"    , "encoder status flag"                              , 0x00000000, 0x00000001, 0x00000000 },
    { 0U, PERM_READ_WRITE, "JPE_HEADER_MODE"     , "header mode definition"                           , 0x00000000, 0x00000003, 0x00000003 },
    { 0U, PERM_READ_ONLY,  "JPE_ENCODE_MODE"     , "encode mode"                                      , 0x00000001, 0x00000001, 0x00000000 },
    { 0U, PERM_READ_ONLY,  "JPE_DEBUG"           , "debug information register"                       , 0x00000000, 0x0000013C, 0x00000000 },
    { 0U, PERM_READ_WRITE, "JPE_ERROR_IMR"       , "JPEG error interrupt mask register"               , 0x00000000, 0x00000690, 0x00000690 },
    { 0U, PERM_READ_ONLY,  "JPE_ERROR_RIS"       , "JPEG error raw interrupt status register"         , 0x00000000, 0x00000690, 0x00000000 },
    { 0U, PERM_READ_ONLY,  "JPE_ERROR_MIS"       , "JPEG error masked interrupt status register"      , 0x00000000, 0x00000690, 0x00000000 },
    { 0U, PERM_WRITE_ONLY, "JPE_ERROR_ICR"       , "JPEG error interrupt set register"                , 0x00000000, 0x00000000, 0x00000690 },
    { 0U, PERM_WRITE_ONLY, "JPE_ERROR_ISR"       , "JPEG error interrupt clear register"              , 0x00000000, 0x00000000, 0x00000690 },
    { 0U, PERM_READ_WRITE, "JPE_STATUS_IMR"      , "JPEG status interrupt mask register"              , 0x00000000, 0x00000030, 0x00000030 },
    { 0U, PERM_READ_ONLY,  "JPE_STATUS_RIS"      , "JPEG status raw interrupt status register"        , 0x00000000, 0x00000030, 0x00000000 },
    { 0U, PERM_READ_ONLY,  "JPE_STATUS_MIS"      , "JPEG status masked interrupt status register"     , 0x00000000, 0x00000030, 0x00000000 },
    { 0U, PERM_WRITE_ONLY, "JPE_STATUS_ICR"      , "JPEG status interrupt clear register"             , 0x00000000, 0x00000000, 0x00000030 },
    { 0U, PERM_WRITE_ONLY, "JPE_STATUS_ISR"      , "JPEG status interrupt set register"               , 0x00000000, 0x00000000, 0x00000030 },
};

#endif /* ___CAMERIC_REG_DESCRIPTION_JPE_H__ */

