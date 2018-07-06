/*****************************************************************************
* This is an unpublished work, the copyright in which vests in sci-worx GmbH.
*
* The information contained herein is the property of sci-worx GmbH and is
* supplied without liability for errors or omissions. No part may be
* reproduced or used except as authorised by contract or other written
* permission.
*
* Copyright (c) 2006 sci-worx GmbH. All rights reserved.
*
*****************************************************************************/
/*!
* \file ExtXMLOutputter.h
*
* <pre>
*
*   Principal Author: kaiserk
*   $RCSfile:  $
*   $Revision:  $
*   Date: Feb 13, 2007
*   Author:  $
*   Company: sci-worx GmbH
*
* Programming Language: C
* Designed for any OS (conformable to ANSI)
*
* Description:
*   ADD_DESCRIPTION_HERE
*
* </pre>
*/
/*****************************************************************************/
#ifndef	__ExtXMLOutputter_H__
#define	__ExtXMLOutputter_H__

#include "Outputter.h"
#include <stdio.h>


typedef struct _additionalInfo
{
    const char* stylesheet;
    const char* executionTime;
    const char* configSpecLabel;
    const char* buildmode;
    FILE*       fileHandle;
}additionalInfo;

void ExtXMLOutputter_setStyleSheet(char *style);
void ExtXMLOutputter_setFileHandle(FILE *handle);
void ExtXMLOutputter_setTestExecutionTime(const char* sTime);
void ExtXMLOutputter_setConfigSpecLabel(const char* sLabel);
void ExtXMLOutputter_setBuildMode(const char* sBuildmode);

OutputterRef ExtXMLOutputter_outputter(void);

#endif/*__ExtXMLOutputter_H__*/
