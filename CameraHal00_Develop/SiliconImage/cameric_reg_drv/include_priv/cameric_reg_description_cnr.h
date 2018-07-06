// SIG-FLOW generated file: DO NOT EDIT!

#ifndef __CAMERIC_REG_DESCRIPTION_CNR_H__
#define __CAMERIC_REG_DESCRIPTION_CNR_H__

static RegDescription_t CnrRegisterMap[] =
{
    { 0U, PERM_READ_WRITE, "ISP_CNR_LINESIZE"       , "Horizontal image size for chroma noise reduction", 0x00000000, 0x00003FFF, 0x00003FFF },
    { 0U, PERM_READ_WRITE, "ISP_CNR_THRESHOLD_C1"   , "Avoid filtering at edges for C1"                 , 0x00000000, 0x00007FFF, 0x00007FFF },
    { 0U, PERM_READ_WRITE, "ISP_CNR_THRESHOLD_C2"   , "Avoid filtering at edges for C2"                 , 0x00000000, 0x00007FFF, 0x00007FFF },
};
 
#endif /* __CAMERIC_REG_DESCRIPTION_CNR_H__ */


