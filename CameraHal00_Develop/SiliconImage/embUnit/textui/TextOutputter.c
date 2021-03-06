/*
 * COPYRIGHT AND PERMISSION NOTICE
 * 
 * Copyright (c) 2003 Embedded Unit Project
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, and/or sell copies of the Software, and to permit persons 
 * to whom the Software is furnished to do so, provided that the above 
 * copyright notice(s) and this permission notice appear in all copies 
 * of the Software and that both the above copyright notice(s) and this 
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT 
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY 
 * SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Except as contained in this notice, the name of a copyright holder 
 * shall not be used in advertising or otherwise to promote the sale, 
 * use or other dealings in this Software without prior written 
 * authorization of the copyright holder.
 *
 * $Id: TextOutputter.c,v 1.4 2003/09/06 13:28:27 arms22 Exp $
*/
#include <stdio.h>
#include "TextOutputter.h"

#define OUTPUT_PREFIX   "###embunit: "

static void TextOutputter_printHeader(OutputterRef self)
{
}

static void TextOutputter_printStartTest(OutputterRef self,TestRef test)
{
	fprintf(stdout,"%s- %s\n", OUTPUT_PREFIX, Test_name(test));
}

static void TextOutputter_printEndTest(OutputterRef self,TestRef test)
{
}

static void TextOutputter_printSuccessful(OutputterRef self,TestRef test,int runCount)
{
	fprintf(stdout,"%s%d) OK %s\n", OUTPUT_PREFIX, runCount, Test_name(test));
}

static void TextOutputter_printFailure(OutputterRef self,TestRef test,char *msg,int line,char *file,int runCount)
{
	fprintf(stdout,"%s%d) NG %s (%s %d) %s\n", OUTPUT_PREFIX, runCount, Test_name(test), file, line, msg);
}

static void TextOutputter_printComment(OutputterRef self,TestRef test,char *msg)
{
	fprintf(stdout,"%s %s\n", OUTPUT_PREFIX, msg);
}

static void TextOutputter_printTime(OutputterRef self,TestRef test,char *msg)
{
}

static void TextOutputter_printStatistics(OutputterRef self,TestResultRef result)
{
	if (result->failureCount) {
		fprintf(stdout,"\n%srun %d failures %d\n", OUTPUT_PREFIX, result->runCount,result->failureCount);
	} else {
		fprintf(stdout,"\n%sOK (%d tests)\n", OUTPUT_PREFIX, result->runCount);
	}
}

static const OutputterImplement TextOutputterImplement = {
	(OutputterPrintHeaderFunction)		TextOutputter_printHeader,
	(OutputterPrintStartTestFunction)	TextOutputter_printStartTest,
	(OutputterPrintEndTestFunction)		TextOutputter_printEndTest,
	(OutputterPrintSuccessfulFunction)	TextOutputter_printSuccessful,
	(OutputterPrintFailureFunction)		TextOutputter_printFailure,
    (OutputterPrintCommentFunction)     TextOutputter_printComment,
    (OutputterPrintTimeFunction)     TextOutputter_printTime,  
	(OutputterPrintStatisticsFunction)	TextOutputter_printStatistics,
};

static const Outputter TextOutputter = {
	(OutputterImplementRef)&TextOutputterImplement,
};

OutputterRef TextOutputter_outputter(void)
{
	return (OutputterRef)&TextOutputter;
}
