/*
 * Test program to run on Linux/POSIX.
 * for the Embeddable Forth Command Interpreter.
 * Written by Andras Zsoter.
 * This is a quick and dirty example how to control the Forth Engine over a telnet connection.
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
#include <curses.h>

forth_cell_t data_stack[256];
forth_cell_t return_stack[256];
struct forth_runtime_context r_ctx;
struct forth_persistent_context p_ctx;
// forth_cell_t dictionary[1024] = { FORTH_TOKEN_BYE };
forth_cell_t search_order[256];

int reset_curses(void)
{
	cbreak();
	noecho();
 	immedok(stdscr,TRUE);
	scrollok(stdscr,TRUE);
	idlok(stdscr,TRUE);
 	keypad(stdscr,TRUE);
 	nonl();
}

int init_curses(void)
{
	initscr();
	start_color();
	reset_curses();
}

int close_curses(void)
{
	endwin();
}

int emit_char(char c)
{
	if ('\r' == c)
	{
		c = '\n';
	}

	addch(c);
}

int write_str(struct forth_runtime_context *rctx, const char *str, forth_cell_t length)
{
	rctx->terminal_col += length;

	if (0 == str)
	{
		return -1;
	}

	if (0 != length)
	{
		addnstr(str, length);
	}

	return 0;
}

int page(struct forth_runtime_context *rctx)
{
	clear();
	rctx->terminal_col = 0;
	return 0;
}

int send_cr(struct forth_runtime_context *rctx)
{
	emit_char('\n');
	rctx->terminal_col = 0;
	return 0;
}

int at_xy(struct forth_runtime_context *rctx, forth_cell_t x, forth_cell_t y)
{
	move(y, x);
	return 0;
}

// These are fake, in reality they should be more complicated to follow the rules in the FORTH standard.
// KEY? and EKEY? using curses.
forth_cell_t key(struct forth_runtime_context *rctx)
{
	char c = getch();

	if (127 == c || 8 == c || 7 == c) 	// Ugly, but I have no idea what the proper symbolic names are.
	{
		c = '\b';
	}

	return (forth_cell_t)c;
}

forth_cell_t key_q(struct forth_runtime_context *rctx)
{
	int c = getch();

	if (ERR == c)
	{
		return FORTH_FALSE;
	}
	ungetch(c);
	return 1;
}

forth_cell_t ekey(struct forth_runtime_context *rctx)
{
	char c = getch();
	return ((forth_cell_t)c) << 8;
}

forth_cell_t ekey_q(struct forth_runtime_context *rctx)
{
	int c = getch();

	if (ERR == c)
	{
		return FORTH_FALSE;
	}
	ungetch(c);
	return 1;
}

forth_cell_t ekey_to_char(struct forth_runtime_context *rctx, forth_cell_t ek)
{
	char c =  ek >> 8;

	if (127 == c || 8 == c || 7 == c) 	// Ugly, but I have no idea what the proper symbolic names are.
	{
		c = '\b';
	}

	return c;
}

forth_scell_t accept_str(struct forth_runtime_context *rctx, char *buffer, forth_cell_t length)
{
	char *res;
	forth_cell_t count = 0;
	char k;

	while (count < length)
	{
		k = key(rctx);

		switch(k)
		{
			case '\r':
			case '\n':
				emit_char(FORTH_CHAR_SPACE);
			return count;

			case '\b':
				if (count > 0)
				{
					emit_char(8);
					delch();
					count--;
				}
				
			break;

			default:
				buffer[count++] = k;
				emit_char(k);
			break;
		}

	}

	return count;
}


int main()
{
	forth_cell_t tmp;

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
	r_ctx.at_xy = &at_xy;

	init_curses();
	
	forth(dictionary, &r_ctx, FORTH_XT_QUIT);

	close_curses();

	return 0;
}

