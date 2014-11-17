#ifndef FORTH_H
#define FORTH_H
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
#include "forth_config.h"
#include "forth_features.h"

// The Version number of the forth engine.
// 3 Bytes: Major, minor, patch.
#define FORTH_ENGINE_VERSION 0x00000004	// I.e.: 0.0.4

#if defined(FORTH_USER_VARIABLES)

#if ((FORTH_USER_VARIABLES) > 0xFFFF)
#error Too many USER variables.
#endif

#if ((FORTH_USER_VARIABLES) == 0)
#error Number of USER variables cannot be zero. #undef FORTH_USER_VARIABLES if you want to disable USER.
#endif

#endif

enum forth_token_t
{
	FORTH_TOKEN_NOP = 0,
	FORTH_TOKEN_ACCEPT,
	FORTH_TOKEN_KEY,
	FORTH_TOKEN_EKEY,
	FORTH_TOKEN_KEYq,	// KEY?
	FORTH_TOKEN_EKEYq,	// EKEY?
	FORTH_TOKEN_EKEY2CHAR,	// EKEY>CHAR
#if defined(FORTH_USER_VARIABLES)
	FORTH_TOKEN_USER_ALLOT, // USER-ALLOT
#endif
	FORTH_TOKEN_ALIGN,
	FORTH_TOKEN_ALIGNED,
	FORTH_TOKEN_ALLOT,
	FORTH_TOKEN_pHERE,	// (HERE)
	FORTH_TOKEN_pTRACE,	// (TRACE)
	FORTH_TOKEN_HERE,
	FORTH_TOKEN_PAD,
	FORTH_TOKEN_CComma,	// C,
	FORTH_TOKEN_CompileComma,	// COMPILE,
	FORTH_TOKEN_Comma,	// ,
	FORTH_TOKEN_Imm_Plus,	// IMM+
	FORTH_TOKEN_CELLS,
	FORTH_TOKEN_DROP,
	FORTH_TOKEN_DUP,
	FORTH_TOKEN_qDUP,	// ?DUP
	FORTH_TOKEN_NIP,
	FORTH_TOKEN_TUCK,
	FORTH_TOKEN_ROT,
	FORTH_TOKEN_ROLL,
	FORTH_TOKEN_PICK,
	FORTH_TOKEN_OVER,
	FORTH_TOKEN_SWAP,
	FORTH_TOKEN_2ROT,
	FORTH_TOKEN_2DUP,
	FORTH_TOKEN_2DROP,
	FORTH_TOKEN_2OVER,
	FORTH_TOKEN_2SWAP,
	FORTH_TOKEN_2Fetch,	// 2@
	FORTH_TOKEN_2Store,	// 2!
	FORTH_TOKEN_Fetch,	// @
	FORTH_TOKEN_Store,	// !
	FORTH_TOKEN_CFetch,	// C@
	FORTH_TOKEN_CStore,	// C!
	FORTH_TOKEN_PlusStore,	// +!
	FORTH_TOKEN_DUMP,
	FORTH_TOKEN_UNUSED,
	FORTH_TOKEN_EXECUTE,
	FORTH_TOKEN_SEARCH_WORDLIST,	// SEARCH-WORDLIST
	FORTH_TOKEN_FIND_WORD,	// FIND-WORD
	FORTH_TOKEN_COMPARE,
	FORTH_TOKEN_LessHash,	// <#
#if !defined(FORTH_NO_DOUBLES)
	FORTH_TOKEN_Hash,	// #
#endif
	FORTH_TOKEN_HashGreater, // #>
	FORTH_TOKEN_HOLD,
	FORTH_TOKEN_Hdot,	// H.
	FORTH_TOKEN_Dot,	// .
	FORTH_TOKEN_DotS,	// .S
	FORTH_TOKEN_Udot,	// U.
	FORTH_TOKEN_DotR,	// .R
	FORTH_TOKEN_UdotR,	// U.R
	FORTH_TOKEN_DotName,	// .NAME
	FORTH_TOKEN_CMOVE,
	FORTH_TOKEN_CMOVE_down,	// CMOVE>
	FORTH_TOKEN_MOVE,
	FORTH_TOKEN_FILL,
	FORTH_TOKEN_LATEST,
	FORTH_TOKEN_pDefining,	// (DEFINING)
	FORTH_TOKEN_PARSE,
	FORTH_TOKEN_PARSE_WORD,	// PARSE-WORD
	FORTH_TOKEN_WORD,
	FORTH_TOKEN_Plus,	// +
	FORTH_TOKEN_Subtract,	// -
	FORTH_TOKEN_Divide,	// /
	FORTH_TOKEN_Multiply,	// *
	FORTH_TOKEN_MOD,	// MOD
	FORTH_TOKEN_Slash_MOD,	// /MOD
#if !defined(FORTH_NO_DOUBLES)
	FORTH_TOKEN_StarSlash,	// */
	FORTH_TOKEN_StarSlash_MOD,	// */MOD
	FORTH_TOKEN_UM_Star,		// UM*
	FORTH_TOKEN_M_Star,		// M*
	FORTH_TOKEN_M_Plus,		// M+
	FORTH_TOKEN_UM_Slash_MOD,	// UM/MOD
#endif
	FORTH_TOKEN_NEGATE,
	FORTH_TOKEN_ABS,
	FORTH_TOKEN_MIN,
	FORTH_TOKEN_MAX,
	FORTH_TOKEN_LSHIFT,
	FORTH_TOKEN_RSHIFT,
	FORTH_TOKEN_2_Star,	// 2*
	FORTH_TOKEN_2_Slash,	// 2/

#if !defined(FORTH_NO_DOUBLES)
	FORTH_TOKEN_D2_Star,	// D2*
	FORTH_TOKEN_D2_Slash,	// D2/

	FORTH_TOKEN_DNEGATE,
	FORTH_TOKEN_DABS,
	FORTH_TOKEN_DMIN,
	FORTH_TOKEN_DMAX,
	FORTH_TOKEN_D_Plus,	// D+
	FORTH_TOKEN_D_Subtract,	// D-
	FORTH_TOKEN_D_Less,	// D<
	FORTH_TOKEN_D_ULess,	// DU<
#endif
	FORTH_TOKEN_D_Equal,	// D=
	FORTH_TOKEN_AND,
	FORTH_TOKEN_OR,
	FORTH_TOKEN_XOR,
	FORTH_TOKEN_INVERT,
	FORTH_TOKEN_toR,	// >R
	FORTH_TOKEN_Rfrom,	// R>
	FORTH_TOKEN_Rfetch,	// R@
	FORTH_TOKEN_2toR,	// 2>R
	FORTH_TOKEN_2Rfrom,	// 2R>
	FORTH_TOKEN_2Rfetch,	// 2R@
	FORTH_TOKEN_NtoR,	// N>R
	FORTH_TOKEN_NRfrom,	// NR>
	FORTH_TOKEN_PROCESS_NUMBER, // PROCESS-NUMBER ( caddr len -- s|d|f 0|1|-1 )
#if !defined(FORTH_NO_DOUBLES)
	FORTH_TOKEN_toNUMBER, 	// >NUMBER ( ud1 caddr1 len1 -- ud2 caddr2 len2 )
#endif
	FORTH_TOKEN_TIB,
	FORTH_TOKEN_HashTIB,	// #TIB
	FORTH_TOKEN_BLK,
	FORTH_TOKEN_QUERY,
	FORTH_TOKEN_REFILL,
	FORTH_TOKEN_pSOURCE_ID,	// (SOURCE-ID)
	FORTH_TOKEN_SOURCE,	// SOURCE
	FORTH_TOKEN_SOURCE_Store,	// SOURCE!
	FORTH_TOKEN_toIN,	// >IN
	FORTH_TOKEN_LINE_NUMBER,	// LINE-NUMBER ( line number inside a file).
	FORTH_TOKEN_SAVE_INPUT,	// SAVE-INPUT
	FORTH_TOKEN_RESTORE_INPUT, // RESTORE-INPUT
	FORTH_TOKEN_BASE,
	FORTH_TOKEN_STATE,
	FORTH_TOKEN_THROW,
	FORTH_TOKEN_DotError, // .ERROR ( err -- )
	FORTH_TOKEN_AT_XY,	// AT-XY
	FORTH_TOKEN_PAGE,

#if defined(FORTH_INCLUDE_MS)
	FORTH_TOKEN_MS,
#endif

#if defined(FORTH_INCLUDE_TIME_DATE)
	FORTH_TOKEN_TIME_DATE,	// TIME&DATE
#endif

	FORTH_TOKEN_CR,
	FORTH_TOKEN_EMIT,
	FORTH_TOKEN_TYPE,
	FORTH_TOKEN_BYE,
	FORTH_TOKEN_ENVIRONMENTq,	// ENVIRONMENT?
	FORTH_TOKEN_WORDS,
	FORTH_TOKEN_pSEE,	// (SEE) ( xt -- )
	FORTH_TOKEN_ONLY,
	FORTH_TOKEN_ALSO,
	FORTH_TOKEN_GET_ORDER,
	FORTH_TOKEN_SET_ORDER,
	FORTH_TOKEN_CURRENT,
	FORTH_TOKEN_CONTEXT,
	FORTH_TOKEN_toBODY,	// >BODY
	FORTH_TOKEN_Equal,	// =
	FORTH_TOKEN_Notequal,	// <>
	FORTH_TOKEN_Less,	// <
	FORTH_TOKEN_Greater,	// >
	FORTH_TOKEN_ULess,	// U<
	FORTH_TOKEN_UGreater,	// U>
	FORTH_TOKEN_0Equal,	// 0=
	FORTH_TOKEN_0Notequal,	// 0<>
	FORTH_TOKEN_0Less,	// 0<
	FORTH_TOKEN_0Greater,	// 0>
	FORTH_TOKEN_LITERAL,
	FORTH_TOKEN_sp0,	// SP0
	FORTH_TOKEN_sp_fetch,	// SP@
	FORTH_TOKEN_sp_store,	// SP!
	FORTH_TOKEN_rp0,	// RP0
	FORTH_TOKEN_rp_fetch,	// RP@
	FORTH_TOKEN_rp_store,	// RP!
	FORTH_TOKEN_handler,	// Exception handler.
	FORTH_TOKEN_abort_msg,	// To hold the string for ABORT".
	FORTH_TOKEN_pDO,	// (DO)
	FORTH_TOKEN_pqDO,	// (?DO)
	FORTH_TOKEN_UNLOOP,	// UNLOOP
	FORTH_TOKEN_LEAVE,	// LEAVE,
	FORTH_TOKEN_I,		// I
	FORTH_TOKEN_J,		// J
	FORTH_TOKEN_pLOOP,	// (LOOP)
	FORTH_TOKEN_pPlusLOOP,	// (+LOOP)
	FORTH_TOKEN_EXIT,
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
	FORTH_TOKEN_FILE_CREATE,	// CREATE-FILE
	FORTH_TOKEN_FILE_OPEN,		// OPEN-FILE
	FORTH_TOKEN_FILE_FLUSH,		// FLUSH-FILE
	FORTH_TOKEN_FILE_DELETE,	// DELETE-FILE
	FORTH_TOKEN_FILE_REPOSITION,	// REPOSITION-FILE
	FORTH_TOKEN_FILE_POSITION,	// FILE-POSITION
	FORTH_TOKEN_FILE_SIZE,		// FILE-SIZE
	FORTH_TOKEN_FILE_READ,		// READ-FILE
	FORTH_TOKEN_FILE_READ_LINE,	// READ-LINE
	FORTH_TOKEN_FILE_WRITE,		// WRITE-FILE
	FORTH_TOKEN_FILE_WRITE_LINE,	// WRITE-LINE
	FORTH_TOKEN_FILE_CLOSE,		// CLOSE-FILE
#endif
#if defined(FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS)
	FORTH_TOKEN_ALLOCATE,
	FORTH_TOKEN_RESIZE,
	FORTH_TOKEN_FREE,
#endif
	FORTH_TOKEN_resolve_branch, // resolve-branch
	FORTH_TOKEN_ix2address,		// IX>ADDRESS
	FORTH_TOKEN_branch,
	FORTH_TOKEN_0branch,
	FORTH_TOKEN_xtlit,
	FORTH_TOKEN_lit,
	FORTH_TOKEN_uslit,
	FORTH_TOKEN_sslit,
	FORTH_TOKEN_strlit,
	FORTH_TOKEN_nest,
	FORTH_TOKEN_unnest,
	FORTH_TOKEN_dovar,
	FORTH_TOKEN_doconst,
	FORTH_TOKEN_docreate,
#if defined(FORTH_USER_VARIABLES)
	FORTH_TOKEN_douser,
#endif
};

struct forth_persistent_context
{
	forth_cell_t	dp;
	forth_cell_t	dp_max;
	forth_cell_t	*dictionary;	
};

struct forth_wordlist
{
	forth_cell_t latest;
	forth_cell_t parent;
	forth_cell_t link;
};

#define FORTH_HEADER_FLAGS_NAME_LENGTH_MASK	0x0000FFFF
#define FORTH_HEADER_FLAGS_IMMEDIATE		0x80000000
#define FORTH_HEADER_FLAGS_TOKEN		0x20000000

#define FORTH_COLON_SYS				0x55aa4884

struct forth_header
{
	forth_cell_t link;
	forth_cell_t flags;
};

struct forth_runtime_context
{
	forth_cell_t	*sp_max;
	forth_cell_t	*sp_min;
	forth_cell_t	*sp0;
	forth_cell_t	*sp;
	forth_cell_t	*rp_max;
	forth_cell_t	*rp_min;
	forth_cell_t	*rp0;
	forth_cell_t	*rp;
	forth_index_t	ip;
	forth_cell_t	base;	// Numeric base.
	forth_cell_t	state;
	forth_cell_t	handler;
	forth_cell_t	abort_msg_len;		// Used by ABORT"
	forth_cell_t	abort_msg_addr;		// Used by ABORT"
	forth_cell_t	blk;		// BLK
	forth_cell_t	source_id;
	char		*source_address;
	forth_cell_t	source_length;
	forth_cell_t	to_in;
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
	forth_dcell_t	source_file_position;
	forth_cell_t	line_no;
	char file_buffer[FORTH_FILE_INPUT_BUFFER_LENGTH];
#endif
	forth_cell_t	*wordlists;	// Wordlists in the search order.
	forth_cell_t	wordlist_slots;	// The number of slots in the search order.
	forth_cell_t	wordlist_cnt;	// The number of workdlists in the search order.
	forth_cell_t	current;	// The current wordlist (where definitions are appended).
	forth_cell_t	defining;	// The word being defined.
	forth_cell_t	trace;		// Enabled execution trace.
	forth_cell_t	terminal_width;
	forth_cell_t	terminal_height;
	forth_cell_t	terminal_col;
	int (*page)(struct forth_runtime_context *rctx);							// PAGE -- If the device cannot do it set to to 0.
	int (*at_xy)(struct forth_runtime_context *rctx, forth_cell_t x, forth_cell_t y);			// AT-XY -- If the device cannot do it set to to 0.
	int (*write_string)(struct forth_runtime_context *rctx, const char *str, forth_cell_t length);		// TYPE
	int (*send_cr)(struct forth_runtime_context *rctx);							// CR
	forth_scell_t (*accept_string)(struct forth_runtime_context *rctx, char *buffer, forth_cell_t length);	// ACCEPT
	forth_cell_t (*key)(struct forth_runtime_context *rctx);						// KEY
	forth_cell_t (*key_q)(struct forth_runtime_context *rctx);						// KEY?
	forth_cell_t (*ekey)(struct forth_runtime_context *rctx);						// EKEY
	forth_cell_t (*ekey_q)(struct forth_runtime_context *rctx);						// EKEY?
	forth_cell_t (*ekey_to_char)(struct forth_runtime_context *rctx, forth_cell_t ekey);			// EKEY>CHAR
	forth_cell_t	tib_count;
	char		tib[TIB_SIZE];
	char 	       *numbuff_ptr;
	char		num_buff[FORTH_NUM_BUFF_LENGTH];
	char		internal_buffer[32];
#if defined(FORTH_APPLICATION_DEFINED_CONTEXT_FIELDS)
	FORTH_APPLICATION_DEFINED_CONTEXT_FIELDS
#endif
#if defined(FORTH_USER_VARIABLES)
	forth_cell_t	user[FORTH_USER_VARIABLES];
#endif
};

extern int forth(forth_cell_t *dictionary, struct forth_runtime_context *rctx, forth_cell_t word_to_exec);

#endif

