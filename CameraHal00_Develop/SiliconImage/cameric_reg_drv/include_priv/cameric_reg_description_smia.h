// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_SMIA_H__
#define __CAMERIC_REG_DESCRIPTION_SMIA_H__

#include <ebase/types.h>

static RegDescription_t SmiaRegisterMap[] =
{
    { 0U, PERM_READ_WRITE, "SMIA_CTRL"              , "Global control register"                    , 0x00000000, 0x000007FF, 0x000007FF },
    { 0U, PERM_READ_ONLY , "SMIA_STATUS"            , "Global status register"                     , 0x00000000, 0x00000701, 0x00000000 },
    { 0U, PERM_READ_WRITE, "SMIA_IMSC"              , "Interrupt mask"                             , 0x00000000, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_ONLY , "SMIA_RIS"               , "Raw interrupt status"                       , 0x00000010, 0x0000003F, 0x00000000 },
    { 0U, PERM_READ_ONLY , "SMIA_MIS"               , "Masked interrupt status"                    , 0x00000000, 0x0000003F, 0x00000000 },
    { 0U, PERM_WRITE_ONLY, "SMIA_ICR"               , "Interrupt clear register"                   , 0x00000000, 0x00000000, 0x0000003F },
    { 0U, PERM_WRITE_ONLY, "SMIA_ISR"               , "Interrupt set register"                     , 0x00000000, 0x00000000, 0x0000003F },
    { 0U, PERM_READ_WRITE, "SMIA_DATA_FORMAT_SEL"   , "Data format selector register"              , 0x00000000, 0x0000000F, 0x0000000F },
    { 0U, PERM_READ_WRITE, "SMIA_SOF_EMB_DATA_LINES", "Start of frame embedded data lines register", 0x00000000, 0x00000007, 0x00000007 },
    { 0U, PERM_READ_WRITE, "SMIA_EMB_HSTART"        , "Embedded data hstart register"              , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SMIA_EMB_HSIZE"         , "Embedded data hsize register"               , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SMIA_EMB_VSTART"        , "Embedded data vstart register"              , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "SMIA_NUM_LINES"         , "Image data lines register"                  , 0x00000000, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_ONLY , "SMIA_EMB_DATA_FIFO"     , "Embedded Data Fifo"                         , 0x00000000, 0xFFFFFFFF, 0x00000000 },
    { 0U, PERM_READ_WRITE, "SMIA_FIFO_FILL_LEVEL"   , "Embedded Data FIFO Fill Level"              , 0x00000000, 0x000003FF, 0x000003FF }
};

#endif /* ___CAMERIC_REG_DESCRIPTION_SMIA_H__ */

