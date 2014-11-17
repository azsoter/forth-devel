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

// This implementation is for SINGLE TREADED testing -- if you are running multiple Forthes on some RTOS you might have to modify it.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "forth_internal.h"

#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)

#define POP() *(rctx->sp++)
#define PUSH(X) *(--(rctx->sp)) = (forth_cell_t)(X)

#define MAX_FILES 8	// This is a system wide limit in this case.

struct forth_file
{
	FILE *file;
	char *name;
};

static struct forth_file files[MAX_FILES];

static int find_slot(void)
{
	int i;

	for (i = 0; i < MAX_FILES; i++)
	{
		if (0 == files[i].file)
		{
			return i + 1;
		}
	}

	return -1;
}

static struct forth_file *get_slot(int i)
{
	i -= 1;

	if ((i < 0) || (i >= MAX_FILES))
	{
		return 0;
	}

	return &(files[i]);
}

static int release_slot(int i)
{
	struct forth_file *f = get_slot(i);

	if (0 == f)
	{
		return -1;
	}

	if (0 != f->name)
	{
		FORTH_FREE_CNAME(f->name);
		f->name = 0;
	}

	if (0 == f->file)
	{
		return 0;
	}

	if (EOF == fclose(f->file))
	{
		f->file = 0;
		return -1;
	}

	f->file = 0;
	return 0;
}

static const char *map_fam2map(forth_cell_t fam, forth_cell_t create)
{
	switch(fam)
	{
		case FORTH_FAM_READ:	return "r";
		case FORTH_FAM_WRITE:	return "w";
		case FORTH_FAM_READ|FORTH_FAM_WRITE: return create ? "w+" : "r+";
		case FORTH_FAM_READ|FORTH_FAM_BINARY:	return "rb";
		case FORTH_FAM_WRITE|FORTH_FAM_BINARY:	return "wb";
		case FORTH_FAM_READ|FORTH_FAM_WRITE|FORTH_FAM_BINARY: return create ? "wb+" : "rb+";
		default: return 0;
	}
}

static void forth_open_or_create_file(struct forth_runtime_context *rctx, forth_cell_t create)
{

	forth_cell_t fam;
	forth_cell_t cnt;
	FILE *f;
	char *fname;
	char *cname;
	const char *mode;

	fam = POP();
	cnt = POP();
	fname = (char *)(POP());
	mode = map_fam2map(fam, create);

	int slot;

	if (0 == mode)
	{
		PUSH(-1);
		PUSH(-37);
		return;
	}

	slot = find_slot();

	if (0 > slot)
	{
		PUSH(-1);
		PUSH(-37);
		return;
	}

	cname = FORTH_ALLOCATE_CNAME(fname, cnt);

	f = fopen(cname, mode);

	// printf("fopen(%s, %s) = %p\n", cname, mode, f);
	

	if (0 == f)
	{
		PUSH(-1);
		PUSH(-37);
		FORTH_FREE_CNAME(cname);
		return;
	}
	else
	{
		files[slot - 1].file = f;	// Perhaps use get_slot() instead???
		files[slot - 1].name = cname;
		PUSH(slot);
		PUSH(0);
	}
}

// CREATE-FILE ( caddr cnt fam -- fid ior )
void forth_create_file(struct forth_runtime_context *rctx)
{
	forth_open_or_create_file(rctx, 1);
}

// OPEN-FILE ( caddr cnt fam -- fid ior )
void forth_open_file(struct forth_runtime_context *rctx)
{
	forth_open_or_create_file(rctx, 0);
}

// DELETE-FILE ( caddr cnt -- ior )
void forth_delete_file(struct forth_runtime_context *rctx)
{
	forth_cell_t cnt;
	char *fname;
	char *cname;

	cnt = POP();
	fname = (char *)(POP());
	cname = FORTH_ALLOCATE_CNAME(fname, cnt);
	if (0 > remove(cname))
	{
		PUSH(-37);
	}
	else
	{
		PUSH(0);
	}
	FORTH_FREE_CNAME(cname);
}

// FLUSH-FILE ( fid --  ior )
void forth_file_flush(struct forth_runtime_context *rctx)
{
	forth_cell_t res = -37;
	struct forth_file *f = get_slot(POP());
	
	if (0 != f)
	{
		res = (0 == fflush(f->file)) ? 0 : -37;
	}

	PUSH(res);
}

// FILE-POSTION ( fid -- ud ior )
void forth_file_position(struct forth_runtime_context *rctx)
{
	long pos = 0;
	forth_cell_t res = -37;
	struct forth_file *f = get_slot(POP());
	if (0 != f)
	{
		pos = ftell(f->file);
		res = (-1 == pos) ? -37 : 0;
	}
	PUSH(FORTH_CELL_LOW(pos));	
	PUSH(FORTH_CELL_HIGH((forth_dcell_t)pos));
	PUSH(res);
}

// FILE-SIZE ( fid -- ud ior )
void forth_file_size(struct forth_runtime_context *rctx)
{
	forth_scell_t res = 0;
	struct forth_file *f = get_slot(POP());
	long pos = -1;
	long end = -1;

	if (0 == f)
	{
		PUSH(0);
		PUSH(0);
		PUSH(-37);
		return;
	}

	pos = ftell(f->file);

	if (-1 == pos)
	{
		PUSH(0);
		PUSH(0);
		PUSH(-37);
		return;
	}

	if ( 0 > fseek(f->file, 0, SEEK_END))
	{
		PUSH(0);
		PUSH(0);
		PUSH(-37);
		return;
	}

	end = ftell(f->file);

	PUSH(FORTH_CELL_LOW(end));
	PUSH(FORTH_CELL_HIGH((forth_dcell_t)end));

	if (-1 == end)
	{
		res = -37;
	}

	if ( 0 > fseek(f->file, pos, SEEK_SET))
	{
		res = -37;
	}

	PUSH(res);
}

// REFILL for files ( -- TRUE|FALSE )
void forth_refill_file(struct forth_runtime_context *rctx)
{
	long pos;
	struct forth_file *f = get_slot(rctx->source_id);
	
	if (0 == f)
	{
		PUSH(0);
		return;
	}

	pos = ftell(f->file);

	if (-1 == pos)
	{
		// puts("refill_file: pos == -1");
		PUSH(0);
		return;
	}

	rctx->source_file_position = (forth_dcell_t)pos;

	if (0 == fgets(rctx->file_buffer, sizeof(rctx->file_buffer) - 1, f->file))
	{
		PUSH(0);
	}
	else
	{
		rctx->source_address = rctx->file_buffer;
		rctx->source_length = strlen(rctx->file_buffer);
		rctx->to_in = 0;
		PUSH(-1);
	}

}

// REPOSITION-FILE ( ud fid -- ior )
void forth_reposition_file(struct forth_runtime_context *rctx)
{
	struct forth_file *f = get_slot(POP());
	forth_cell_t high = POP();
	forth_cell_t low = POP();
	long pos = FORTH_DCELL(high, low);

	if (0 == f)
	{
		PUSH(-37);
		return;
	}

	if (0 > fseek(f->file, pos, SEEK_SET))
	{
		PUSH(-36);
	}
	else
	{
		PUSH(0);
	}
}

// READ-LINE ( caddr cnt1 fid -- cnt2 ior )
void forth_read_line(struct forth_runtime_context *rctx)
{
	struct forth_file *f = get_slot(POP());
	forth_cell_t cnt = POP();
	forth_cell_t read;
	char *addr = (char *)(POP());
	char *res;

	if (0 == f)
	{
		PUSH(0);
		PUSH(-37);
		return;
	}

	res = fgets(addr, cnt + 1, f->file); // According to the Forth standard, the buffer must be longer than cnt1 by 2, so +1 shoudl be OK.

	if (0 == res)
	{
		PUSH(0);
		PUSH(-37);
		return;
	}

	read = strlen(res);

	PUSH(read);
	PUSH(0);
}

// READ-FILE ( caddr cnt1 fid -- cnt2 ior )
void forth_read_file(struct forth_runtime_context *rctx)
{
	struct forth_file *f = get_slot(POP());
	forth_cell_t cnt = POP();
	forth_scell_t read;
	char *addr = (char *)(POP());

	if (0 == f)
	{
		PUSH(-37);
		return;
	}

	read = fwrite(addr, sizeof(char), cnt, f->file);

	PUSH(read);

	if (cnt != read)
	{
		if (0 != ferror(f->file))
		{
			PUSH(-37);
			return;
		}
	}

	PUSH(0);
	return;
}

// WRITE-FILE ( caddr cnt fid -- ior )
void forth_write_file(struct forth_runtime_context *rctx)
{
	forth_scell_t res;
	struct forth_file *f = get_slot(POP());
	forth_cell_t cnt = POP();
	char *addr = (char *)(POP());

	if (0 == f)
	{
		PUSH(-37);
		return;
	}

	res = (cnt == fwrite(addr, sizeof(char), cnt, f->file)) ? 0 : -37;
	PUSH(res);
}

// WRITE-LINE ( caddr cnt fid -- ior )
void forth_write_line(struct forth_runtime_context *rctx)
{
	forth_cell_t i;
	struct forth_file *f = get_slot(POP());
	forth_cell_t cnt = POP();
	char *addr = (char *)(POP());

	if (0 == f)
	{
		PUSH(-37);
		return;
	}

	// Should do for now, perhaps there is some more efficient way to write a string that is not 0 terminated....
	for (i = 0; i < cnt; i++)
	{
		if (EOF == fputc(addr[i], f->file))
		{
			PUSH(-37);
			return;
		}
	}

	if (0 > fprintf(f->file, "\n"))	// End of line.
	{
		PUSH(-37);
	}
	else
	{
		PUSH(0);
	}
}

// CLOSE-FILE ( fid -- ior )
void forth_close_file(struct forth_runtime_context *rctx)
{
	if (0 > release_slot(POP()))
	{
		PUSH(-37);
	}
	else
	{
		PUSH(0);
	}
}
	
// Map the FID to a file name. If there is no such FID return 0 0, otherwise return c-addr len.
void forth_map_fid_to_name(struct forth_runtime_context *rctx)
{
	struct forth_file *f = get_slot(POP());

	if ((0 == f) || (0 == f->name))
	{
		PUSH(0);
		PUSH(0);
	}
	
	PUSH(f->name);
	PUSH(strlen(f->name));
}

#endif


