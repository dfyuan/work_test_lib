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
* \file ExtXMLOutputter.c
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
*   Extended Xml Outputter with the capability to store extra information.
*
* </pre>
*/
/*****************************************************************************/
#include <stdio.h>
#include "ExtXMLOutputter.h"

#include <memory.h>
#include <string.h>
#include <time.h>

/******************************************************************************
* TYPEDEFS / DEFINES
*****************************************************************************/
#define MAX_STRING_LENGTH 	256
#define MAX_TIME_LENGTH		10

static additionalInfo gInfo;
static char sString[MAX_STRING_LENGTH] = {0};
static char sComment[MAX_STRING_LENGTH] = {0};
static char sTime[MAX_TIME_LENGTH] = {0};


static void ExtXMLOutputter_printHeader(OutputterRef self)
{
	fprintf(gInfo.fileHandle,"<?xml version=\"1.0\" encoding='shift_jis' standalone='yes' ?>\n");
    if (gInfo.stylesheet) {
	    fprintf(gInfo.fileHandle,"<?xml-stylesheet type=\"text/xsl\" href=\"%s\" ?>\n",gInfo.stylesheet);
    }

	fprintf(gInfo.fileHandle,"<TestRun>\n");
}

static void ExtXMLOutputter_printStartTest(OutputterRef self,TestRef test)
{
    /* reset the comment field*/
    memset( sComment, 0, MAX_STRING_LENGTH);
    memset( sTime, 0, MAX_TIME_LENGTH);    
	fprintf(gInfo.fileHandle,"<%s>\n",Test_name(test));
}

static void ExtXMLOutputter_printEndTest(OutputterRef self,TestRef test)
{     
    fprintf(gInfo.fileHandle,"</%s>\n",Test_name(test));
}

static void ExtXMLOutputter_printSuccessful(OutputterRef self,TestRef test,int runCount)
{
	fprintf(gInfo.fileHandle,"<Test id=\"%d\">\n",runCount);
	fprintf(gInfo.fileHandle,"<Name>%s</Name>\n",Test_name(test));
    fprintf(gInfo.fileHandle,"<Comment>%s</Comment>\n",sComment);
    fprintf(gInfo.fileHandle,"<Test_Time>%s</Test_Time>\n", sTime);  
	fprintf(gInfo.fileHandle,"</Test>\n");
}

static void ExtXMLOutputter_printFailure(OutputterRef self,TestRef test,char *msg,int line,char *file,int runCount)
{
    /* assure that we use '/' instead of '\' to make our xsl template happy*/
    int  ch = '\\';
    char* pDest;

    /* we are not allowed to manipulate the incomming string*/
    memset( sString, 0, MAX_STRING_LENGTH);
    strcpy( sString, file );

    do {
        pDest = (char*)memchr( sString, ch, strlen( sString ) );
        if ( pDest != NULL){
            *pDest = '/';
        }
    }while(pDest != NULL);
        
	fprintf(gInfo.fileHandle,"<FailedTest id=\"%d\">\n",runCount);
	fprintf(gInfo.fileHandle,"<Name>%s</Name>\n",Test_name(test));
    fprintf(gInfo.fileHandle,"<Comment>%s</Comment>\n",sComment);
    fprintf(gInfo.fileHandle,"<Test_Time>%s</Test_Time>\n", sTime);  
	fprintf(gInfo.fileHandle,"<Location>\n");
	fprintf(gInfo.fileHandle,"<File>%s</File>\n",file);
	fprintf(gInfo.fileHandle,"<Line>%d</Line>\n",line);
	fprintf(gInfo.fileHandle,"</Location>\n");
	fprintf(gInfo.fileHandle,"<Message>%s</Message>\n",msg);
	fprintf(gInfo.fileHandle,"</FailedTest>\n");
}

static void ExtXMLOutputter_printComment(OutputterRef self,TestRef test,char *msg)
{
    if(strlen(msg) >= MAX_STRING_LENGTH)
    {
        strncpy( sComment, msg, MAX_STRING_LENGTH-4);
        sComment[MAX_STRING_LENGTH-4] = '.';
        sComment[MAX_STRING_LENGTH-3] = '.';
        sComment[MAX_STRING_LENGTH-2] = '.';
        sComment[MAX_STRING_LENGTH-1] = '\0';
    }
    else
    {
        strcpy( sComment, msg );
    }
}

static void ExtXMLOutputter_printTime(OutputterRef self,TestRef test,char *msg)
{
    if(strlen(msg) >= MAX_TIME_LENGTH)
    {
        strncpy( sTime, msg, MAX_TIME_LENGTH-1);
        sTime[MAX_TIME_LENGTH-1] = '\0';
    }
    else
    {
        strcpy( sTime, msg );
    }
	
}

static void ExtXMLOutputter_printStatistics(OutputterRef self,TestResultRef result)
{
	fprintf(gInfo.fileHandle,"<Statistics>\n");
    
    // result statistics
	fprintf(gInfo.fileHandle,"<Tests>%d</Tests>\n",result->runCount);
	if (result->failureCount) {
	fprintf(gInfo.fileHandle,"<Failures>%d</Failures>\n",result->failureCount);
	}
	fprintf(gInfo.fileHandle,"</Statistics>\n");

    // additional information used for the test result page
    fprintf(gInfo.fileHandle,"<TestInformation>\n");
    fprintf(gInfo.fileHandle,"<Time>%s</Time>\n",gInfo.executionTime);
    fprintf(gInfo.fileHandle,"<Label>%s</Label>\n",gInfo.configSpecLabel);
    fprintf(gInfo.fileHandle,"<BuildMode>%s</BuildMode>\n",gInfo.buildmode);
    fprintf(gInfo.fileHandle,"</TestInformation>\n");    
    
	fprintf(gInfo.fileHandle,"</TestRun>\n");
}

static const OutputterImplement ExtXMLOutputterImplement = {
	(OutputterPrintHeaderFunction)		ExtXMLOutputter_printHeader,
	(OutputterPrintStartTestFunction)	ExtXMLOutputter_printStartTest,
	(OutputterPrintEndTestFunction)		ExtXMLOutputter_printEndTest,
	(OutputterPrintSuccessfulFunction)	ExtXMLOutputter_printSuccessful,
	(OutputterPrintFailureFunction)		ExtXMLOutputter_printFailure,
    (OutputterPrintCommentFunction)     ExtXMLOutputter_printComment,
    (OutputterPrintTimeFunction)        ExtXMLOutputter_printTime,        
	(OutputterPrintStatisticsFunction)	ExtXMLOutputter_printStatistics,
};

static const Outputter ExtXMLOutputter = {
	(OutputterImplementRef)&ExtXMLOutputterImplement,
};

void ExtXMLOutputter_setStyleSheet(char *style)
{
    gInfo.stylesheet = style;
}

void ExtXMLOutputter_setFileHandle(FILE *handle)
{
    gInfo.fileHandle = handle;
}
void ExtXMLOutputter_setTestExecutionTime(const char* sTime)
{
    gInfo.executionTime = sTime;
}


void ExtXMLOutputter_setConfigSpecLabel(const char* sLabel)
{
    gInfo.configSpecLabel = sLabel;
}


void ExtXMLOutputter_setBuildMode(const char* sBuildmode)
{
    gInfo.buildmode = sBuildmode;
}


OutputterRef ExtXMLOutputter_outputter(void)
{
	return (OutputterRef)&ExtXMLOutputter;
}
