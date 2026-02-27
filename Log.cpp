/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 19.03.2021

  Copyright (C) 2021, Johannes Natter

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <string.h>

#ifndef CONFIG_PROC_HAVE_DRIVERS
#if defined(__STDCPP_THREADS__)
#define CONFIG_PROC_HAVE_DRIVERS				1
#else
#define CONFIG_PROC_HAVE_DRIVERS				0
#endif
#endif

#ifndef CONFIG_PROC_LOG_HAVE_CHRONO
#if defined(__unix__) || defined(_WIN32)
#define CONFIG_PROC_LOG_HAVE_CHRONO				1
#else
#define CONFIG_PROC_LOG_HAVE_CHRONO				0
#endif
#endif

#ifndef CONFIG_PROC_LOG_HAVE_STDOUT
#if defined(__unix__) || defined(_WIN32)
#define CONFIG_PROC_LOG_HAVE_STDOUT				1
#else
#define CONFIG_PROC_LOG_HAVE_STDOUT				0
#endif
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#if CONFIG_PROC_LOG_HAVE_CHRONO
#include <chrono>
#include <time.h>
#endif
#if CONFIG_PROC_HAVE_DRIVERS
#include <thread>
#include <mutex>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
#if CONFIG_PROC_LOG_HAVE_CHRONO
using namespace chrono;
#endif

typedef void (*FuncEntryLogCreate)(
			const int severity,
			const void *pProc,
			const char *filename,
			const char *function,
			const int line,
			const int16_t code,
			const char *msg,
			const size_t len);

typedef uint32_t (*FuncCntTimeCreate)();

static FuncEntryLogCreate pFctEntryLogCreate = NULL;
static FuncCntTimeCreate pFctCntTimeCreate = NULL;
static int widthCntTime = 0;

#if CONFIG_PROC_LOG_HAVE_CHRONO
static system_clock::time_point tOld;
const int cDiffSecMax = 9;
const int cDiffMsMax = 999;
#endif

const char *tabStrSev[] = { "INV", "ERR", "WRN", "INF", "DBG", "COR" };

#ifdef _WIN32
const WORD tabColors[] =
{
	7, /* default */	4, /* red */		6, /* yellow */
	7, /* default */	3, /* cyan */		6, /* purple */
};
#else
const char *tabColors[] =
{
	"\033[39m",   /* default */	"\033[0;31m", /* red */		"\033[0;33m", /* yellow */
	"\033[39m",   /* default */	"\033[0;36m", /* cyan */		"\033[0;35m", /* purple */
};
#endif

#ifdef CONFIG_PROC_LOG_COLOR_INF
#define dColorInfo CONFIG_PROC_LOG_COLOR_INF
#else
#define dColorInfo dColorDefault
#endif
const size_t cLenWherePad = 68;
//const size_t cLogEntryBufferSize = 230;
const size_t cLogEntryBufferSize = 104;

// Example                                                  _ pBufEnd
//                                                        _/
// Allocated buffer     |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0|
// Block 1              |<b1>0xxxxxxxxxxxxxxxxxxxxxxxxxxxx0|
// Block 2              |<b1>0<b2>0xxxxxxxxxxxxxxxxxxxxxxx0|
// Block 3 error        |<b1>0<b2>0-                     00|
// Block 4 saturated    |<b1>0<b2>0-                     00|
// Block 5 saturated    |<b1>0<b2>0-                     00|
// Block 4 and 5 have same ptr at the end => pBufEnd

static int levelLog = 3;
#if CONFIG_PROC_HAVE_DRIVERS
static mutex mtxPrint;
#endif

void levelLogSet(int lvl)
{
	levelLog = lvl;
}

void entryLogCreateSet(FuncEntryLogCreate pFct)
{
#if CONFIG_PROC_HAVE_DRIVERS
	lock_guard<mutex> lock(mtxPrint); // Guard not defined!
#endif
	pFctEntryLogCreate = pFct;
}

void cntTimeCreateSet(FuncCntTimeCreate pFct, int width)
{
	if (width < -20 || width > 20)
		return;
#if CONFIG_PROC_HAVE_DRIVERS
	lock_guard<mutex> lock(mtxPrint); // Guard not defined!
#endif
	pFctCntTimeCreate = pFct;
	widthCntTime = width;
}

int16_t entryLogSimpleCreate(
			const int isErr,
			const int16_t code,
			const char *msg, ...)
{
#if CONFIG_PROC_HAVE_DRIVERS
	lock_guard<mutex> lock(mtxPrint); // Guard not defined!
#endif
	FILE *pStream = isErr ? stderr : stdout;
	int len;
	va_list args;

	va_start(args, msg);
	len = vfprintf(pStream, msg, args);
	if (len < 0)
	{
		va_end(args);
		return code;
	}
	va_end(args);

	fprintf(pStream, "\r\n");
	fflush(pStream);

	return code;
}

static int pBufSaturated(int len, char * &pBuf, char *pBufEnd)
{
	if (len > pBufEnd - pBuf)
		len = pBufEnd - pBuf;

	pBuf += len;

	return pBuf == pBufEnd;
}

static char *strErr(char *pBufStart, char *pBufEnd)
{
	char *pBuf = pBufStart;

	fprintf(stderr, "strErr()\n");

	for (; pBuf < pBufEnd; ++pBuf)
		*pBuf = pBuf == pBufStart ? '-' : ' ';

	if (pBuf > pBufStart)
		*--pBuf = 0;

	return pBuf;
}

#if CONFIG_PROC_LOG_HAVE_CHRONO
static char *blockTimeAbsAdd(char *pBuf, char *pBufEnd, system_clock::time_point &t)
{
	char *pBufStart = pBuf;
	int len;
	int res;

	fprintf(stderr, "blockTimeAbsAdd()\n");

	// build day
	time_t tTt = system_clock::to_time_t(t);
	char timeBuf[32];
	tm tTm {};
#ifdef _WIN32
	::localtime_s(&tTm, &tTt);
#else
	::localtime_r(&tTt, &tTm);
#endif
	res = strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d", &tTm);
	if (!res)
		return strErr(pBufStart, pBufEnd);

	// build time
	system_clock::duration dur = t.time_since_epoch();

	hours durDays = duration_cast<hours>(dur) / 24;
	dur -= durDays * 24;

	hours durHours = duration_cast<hours>(dur);
	dur -= durHours;

	minutes durMinutes = duration_cast<minutes>(dur);
	dur -= durMinutes;

	seconds durSecs = duration_cast<seconds>(dur);
	dur -= durSecs;

	milliseconds durMillis = duration_cast<milliseconds>(dur);
	dur -= durMillis;

	len = snprintf(pBuf, pBufEnd - pBuf,
					"%s  %02d:%02d:%02d.%03d ",
					timeBuf,
					int(durHours.count()), int(durMinutes.count()),
					int(durSecs.count()), int(durMillis.count()));
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	if (pBufSaturated(len, pBuf, pBufEnd))
		return strErr(pBufStart, pBufEnd);

	++pBuf;

	return pBuf;
}

static char *blockTimeRelAdd(char *pBuf, char *pBufEnd, system_clock::time_point &t)
{
	char *pBufStart = pBuf;
	milliseconds durDiffMs = duration_cast<milliseconds>(t - tOld);
	long long tDiff = durDiffMs.count();
	int tDiffSec = int(tDiff / 1000);
	int tDiffMs = int(tDiff % 1000);
	bool diffMaxed = false;
	int len;

	fprintf(stderr, "blockTimeRelAdd()\n");

	if (tDiffSec > cDiffSecMax)
	{
		tDiffSec = cDiffSecMax;
		tDiffMs = cDiffMsMax;

		diffMaxed = true;
	}

	len = snprintf(pBuf, pBufEnd - pBuf,
					"%c%d.%03d  ",
					diffMaxed ? '>' : '+', tDiffSec, tDiffMs);
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	if (pBufSaturated(len, pBuf, pBufEnd))
		return strErr(pBufStart, pBufEnd);

	++pBuf;

	return pBuf;
}
#endif
static char *blockTimeCntAdd(char *pBuf, char *pBufEnd)
{
	char *pBufStart = pBuf;

	fprintf(stderr, "blockTimeCntAdd()\n");

	if (!pFctCntTimeCreate)
	{
		if (pBuf < pBufEnd) *pBuf++ = 0;
#if 0
		return strErr(pBufStart, pBufEnd);
#else
		return pBuf;
#endif
	}

	uint32_t cntTime = pFctCntTimeCreate();
	int len;

	len = snprintf(pBuf, pBufEnd - pBuf,
					"%*" PRIu32 "  ",
					widthCntTime, cntTime);
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	if (pBufSaturated(len, pBuf, pBufEnd))
		return strErr(pBufStart, pBufEnd);

	return pBuf;
}

static char *blockWhereAdd(
			char *pBuf, char *pBufEnd,
			char *pBufPad,
			const void *pProc,
			const char *filename,
			const char *function,
			const int line)
{
	char *pBufStart = pBuf;
	int len;

	fprintf(stderr, "blockWhereAdd()\n");

	len = snprintf(pBuf, pBufEnd - pBuf,
				"%-20s  ", function);
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	(void)pBufSaturated(len, pBuf, pBufEnd);

	if (pProc)
	{
		len = snprintf(pBuf, pBufEnd - pBuf,
				"%p ", pProc);
		if (len < 0)
			return strErr(pBufStart, pBufEnd);

		(void)pBufSaturated(len, pBuf, pBufEnd);
	}

	len = snprintf(pBuf, pBufEnd - pBuf,
				"%s:%-4d  ", filename, line);
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	(void)pBufSaturated(len, pBuf, pBufEnd);

	// padding
	for (; pBuf < pBufPad && pBuf < pBufEnd; ++pBuf)
		*pBuf = ' ';

	char *pBufPadded = pBuf;

	if (pBuf > pBufStart)
		*--pBuf = 0;
	if (pBuf > pBufStart)
		*--pBuf = ' ';
	if (pBuf > pBufStart)
		*--pBuf = ' ';

	return pBufPadded;
}

static char *blockSeverityAdd(
			char *pBuf, char *pBufEnd,
			const int severity)
{
	char *pBufStart = pBuf;
	int len;

	fprintf(stderr, "blockSeverityAdd()\n");

	len = snprintf(pBuf, pBufEnd - pBuf, "%s  ", tabStrSev[severity]);
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	if (pBufSaturated(len, pBuf, pBufEnd))
		return strErr(pBufStart, pBufEnd);

	++pBuf;

	return pBuf;
}

static char *blockWhatUserAdd(
			char *pBuf, char *pBufEnd,
			const char *msg, va_list args)
{
	char *pBufStart = pBuf;
	int len;

	fprintf(stderr, "blockWhatUserAdd()\n");

	len = vsnprintf(pBuf, pBufEnd - pBuf, msg, args);
	if (len < 0)
		return strErr(pBufStart, pBufEnd);

	(void)pBufSaturated(len, pBuf, pBufEnd);

	++pBuf;

	return pBuf;
}
#if CONFIG_PROC_LOG_HAVE_STDOUT
static void toConsoleWrite(
			const int severity,
#if CONFIG_PROC_LOG_HAVE_CHRONO
			char *pTimeAbs,
			char *pTimeRel,
#endif
			char *pTimeCnt,
			char *pWhere,
			char *pSeverity,
			char *pWhatUser)
{
	if (severity > levelLog)
		return;

	FILE *fOut = severity < 3 ? stderr : stdout;
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(severity < 3 ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO infoConsole;

	GetConsoleScreenBufferInfo(hConsole, &infoConsole);
	WORD colorBkup = infoConsole.wAttributes;

	SetConsoleTextAttribute(hConsole, tabColors[severity]);
	fprintf(fOut, "%s\r\n", pBufStart);
	fflush(fOut);
	SetConsoleTextAttribute(hConsole, colorBkup);
#else
	fprintf(fOut,
			"\033[38:5:245m"
#if CONFIG_PROC_LOG_HAVE_CHRONO
			"%s%s"
#endif
			"%s%s%s"
			"%s%s%s"
			"%s\r\n",
#if CONFIG_PROC_LOG_HAVE_CHRONO
			pTimeAbs, pTimeRel,
#endif
			pTimeCnt, pWhere, tabColors[0],
			tabColors[severity], pSeverity, tabColors[0],
			pWhatUser);
#endif
}
#endif

int16_t entryLogCreate(
			const int severity,
			const void *pProc,
			const char *filename,
			const char *function,
			const int line,
			const int16_t code,
			const char *msg, ...)
{
#if CONFIG_PROC_HAVE_DRIVERS
	lock_guard<mutex> lock(mtxPrint); // Guard not defined!
#endif
	if (severity < 1 || severity > 5)
		return code;

	char *pBufStart = (char *)malloc(cLogEntryBufferSize);
	if (!pBufStart)
		return code;

	char *pBufEnd = pBufStart + cLogEntryBufferSize - 1;
	*pBufEnd = 0;

	// WHEN
#if CONFIG_PROC_LOG_HAVE_CHRONO
	system_clock::time_point t = system_clock::now();
	char *pTimeAbs = pBufStart;
	char *pTimeRel = blockTimeAbsAdd(pTimeAbs, pBufEnd, t);
	char *pTimeCnt = blockTimeRelAdd(pTimeRel, pBufEnd, t);
	tOld = t;
#else
	char *pTimeCnt = pBufStart;
#endif
	char *pWhere = blockTimeCntAdd(pTimeCnt, pBufEnd);

	// WHERE
	char *pSeverity = blockWhereAdd(
			pWhere, pBufEnd,
			pWhere + cLenWherePad,
			pProc, filename, function, line);

	// WHAT
	char *pWhatUser = blockSeverityAdd(pSeverity, pBufEnd, severity);
	va_list args;
	va_start(args, msg);
	(void)blockWhatUserAdd(pWhatUser, pBufEnd, msg, args);
	va_end(args);

	// Out
#if CONFIG_PROC_LOG_HAVE_STDOUT
	toConsoleWrite(severity,
#if CONFIG_PROC_LOG_HAVE_CHRONO
			pTimeAbs,
			pTimeRel,
#endif
			pTimeCnt,
			pWhere,
			pSeverity,
			pWhatUser);
#endif
	if (pFctEntryLogCreate)
		pFctEntryLogCreate(severity,
			pProc, filename, function, line, code,
			pBufStart, pBufEnd - pBufStart);

	fprintf(stderr, "pBufStart  = %p,   0,  0,   0\n", pBufStart);
	fprintf(stderr, "pTimeAbs   = %p, %3ld, %2ld, %3ld, %s\n", pTimeAbs, pTimeAbs - pBufStart, pTimeAbs - pBufStart, strlen(pTimeAbs), pTimeAbs);
	fprintf(stderr, "pTimeRel   = %p, %3ld, %2ld, %3ld, %s\n", pTimeRel, pTimeRel - pBufStart, pTimeRel - pTimeAbs, strlen(pTimeRel), pTimeRel);
	fprintf(stderr, "pTimeCnt   = %p, %3ld, %2ld, %3ld, %s\n", pTimeCnt, pTimeCnt - pBufStart, pTimeCnt - pTimeRel, strlen(pTimeCnt), pTimeCnt);
	fprintf(stderr, "pWhere     = %p, %3ld, %2ld, %3ld, %s\n", pWhere, pWhere - pBufStart, pWhere - pTimeCnt, strlen(pWhere), pWhere);
	fprintf(stderr, "pSeverity  = %p, %3ld, %2ld, %3ld, %s\n", pSeverity, pSeverity - pBufStart, pSeverity - pWhere, strlen(pSeverity), pSeverity);
	fprintf(stderr, "pWhatUser  = %p, %3ld, %2ld, %3ld, %s\n", pWhatUser, pWhatUser - pBufStart, pWhatUser - pSeverity, strlen(pWhatUser), pWhatUser);
	fprintf(stderr, "pBufEnd    = %p, %3ld, %2ld, %3ld\n", pBufEnd, pBufEnd - pBufStart, pBufEnd - pWhatUser, strlen(pBufEnd));

	free(pBufStart);

	exit(1);

	return code;
}

