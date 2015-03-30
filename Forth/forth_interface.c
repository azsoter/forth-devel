/*
* Copyright (c) 2015 Andras Zsoter
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

#include <string.h>
#include "forth.h"
#include "forth_internal.h"
#include "forth_interface.h"
#include "forth_dict.h"

#if defined(FORTH_EXTERNAL_PRIMITIVES)
static forth_cell_t forth_align_dp(forth_cell_t *dictionary)
{
	forth_cell_t dp = FORTH_ALIGN(dictionary[FORTH_DP_LOCATION]);
	dictionary[FORTH_DP_LOCATION] = dp;
	return dp / sizeof(forth_cell_t);
}

static forth_cell_t forth_allot(forth_cell_t *dictionary, forth_cell_t byte_count)
{
	forth_cell_t dp = dictionary[FORTH_DP_LOCATION] + byte_count;
	dictionary[FORTH_DP_LOCATION] = dp;
	return dp / sizeof(forth_cell_t);
}

static forth_cell_t forth_here(forth_cell_t *dictionary)
{
	forth_cell_t dp = dictionary[FORTH_DP_LOCATION];
	return dp / sizeof(forth_cell_t);
	
}

forth_cell_t forth_create_name(struct forth_runtime_context *rctx, const char *name)
{
	size_t len;
	forth_cell_t start_ix;
	void *start_address;
	forth_cell_t header_ix;
	forth_cell_t *dictionary = rctx->dictionary;

	if ((0 == dictionary) || (0 == name))
	{
		return -9;	// Invalid address.
	}

	len = strlen(name);

	if (0 == len)
	{
		return -16;	// Zero length name.
	}

	if ((FORTH_HEADER_FLAGS_NAME_LENGTH_MASK) < len)
	{
		return -19;	// Definition name too long.
	}

	start_ix = forth_align_dp(dictionary);
	start_address = (void *)&dictionary[forth_here(dictionary)];
	forth_allot(dictionary, len);
	header_ix = forth_align_dp(dictionary);
	forth_allot(dictionary, 2 * sizeof(forth_cell_t));

	memcpy(start_address, name, len);
	dictionary[header_ix] = ((struct forth_wordlist *)(&dictionary[rctx->current]))->latest;
	dictionary[header_ix + 1] = len;
	return 0;
}

forth_cell_t forth_register_external_primitive(struct forth_runtime_context *rctx, const char *name, forth_cell_t index)
{
	forth_cell_t here;
	forth_cell_t *dictionary = rctx->dictionary;
	forth_cell_t res = forth_create_name(rctx, name);

	if (0 != res)
	{
		return res;
	}

	here = forth_here(dictionary);

	forth_allot(dictionary, 2 * sizeof(forth_cell_t));

	dictionary[here] = FORTH_PACK_TOKEN(FORTH_TOKEN_doextern);
	dictionary[here + 1] = index;
	((struct forth_wordlist *)(&dictionary[rctx->current]))->latest  = here - 2;
	return 0;
}
#endif

