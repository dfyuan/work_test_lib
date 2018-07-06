// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_SRSZ_H__
#define __CAMERIC_REG_DESCRIPTION_SRSZ_H__

#include <ebase/types.h>

static RegDescription_t SrszRegisterMap[] = 
{
    { 0U, PERM_READ_WRITE, "SRSZ_CTRL"          , "Global control register"                          , 0x00000000, 0x000001FF, 0x000001FF },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_HY"      , "Horizontal luminance scale factor register"       , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_HCB"     , "Horizontal Cb scale factor register"              , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_HCR"     , "Horizontal Cr scale factor register"              , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_VY"      , "Vertical luminance scale factor register"         , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_VC"      , "Vertical chrominance scale factor register"       , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_PHASE_HY"      , "Horizontal luminance phase register"              , 0x00000000, 0x00003FFF, 0x00003FFF }, 
    { 0U, PERM_READ_WRITE, "SRSZ_PHASE_HC"      , "Horizontal chrominance phase register"            , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_PHASE_VY"      , "Vertical luminance phase register"                , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_PHASE_VC"      , "Vertical chrominance phase register"              , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_LUT_ADDR", "Address pointer of up-scaling look up table"      , 0x00000000, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_WRITE, "SRSZ_SCALE_LUT"     , "Entry of up-scaling look up table"                , 0x00000000, 0x0000003F, 0x0000003F },
    { 0U, PERM_READ_ONLY , "SRSZ_CTRL_SHD"      , "Global control shadow register"                   , 0x00000000, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_ONLY , "SRSZ_SCALE_HY_SHD"  , "Horizontal luminance scale factor shadow register", 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_SCALE_HCB_SHD" , "Horizontal Cb scale factor shadow register"       , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_SCALE_HCR_SHD" , "Horizontal Cr scale factor shadow register"       , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_SCALE_VY_SHD"  , "Vertical luminance scale factor shadow register"  , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_SCALE_VC_SHD"  , "Vertical chrominance scale factor shadow register", 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_PHASE_HY_SHD"  , "Horizontal luminance phase shadow register"       , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_PHASE_HC_SHD"  , "Horizontal chrominance phase shadow register"     , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_PHASE_VY_SHD"  , "Vertical luminance phase shadow register"         , 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_ONLY , "SRSZ_PHASE_VC_SHD"  , "Vertical chrominance phase shadow register"       , 0x00000000, 0x00003FFF, 0x00003FFF }
};

#endif

