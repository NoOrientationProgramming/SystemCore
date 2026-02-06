/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 30.11.2019

  Copyright (C) 2019, Johannes Natter

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

#include "SystemDebugging.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StSendReadyWait) \
		gen(StMain) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 0
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

#define dForEach_CmdState(gen) \
		gen(StCmdRcvdWait) \
		gen(StCmdInterpret) \
		gen(StCmdSendStart) \

#define dGenCmdStateEnum(s) s,
dProcessStateEnum(CmdState);

#if 0
#define dGenCmdStateString(s) #s,
dProcessStateStr(CmdState);
#endif

using namespace std;

#define CMD(x)		(!strncmp(pSwt->mBufInCmd, x, strlen(x)))

#ifndef dKeyModeDebug
#define dKeyModeDebug "aaaaa"
#endif

const uint16_t cCntDelayMin = 5000;

static SingleWireTransfering *pSwt = NULL;
#if CONFIG_PROC_HAVE_DRIVERS
static mutex mtxLogEntries;
#endif
static int levelLog = 3;
static bool logOvf = false;
static uint16_t idxInfo = 0;

const size_t dNumCmds = 23;
Command commands[dNumCmds] = {};

static bool mLogImmediateSend = false;

SystemDebugging::SystemDebugging(Processing *pTreeRoot)
	: Processing("SystemDebugging")
	, mpTreeRoot(pTreeRoot)
	, mpSend(NULL)
	, mpUser(NULL)
	, mReady(false)
	, mStateCmd(StCmdRcvdWait)
	, mCntDelay(0)
{
	mState = StStart;
}

/* member functions */

void SystemDebugging::fctDataSendSet(FuncDataSend pFct, void *pUser)
{
	mpSend = pFct;
	mpUser = pUser;
}

void SystemDebugging::logImmediateSendSet(bool val)
{
	mLogImmediateSend = val;
}

void SystemDebugging::dataReceived(char *pData, size_t len)
{
	if (!pSwt)
		return;

	pSwt->dataReceived(pData, len);
}

void SystemDebugging::dataSent()
{
	if (!pSwt)
		return;

	pSwt->dataSent();
}

bool SystemDebugging::ready()
{
	return mReady;
}

bool SystemDebugging::logOverflowed()
{
	return logOvf;
}

Command *freeCmdStructGet()
{
	Command *pCmd = commands;

	for (size_t i = 0; i < dNumCmds; ++i, ++pCmd)
	{
		if (pCmd->pId && pCmd->pFctExec)
			continue;

		return pCmd;
	}

	return NULL;
}

bool cmdReg(
		const char *pId,
		FuncCommand pFct,
		const char *pShortcut,
		const char *pDesc,
		const char *pGroup)
{
	if (cSzBufInCmd < 3)
	{
		errLog(-1, "err");
		return false;
	}

	bool foundPipe = false;
	bool foundCh = false;
	bool foundTerm = false;
	size_t i = 0;

	while (1)
	{
		if (i >= cSzBufInCmd - 2)
			break;

		if (pId[i] == '|')  foundPipe = true;
		if (pId[i] == '\0') foundTerm = true;

		if (foundTerm || foundPipe)
			break;

		foundCh = true;
		++i;
	}

	if (foundPipe || !foundTerm || !foundCh)
	{
		errLog(-1, "err");
		return false;
	}

	Command *pCmd = freeCmdStructGet();

	if (!pCmd)
	{
		errLog(-1, "err");
		return false;
	}

	pCmd->pId = pId;
	pCmd->pFctExec = pFct;
	pCmd->pShortcut = pShortcut;
	pCmd->pDesc = pDesc;
	pCmd->pGroup = pGroup;

	infLog("reg '%s'", pId);

	return true;
}

void SystemDebugging::levelLogSet(int lvl)
{
	levelLog = lvl;
}

Success SystemDebugging::process()
{
	switch (mState)
	{
	case StStart:

		if (!mpTreeRoot)
			return procErrLog(-1, "err");

		if (!mpSend)
			return procErrLog(-1, "err");

		if (SingleWireTransfering::idStarted & cStartedDbg)
			return procErrLog(-1, "err");

		SingleWireTransfering::idStarted |= cStartedDbg;

		pSwt = SingleWireTransfering::create();
		if (!pSwt)
			return procErrLog(-1, "err");

		pSwt->fctDataSendSet(mpSend, mpUser);
		pSwt->mSyncedTransfer = mLogImmediateSend;

		start(pSwt);

		cmdReg("infoHelp", cmdInfoHelp);
		cmdReg("levelLogSys", cmdLevelLogSysSet);

		mState = StSendReadyWait;

		break;
	case StSendReadyWait:

		if (!pSwt->mSendReady)
			break;

		entryLogCreateSet(SystemDebugging::entryLogEnqueue);

		mReady = true;

		mState = StMain;

		break;
	case StMain:

		commandInterpret();
		procTreeSend();

		break;
	default:
		break;
	}

	return Pending;
}

void SystemDebugging::commandInterpret()
{
	char *pBuf, *pBufEnd;
	size_t lenCmd, szBuf;
	Command *pCmd;

	switch (mStateCmd)
	{
	case StCmdRcvdWait: // fetch

		if (!(pSwt->mValidBuf & cBufValidInCmd))
			break;

		if (pSwt->mValidBuf & cBufValidOutCmd)
			break;

		mStateCmd = StCmdInterpret;

		break;
	case StCmdInterpret: // interpret/decode and execute

		//procInfLog("Received command: %s", pSwt->mBufInCmd);

		szBuf = sizeof(pSwt->mBufOutCmd);
		if (szBuf < 3)
		{
			pSwt->mValidBuf &= ~cBufValidInCmd; // don't answer
			mStateCmd = StCmdRcvdWait;
			break;
		}

		pBuf = pSwt->mBufOutCmd;
		pBufEnd = pBuf + szBuf;

		// <content ID>...<zero byte><content end>
		pBuf += 1; // make offset for content ID
		pBufEnd -= 2; // point to second to last byte

		*pBuf = 0; // dInfo!

		if (CMD(dKeyModeDebug))
		{
			pSwt->mModeDebug |= 1;
			dInfo("Debug mode %d", pSwt->mModeDebug);
			mStateCmd = StCmdSendStart;
			break;
		}

		if (!pSwt->mModeDebug)
		{
			pSwt->mValidBuf &= ~cBufValidInCmd; // don't answer
			mStateCmd = StCmdRcvdWait;
			break;
		}

		size_t lenId, lenShort;

		pCmd = commands;
		for (size_t i = 0; i < dNumCmds; ++i, ++pCmd)
		{
			lenId = strlen(pCmd->pId);
			if (pCmd->pShortcut && pCmd->pShortcut[0])
				lenShort = strlen(pCmd->pShortcut);
			else
				lenShort = 0;

			if (lenId >= cSzBufInCmd)
				continue;

			if (lenShort >= cSzBufInCmd)
				continue;

			if (CMD(pCmd->pId))
				lenCmd = lenId;
			else
			if (lenShort)
			{
				if (strncmp(pSwt->mBufInCmd, pCmd->pShortcut, lenShort))
					continue;

				if (pSwt->mBufInCmd[lenShort] && pSwt->mBufInCmd[lenShort] != ' ')
					continue;

				lenCmd = lenShort;
			}
			else
				continue;

			if (!pCmd->pFctExec)
				continue;

			char *pArg = pSwt->mBufInCmd + lenCmd;

			if (*pArg)
				++pArg;

			pCmd->pFctExec(pArg, pBuf, pBufEnd);

			*pBufEnd = 0;

			mStateCmd = StCmdSendStart;
			return;
		}

		dInfo("Unknown command");
		mStateCmd = StCmdSendStart;

		break;
	case StCmdSendStart: // write back

		pSwt->mBufInCmd[0] = 0;

		pSwt->mValidBuf |= cBufValidOutCmd;
		pSwt->mValidBuf &= ~cBufValidInCmd;

		mStateCmd = StCmdRcvdWait;

		break;
	default:
		break;
	}
}

void SystemDebugging::procTreeSend()
{
	if (!pSwt->mModeDebug)
		return; // minimize CPU load in production

	if (mCntDelay < cCntDelayMin)
	{
		++mCntDelay;
		return;
	}

	size_t szBuf = sizeof(pSwt->mBufOutProc);
	if (szBuf < 3)
		return;

	if (pSwt->mValidBuf & cBufValidOutProc)
		return;
	pSwt->mValidBuf |= cBufValidOutProc;

	mCntDelay = 0;

	char *pBuf = pSwt->mBufOutProc;
	char *pBufEnd = pBuf + szBuf;

	// <content ID>...<zero byte><content end>
	pBuf += 1; // make offset for content ID
	pBufEnd -= 2; // point to second to last byte

	*pBuf= 0;

	mpTreeRoot->processTreeStr(
				pBuf, pBufEnd,
				true, true);

	*pBufEnd = 0;
}

void SystemDebugging::processInfo(char *pBuf, char *pBufEnd)
{
	(void)pBuf;
	(void)pBufEnd;
#if 0
	dInfo("State\t\t%s\n", ProcStateString[mState]);
	dInfo("State cmd\t\t%s\n", CmdStateString[mStateCmd]);
#endif
}

/* static functions */

void SystemDebugging::cmdInfoHelp(char *pArgs, char *pBuf, char *pBufEnd)
{
	Command *pCmd;
	(void)pArgs;

	if (idxInfo >= dNumCmds)
		goto emptySend;

	pCmd = &commands[idxInfo];
	++idxInfo;

	if (!pCmd->pId || !pCmd->pFctExec)
		goto emptySend;

	dInfo("%s|%s|%s|%s",
		pCmd->pId,
		pCmd->pShortcut,
		pCmd->pDesc,
		pCmd->pGroup);

	return;

emptySend:
	*pBuf = 0;
	idxInfo = 0;
}

void SystemDebugging::cmdLevelLogSysSet(char *pArgs, char *pBuf, char *pBufEnd)
{
	const int lvlDefault = 2;
	int lvl = lvlDefault;

	if (pArgs && *pArgs >= '0' && *pArgs <= '5')
		lvl = *pArgs - '0';

	levelLogSet(lvl);
	dInfo("System log level set to %d", lvl);
}

void SystemDebugging::entryLogEnqueue(
		const int severity,
		const void *pProc,
		const char *filename,
		const char *function,
		const int line,
		const int16_t code,
		const char *msg,
		const size_t len)
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mtxLogEntries);
#endif
	(void)pProc;
	(void)filename;
	(void)function;
	(void)line;
	(void)code;

	if (severity > levelLog)
		return;

	if (!pSwt)
		return;

	size_t szBuf = sizeof(pSwt->mBufOutLog);
	if (szBuf < 3)
		return;

	if (pSwt->mValidBuf & cBufValidOutLog)
	{
		logOvf = true;
		return;
	}

	pSwt->mValidBuf |= cBufValidOutLog;

	char *pBuf = pSwt->mBufOutLog;
	char *pBufEnd = pBuf + szBuf;

	// <content ID>...<zero byte><content end>
	pBuf += 1; // make offset for content ID
	pBufEnd -= 2; // point to second to last byte

	size_t lenMax = pBufEnd - pBuf;
	size_t lenReq = PMIN(len, lenMax);

	*pBuf= 0;

	memcpy(pBuf, msg, lenReq);

	pBuf[lenReq] = 0;

	if (!pSwt->mSyncedTransfer)
		return;

	pSwt->logImmediateSend();
}

