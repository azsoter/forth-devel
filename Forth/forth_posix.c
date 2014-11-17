/*
* Copyright (c) 2014 Andras Zsoter
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/

// http://forth.teleonomix.com/

#include <unistd.h>
#include <time.h>
#include "forth.h"
#include "forth_internal.h"

#define POP() *(rctx->sp++)
#define PUSH(X) *(--(rctx->sp)) = (forth_cell_t)(X)


#if defined(FORTH_INCLUDE_MS)
void forth_ms(struct forth_runtime_context *rctx)
{
	forth_cell_t dly = POP();
	usleep(dly * 1000);
}
#endif

#if defined(FORTH_INCLUDE_TIME_DATE)
void forth_time_date(struct forth_runtime_context *rctx)
{
	struct tm *stm;
	time_t t;
	time(&t);
	stm = localtime(&t);
	PUSH(stm->tm_sec);
	PUSH(stm->tm_min);
	PUSH(stm->tm_hour);
	PUSH(stm->tm_mday);
	PUSH((1 + stm->tm_mon));
	PUSH((1900 + stm->tm_year));
}
#endif

