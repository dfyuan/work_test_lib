/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file mapcaps.cpp
 *
 * @brief
 *   Mapping of ISI capabilities / configuration to CamerIC modes.
 *
 *****************************************************************************/
#include "cam_api/mapcaps.h"


/******************************************************************************
 * local function definitions
 *****************************************************************************/
bool operator==(const CamEngineWindow_t &lhs, const CamEngineWindow_t &rhs)
{
    bool same =  (lhs.hOffset == rhs.hOffset)
              && (lhs.vOffset == rhs.vOffset)
              && (lhs.width   == rhs.width  )
              && (lhs.height  == rhs.height );
    return same;
}

bool operator!=(const CamEngineWindow_t &lhs, const CamEngineWindow_t &rhs)
{
    return !(lhs == rhs);
}


/******************************************************************************
 * local type definitions
 *****************************************************************************/
template <typename  T>
struct IsiCapsMap
{
    uint32_t    cap;
    T           value;
    const char* description;

    static const IsiCapsMap<T> map[];
};

/******************************************************************************
 * API function definitions
 *****************************************************************************/
template <typename  T>
T isiCapValue( uint32_t cap )
{
    for (unsigned int i = 1; i < (sizeof(IsiCapsMap<T>::map) / sizeof(*IsiCapsMap<T>::map)); ++i)
    {
        if ( cap == IsiCapsMap<T>::map[i].cap )
        {
            return IsiCapsMap<T>::map[i].value;
        }
    }
    return IsiCapsMap<T>::map[0].value;
}

template <typename  T>
bool isiCapValue( T& value, uint32_t cap )
{
    value = isiCapValue<T>( cap );
    return (value != IsiCapsMap<T>::map[0].value);
}

template <typename  T>
const char* isiCapDescription( uint32_t cap )
{
    for (unsigned int i = 1; i < (sizeof(IsiCapsMap<T>::map) / sizeof(*IsiCapsMap<T>::map)); ++i)
    {
        if ( cap == IsiCapsMap<T>::map[i].cap )
        {
            return IsiCapsMap<T>::map[i].description;
        }
    }
    return IsiCapsMap<T>::map[0].description;
}

template <typename  T>
bool isiCapDescription( const char* desc, uint32_t cap )
{
    desc = isiCapDescription<T>( cap );
    return (desc != IsiCapsMap<T>::map[0].description);
}

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
#define instantiateCapsMapAndSpecializedFuncs(__type__, ...)                            \
    template<> const IsiCapsMap<__type__> IsiCapsMap<__type__>::map[] = {__VA_ARGS__};  \
    template   __type__ isiCapValue( uint32_t cap );                                    \
    template   bool isiCapValue( __type__& value, uint32_t cap );                       \
    template   const char* isiCapDescription<__type__>( uint32_t cap );                 \
    template   bool isiCapDescription<__type__>( const char* desc, uint32_t cap );


/******************************************************************************
 * specialized API function & internal capability maps instantiations
 *****************************************************************************/
instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspSampleEdge_t,
    {0                      , CAMERIC_ISP_SAMPLE_EDGE_INVALID   , "CAMERIC_ISP_SAMPLE_EDGE_INVALID"},
    {ISI_EDGE_FALLING       , CAMERIC_ISP_SAMPLE_EDGE_FALLING   , "CAMERIC_ISP_SAMPLE_EDGE_FALLING"},
    {ISI_EDGE_RISING        , CAMERIC_ISP_SAMPLE_EDGE_RISING    , "CAMERIC_ISP_SAMPLE_EDGE_RISING"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspPolarity_t,
    {0                      , CAMERIC_ISP_POLARITY_INVALID      , "CAMERIC_ISP_POLARITY_INVALID"},
    {ISI_HPOL_SYNCPOS       , CAMERIC_ISP_POLARITY_HIGH         , "CAMERIC_ISP_POLARITY_HIGH"},
    {ISI_HPOL_SYNCNEG       , CAMERIC_ISP_POLARITY_LOW          , "CAMERIC_ISP_POLARITY_LOW"},
    {ISI_HPOL_REFPOS        , CAMERIC_ISP_POLARITY_HIGH         , "CAMERIC_ISP_POLARITY_HIGH"},
    {ISI_HPOL_REFNEG        , CAMERIC_ISP_POLARITY_LOW          , "CAMERIC_ISP_POLARITY_LOW"},
    {ISI_VPOL_POS           , CAMERIC_ISP_POLARITY_HIGH         , "CAMERIC_ISP_POLARITY_HIGH"},
    {ISI_VPOL_NEG           , CAMERIC_ISP_POLARITY_LOW          , "CAMERIC_ISP_POLARITY_LOW"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspBayerPattern_t,
    {0                      , CAMERIC_ISP_BAYER_PATTERN_INVALID , "INVALID"},
    {ISI_BPAT_RGRGGBGB      , CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB, "RGRGGBGB"},
    {ISI_BPAT_GRGRBGBG      , CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG, "GRGRBGBG"},
    {ISI_BPAT_GBGBRGRG      , CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG, "GBGBRGRG"},
    {ISI_BPAT_BGBGGRGR      , CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR, "BGBGGRGR"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspColorSubsampling_t,
    {0                      , CAMERIC_ISP_CONV422_INVALID       , "CAMERIC_ISP_CONV422_INVALID"},
    {ISI_CONV422_COSITED    , CAMERIC_ISP_CONV422_COSITED       , "CAMERIC_ISP_CONV422_COSITED"},
    {ISI_CONV422_INTER      , CAMERIC_ISP_CONV422_INTERLEAVED   , "CAMERIC_ISP_CONV422_INTERLEAVED"},
    {ISI_CONV422_NOCOSITED  , CAMERIC_ISP_CONV422_NONCOSITED    , "CAMERIC_ISP_CONV422_NONCOSITED"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspCCIRSequence_t,
    {0                      , CAMERIC_ISP_CCIR_SEQUENCE_INVALID , "CAMERIC_ISP_CCIR_SEQUENCE_INVALID"},
    {ISI_YCSEQ_YCBYCR       , CAMERIC_ISP_CCIR_SEQUENCE_YCbYCr  , "CAMERIC_ISP_CCIR_SEQUENCE_YCbYCr"},
    {ISI_YCSEQ_YCRYCB       , CAMERIC_ISP_CCIR_SEQUENCE_YCrYCb  , "CAMERIC_ISP_CCIR_SEQUENCE_YCrYCb"},
    {ISI_YCSEQ_CBYCRY       , CAMERIC_ISP_CCIR_SEQUENCE_CbYCrY  , "CAMERIC_ISP_CCIR_SEQUENCE_CbYCrY"},
    {ISI_YCSEQ_CRYCBY       , CAMERIC_ISP_CCIR_SEQUENCE_CrYCbY  , "CAMERIC_ISP_CCIR_SEQUENCE_CrYCbY"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspFieldSelection_t,
    {0                      , CAMERIC_ISP_FIELD_SELECTION_INVALID, "CAMERIC_ISP_FIELD_SELECTION_INVALID"},
    {ISI_FIELDSEL_BOTH      , CAMERIC_ISP_FIELD_SELECTION_BOTH  , "CAMERIC_ISP_FIELD_SELECTION_BOTH"},
    {ISI_FIELDSEL_EVEN      , CAMERIC_ISP_FIELD_SELECTION_EVEN  , "CAMERIC_ISP_FIELD_SELECTION_EVEN"},
    {ISI_FIELDSEL_ODD       , CAMERIC_ISP_FIELD_SELECTION_ODD   , "CAMERIC_ISP_FIELD_SELECTION_ODD"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspInputSelection_t,
    {0                      , CAMERIC_ISP_INPUT_INVALID         , "INVALID"},
    {ISI_BUSWIDTH_12BIT     , CAMERIC_ISP_INPUT_12BIT           , "12BIT"},
    {ISI_BUSWIDTH_10BIT_ZZ  , CAMERIC_ISP_INPUT_10BIT_ZZ        , "10BIT_ZZ"},
    {ISI_BUSWIDTH_10BIT_EX  , CAMERIC_ISP_INPUT_10BIT_EX        , "10BIT_EX"},
    {ISI_BUSWIDTH_8BIT_ZZ   , CAMERIC_ISP_INPUT_8BIT_ZZ         , "8BIT_ZZ"},
    {ISI_BUSWIDTH_8BIT_EX   , CAMERIC_ISP_INPUT_8BIT_EX         , "8BIT_EX"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamerIcIspMode_t,
    {0                      , CAMERIC_ISP_MODE_INVALID          , "CAMERIC_ISP_MODE_INVALID"},
    {ISI_MODE_PICT          , CAMERIC_ISP_MODE_RAW              , "CAMERIC_ISP_MODE_RAW"},
    {ISI_MODE_BT656         , CAMERIC_ISP_MODE_656              , "CAMERIC_ISP_MODE_656"},
    {ISI_MODE_BT601         , CAMERIC_ISP_MODE_601              , "CAMERIC_ISP_MODE_601"},
    {ISI_MODE_BAYER         , CAMERIC_ISP_MODE_BAYER_RGB        , "CAMERIC_ISP_MODE_BAYER_RGB"},
    {ISI_MODE_DATA          , CAMERIC_ISP_MODE_DATA             , "CAMERIC_ISP_MODE_DATA"},
    {ISI_MODE_BAY_BT656     , CAMERIC_ISP_MODE_RGB656           , "CAMERIC_ISP_MODE_RGB656"},
    {ISI_MODE_RAW_BT656     , CAMERIC_ISP_MODE_RAW656           , "CAMERIC_ISP_MODE_RAW656"},
    {ISI_MODE_MIPI          , CAMERIC_ISP_MODE_BAYER_RGB        , "CAMERIC_ISP_MODE_BAYER_RGB"}
)


instantiateCapsMapAndSpecializedFuncs
(
    MipiDataType_t,
    {0                            , MIPI_DATA_TYPE_MAX            , "INVALID"},
    {ISI_MIPI_OFF                 , MIPI_DATA_TYPE_MAX            , "OFF"},
    {ISI_MIPI_MODE_YUV420_8       , MIPI_DATA_TYPE_YUV420_8       , "YUV420_8"},
    {ISI_MIPI_MODE_YUV420_10      , MIPI_DATA_TYPE_YUV420_10      , "YUV420_10"},
    {ISI_MIPI_MODE_LEGACY_YUV420_8, MIPI_DATA_TYPE_LEGACY_YUV420_8, "LEGACY_YUV420_8"},
    {ISI_MIPI_MODE_YUV420_CSPS_8  , MIPI_DATA_TYPE_YUV420_8_CSPS  , "YUV420_8_CSPS"},
    {ISI_MIPI_MODE_YUV420_CSPS_10 , MIPI_DATA_TYPE_YUV420_10_CSPS , "YUV420_10_CSPS"},
    {ISI_MIPI_MODE_YUV422_8       , MIPI_DATA_TYPE_YUV422_8       , "YUV422_8"},
    {ISI_MIPI_MODE_YUV422_10      , MIPI_DATA_TYPE_YUV422_10      , "YUV422_10"},
    {ISI_MIPI_MODE_RGB444         , MIPI_DATA_TYPE_RGB444         , "RGB444"},
    {ISI_MIPI_MODE_RGB555         , MIPI_DATA_TYPE_RGB555         , "RGB555"},
    {ISI_MIPI_MODE_RGB565         , MIPI_DATA_TYPE_RGB565         , "RGB565"},
    {ISI_MIPI_MODE_RGB666         , MIPI_DATA_TYPE_RGB666         , "RGB666"},
    {ISI_MIPI_MODE_RGB888         , MIPI_DATA_TYPE_RGB888         , "RGB888"},
    {ISI_MIPI_MODE_RAW_6          , MIPI_DATA_TYPE_RAW_6          , "RAW_6"},
    {ISI_MIPI_MODE_RAW_7          , MIPI_DATA_TYPE_RAW_7          , "RAW_7"},
    {ISI_MIPI_MODE_RAW_8          , MIPI_DATA_TYPE_RAW_8          , "RAW_8"},
    {ISI_MIPI_MODE_RAW_10         , MIPI_DATA_TYPE_RAW_10         , "RAW_10"},
    {ISI_MIPI_MODE_RAW_12         , MIPI_DATA_TYPE_RAW_12         , "RAW_12"}
)


instantiateCapsMapAndSpecializedFuncs
(
    CamEngineWindow_t,
    {0                , {0, 0, 0, 0}          , "RES_INVALID"},
    {ISI_RES_VGAP5       ,  {0, 0, 640 ,  480}    ,    "RES_VGAP5"},
    {ISI_RES_VGAP10      ,  {0, 0, 640 ,  480}    ,    "RES_VGAP10"},
    {ISI_RES_VGAP15      ,  {0, 0, 640 ,  480}    ,    "RES_VGAP15"},
    {ISI_RES_VGAP20      ,  {0, 0, 640 ,  480}    ,    "RES_VGAP20"},
    {ISI_RES_VGAP30      ,  {0, 0, 640 ,  480}    ,    "RES_VGAP30"},
    {ISI_RES_VGAP60      ,  {0, 0, 640 ,  480}    ,    "RES_VGAP60"},
    {ISI_RES_VGAP120     ,  {0, 0, 640 ,  480}    ,    "RES_VGAP120"},
    {ISI_RES_SVGAP5       ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP5"},
    {ISI_RES_SVGAP10      ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP10"},
    {ISI_RES_SVGAP15      ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP15"},
    {ISI_RES_SVGAP20      ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP20"},
    {ISI_RES_SVGAP30      ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP30"},
    {ISI_RES_SVGAP60      ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP60"},
    {ISI_RES_SVGAP120     ,  {0, 0, 800 ,  600}    ,    "RES_SVGAP120"},
    {ISI_RES_1280_960P10  ,  {0, 0, 1280 ,  960}   ,"RES_1280x960P10"},
    {ISI_RES_1280_960P15  ,  {0, 0, 1280 ,  960}   ,"RES_1280x960P15"},
    {ISI_RES_1280_960P20  ,  {0, 0, 1280 ,  960}   ,"RES_1280x960P20"},
    {ISI_RES_1280_960P25  ,  {0, 0, 1280 ,  960}   ,"RES_1280x960P25"},
    {ISI_RES_1280_960P30  ,  {0, 0, 1280 ,  960}   ,"RES_1280x960P30"},
    {ISI_RES_2592_1944P5      ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P5"},
    {ISI_RES_2592_1944P7      ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P7"},
    {ISI_RES_2592_1944P10     ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P10"},
    {ISI_RES_2592_1944P15     ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P15"},  
    {ISI_RES_2592_1944P20     ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P20"}, 
    {ISI_RES_2592_1944P25     ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P25"},  
    {ISI_RES_2592_1944P30     ,  {0, 0, 2592 ,  1944}    ,    "RES_2592x1944P30"}, 

	{ISI_RES_2688_1520P30	 ,	{0, 0, 2688 ,  1520}    ,	 "RES_2688x1520P30"},
	{ISI_RES_2688_1520P25	 ,	{0, 0, 2688 ,  1520}    ,	 "RES_2688x1520P25"},
	{ISI_RES_2688_1520P20	 ,	{0, 0, 2688 ,  1520}    ,	 "RES_2688x1520P20"},
	{ISI_RES_2688_1520P15	 ,	{0, 0, 2688 ,  1520}    ,	 "RES_2688x1520P15"},
	{ISI_RES_2688_1520P10	 ,	{0, 0, 2688 ,  1520}    ,	 "RES_2688x1520P10"},
	{ISI_RES_2688_1520P7	 ,	{0, 0, 2688 ,  1520}    ,	 "RES_2688x1520P7"},

	{ISI_RES_672_376P60	 ,	{0, 0, 672 ,  376}    ,	 "RES_672x376P60"},
	{ISI_RES_672_376P30	 ,	{0, 0, 672 ,  376}    ,	 "RES_672x376P30"},
	{ISI_RES_672_376P25	 ,	{0, 0, 672 ,  376}    ,	 "RES_672x376P25"},
	{ISI_RES_672_376P20	 ,	{0, 0, 672 ,  376}    ,	 "RES_672x376P20"},
	{ISI_RES_672_376P15	 ,	{0, 0, 672 ,  376}    ,	 "RES_672x376P15"},
	{ISI_RES_672_376P10	 ,	{0, 0, 672 ,  376}    ,	 "RES_672x376P10"},

	{ISI_RES_2104_1184P40	 ,	{0, 0, 2104 ,  1184}    ,	 "RES_2104x1184P40"},
	{ISI_RES_1396_788P40	 ,  {0, 0, 1396 ,  788}    ,     "RES_1396x788P40"},
        
    {ISI_RES_1296_972P7      ,  {0, 0, 1296 ,  972}    ,    "RES_1296x972P7"},
    {ISI_RES_1296_972P10      ,  {0, 0, 1296 ,  972}    ,    "RES_1296x972P10"},
    {ISI_RES_1296_972P15     ,  {0, 0, 1296 ,  972}    ,    "RES_1296x972P15"},
    {ISI_RES_1296_972P20     ,  {0, 0, 1296 ,  972}    ,    "RES_1296x972P20"},
    {ISI_RES_1296_972P25     ,  {0, 0, 1296 ,  972}    ,    "RES_1296x972P25"},
    {ISI_RES_1296_972P30     ,  {0, 0, 1296 ,  972}    ,    "RES_1296x972P30"},        
    {ISI_RES_3264_2448P7      ,  {0, 0, 3264 ,  2448}    ,    "RES_3264x2448P7"},
    {ISI_RES_3264_2448P10     ,  {0, 0, 3264 ,  2448}    ,    "RES_3264x2448P10"},
    {ISI_RES_3264_2448P15     ,  {0, 0, 3264 ,  2448}    ,    "RES_3264x2448P15"},
    {ISI_RES_3264_2448P20     ,  {0, 0, 3264 ,  2448}    ,    "RES_3264x2448P20"},
    {ISI_RES_3264_2448P25     ,  {0, 0, 3264 ,  2448}    ,    "RES_3264x2448P25"},
    {ISI_RES_3264_2448P30     ,  {0, 0, 3264 ,  2448}    ,    "RES_3264x2448P30"},        
    {ISI_RES_1632_1224P7      ,  {0, 0, 1632 ,  1224}    ,    "RES_1632x1224P7"},
    {ISI_RES_1632_1224P10      ,  {0, 0, 1632 ,  1224}    ,    "RES_1632x1224P10"},
    {ISI_RES_1632_1224P15     ,  {0, 0, 1632 ,  1224}    ,    "RES_1632x1224P15"},
    {ISI_RES_1632_1224P20      ,  {0, 0, 1632 ,  1224}    ,    "RES_1632x1224P20"},
    {ISI_RES_1632_1224P25      ,  {0, 0, 1632 ,  1224}    ,    "RES_1632x1224P25"},
    {ISI_RES_1632_1224P30     ,  {0, 0, 1632 ,  1224}    ,    "RES_1632x1224P30"},    
    {ISI_RES_3120_3120P30     ,  {0, 0, 3120 ,  3120}    ,    "RES_3120x3120P30"},
    {ISI_RES_4416_3312P7      ,  {0, 0, 4416 ,  3312}    ,    "RES_4416x3312P7"},
    {ISI_RES_4416_3312P15     ,  {0, 0, 4416 ,  3312}    ,    "RES_4416x3312P15"},
    {ISI_RES_4416_3312P30     ,  {0, 0, 4416 ,  3312}   ,     "RES_4416x3312P30"},
    {ISI_RES_2208_1656P7      ,  {0, 0, 2208 ,  1656}    ,    "RES_2208x1656P7"},
    {ISI_RES_2208_1656P15     ,  {0, 0, 2208 ,  1656}    ,    "RES_2208x1656P15"},
    {ISI_RES_2208_1656P30     ,  {0, 0, 2208 ,  1656}   ,     "RES_2208x1656P30"},
    {ISI_RES_1600_1200P7      ,  {0, 0, 1600 ,  1200}    ,    "RES_1600x1200P7"},
    {ISI_RES_1600_1200P10     ,  {0, 0, 1600 ,  1200}    ,    "RES_1600x1200P10"},
    {ISI_RES_1600_1200P15     ,  {0, 0, 1600 ,  1200}    ,    "RES_1600x1200P15"},
    {ISI_RES_1600_1200P20     ,  {0, 0, 1600 ,  1200}    ,    "RES_1600x1200P20"},
    {ISI_RES_1600_1200P30     ,  {0, 0, 1600 ,  1200}   ,     "RES_1600x1200P30"},    
    {ISI_RES_4224_3136P4      ,  {0, 0, 4224 ,  3136}    ,    "RES_4224x3136P4"},
    {ISI_RES_4224_3136P7      ,  {0, 0, 4224 ,  3136}    ,    "RES_4224x3136P7"},
    {ISI_RES_4224_3136P10     ,  {0, 0, 4224 ,  3136}   ,     "RES_4224x3136P10"}, 
    {ISI_RES_4224_3136P15     ,  {0, 0, 4224 ,  3136}    ,    "RES_4224x3136P15"},
    {ISI_RES_4224_3136P20     ,  {0, 0, 4224 ,  3136}   ,     "RES_4224x3136P20"}, 
    {ISI_RES_4224_3136P25     ,  {0, 0, 4224 ,  3136}   ,     "RES_4224x3136P25"}, 
    {ISI_RES_4224_3136P30     ,  {0, 0, 4224 ,  3136}   ,     "RES_4224x3136P30"},        

    {ISI_RES_4208_3120P4      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P4"},
    {ISI_RES_4208_3120P7      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P7"},
    {ISI_RES_4208_3120P10      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P10"},
    {ISI_RES_4208_3120P15      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P15"},
    {ISI_RES_4208_3120P20      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P20"},
    {ISI_RES_4208_3120P25      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P25"},
    {ISI_RES_4208_3120P30      ,  {0, 0, 4208 ,  3120}    ,    "RES_4208x3120P30"},

    {ISI_RES_2112_1568P7      ,  {0, 0, 2112 ,  1568}    ,    "RES_2112x1568P7"},
    {ISI_RES_2112_1568P10     ,  {0, 0, 2112 ,  1568}    ,    "RES_2112x1568P10"},    
    {ISI_RES_2112_1568P15     ,  {0, 0, 2112 ,  1568}    ,    "RES_2112x1568P15"},
    {ISI_RES_2112_1568P20     ,  {0, 0, 2112 ,  1568}   ,     "RES_2112x1568P20"},    
    {ISI_RES_2112_1568P25     ,  {0, 0, 2112 ,  1568}   ,     "RES_2112x1568P25"},   
    {ISI_RES_2112_1568P30     ,  {0, 0, 2112 ,  1568}   ,     "RES_2112x1568P30"},
    {ISI_RES_2112_1568P40     ,  {0, 0, 2112 ,  1568}   ,     "RES_2112x1568P40"},    
    {ISI_RES_2112_1568P50     ,  {0, 0, 2112 ,  1568}   ,     "RES_2112x1568P50"},   
    {ISI_RES_2112_1568P60     ,  {0, 0, 2112 ,  1568}   ,     "RES_2112x1568P60"},

    {ISI_RES_3120_3120P15     ,  {0, 0, 3120 ,  3120}   ,     "RES_3120x3120P15"},
    {ISI_RES_3120_3120P25     ,  {0, 0, 3120 ,  3120}   ,     "RES_3120x3120P25"},
    {ISI_RES_2112_1560P30     ,  {0, 0, 2112 ,  1560}   ,     "RES_2112x1560P30"},
    {ISI_RES_4224_3120P15     ,  {0, 0, 2112 ,  1560}   ,     "RES_4224_3120P15"},

    {ISI_RES_2104_1560P7      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P7"},
    {ISI_RES_2104_1560P10      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P10"},
    {ISI_RES_2104_1560P15      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P15"},
    {ISI_RES_2104_1560P20      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P20"},
    {ISI_RES_2104_1560P25      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P25"},
    {ISI_RES_2104_1560P30      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P30"},
    {ISI_RES_2104_1560P40      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P40"},
    {ISI_RES_2104_1560P50      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P50"},
    {ISI_RES_2104_1560P60      ,  {0, 0, 2104 ,  1560}    ,    "RES_2104x1560P60"},

    {ISI_RES_1640_1232P10      ,  {0, 0, 1640 ,  1232}    ,    "RES_1640x1232P10"},
    {ISI_RES_1640_1232P15      ,  {0, 0, 1640 ,  1232}    ,    "RES_1640x1232P15"},
    {ISI_RES_1640_1232P20      ,  {0, 0, 1640 ,  1232}    ,    "RES_1640x1232P20"},
    {ISI_RES_1640_1232P25      ,  {0, 0, 1640 ,  1232}    ,    "RES_1640x1232P25"},
    {ISI_RES_1640_1232P30      ,  {0, 0, 1640 ,  1232}    ,    "RES_1640x1232P30"},

    {ISI_RES_3280_2464P7       ,  {0, 0, 3280 ,  2464}    ,    "RES_3280_2464P7"},
    {ISI_RES_3280_2464P15      ,  {0, 0, 3280 ,  2464}    ,    "RES_3280_2464P15"},
    {ISI_RES_3280_2464P20      ,  {0, 0, 3280 ,  2464}    ,    "RES_3280_2464P20"},
    {ISI_RES_3280_2464P25      ,  {0, 0, 3280 ,  2464}    ,    "RES_3280_2464P25"},
    {ISI_RES_3280_2464P30      ,  {0, 0, 3280 ,  2464}    ,    "RES_3280_2464P30"},

    {ISI_RES_TV720P5  , {0, 0, 1280,  720}    , "RES_TV720P5"},
    {ISI_RES_TV720P15 , {0, 0, 1280,  720}    , "RES_TV720P15"},
    {ISI_RES_TV720P30 , {0, 0, 1280,  720}    , "RES_TV720P30"},
    {ISI_RES_TV720P60 , {0, 0, 1280,  720}    , "RES_TV720P60"},        
    {ISI_RES_TV1080P5 , {0, 0, 1920, 1080}    , "RES_TV1080P5"},
    {ISI_RES_TV1080P6 , {0, 0, 1920, 1080}    , "RES_TV1080P6"},
    {ISI_RES_TV1080P10, {0, 0, 1920, 1080}    , "RES_TV1080P10"},
    {ISI_RES_TV1080P12, {0, 0, 1920, 1080}    , "RES_TV1080P12"},
    {ISI_RES_TV1080P15, {0, 0, 1920, 1080}    , "RES_TV1080P15"},
    {ISI_RES_TV1080P20, {0, 0, 1920, 1080}    , "RES_TV1080P20"},
    {ISI_RES_TV1080P24, {0, 0, 1920, 1080}    , "RES_TV1080P24"},
    {ISI_RES_TV1080P25, {0, 0, 1920, 1080}    , "RES_TV1080P25"},
    {ISI_RES_TV1080P30, {0, 0, 1920, 1080}    , "RES_TV1080P30"},
    {ISI_RES_TV1080P50, {0, 0, 1920, 1080}    , "RES_TV1080P50"},
    {ISI_RES_TV1080P60, {0, 0, 1920, 1080}    , "RES_TV1080P60"}
)


