#ifndef FORTH_INTERNAL_H
#define FORTH_INTERNAL_H
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

#include "forth_features.h"
#include "forth.h"

#if defined(FORTH_INCLUDE_MS)
extern void forth_ms(struct forth_runtime_context *rctx);
#endif

#if defined(FORTH_INCLUDE_TIME_DATE)
extern void forth_time_date(struct forth_runtime_context *rctx);
#endif

#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)

#define FORTH_FAM_READ 		1
#define FORTH_FAM_WRITE		2
#define FORTH_FAM_BINARY	8

// Take parameters from the stack in rctx and push results there.

// CREATE-FILE ( caddr cnt fam -- fid ior )
extern void forth_create_file(struct forth_runtime_context *rctx);

// DELETE-FILE ( caddr cnt -- ior )
extern void forth_delete_file(struct forth_runtime_context *rctx);

// FLUSH-FILE ( fid --  ior )
extern void forth_file_flush(struct forth_runtime_context *rctx);

// OPEN-FILE ( caddr cnt fam -- fid ior )
extern void forth_open_file(struct forth_runtime_context *rctx);

// REFILL for files ( -- TRUE|FALSE )
extern void forth_refill_file(struct forth_runtime_context *rctx);

// FILE-POSTION ( fid -- ud ior )
extern void forth_file_position(struct forth_runtime_context *rctx);

// REPOSITION-FILE ( ud fid -- ior )
extern void forth_reposition_file(struct forth_runtime_context *rctx);

// FILE-SIZE ( fid -- ud ior )
extern void forth_file_size(struct forth_runtime_context *rctx);

// READ-FILE ( caddr cnt1 fid -- cnt2 ior )
extern void forth_read_file(struct forth_runtime_context *rctx);

// READ-LINE ( caddr cnt1 fid -- cnt2 ior )
extern void forth_read_line(struct forth_runtime_context *rctx);

// WRITE-FILE ( caddr cnt fid -- ior )
extern void forth_write_file(struct forth_runtime_context *rctx);

// WRITE-LINE ( caddr cnt fid -- ior )
extern void forth_write_line(struct forth_runtime_context *rctx);

// CLOSE-FILE ( fid -- ior )
extern void forth_close_file(struct forth_runtime_context *rctx);

// Map the FID to a file name. If there is no such FID return 0 0, otherwise return c-addr len.
extern void forth_map_fid_to_name(struct forth_runtime_context *rctx);

#endif

#if defined(FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS)

// ALLOCATE ( size -- addr ior )
extern void forth_allocate(struct forth_runtime_context *rctx);

// RESIZE ( addr1 size -- addr2 ior )
extern void forth_resize(struct forth_runtime_context *rctx);

// FREE (addr -- ior )
extern void forth_free(struct forth_runtime_context *rctx);


#endif


#endif

