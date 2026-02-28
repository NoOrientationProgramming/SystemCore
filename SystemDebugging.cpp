/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 12.05.2019

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
#if CONFIG_PROC_LOG_HAVE_CHRONO
#include <chrono>
#endif

#include "SystemDebugging.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 0
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;
#if CONFIG_PROC_LOG_HAVE_CHRONO
using namespace chrono;
#endif

typedef list<struct SystemDebuggingPeer>::iterator PeerIter;
queue<string> SystemDebugging::qLogEntries;
int SystemDebugging::levelLog = 3;
#if CONFIG_PROC_HAVE_DRIVERS
static mutex mtxLogEntries;
#endif
#if CONFIG_PROC_LOG_HAVE_CHRONO
static system_clock::time_point tLoggedInQueue;
#endif

const size_t SystemDebugging::maxPeers = 100;

const string cSeqCtrlC = "\xff\xf4\xff\xfd\x06";
const size_t cLenSeqCtrlC = cSeqCtrlC.size();

static char buffProcTree[8192];

SystemDebugging::SystemDebugging(Processing *pTreeRoot)
	: Processing("SystemDebugging")
	, mpTreeRoot(pTreeRoot)
	, mpLstProc(NULL)
	, mpLstLog(NULL)
	, mpLstCmd(NULL)
	, mpLstCmdAuto(NULL)
	, mPeerList()
	, mProcTree("")
	, mListenLocal(false)
	, mProcTreeChanged(false)
	, mProcTreePeerAdded(false)
	, mPeerLogOnceConnected(false)
	, mUpdateMs(500)
	, mProcTreeChangedTime(0)
	, mPortStart(3000)
{
	mState = StStart;
}

/* member functions */

void SystemDebugging::listenLocalSet()
{
	mListenLocal = true;
}

void SystemDebugging::portStartSet(uint16_t port)
{
	mPortStart = port;
}

bool SystemDebugging::ready()
{
	return mPeerLogOnceConnected;
}

void SystemDebugging::levelLogSet(int lvl)
{
	levelLog = lvl;
}

Success SystemDebugging::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (!mpTreeRoot)
			return procErrLog(-1, "tree root not set");

		success = listenersStart();
		if (success != Positive)
			return procErrLog(-1, "could not start listeners");

		cmdReg("levelLog", &SystemDebugging::cmdLevelLogSet, "", "Set the log level for stdout", cInternalCmdCls);
		cmdReg("levelLogSys", &SystemDebugging::cmdLevelLogSysSet, "", "Set the log level for socket", cInternalCmdCls);

		entryLogCreateSet(SystemDebugging::entryLogEnqueue);

		mState = StMain;

		break;
	case StMain:

		peerListUpdate();
		commandAutoProcess();

		processTreeSend();
#if CONFIG_PROC_HAVE_LOG
		logEntriesSend();
#endif
		break;
	default:
		break;
	}

	return Pending;
}

Success SystemDebugging::listenersStart()
{
	mPeerList.clear();

	// proc tree
	mpLstProc = TcpListening::create();
	if (!mpLstProc)
		return procErrLog(-1, "could not create process");

	mpLstProc->portSet(mPortStart, mListenLocal);

	start(mpLstProc);
#if CONFIG_PROC_HAVE_LOG
	// log
	mpLstLog = TcpListening::create();
	if (!mpLstLog)
		return procErrLog(-1, "could not create process");

	mpLstLog->portSet(mPortStart + 2, mListenLocal);

	start(mpLstLog);
#endif
	// command
	mpLstCmd = TcpListening::create();
	if (!mpLstCmd)
		return procErrLog(-1, "could not create process");

	mpLstCmd->portSet(mPortStart + 4, mListenLocal);
	mpLstCmd->maxConnQueuedSet(4);

	start(mpLstCmd);

	mpLstCmdAuto = TcpListening::create();
	if (!mpLstCmdAuto)
		return procErrLog(-1, "could not create process");

	mpLstCmdAuto->portSet(mPortStart + 6, mListenLocal);
	mpLstCmdAuto->maxConnQueuedSet(4);

	start(mpLstCmdAuto);

	return Positive;
}

void SystemDebugging::peerListUpdate()
{
	peerCheck();
	peerAdd(mpLstProc, PeerProc, "process tree");
#if CONFIG_PROC_HAVE_LOG
	peerAdd(mpLstLog, PeerLog, "log");
#endif
	peerAdd(mpLstCmd, PeerCmd, "command");
}

void SystemDebugging::commandAutoProcess()
{
	PipeEntry<SOCKET> peerFd;
	SystemCommanding *pCmd;

	while (1)
	{
		if (mpLstCmdAuto->ppPeerFd.get(peerFd) < 1)
			break;

		pCmd = SystemCommanding::create(peerFd.particle);
		if (!pCmd)
		{
			procErrLog(-1, "could not create process");
			continue;
		}

		pCmd->modeAutoSet();

		whenFinishedRepel(start(pCmd));
	}
}

bool SystemDebugging::disconnectRequestedCheck(TcpTransfering *pTrans)
{
	if (!pTrans)
		return false;

	char buf[31];
	ssize_t lenReq, lenPlanned, lenDone;

	lenReq = sizeof(buf) - 1;
	lenPlanned = lenReq;

	buf[0] = 0;
	buf[lenReq] = 0;

	lenDone = pTrans->read(buf, lenPlanned);
	if (!lenDone)
		return false;

	if (lenDone < 0)
		return true;

	buf[lenDone] = 0;

	if ((buf[0] == 0x03) || (buf[0] == 0x04))
	{
		procDbgLog("end of transmission");
		return true;
	}

	if (!strncmp(buf, cSeqCtrlC.c_str(), cLenSeqCtrlC))
	{
		procDbgLog("transmission cancelled");
		return true;
	}

	return false;
}

void SystemDebugging::peerCheck()
{
	PeerIter iter;
	struct SystemDebuggingPeer peer;
	Processing *pProc;
	bool disconnectReq, removeReq;

	iter = mPeerList.begin();
	while (iter != mPeerList.end())
	{
		peer = *iter;
		pProc = peer.pProc;

		if (peer.type == PeerProc)
			disconnectReq = disconnectRequestedCheck((TcpTransfering *)pProc);
		else
		if (peer.type == PeerLog)
		{
			TcpTransfering *pTrans = (TcpTransfering *)pProc;
			disconnectReq = disconnectRequestedCheck(pTrans);
			mPeerLogOnceConnected |= pTrans->mSendReady;
		}
		else
			disconnectReq = false;

		removeReq = (pProc->success() != Pending) || disconnectReq;
		if (!removeReq)
		{
			++iter;
			continue;
		}

		procDbgLog("removing %s peer. process: %p", peer.typeDesc.c_str(), pProc);
		repel(pProc);

		iter = mPeerList.erase(iter);
	}
}

void SystemDebugging::peerAdd(TcpListening *pListener, enum PeerType peerType, const char *pTypeDesc)
{
	PipeEntry<SOCKET> peerFd;
	Processing *pProc = NULL;
	struct SystemDebuggingPeer peer;

	while (1)
	{
		if (pListener->ppPeerFd.get(peerFd) < 1)
			break;

		if (peerType == PeerCmd)
		{
			pProc = SystemCommanding::create(peerFd.particle);
			if (!pProc)
			{
				procErrLog(-1, "could not create process");
				continue;
			}

			whenFinishedRepel(start(pProc));

			continue;
		}

		pProc = TcpTransfering::create(peerFd.particle);
		if (!pProc)
		{
			procErrLog(-1, "could not create process");
			continue;
		}

		start(pProc);

		procDbgLog("adding %s peer. process: %p", pTypeDesc, pProc);

		peer.type = peerType;
		peer.typeDesc = pTypeDesc;
		peer.pProc = pProc;

		mPeerList.push_back(peer);

		if (peerType == PeerProc)
		{
			mProcTreeChangedTime -= mUpdateMs;
			mProcTreePeerAdded = true;
		}
	}
}

void SystemDebugging::processTreeSend()
{
	if (mProcTreeChanged)
	{
		uint32_t diffMs = nowMs() - mProcTreeChangedTime;

		if (diffMs < mUpdateMs)
			return;

		mProcTreeChanged = false;
	}

	*buffProcTree = 0;

	bool detailed = true;
	bool colored = true;

	mpTreeRoot->processTreeStr(
			buffProcTree,
			buffProcTree + sizeof(buffProcTree),
			detailed,
			colored);

	string procTree(buffProcTree);

	bool procTreeUpdated = procTree != mProcTree || mProcTreePeerAdded;

	if (!procTreeUpdated)
		return;

	mProcTreePeerAdded = false;

	//procDbgLog("process tree changed");
	//procDbgLog("\n%s", procTree.c_str());

	string msg("\033[2J\033[H");

	msg += procTree;

	PeerIter iter;
	struct SystemDebuggingPeer peer;
	TcpTransfering *pTrans = NULL;

	iter = mPeerList.begin();
	while (iter != mPeerList.end())
	{
		peer = *iter++;
		pTrans = (TcpTransfering *)peer.pProc;

		if (!pTrans->mSendReady)
			continue;

		if (peer.type == PeerProc)
			pTrans->send(msg.c_str(), msg.size());
	}

	mProcTree = procTree;

	mProcTreeChanged = true;
	mProcTreeChangedTime = nowMs();
}

#if CONFIG_PROC_HAVE_LOG
void SystemDebugging::logEntriesSend()
{
	string msg;
	PeerIter iter;
	struct SystemDebuggingPeer peer;
	TcpTransfering *pTrans = NULL;

	while (1)
	{
		{
#if CONFIG_PROC_HAVE_DRIVERS
			Guard lock(mtxLogEntries);
#endif
			if (!qLogEntries.size())
				break;

			msg = qLogEntries.front();
			qLogEntries.pop();
		}

		if (!msg.size())
			break;

		msg += "\r\n";

		iter = mPeerList.begin();
		while (iter != mPeerList.end())
		{
			peer = *iter++;
			pTrans = (TcpTransfering *)peer.pProc;

			if (!pTrans->mSendReady)
				continue;

			if (peer.type == PeerLog)
				pTrans->send(msg.c_str(), msg.size());
		}
	}
}
#endif

void SystemDebugging::processInfo(char *pBuf, char *pBufEnd)
{
	dInfo("Update period [ms]\t\t%d\n", (int)mUpdateMs);
}

/* static functions */
void SystemDebugging::cmdLevelLogSet(char *pArgs, char *pBuf, char *pBufEnd)
{
	const int lvlDefault = 2;
	int lvl = pArgs ? atoi(pArgs) : lvlDefault;

	if (lvl < 0)
		lvl = lvlDefault;

	::levelLogSet(lvl);
	dInfo("Log level set to %d", lvl);
}

void SystemDebugging::cmdLevelLogSysSet(char *pArgs, char *pBuf, char *pBufEnd)
{
	const int lvlDefault = 2;
	int lvl = pArgs ? atoi(pArgs) : lvlDefault;

	if (lvl < 0)
		lvl = lvlDefault;

	levelLogSet(lvl);
	dInfo("System log level set to %d", lvl);
}

static const char *tabColors[] =
{
	"\033[39m",   /* default */	"\033[0;31m", /* red */		"\033[0;33m", /* yellow */
	"\033[39m",   /* default */	"\033[0;36m", /* cyan */		"\033[0;35m", /* purple */
};

void SystemDebugging::entryLogEnqueue(
			const int severity,
#if CONFIG_PROC_LOG_HAVE_CHRONO
			const char *pTimeAbs,
			const char *pTimeRel,
			const system_clock::time_point &tLogged,
#endif
			const char *pTimeCnt,
			const char *pWhere,
			const char *pSeverity,
			const char *pWhatUser)
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mtxLogEntries);
#endif
	if (severity > levelLog)
		return;

	string str;

	str += "\033[38:5:245m";
#if CONFIG_PROC_LOG_HAVE_CHRONO
	str += pTimeAbs;
	str += pTimeRel;
#endif
	str += pTimeCnt;
	str += pWhere;
	str += tabColors[0];

	str += tabColors[severity];
	str += pSeverity;
	str += tabColors[0];

	str += pWhatUser;

	qLogEntries.emplace(str);
#if CONFIG_PROC_LOG_HAVE_CHRONO
	tLoggedInQueue = tLogged;
#endif
}

