/*
 * Test program to run on Linux/POSIX.
 * for the Embeddable Forth Command Interpreter.
 * Written by Andras Zsoter.
 * This is a quick and dirty example how to control the Forth Engine on a terminal.
 * You can treat the contents of this file as public domain.
 *
 * Attribution is appreciated but not mandatory for the contents of this file.
 *
 */

// http://forth.teleonomix.com/


#include "forth.h"
#include "forth_internal.h"
#include "forth_dict.h"

#include <stdio.h>
#include <string.h>

forth_cell_t data_stack[256];
forth_cell_t return_stack[256];
struct forth_runtime_context r_ctx;
struct forth_persistent_context p_ctx;
// forth_cell_t dictionary[1024] = { FORTH_TOKEN_BYE };
forth_cell_t search_order[256];

int write_str(struct forth_runtime_context *rctx, const char *str, forth_cell_t length)
{
	rctx->terminal_col += length;

	while(length--)
	{
		putchar(*str++);
	}

	fflush(stdout);
	return 0;
}

int page(struct forth_runtime_context *rctx)
{
	rctx->terminal_col = 0;
	putchar(12);
	fflush(stdout);
	return 0;
}

int send_cr(struct forth_runtime_context *rctx)
{
	puts("");
	fflush(stdout);
	rctx->terminal_col = 0;
	return 0;
}

forth_scell_t accept_str(struct forth_runtime_context *rctx, char *buffer, forth_cell_t length)
{
	char *res;
	// This is not actually correct, but will do for now.
	res = fgets(buffer, length - 1, stdin);
	if (0 == res)
	{
		return -1;
	}
	else
	{
		return strlen(res);
	}
}

// These are fake, in reality they should be more complicated to follow the rules in the FORTH standard.
// KEY? and EKEY? cannot be implemented using stdio, they would need something like a low level terminal interface.
forth_cell_t key(struct forth_runtime_context *rctx)
{
	char c = getchar();
	return (forth_cell_t)c;
}

forth_cell_t key_q(struct forth_runtime_context *rctx)
{
	return 1;
}

forth_cell_t ekey(struct forth_runtime_context *rctx)
{
	char c = getchar();
	return ((forth_cell_t)c) << 8;
}

forth_cell_t ekey_q(struct forth_runtime_context *rctx)
{
	return 1;
}

forth_cell_t ekey_to_char(struct forth_runtime_context *rctx, forth_cell_t ek)
{
	return ek >> 8;
}

#if defined(FORTH_EXTERNAL_PRIMITIVES)
#include "forth_interface.h"
forth_cell_t test_square(forth_runtime_context_p rctx)
{
	*(rctx->sp) = *(rctx->sp) * *(rctx->sp);
	return 0;
}

forth_cell_t test_inc(forth_runtime_context_p rctx)
{
	*(rctx->sp) += 1;
	return 0;
}

forth_external_primitive external_primitive_table[] = { test_square, test_inc };

static void register_primitive(struct forth_runtime_context *rctx, const char *name, forth_cell_t index)
{
	int res = forth_register_external_primitive(rctx, name, index);

	if (0 != res)
	{
		printf("Return value = %d at item %d (%s).\n", res, index, name);
	}
}

static int init_externals(struct forth_runtime_context *rctx)
{
	int i;
	for (i = 0; i < sizeof(external_primitive_table) / sizeof(forth_external_primitive); i++)
	{
		if (test_square == external_primitive_table[i])
		{
			register_primitive(rctx, "SQUARE", i);
		}
		else if (test_inc == external_primitive_table[i])
		{
			register_primitive(rctx, "INC", i);
		}
		else
		{
			printf("Undefined external primitive at index = %d.\n", i);
		}
	}

	return 0;
}
#endif

int main()
{
	// forth_cell_t tmp;

	r_ctx.dictionary = dictionary;
	r_ctx.sp0 = &data_stack[255];
	r_ctx.sp = &data_stack[255];
	r_ctx.sp_max = &data_stack[255];
	r_ctx.sp_min = data_stack;

	r_ctx.rp0 = &return_stack[255];
	r_ctx.rp = &return_stack[255];
	r_ctx.rp_max = &return_stack[255];
	r_ctx.rp_min = return_stack;

	r_ctx.handler = 0;
	r_ctx.ip =0;
	r_ctx.base = 10;
	// r_ctx.base = 16;

	r_ctx.wordlists = (forth_cell_t *)&search_order;
	r_ctx.wordlist_slots = 256;
	r_ctx.wordlist_cnt = 2;
	search_order[255] = FORTH_WID_Root_WORDLIST;
	search_order[254] = FORTH_WID_FORTH_WORDLIST;
	r_ctx.current = FORTH_WID_FORTH_WORDLIST;
	// r_ctx.wordlist_cnt =
	r_ctx.terminal_width = 80;
	r_ctx.terminal_height = 25;
	r_ctx.write_string = &write_str;
	r_ctx.page = &page;
	r_ctx.send_cr = &send_cr;
	r_ctx.accept_string = &accept_str;
	r_ctx.key = &key;
	r_ctx.key_q = &key_q;
	r_ctx.ekey = &ekey;
	r_ctx.ekey_q = &ekey_q;
	r_ctx.ekey_to_char = &ekey_to_char;
#if defined(FORTH_EXTERNAL_PRIMITIVES)
	r_ctx.external_primitive_table = external_primitive_table;
	init_externals(&r_ctx);
#endif
	forth(&r_ctx, FORTH_XT_QUIT);

	return 0;
}

