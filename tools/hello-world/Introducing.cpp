/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 13.03.2026

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

#include "Introducing.h"
#include "SystemDebugging.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \
		gen(StNop) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

Introducing::Introducing()
	: Processing("Introducing")
	//, mStartMs(0)
	, mpThreadedChild(NULL)
{
	mState = StStart;
}

/* member functions */

Success Introducing::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	//Success success;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		levelLogSet(5);

		ok = debuggerStart();
		if (!ok)
			procWrnLog("could not start debugger");

		for (size_t i = 0; i < 3; ++i)
			childrenStart(i);

		userInfLog("Hello!");

		mState = StMain;

		break;
	case StMain:

		break;
	case StNop:

		break;
	default:
		break;
	}

	return Pending;
}

bool Introducing::debuggerStart()
{
	SystemDebugging *pDbg;

	pDbg = SystemDebugging::create(this);
	if (!pDbg)
	{
		procWrnLog("could not create process");
		return false;
	}

	start(pDbg);

	return true;
}

void Introducing::childrenStart(int idx)
{
	ChildExecuting *pChild;

	procInfLog("starting child %d", idx);

	pChild = ChildExecuting::create();
	if (!pChild)
	{
		procWrnLog("could not create process");
		return;
	}

	/*
	 * Configuration area
	 */
	pChild->mAsService = idx != 2;
	pChild->mDelayShutdown = idx == 3;

	/*
	 * Start the process
	 */

	if (idx)
	{
		start(pChild); // DrivenByParent (no new thread) is the default.
		return;
	}

	mpThreadedChild = pChild;

	/*
	 * Make this a multi-threaded application.
	 * Why? Because it's fun and easy!
	 * Just one flag is needed.
	 * In production systems you should create a new threads only if:
	 * - There are two or more CPU-bound processes AND
	 * - the target system hat two or more CPUs!
	 */
	start(pChild, DrivenByNewInternalDriver);
}

void Introducing::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

