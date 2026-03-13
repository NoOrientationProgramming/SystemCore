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

#include <chrono>

#include "ChildExecuting.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \
		gen(StTalk) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;
using namespace chrono;

ChildExecuting::ChildExecuting()
	: Processing("ChildExecuting")
	, mAsService(false)
	, mDelayShutdown(false)
	, mStartMs(0)
	, mToldYa(false)
{
	mState = StStart;
}

/* member functions */

Success ChildExecuting::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	//Success success;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (!mAsService)
		{
			procInfLog("I will wait some time.");

			mStartMs = curTimeMs;
			mState = StTalk;
			break;
		}

		mState = StMain;

		break;
	case StMain:

		break;
	case StTalk:

		if (diffMs < 5000)
			break;

		procInfLog("Waiting done.");

		return Positive;

		break;
	default:
		break;
	}

	return Pending;
}

Success ChildExecuting::shutdown()
{
	if (!mDelayShutdown)
	{
		procWrnLog("I am not used anymore!");
		return Positive;
	}

	if (!mToldYa)
	{
		mToldYa = true;
		procWrnLog("I am not used anymore!");
		procWrnLog("I will delay my shutdown for one cycle.");
		return Pending;
	}

	return Positive;
}

void ChildExecuting::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	dInfo("Service process\t\t%s\n", mAsService ? "Yes" : "No");
	dInfo("Shutdown will be delayed\t%s\n", mDelayShutdown ? "Yes" : "No");
}

/* static functions */

uint32_t ChildExecuting::millis()
{
	auto now = steady_clock::now();
	auto nowMs = time_point_cast<milliseconds>(now);
	return (uint32_t)nowMs.time_since_epoch().count();
}

