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

#ifdef _WIN32
const WORD red = 4;
const WORD yellow = 6;
const WORD cyan = 3;
const WORD dColorDefault = 7;
#else
const char *red = "\033[0;31m";
const char *yellow = "\033[0;33m";
const char *cyan = "\033[0;36m";
const char *dColorDefault = "\033[39m";
#endif

#ifdef CONFIG_PROC_LOG_COLOR_INF
#define dColorInfo CONFIG_PROC_LOG_COLOR_INF
#else
#define dColorInfo dColorDefault
#endif

const size_t cLogEntryBufferSize = 230;
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
	pFctEntryLogCreate = pFct;
}

void cntTimeCreateSet(FuncCntTimeCreate pFct, int width)
{
	if (width < -20 || width > 20)
		return;

	pFctCntTimeCreate = pFct;
	widthCntTime = width;
}

static const char *severityToStr(const int severity)
{
	switch (severity)
	{
	case 1: return "ERR";
	case 2: return "WRN";
	case 3: return "INF";
	case 4: return "DBG";
	case 5: return "COR";
	default: break;
	}
	return "INV";
}

static int pBufSaturate(int lenDone, char * &pBuf, const char *pBufEnd)
{
	if (lenDone < 0)
		return lenDone;

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

static int blockWhenAdd(char *pBuf, char *pBufEnd)
{
	int szBuf = pBufEnd - pBuf;
	int lenDone;
	int res;
#if CONFIG_PROC_LOG_HAVE_CHRONO
	// get time
	system_clock::time_point t = system_clock::now();
	milliseconds durDiffMs = duration_cast<milliseconds>(t - tOld);

	// build day
	time_t tTt = system_clock::to_time_t(t);
	char timeBuf[32];
	tm tTm {};
#ifdef _WIN32
	::localtime_s(&tTm, &tTt);
#else
	::localtime_r(&tTt, &tTm);
#endif
	res = strftime(pBuf, sizeof(szBuf), "%Y-%m-%d", &tTm);
	if (!res)
		return 0;

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

	// build diff
	long long tDiff = durDiffMs.count();
	int tDiffSec = int(tDiff / 1000);
	int tDiffMs = int(tDiff % 1000);
	bool diffMaxed = false;

	if (tDiffSec > cDiffSecMax)
	{
		tDiffSec = cDiffSecMax;
		tDiffMs = cDiffMsMax;

		diffMaxed = true;
	}

	lenDone = snprintf(pBuf, pBufEnd - pBuf,
					"%s  %02d:%02d:%02d.%03d "
					"%c%d.%03d  ",
					timeBuf,
					int(durHours.count()), int(durMinutes.count()),
					int(durSecs.count()), int(durMillis.count()),
					diffMaxed ? '>' : '+', tDiffSec, tDiffMs);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		return -1;
#endif
	if (pFctCntTimeCreate)
	{
		uint32_t cntTime = pFctCntTimeCreate();

		lenDone = snprintf(pBuf, pBufEnd - pBuf,
						"%*" PRIu32 "  ",
						widthCntTime, cntTime);

		if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
			return -1;
	}

	return lenDone;
}

static int blockWhereAdd(
			char *pBuf, char *pBufEnd,
			const void *pProc,
			const char *filename,
			const char *function,
			const int line)
{
	int lenPrefix2 = 73;
	int lenDone;

	lenDone = snprintf(pBuf, pBufEnd - pBuf, "%-20s  ", function);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		return -1;

	if (pProc)
	{
		lenDone = snprintf(pBuf, pBufEnd - pBuf, "%p ", pProc);
		if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
			return -1;
	}

	lenDone = snprintf(pBuf, pBufEnd - pBuf, "%s:%-4d  ", filename, line);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		return -1;

	// prefix padding
	while (lenDone < lenPrefix2 && pBuf < pBufEnd)
	{
		*pBuf++ = ' ';
		++lenDone;
	}

	return lenDone;
}

static int blockWhatAdd(
			char *pBuf, char *pBufEnd,
			const int severity,
			const char *msg, va_list args)
{
	int lenDone;

	snprintf(pBuf, pBufEnd - pBuf, "%s  ", severityToStr(severity));

	lenDone = vsnprintf(pBuf, pBufEnd - pBuf, msg, args);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		return -1;

	return lenDone;
}

static int toConsoleWrite(char *pBufStart,
			const int severity)
{
#if CONFIG_PROC_LOG_HAVE_STDOUT
	// create log entry
	if (severity > levelLog)
		return 0;
#if 0
#if CONFIG_PROC_LOG_HAVE_CHRONO
	tOld = t;
#endif
#endif
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(severity < 3 ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO infoConsole;

	GetConsoleScreenBufferInfo(hConsole, &infoConsole);

	WORD colorBkup = infoConsole.wAttributes;

	if (severity == 1)
	{
		SetConsoleTextAttribute(hConsole, red);
		fprintf(stderr, "%s\r\n", pBufStart);
		fflush(stderr);
		SetConsoleTextAttribute(hConsole, colorBkup);
	}
	else
	if (severity == 2)
	{
		SetConsoleTextAttribute(hConsole, yellow);
		fprintf(stderr, "%s\r\n", pBufStart);
		fflush(stderr);
		SetConsoleTextAttribute(hConsole, colorBkup);
	}
	else
	if (severity >= 4)
	{
		SetConsoleTextAttribute(hConsole, cyan);
		fprintf(stdout, "%s\r\n", pBufStart);
		fflush(stdout);
		SetConsoleTextAttribute(hConsole, colorBkup);
	}
	else
	{
		SetConsoleTextAttribute(hConsole, dColorInfo);
		fprintf(stdout, "%s\r\n", pBufStart);
		fflush(stdout);
		SetConsoleTextAttribute(hConsole, colorBkup);
	}
#else
	if (severity == 1)
		fprintf(stderr, "%s%s%s\r\n", red, pBufStart, dColorDefault);
	else
	if (severity == 2)
		fprintf(stderr, "%s%s%s\r\n", yellow, pBufStart, dColorDefault);
	else
	if (severity >= 4)
		fprintf(stdout, "%s%s%s\r\n", cyan, pBufStart, dColorDefault);
	else
		fprintf(stdout, "%s%s%s\r\n", dColorInfo, pBufStart, dColorDefault);
#endif
#endif
	return 0;
}

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
	char *pBufStart = (char *)malloc(cLogEntryBufferSize);
	if (!pBufStart)
		return code;

	char *pBuf = pBufStart;
	char *pBufEnd = pBuf + cLogEntryBufferSize - 1;
	int lenDone;

	*pBuf = 0;
	*pBufEnd = 0;

	// WHEN
	lenDone = blockWhenAdd(pBuf, pBufEnd);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		goto exitLogEntryCreate;

	// WHERE
	lenDone = blockWhereAdd(pBuf, pBufEnd, pProc, filename, function, line);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		goto exitLogEntryCreate;

	// WHAT
	va_list args;
	va_start(args, msg);
	lenDone = blockWhatAdd(pBuf, pBufEnd, severity, msg, args);
	va_end(args);
	if (pBufSaturate(lenDone, pBuf, pBufEnd) < 0)
		goto exitLogEntryCreate;

	// Out
	toConsoleWrite(pBufStart, severity);

	if (pFctEntryLogCreate)
		pFctEntryLogCreate(severity,
			pProc, filename, function, line, code,
			pBufStart, pBuf - pBufStart);

exitLogEntryCreate:
	free(pBufStart);

	return code;
}

