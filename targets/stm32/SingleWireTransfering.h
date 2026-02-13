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

#ifndef SINGLE_WIRE_TRANSFERING_H
#define SINGLE_WIRE_TRANSFERING_H

#include "Processing.h"

const uint8_t cStartedTrans = 1 << 0;
const uint8_t cStartedDbg = 1 << 1;

const uint8_t cBufValidInCmd = 1 << 0;
const uint8_t cBufValidOutCmd = 1 << 2;
const uint8_t cBufValidOutLog = 1 << 4;
const uint8_t cBufValidOutProc = 1 << 6;

typedef void (*FuncDataSend)(char *pData, size_t len, void *pUser);

const uint8_t cSzBufMin = 3;
const uint8_t cSzBufInCmd = 64; // max. 255!
const size_t cSzBufOutProc = 1024;
const size_t cSzBufOutLog = 256;
const size_t cSzBufOutCmd = 128;

class SingleWireTransfering : public Processing
{

public:

	static SingleWireTransfering *create()
	{
		return new dNoThrow SingleWireTransfering;
	}

	uint8_t mModeDebug;
	bool mSyncedTransfer;

	void fctDataSendSet(FuncDataSend pFct, void *pUser);
	void dataReceived(char *pData, size_t len);
	void dataSent();
	void logImmediateSend();

	bool mSendReady;

	char mBufInCmd[cSzBufInCmd];
	char mBufOutProc[cSzBufOutProc];
	char mBufOutLog[cSzBufOutLog];
	char mBufOutCmd[cSzBufOutCmd];
	uint8_t mValidBuf;

	static uint8_t idStarted;

protected:

	virtual ~SingleWireTransfering() {}

private:

	SingleWireTransfering();
	SingleWireTransfering(const SingleWireTransfering &)
		: Processing("")
		, mModeDebug(0)
		, mSendReady(false)
		, mValidBuf(0)
		, mpSend(NULL)
		, mpUser(NULL)
		, mIdxBufDataWrite(0)
		, mBufTxPending(0)
		, mContentIdOut(0)
		, mValidIdTx(0)
		, mpDataTx(NULL)
		, mLenTx(0)
		, mStateRcv(0)
	{
		mState = 0;

		mBufId[0] = 0;
		mBufId[1] = 0;

		mBufInCmd[0] = 0;
		mBufOutProc[0] = 0;
		mBufOutLog[0] = 0;
		mBufOutCmd[0] = 0;
	}
	SingleWireTransfering &operator=(const SingleWireTransfering &)
	{
		mModeDebug = 0;
		mSendReady = false;
		mValidBuf = 0;
		mpSend = NULL;
		mpUser = NULL;
		mIdxBufDataWrite = 0;
		mBufTxPending = 0;
		mContentIdOut = 0;
		mValidIdTx = 0;
		mpDataTx = NULL;
		mLenTx = 0;
		mStateRcv = 0;

		mState = 0;

		mBufId[0] = 0;
		mBufId[1] = 0;

		mBufInCmd[0] = 0;
		mBufOutProc[0] = 0;
		mBufOutLog[0] = 0;
		mBufOutCmd[0] = 0;

		return *this;
	}

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	void processInfo(char *pBuf, char *pBufEnd);

	void byteProcess(uint8_t ch);
	bool dataInTerminate();
	void contentOutSend();

	/* member variables */
	FuncDataSend mpSend;
	void *mpUser;
	char mBufId[2];
	uint8_t mIdxBufDataWrite;
	volatile uint8_t mBufTxPending;
	char mContentIdOut;
	uint8_t mValidIdTx;
	char *mpDataTx;
	uint16_t mLenTx;
	uint8_t mStateRcv;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

