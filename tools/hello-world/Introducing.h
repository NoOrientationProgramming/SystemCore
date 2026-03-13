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

#ifndef INTRODUCING_H
#define INTRODUCING_H

#include "Processing.h"
#include "ChildExecuting.h"

class Introducing : public Processing
{

public:

	/*
	 * All processes reside on the HEAP.
	 * Exceptions are not recommended => dNoThrow
	 */
	static Introducing *create()
	{
		return new dNoThrow Introducing;
	}

protected:

	virtual ~Introducing() {}

private:

	Introducing();
	Introducing(const Introducing &) = delete;
	Introducing &operator=(const Introducing &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */

	/*
	 * This is the _main_ function. It is used in EVERY SINGLE
	 * process and is executed on every 'clock cycle'.
	 * For all processes which are driven by the main thread,
	 * the clock cycle is this odd combination of executing
	 * multiple ticks and then sleep, as shown in main.cpp.
	 * This is indented. It's a 'simple scheduler' which
	 * is suitable for most projects.
	 * You can adapt the main scheduler to your needs or
	 * create your own 'clock domains' by using new threads,
	 * thread pools, or other specialized schedulers.
	 */
	Success process();

	/*
	 * For debugging, every process can show some
	 * information related to its work.
	 * If this function is used, the result of the
	 * print statements are visible in the process tree.
	 * The SystemDebugging() process must be started
	 * somewhere in the tree (usually root) to get this
	 * information via: telnet :: 3000
	 * This is an optional function. It can be removed.
	 * In this case, only the process name is visible
	 * in the tree when started.
	 */
	void processInfo(char *pBuf, char *pBufEnd);

	/*
	 * Internal functions for this 'Hello World' project.
	 */
	bool debuggerStart();
	void childrenStart(size_t idx);

	/* member variables */
	/*
	 * This variable is optional but
	 * used for timeouts very often.
	 */
	//uint32_t mStartMs;
	/*
	 * A child can be supervised.
	 * The pointer is valid as long as the child
	 * hasn't been repelled.
	 */
	ChildExecuting *mpThreadedChild;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

