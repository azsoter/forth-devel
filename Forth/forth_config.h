#ifndef FORTH_CONFIG_H
#define FORTH_CONFIG_H
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

#include <stdint.h>

#define FORTH_32BIT 1
#undef  FORTH_64BIT

typedef uint8_t	forth_byte_t;

#if defined(FORTH_32BIT)

	typedef uint32_t forth_cell_t;
	typedef int32_t  forth_scell_t;
	typedef uint64_t forth_dcell_t;
	typedef int64_t  forth_sdcell_t;
	typedef forth_cell_t forth_xt_t;
#	define FORTH_CELL_HEX_DIGITS 8
#	define FORTH_NUM_BUFF_LENGTH (128 + 4)

#	define	FORTH_CELL_LOW(X) ((forth_cell_t)(0xFFFFFFFF & (X)))
#	define	FORTH_CELL_HIGH(X) ((forth_cell_t)(0xFFFFFFFF & ((X)>> 32)))
#	define  FORTH_DCELL(H, L)  ((((forth_dcell_t)(H)) << 32) + (L))
	typedef forth_cell_t forth_index_t;

// Works when sizeof(forth_cell_t) is a power of 2.
//
#	define FORTH_ALIGN(X) ((((forth_cell_t)(X)) + (sizeof(forth_cell_t) - 1)) & ~(sizeof(forth_cell_t) - 1))

#	define FORTH_MASK_TOKEN_INDICATOR 0x80000000
#	define FORTH_IS_NOT_TOKEN(X)	(0 == ((X) & (FORTH_MASK_TOKEN_INDICATOR )))
#	define FORTH_IS_TOKEN(X)	(((X) & (FORTH_MASK_TOKEN_INDICATOR )))
#	define FORTH_MASK_TOKEN_MASK   	0x00007FFF

#	define FORTH_BITSHIFT_for_TOKEN	16
#	define FORTH_EXTRACT_TOKEN(X)	(((X) >> FORTH_BITSHIFT_for_TOKEN) & (FORTH_MASK_TOKEN_MASK))
#	define FORTH_PACK_TOKEN(X)	(((X) << FORTH_BITSHIFT_for_TOKEN) | FORTH_MASK_TOKEN_INDICATOR)

#	define FORTH_PARAM_MAX		0xFFFF
#	define FORTH_PARAM_PACK(X)	((forth_cell_t)((X) & 0xFFFF))
#	define FORTH_PARAM_EXTRACT(X)	((X) & 0xFFFF)
#	define FORTH_PARAM_SIGNED(X)	((int16_t)(FORTH_PARAM_EXTRACT(X)))
#	define FORTH_PARAM_UNSIGNED(X)	((uint16_t)(FORTH_PARAM_EXTRACT(X)))
#	define FORTH_INDEX_EXTRACT(X)	((X) & 0x7FFFFFFF)
#	define FORTH_ORIG_SYS_ID	0x4F000000
#	define FORTH_DEST_SYS_ID	0x44000000
#	define FORTH_DO_SYS_ID		0x4C000000
#	define FORTH_SYS_ID_MASK	0x00FFFFFF

#else
#		if defined(FORTH_64BIT)
#		error 64bit systems are not supported yet.
#		else
#		error Either 32 or 64 bit must be selected.
#		endif
#endif

// For implementing well-formed flags in Forth
#define FORTH_FALSE (0)
#define FORTH_TRUE  (-1)
#define FORTH_CHAR_SPACE 0x20

#endif

