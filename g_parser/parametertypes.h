////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// THIS IS AUTO-GENERATED CODE.  PLEASE DO NOT EDIT (File bug reports against tools).
///
/// Auto-generated by: ParameterParser.exe V1.1.0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  parametertypes.h
/// @brief Auto-generated Chromatix parameter file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PARAMETERTYPES_H
#define PARAMETERTYPES_H

#ifdef CAMX

#include <stdio.h>
#include <string.h>
#include "camxtypes.h"
#include "camxmem.h"
#include "camxosutils.h"
#include "camxutils.h"

#define PARAMETER_NEW     CAMX_NEW
#define PARAMETER_DELETE  CAMX_DELETE
#define PARAMETER_STRLEN  CamX::OsUtils::StrLen
#define PARAMETER_SPRINTF CamX::OsUtils::SNPrintF
#define PARAMETER_STRCMP  CamX::OsUtils::StrCmp
#define PARAMETER_STRCPY(dest, len, src)  CamX::OsUtils::StrLCpy(dest, src, len)
#define PARAMETER_STRCAT(dest, destLen, src)  CamX::OsUtils::StrLCat(dest, destLen, src)
#define PARAMETER_STRNICMP(str1, str2, nChars)  CamX::OsUtils::StrNICmp(str1, str2, nChars)
#define PARAMETER_INITIALIZE(var)  CamX::Utils::Memset(&var, 0, sizeof(var))
#else

#include <Windows.h>
#include <string.h>
#include <stdio.h>

#define PARAMETER_NEW     new
#define PARAMETER_DELETE  delete
#define PARAMETER_STRLEN  strlen
#define PARAMETER_SPRINTF sprintf_s
#define PARAMETER_STRCMP  strcmp
#define PARAMETER_STRCPY(dest, len, src)  strcpy_s(dest, len, src)
#define PARAMETER_STRCAT(dest, len, src)  strcat_s(dest, len, src)
#define PARAMETER_STRNICMP(str1, str2, nChars)  _strnicmp(str1, str2, nChars)
#define PARAMETER_INITIALIZE(var)  memset(&var, 0, sizeof(var))

enum CamxResult
{
    CamxResultSuccess,
    CamxResultEFailed
};

#endif

#endif
