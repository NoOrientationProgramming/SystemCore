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
	"\033[39m",   /* default */	"\033[0;31m", /* red */
	"\033[0;33m", /* yellow */	"\033[39m",   /* default */
	"\033[0;36m", /* cyan */		"\033[0;35m", /* purple */
};
#endif

#ifdef CONFIG_PROC_LOG_COLOR_INF
#define dColorInfo CONFIG_PROC_LOG_COLOR_INF
#else
#define dColorInfo dColorDefault
#endif

const size_t cLenTimeAbs = 25;
const size_t cLenTimeRel = 8;
const size_t cLenWhere = 68;
const size_t cLenStrSeverity = 5;
const size_t cLenWhatUser = 46;
const size_t cLogEntryBufferSize =
		cLenTimeAbs + 1 +
		cLenTimeRel + 1 +
		cLenWhere + 1 +
		cLenStrSeverity + 1 +
		cLenWhatUser + 1;

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

	pFctCntTimeCreate = pFct;
	widthCntTime = width;
}

static int pBufSaturate(int lenDone, char * &pBuf, const char *pBufEnd)
{
	if (lenDone <= 0)
		return -1;

	if (lenDone > pBufEnd - pBuf)
		lenDone = pBufEnd - pBuf;

	pBuf += lenDone;

	return lenDone;
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
	int lenDone;
	va_list args;

	va_start(args, msg);
	lenDone = vfprintf(pStream, msg, args);
	if (lenDone < 0)
	{
		va_end(args);
		return code;
	}
	va_end(args);

	fprintf(pStream, "\r\n");
	fflush(pStream);

	return code;
}

static void strErr(char *pBuf)
{
	if (!pBuf) return;
	*pBuf++ = '-';
	*pBuf++ = 0;
}

#if CONFIG_PROC_LOG_HAVE_CHRONO
static void blockTimeAbsAdd(char *pBuf, char *pBufEnd, system_clock::time_point &t)
{
	int lenDone;
	int res;

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
	{
		strErr(pBuf);
		return;
	}

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

	lenDone = snprintf(pBuf, pBufEnd - pBuf,
					"%s  %02d:%02d:%02d.%03d ",
					timeBuf,
					int(durHours.count()), int(durMinutes.count()),
					int(durSecs.count()), int(durMillis.count()));
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		strErr(pBuf);
}

static void blockTimeRelAdd(char *pBuf, char *pBufEnd, system_clock::time_point &t)
{
	milliseconds durDiffMs = duration_cast<milliseconds>(t - tOld);
	long long tDiff = durDiffMs.count();
	int tDiffSec = int(tDiff / 1000);
	int tDiffMs = int(tDiff % 1000);
	bool diffMaxed = false;
	int lenDone;

	if (tDiffSec > cDiffSecMax)
	{
		tDiffSec = cDiffSecMax;
		tDiffMs = cDiffMsMax;

		diffMaxed = true;
	}

	lenDone = snprintf(pBuf, pBufEnd - pBuf,
					"%c%d.%03d  ",
					diffMaxed ? '>' : '+', tDiffSec, tDiffMs);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		strErr(pBuf);
}
#endif

static void blockTimeCntAdd(char *pBuf, char *pBufEnd)
{
	if (!pFctCntTimeCreate)
	{
		strErr(pBuf);
		return;
	}

	uint32_t cntTime = pFctCntTimeCreate();
	int lenDone;

	lenDone = snprintf(pBuf, pBufEnd - pBuf,
					"%*" PRIu32 "  ",
					widthCntTime, cntTime);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		strErr(pBuf);
}

static void blockWhereAdd(
			char *pBuf, char *pBufEnd,
			const void *pProc,
			const char *filename,
			const char *function,
			const int line)
{
	int lenDone;

	lenDone = snprintf(pBuf, pBufEnd - pBuf, "%-20s  ", function);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
	{
		strErr(pBuf);
		return;
	}

	if (pProc)
	{
		lenDone = snprintf(pBuf, pBufEnd - pBuf, "%p ", pProc);
		if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		{
			strErr(pBuf);
			return;
		}
	}

	lenDone = snprintf(pBuf, pBufEnd - pBuf, "%s:%-4d  ", filename, line);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
	{
		strErr(pBuf);
		return;
	}

	// prefix padding
	while (pBuf < pBufEnd)
		*pBuf++ = ' ';

	*--pBuf = 0;
}

static void blockSeverityAdd(
			char *pBuf, char *pBufEnd,
			const int severity)
{
	int res;

	res = snprintf(pBuf, pBufEnd - pBuf, "%s  ", tabStrSev[severity]);
	if (!res)
		strErr(pBuf);
}

static void blockWhatUserAdd(
			char *pBuf, char *pBufEnd,
			const char *msg, va_list args)
{
	int lenDone;

	lenDone = vsnprintf(pBuf, pBufEnd - pBuf, msg, args);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
	{
		strErr(pBuf);
		return;
	}
}

#if CONFIG_PROC_LOG_HAVE_STDOUT
static void toConsoleWrite(
			const int severity,
			char *pTimeAbs,
			char *pTimeRel,
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
			"\033[38:5:245m%s%s%s\033[0m"
			"%s%s%s"
			"%s\r\n",
			pTimeAbs, pTimeRel, pWhere,
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

	char *pTimeAbs = pBufStart;
	char *pTimeRel = pTimeAbs + cLenTimeAbs + 1;
	char *pWhere = pTimeRel + cLenTimeRel + 1;
	char *pSeverity = pWhere + cLenWhere + 1;
	char *pWhatUser = pSeverity + cLenStrSeverity + 1;
	char *pBufEnd = pWhatUser + cLenWhatUser + 1;

	// WHEN
#if CONFIG_PROC_LOG_HAVE_CHRONO
	system_clock::time_point t = system_clock::now();
	blockTimeAbsAdd(pTimeAbs, pTimeRel, t);
	blockTimeRelAdd(pTimeRel, pWhere, t);
	tOld = t;
#endif
	blockTimeCntAdd(NULL, NULL);

	// WHERE
	blockWhereAdd(pWhere, pSeverity, pProc, filename, function, line);

	// WHAT
	blockSeverityAdd(pSeverity, pWhatUser, severity);
	va_list args;
	va_start(args, msg);
	blockWhatUserAdd(pWhatUser, pBufEnd, msg, args);
	va_end(args);

	// Out
#if CONFIG_PROC_LOG_HAVE_STDOUT
	toConsoleWrite(severity,
			pTimeAbs,
			pTimeRel,
			pWhere,
			pSeverity,
			pWhatUser);
#endif
	if (pFctEntryLogCreate)
		pFctEntryLogCreate(severity,
			pProc, filename, function, line, code,
			pBufStart, pBufEnd - pBufStart);

	free(pBufStart);

	return code;
}

