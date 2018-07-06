// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_DPF_H__
#define __CAMERIC_REG_DESCRIPTION_DPF_H__

#include <ebase/types.h>

#define CAMERIC_NUMBER_REGISTER_DPF     29
static RegDescription_t DpfRegisterMap[CAMERIC_NUMBER_REGISTER_DPF] = 
{
    { 0U, PERM_READ_WRITE, "ISP_DPF_MODE"           , "Mode control for Denoising Pre-Filter block" , 0x00000000, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_STRENGTH_R"     , "filter strength of the RED filter"           , 0x00000040, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_STRENGTH_G"     , "filter strength of the GREEN filter"         , 0x00000040, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_STRENGTH_B"     , "filter strength of the BLUE filter"          , 0x00000040, 0x000000FF, 0x000000FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_S_WEIGHT_G_1_4" , "Spatial Weights green channel 1 2 3 4"       , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPF_S_WEIGHT_G_5_6" , "Spatial Weights green channel 5 6"           , 0x00001010, 0x00001F1F, 0x00001F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPF_S_WEIGHT_RB_1_4", "Spatial Weights red/blue channels 1 2 3 4"   , 0x10101010, 0x1F1F1F1F, 0x1F1F1F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPF_S_WEIGHT_RB_5_6", "Spatial Weights red/blue channels 5 6"       , 0x00001010, 0x00001F1F, 0x00001F1F },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF0"     , "Noise Level Lookup Coefficient 0"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF1"     , "Noise Level Lookup Coefficient 1"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF2"     , "Noise Level Lookup Coefficient 2"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF3"     , "Noise Level Lookup Coefficient 3"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF4"     , "Noise Level Lookup Coefficient 4"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF5"     , "Noise Level Lookup Coefficient 5"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF6"     , "Noise Level Lookup Coefficient 6"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF7"     , "Noise Level Lookup Coefficient 7"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF8"     , "Noise Level Lookup Coefficient 8"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF9"     , "Noise Level Lookup Coefficient 9"            , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF10"    , "Noise Level Lookup Coefficient 10"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF11"    , "Noise Level Lookup Coefficient 11"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF12"    , "Noise Level Lookup Coefficient 12"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF13"    , "Noise Level Lookup Coefficient 13"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF14"    , "Noise Level Lookup Coefficient 14"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF15"    , "Noise Level Lookup Coefficient 15"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NLL_COEFF16"    , "Noise Level Lookup Coefficient 16"           , 0x000003FF, 0x000003FF, 0x000003FF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NF_GAIN_R"      , "Noise function gain for red pixels"          , 0x00000100, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NF_GAIN_GR"     , "Noise function gain for green in red pixels" , 0x00000100, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NF_GAIN_GB"     , "Noise function gain for green in blue pixels", 0x00000100, 0x00000FFF, 0x00000FFF },
    { 0U, PERM_READ_WRITE, "ISP_DPF_NF_GAIN_B"      , "Noise function gain for blue pixels"         , 0x00000100, 0x00000FFF, 0x00000FFF }
};

#endif /* __CAMERIC_REG_DESCRIPTION_DPF_H__ */

