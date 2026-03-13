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

#if defined(_WIN32)
#include <windows.h>
#else
#include <signal.h>
#endif
#include <iostream>
#include <chrono>
#include <thread>

#include "Introducing.h"

using namespace std;
using namespace chrono;

/*
 * This is the static pointer of our main application.
 * Every process is allocated on the HEAP.
 * But don't be scared!
 * It's very hard to produce a memory leak,
 * a double free, or something like that ;)
 */
Introducing *pApp = NULL;

// OS signal handler => Tell the application what to do on Ctrl-C
#if defined(_WIN32)
BOOL WINAPI applicationCloseRequest(DWORD signal)
{
	if (signal != CTRL_C_EVENT)
		return FALSE;

	cout << endl;
	pApp->unusedSet();

	return TRUE;
}
#else
void applicationCloseRequest(int signum)
{
	(void)signum;

	cout << endl;
	pApp->unusedSet();
}
#endif

int main(int argc, char *argv[])
{
	// Register OS signal handlers
#if defined(_WIN32)
	// https://learn.microsoft.com/en-us/windows/console/setconsolectrlhandler
	BOOL okWin = SetConsoleCtrlHandler(applicationCloseRequest, TRUE);
	if (!okWin)
	{
		errLog(-1, "could not set ctrl handler");
		return 1;
	}
#else
	// http://man7.org/linux/man-pages/man7/signal.7.html
	signal(SIGINT, applicationCloseRequest);
	signal(SIGTERM, applicationCloseRequest);
#endif
	(void)argc;
	(void)argv;

	// Create the app. Do not use exceptions.
	pApp = Introducing::create();
	if (!pApp)
	{
		errLog(-1, "could not create process");
		return 1;
	}

	/*
	 * This is the main driver loop.
	 * A driver is _something_ that pushes your processes forward.
	 * A driver can be:
	 * - The parent process
	 * - A new internal driver: A new thread
	 * - An external driver: Can be any schedular or a thread pool
	 * The first process (root) is driven by the main driver loop (main thread).
	 * It can be adjusted to the needs of your project.
	 * The numbers 12 and 15 are intended to be magic numbers!
	 */
	while (1)
	{
		/*
		 * This is our working day.
		 * Do an arbitrary amount.
		 */
		for (int i = 0; i < 12; ++i)
			pApp->treeTick();

		/*
		 * Every real system needs sleep!
		 * Sleep as long as you like.
		 * This is the only place in which we use a sleep!
		 */
		this_thread::sleep_for(chrono::milliseconds(15));

		/*
		 * Check if the application is still doing _something_!
		 * This also includes shutting down the tree when
		 * the app isn't used anymore or has finished its work.
		 */
		if (pApp->progress())
			continue;

		/*
		 * No work left.
		 * All processes in the entire tree have been shut down!
		 */
		break;
	}

	/*
	 * The processing of the app logic has finished.
	 * Check the success:
	 *   Positive or some negative number
	 * The success of a process tells you if the process
	 * has created its result successfully.
	 * Every process has its own result!
	 * => You decide in the public area of every concrete process class
	 *    what the result of the process is.
	 */
	Success success = pApp->success();

	// Counterpart to ::create()
	Processing::destroy(pApp);

	/*
	 * Closing the application means
	 * executing all 'global destructors' which have been registered
	 * in the process tree. For example:
	 * Executing 'close'-functions from external libraries.
	 */
	Processing::applicationClose();

	// Convert ..
	return !(success == Positive);
}

