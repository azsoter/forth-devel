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

#include <stdlib.h>
#include "forth_internal.h"

#if defined(FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS)

#define POP() *(rctx->sp++)
#define PUSH(X) *(--(rctx->sp)) = (forth_cell_t)(X)

// ALLOCATE ( size -- addr ior )
void forth_allocate(struct forth_runtime_context *rctx)
{
	forth_cell_t size = POP();
	void *addr = malloc(size);
	PUSH(addr);

	if (0 == addr)
	{
		PUSH(-9);
	}
	else
	{
		PUSH(0);
	}
}

// RESIZE ( addr1 size -- addr2 ior )
void forth_resize(struct forth_runtime_context *rctx)
{
	forth_cell_t size = POP();
	void *addr = (void *)(POP());
	void *new_addr;

	new_addr = realloc(addr, size);
	
	if (0 == new_addr)
	{
		PUSH(addr);
		PUSH(-9);
	}
	else
	{
		PUSH(new_addr);
		PUSH(0);
	}
}

// FREE (addr -- ior )
void forth_free(struct forth_runtime_context *rctx)
{
	void *addr = (void *)(POP());
	free(addr);
	PUSH(0);
}



#endif
