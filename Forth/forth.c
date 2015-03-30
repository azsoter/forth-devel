/*
* Copyright (c) 2014-2015 Andras Zsoter
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

#include <string.h>
#include <ctype.h>
#include "forth.h"
#include "forth_internal.h"
#include "forth_dict.h"
#include "forth_interface.h"

#undef DEBUG_PARSE
#undef DEBUG_SEARCH
#undef DEBUG_PROCESS_NUMBER

#if defined(DEBUG_PARSE) || defined(DEBUG_SEARCH) || defined(DEBUG_PROCESS_NUMBER)
#include <stdio.h>
#endif

static char forth_val2digit(forth_byte_t val)
{
	return (char)((val < 10) ? ( val + '0') : ((val - 10) +'A'));
} 

static char *forth_format_unsigned(forth_cell_t value, forth_cell_t base, forth_byte_t width, char *buff)
{
	char *p = buff;
	forth_byte_t len = 0;

 	if (base < 2)
	{
		base = 10;
	}

 	do {
		*(--p) = forth_val2digit(value % base);
		value /= base;
		len += 1;
    	} while(value);

 	while (width > len)
	{
		*(--p) = '0';
		len++;
	}
 
	return p; 
}

static int forth_hdot(struct forth_runtime_context *rctx, forth_cell_t value)
{
	char *buffer = rctx->num_buff;
	char *end = buffer + (FORTH_CELL_HEX_DIGITS) + 1;
	char *p;
	*end = FORTH_CHAR_SPACE;
	p = forth_format_unsigned(value, 16, FORTH_CELL_HEX_DIGITS, end);
	return rctx->write_string(rctx, p, (end - p) + 1);
}

static int forth_udot(struct forth_runtime_context *rctx, forth_cell_t base, forth_cell_t value)
{
	char *buffer = rctx->num_buff;
	char *end = buffer + sizeof(rctx->num_buff)/* + 2*/;
	char *p;
	*end = FORTH_CHAR_SPACE;
	p = forth_format_unsigned(value, base, 1, end);
	return rctx->write_string(rctx, p, (end - p) + 1);
}

static int forth_dot(struct forth_runtime_context *rctx, forth_cell_t base, forth_cell_t value)
{
	char *buffer = rctx->num_buff;
	char *end = buffer + sizeof(rctx->num_buff)/* + 2*/;
	forth_scell_t val = (forth_scell_t)value;
	char *p;
	*end = FORTH_CHAR_SPACE;
	if (val < 0)
	{
		value = (forth_cell_t)-val;
	}
	p = forth_format_unsigned(value, base, 1, end);
	if (val < 0)
	{
		*--p = '-';
	}
	return rctx->write_string(rctx, p, (end - p) + 1);
}

static int forth_dot_r(struct forth_runtime_context *rctx, forth_cell_t base, forth_cell_t value, forth_cell_t width, forth_cell_t is_signed)
{
	char *buffer = rctx->num_buff;
	char *end = buffer + sizeof(rctx->num_buff)/* + 2*/;
	forth_scell_t val = (forth_scell_t)value;
	char *p;
	forth_cell_t nlen;
	forth_cell_t i;

	char c =  FORTH_CHAR_SPACE;
	if (is_signed && (val < 0))
	{
		value = (forth_cell_t)-val;
	}

	p = forth_format_unsigned(value, base, 1, end);

	if (is_signed && (val < 0))
	{
		*--p = '-';
	}

	nlen = (end - p);

	// printf("nlen = %u\n", nlen);

	for (i = nlen; i < width; i++)
	{
		rctx->write_string(rctx, &c, 1);
	}

	return rctx->write_string(rctx, p, nlen);
}

#if 0
static int forth_dots(struct forth_runtime_context *rctx)
{
	forth_cell_t *sp;
	int res;

	res = rctx->write_string(rctx, "[", 1);

	if (res < 0)
	{
		return res;
	}

	// res = forth_dot(rctx, rctx->base, rctx->sp0 - rctx->sp);
	res = forth_dot_r(rctx, 10, rctx->sp0 - rctx->sp, 1, 0);

	if (res < 0)
	{
		return res;
	}

	res = rctx->write_string(rctx, "] ", 2);

	if (res < 0)
	{
		return res;
	}

	for (sp = rctx->sp0 - 1; sp >= rctx->sp; sp--)
	{
		res = forth_dot(rctx, rctx->base, *sp);

		if (res < 0)
		{
			return res;
		}
	}

	return rctx->send_cr(rctx);
}
#else
/*
static int forth_hdot(struct forth_runtime_context *rctx, forth_cell_t value)
{
	char *buffer = rctx->num_buff;
	char *end = buffer + (FORTH_CELL_HEX_DIGITS) + 1;
	char *p;
	*end = FORTH_CHAR_SPACE;
	p = forth_format_unsigned(value, 16, FORTH_CELL_HEX_DIGITS, end);
	return rctx->write_string(rctx, p, (end - p) + 1);
}
*/
static int forth_dots(struct forth_runtime_context *rctx)
{
	char buff[20];
	char *p;
	char *end;
	forth_cell_t *sp;
	int res;
	forth_scell_t item;

	end = buff + sizeof(buff);
	p = end - 1;
	*p = FORTH_CHAR_SPACE;
	*--p = ']';

	p = forth_format_unsigned(rctx->sp0 - rctx->sp, 10, 1, p);

	*--p = '[';

	res = rctx->write_string(rctx, p, (end - p));

	if (res < 0)
	{
		return res;
	}

	for (sp = rctx->sp0 - 1; sp >= rctx->sp; sp--)
	{
		item = *sp;

		if (item < 0)
		{
			item = -item;
		}

		p = forth_format_unsigned((forth_cell_t)item, rctx->base, 1, end - 1);

		if (((forth_scell_t)*sp) < 0)
		{
			*--p = '-';
		}

		res = rctx->write_string(rctx, p, (end - p));

		if (res < 0)
		{
			return res;
		}
	}

	return rctx->send_cr(rctx);
}
#endif
// ---------------------------------------------------------------------------------------
int forth_type0(struct forth_runtime_context *rctx, const char *s)
{
	size_t len;

	if (0 == s)
	{
		return 0;
	}

	len = strlen(s);

	if (0 == len)
	{
		return 0;
	}

	return rctx->write_string(rctx, s, len);
}

int forth_dump(struct forth_runtime_context *rctx, const char *addr, forth_cell_t len)
{
	char byte_buffer[3];
	char buff[8];
	forth_cell_t i;
	char c;
	forth_cell_t cnt;

	byte_buffer[2] = FORTH_CHAR_SPACE;

	if (0 == len)
	{
		return 0;
	}

	for(i = 0; i < len; i++)
	{
 		if ( 0 == (i % 8))
	 	{
			if (i)
                	{
                    		if (0 > rctx->write_string(rctx, buff,8))
				{
					return -1;
				}
                	}
			rctx->send_cr(rctx);
			forth_hdot(rctx, (forth_cell_t)addr);
                	forth_type0(rctx, ": "); 
			memset(buff,FORTH_CHAR_SPACE, 8);
	 	}
		c = *addr++;
		// Check for printable characters.
 		buff[i % 8] = ( (c <128) && (c>31) ) ? c : '.';
		byte_buffer[0] = forth_val2digit(0x0F & (c >> 4));
		byte_buffer[1] = forth_val2digit(0x0F & c);
      		if (0 > rctx->write_string(rctx, byte_buffer, 3))
		{
			return -1;
		}
    	}

	cnt = (i % 8) ? (i % 8) : 8;

	if (8 != cnt)
	{
		byte_buffer[0] = FORTH_CHAR_SPACE;
		byte_buffer[1] = FORTH_CHAR_SPACE;
		for (i = (8 - cnt); i != 0; i--)
		{
      			if (0 > rctx->write_string(rctx, byte_buffer, 3))
			{
				return -1;
			}
		}		
	}

   	rctx->write_string(rctx, buff, cnt);
	return rctx->send_cr(rctx);
}
// ---------------------------------------------------------------------------------------
static void forth_skip_delimiters(const char **buffer, forth_cell_t *length, char delimiter)
{
	const char *buff = *buffer;
	forth_cell_t len = *length;

	if ((FORTH_CHAR_SPACE) == delimiter)
	{
		while ((0 != len) && isspace(*buff))
		{
			buff++;
			len--;
		}
	}
	else
	{
		while ((0 != len) && (delimiter == *buff))
		{
			buff++;
			len--;
		}
	}

	*buffer = buff;
	*length = *length - len;
}

static void forth_parse_till_delimiter(const char **buffer, forth_cell_t *length, char delimiter)
{
	const char *buff = *buffer;
	forth_cell_t len = *length;

#if defined(DEBUG_PARSE)
	printf("-----------------------------------------------------------------\n");
	write_str(buff, len);
	printf("\n-----------------------------------------------------------------\n");
#endif
	if ((FORTH_CHAR_SPACE) == delimiter)
	{
		while ((0 != len) && !isspace(*buff))
		{
			// putchar(*buff);
			buff++;
			len--;
		}
	}
	else
	{
		while ((0 != len) && (delimiter != *buff))
		{
			// putchar(*buff);
			buff++;
			len--;
		}
	}

	// *buffer = buff;
	*length = *length - len;
}

static void forth_parse(struct forth_runtime_context *rctx, char delimiter)
{
	const char *address;
	forth_cell_t length;

#if defined(DEBUG_PARSE)
	printf("%s: addr = %p, len = %u, >IN = %u\n", __FUNCTION__, rctx->source_address, rctx->source_length, rctx->to_in);
#endif
	if (rctx->to_in < rctx->source_length)
	{
		address = rctx->source_address + rctx->to_in;
		length = rctx->source_length - rctx->to_in;

#if defined(DEBUG_PARSE)
		printf("%s (start) address=%p length=%u\n", __FUNCTION__, address, length);
#endif
		forth_parse_till_delimiter(&address, &length, delimiter);
#if defined(DEBUG_PARSE)
		printf("%s (ret) address=%p length=%u\n", __FUNCTION__, address, length);
		rctx->write_string(rctx, address, length);
#endif

		rctx->to_in += length;

		if (rctx->to_in < rctx->source_length)
		{
			rctx->to_in++;
		}

		*--(rctx->sp) = (forth_cell_t)address;
		*--(rctx->sp) = length;
	}
	else
	{
		*--(rctx->sp) = 0;
		*--(rctx->sp) = 0;
	}
}

static void forth_parse_word(struct forth_runtime_context *rctx, char dlm)
{
	const char *address;
	forth_cell_t length;

#if defined(DEBUG_PARSE)
	printf("%s: addr = %p, len = %u, >IN = %u\n", __FUNCTION__, rctx->source_address, rctx->source_length, rctx->to_in);
	rctx->write_string(rctx, rctx->source_address, rctx->source_length); puts("");
#endif

	if (rctx->to_in < rctx->source_length)
	{
		address = rctx->source_address + rctx->to_in;
		length = rctx->source_length - rctx->to_in;

#if defined(DEBUG_PARSE)
		printf("%s: >>Skip: addr = %p, len = %u\n", __FUNCTION__, address, length);
#endif
		forth_skip_delimiters(&address, &length, dlm);
#if defined(DEBUG_PARSE)
		printf("%s: <<Skip: addr = %p, len = %u\n", __FUNCTION__, address, length);
#endif

		rctx->to_in += length;

		forth_parse(rctx, dlm);
	}
	else
	{
		*--(rctx->sp) = 0;
		*--(rctx->sp) = 0;
	}
}

// ---------------------------------------------------------------------------------------
static forth_scell_t forth_accept(struct forth_runtime_context *rctx, char *buffer, forth_cell_t len)
{
	return rctx->accept_string(rctx, buffer, len);
}

static forth_scell_t forth_query(struct forth_runtime_context *rctx)
{
	forth_scell_t len;
	rctx->blk = 0;
	rctx->source_id = 0;
	rctx->source_address = rctx->tib;
	rctx->source_length = 0;
	rctx->to_in = 0;
	len = forth_accept(rctx, rctx->tib, TIB_SIZE);
	// printf("ACCEPT DONE\n"); fflush(stdout);

	if (0 > len)
	{
		rctx->tib_count = 0;
	}
	else
	{
		rctx->tib_count = len;
		rctx->source_length = rctx->tib_count;
	}

	return len;
}
// ---------------------------------------------------------------------------------------
static forth_cell_t forth_search_wordlist(forth_cell_t dictionary[], const struct forth_wordlist *wl, const char *name, forth_cell_t len)
{
	forth_cell_t ix = wl->latest;
	struct forth_header *h = (struct forth_header *)(&dictionary[ix]);
	forth_cell_t len_aligned = FORTH_ALIGN(len);
	forth_cell_t name_length;

#if defined(DEBUG_SEARCH)
	int i;
	printf("Searching For: ");
	for (i = 0; i < len; i++)
	{
		putchar(name[i]);
	}
	putchar('\n');
	printf("wl=%p wl->latest = 0x%08x wl->parent = 0x%08x wl->link = 0x%08x\n", wl, wl->latest, wl->parent, wl->link);
#endif
	while (0 != ix)
	{
#if defined(DEBUG_SEARCH)
		printf("h->link = 0x%08x h->flags=0x%08x\n", h->link, h->flags);
		printf("ix = 0x%08x h = %p\n", ix, h);
#endif
		name_length = ((FORTH_HEADER_FLAGS_NAME_LENGTH_MASK) & h->flags);
#if defined(DEBUG_SEARCH)
		printf("len = %d name_length = %d Aligned len = %d Aligned name_length = %d\n", len, name_length, len_aligned, FORTH_ALIGN(name_length));
		printf("name_addr = %p\n",((char*)h) - FORTH_ALIGN(name_length)); 
		for (i = 0; i < name_length; i++)
		{
			putchar( *(((char*)h) - FORTH_ALIGN(name_length) + i));
		}
		putchar('\n');
#endif
		if (len == name_length)
		{
			if (0 == strncasecmp(name, ((char *)(h)) - len_aligned, len))
			{
				return ix;
			}
		}

		ix = h->link;
		h = (struct forth_header *)(&dictionary[ix]);
	}

	return FORTH_TRUE;
}

static forth_cell_t forth_find_word(const struct forth_runtime_context *rctx, forth_cell_t dictionary[], const char *name, forth_cell_t len)
{
#if 0
	const struct forth_wordlist *wl = (const struct forth_wordlist *)(&dictionary[FORTH_WID_FORTH_WORDLIST]);
	return forth_search_wordlist(dictionary, wl, name, len);
#else
	const struct forth_wordlist *wl;
	forth_cell_t xt;
	forth_cell_t wid;
	// forth_cell_t i;
	forth_cell_t cnt;

	cnt = rctx->wordlist_cnt;

	while (0 != cnt)
	{
		wid = rctx->wordlists[rctx->wordlist_slots - cnt];
		wl = (const struct forth_wordlist *)(&dictionary[wid]);
		xt = forth_search_wordlist(dictionary, wl, name, len);

		if (FORTH_TRUE != xt)
		{
			return xt;
		}

		cnt--;
	}

	wid = FORTH_WID_Root_WORDLIST;
	wl = (const struct forth_wordlist *)(&dictionary[wid]);
	xt = forth_search_wordlist(dictionary, wl, name, len);
	if (FORTH_TRUE != xt)
	{
		return xt;
	}

	return FORTH_TRUE;
	
#endif
}

// ---------------------------------------------------------------------------------------
static forth_byte_t map_digit(char c)
{
	// ASCII
	if ((c >= '0') && (c <= '9'))
	{
		return c - '0';
	}
	else if ((c >= 'a') && (c <= 'z'))
	{
		return 10 + (c - 'a');
	}
	else if ((c >= 'A') && (c <= 'Z'))
	{
		return 10 + (c - 'A');
	}
	return 255;
}

int forth_process_number(struct forth_runtime_context *rctx, const char *buff, forth_cell_t len)
{
	int sign = 1;
	int is_double = 0;
	char c;
	forth_dcell_t d = 0;
	forth_byte_t b;
	forth_cell_t base = rctx->base;

#if defined(DEBUG_PROCESS_NUMBER)
	// printf("buff = %p %u\n",buff, len);
	rctx->write_string(rctx, buff, len); rctx->send_cr(rctx);
#endif

	if ((0 != len) && ('-' == *buff))
	{
		sign = -1;
		buff++;
		len--;
	}
	else if ((0 != len) && ('+' == *buff))
	{
		buff++;
		len--;
	}

	if (0 == len)
	{
		return -1;
	}

#if defined(FORTH_ALLOW_0X_HEX)
	if (len > 2)
	{
		if (('0' == buff[0]) && (('x' == buff[1]) || ('X' == buff[1])))
		{
			base = 16;
			len -= 2;
			buff += 2;
		}
	}
#endif

	while (0 != len)
	{
		c = *buff++;
		len--;

		if ('.' == c)
		{
			is_double = 1;
			continue;
		}

		b = map_digit(c);

		if (b >= base)
		{
			return -1;
		}

		d = (d * base) + b;
	}

	if (-1 == sign)
	{
		d = (forth_dcell_t)(-(forth_sdcell_t)d);
	}

	*--(rctx->sp) = FORTH_CELL_LOW(d);

	if (is_double)
	{
		*--(rctx->sp) = FORTH_CELL_HIGH(d);
		*--(rctx->sp) = 1;
	}
	else
	{
		*--(rctx->sp) = 0;
	}


#if defined(DEBUG_PROCESS_NUMBER)
	puts("-----------------------------------------------------------");
	forth_dots(rctx);
	puts("-----------------------------------------------------------");
#endif
	return 0;
}
// ---------------------------------------------------------------------------------------
static forth_cell_t forth_literal(forth_cell_t *dp, forth_cell_t value)
{
	forth_cell_t packed = FORTH_PARAM_PACK(value);

	if (FORTH_PARAM_UNSIGNED(packed) == value)
	{
		dp[0] = FORTH_PACK_TOKEN(FORTH_TOKEN_uslit) | packed;
		return 1;
	}
	else if (FORTH_PARAM_SIGNED(packed) == value)
	{
		dp[0] = FORTH_PACK_TOKEN(FORTH_TOKEN_sslit) | packed;
		return 1;
	}

	dp[0] = FORTH_PACK_TOKEN(FORTH_TOKEN_lit);
	dp[1] = value;
	return 2;

}

// ---------------------------------------------------------------------------------------
static int forth_words(forth_cell_t dictionary[], struct forth_runtime_context *rctx)
{
	//const struct forth_wordlist *wl = (const struct forth_wordlist *)(rctx->wordlists);
	//const struct forth_wordlist *wl = (const struct forth_wordlist *)(rctx->wordlists);
	// const struct forth_wordlist *wl = (const struct forth_wordlist *)(&dictionary[FORTH_WID_FORTH_WORDLIST]);
	const struct forth_wordlist *wl = (const struct forth_wordlist *)(&dictionary[rctx->wordlists[rctx->wordlist_slots - rctx->wordlist_cnt]]);
	forth_cell_t hx = wl->latest;
	const struct forth_header *h;
	// forth_cell_t xt;
	forth_cell_t name_length;
	char c = FORTH_CHAR_SPACE;

	if (0 > rctx->send_cr(rctx))
	{
		return -1;
	}

	while (0 != hx)
	{
		h = (const struct forth_header *)(&dictionary[hx]);
		name_length = (h->flags & (FORTH_HEADER_FLAGS_NAME_LENGTH_MASK));
		if ((rctx->terminal_width - rctx->terminal_col) <= name_length)
		{
			if (0 > rctx->send_cr(rctx))
			{
				return -1;
			}
		}
		if (0 > rctx->write_string(rctx, (const char *)(&dictionary[hx - ((FORTH_ALIGN(name_length)) / sizeof(forth_cell_t))]), name_length))
		{
			return -1;
		}

		if (0 > rctx->write_string(rctx, &c, 1))
		{
			return -1;
		}

		hx = h->link;
	}

	return rctx->send_cr(rctx);
}

// ---------------------------------------------------------------------------------------
forth_scell_t forth_compare_strings(const char *s1, forth_cell_t len1, const char *s2, forth_cell_t len2)
{
	// forth_scell_t diff;

	while(1)
	{
		if ((0 == len1) && (0 == len2))	// No characters left -- the strings are the same.
		{
			return 0;
		}
		else if (0 == len1) // Only the first string has ended
		{
			return -1;
		}
		else if (0 == len2) // Only the second string has ended
		{
			return 1;
		}

		if (*s1 != *s2)
		{
			return (0 > ( ((signed char)*s1) - ((signed char)*s2) )) ? -1 : 1;
		}

		len1--;
		s1++;
		len2--;
		s2++;
	}
}

// ---------------------------------------------------------------------------------------
void forth_print_error(struct forth_runtime_context *rctx, forth_scell_t code)
{
	if ((0 == code) || (1 == code))
	{
		return;
	}

	if ((-2 == code) && (0 == rctx->abort_msg_len))
	{
		return;
	}

#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
	if (rctx->source_id > 0)	// FILE
	{
		rctx->sp--;
		rctx->sp[0] = rctx->source_id;
		forth_map_fid_to_name(rctx);
		rctx->write_string(rctx, (char *)(rctx->sp[1]), rctx->sp[0]);
		rctx->sp += 2;
		forth_type0(rctx, ": ");
		forth_dot(rctx, 10, rctx->line_no);
		forth_type0(rctx, ",");
	}
#endif
	forth_dot(rctx, 10, rctx->to_in);
	forth_type0(rctx, " Error: ");
	forth_dot(rctx, 10, code);

	switch(code)
	{
	case 0: break; // Do nothing for 0.
	case -1: forth_type0(rctx, "ABORT"); break;
	case -2:
		if ((0 != rctx->abort_msg_len) && (0 != rctx->abort_msg_addr))
		{
			rctx->write_string(rctx, (char *)(rctx->abort_msg_addr), rctx->abort_msg_len);
			rctx->abort_msg_addr = 0;
			rctx->abort_msg_len = 0;
		}
		else
		{
			forth_type0(rctx, "aborted");
		}
		break;
	case -3: forth_type0(rctx, "stack overflow"); break;
	case -4: forth_type0(rctx, "stack underflow"); break;
	case -5: forth_type0(rctx, "return stack overflow"); break;
	case -6: forth_type0(rctx, "return stack underflow"); break;
	case -7: forth_type0(rctx, "do-loops nested too deeply during execution"); break;
	case -8: forth_type0(rctx, "dictionary overflow"); break;
	case -9: forth_type0(rctx, "invalid memory address"); break;
	case -10: forth_type0(rctx, "division by zero"); break;
	case -11: forth_type0(rctx, "result out of range"); break;
	case -12: forth_type0(rctx, "argument type mismatch"); break;
	case -13: forth_type0(rctx, "undefined word"); break;
	case -14: forth_type0(rctx, "interpreting a compile-only word"); break;
	case -15: forth_type0(rctx, "invalid FORGET"); break;
	case -16: forth_type0(rctx, "attempt to use zero-length string as a name"); break;
	case -17: forth_type0(rctx, "pictured numeric output string overflow"); break;
	case -18: forth_type0(rctx, "parsed string overflow"); break;
	case -19: forth_type0(rctx, "definition name too long"); break;
	case -20: forth_type0(rctx, "write to a read-only location"); break;
	case -21: forth_type0(rctx, "unsupported operation"); break; // (e.g., AT-XY on a too-dumb terminal)
	case -22: forth_type0(rctx, "control structure mismatch"); break;
	case -23: forth_type0(rctx, "address alignment exception"); break;
	case -24: forth_type0(rctx, "invalid numeric argument"); break;
	case -25: forth_type0(rctx, "return stack imbalance"); break;
	case -26: forth_type0(rctx, "loop parameters unavailable"); break;
	case -27: forth_type0(rctx, "invalid recursion"); break;
	case -28: forth_type0(rctx, "user interrupt"); break;
	case -29: forth_type0(rctx, "compiler nesting"); break;
	case -30: forth_type0(rctx, "obsolescent feature"); break;
	case -31: forth_type0(rctx, ">BODY used on non-CREATEd definition"); break;
	case -32: forth_type0(rctx, "invalid name argument (e.g., TO xxx)"); break;
	case -33: forth_type0(rctx, "block read exception"); break;
	case -34: forth_type0(rctx, "block write exception"); break;
	case -35: forth_type0(rctx, "invalid block number"); break;
	case -36: forth_type0(rctx, "invalid file position"); break;
	case -37: forth_type0(rctx, "file I/O exception"); break;
	case -38: forth_type0(rctx, "non-existent file"); break;
	case -39: forth_type0(rctx, "unexpected end of file"); break;
	case -40: forth_type0(rctx, "invalid BASE for floating point conversion"); break;
	case -41: forth_type0(rctx, "loss of precision"); break;
	case -42: forth_type0(rctx, "floating-point divide by zero"); break;
	case -43: forth_type0(rctx, "floating-point result out of range"); break;
	case -44: forth_type0(rctx, "floating-point stack overflow"); break;
	case -45: forth_type0(rctx, "floating-point stack underflow"); break;
	case -46: forth_type0(rctx, "floating-point invalid argument"); break;
	case -47: forth_type0(rctx, "compilation word list deleted"); break;
	case -48: forth_type0(rctx, "invalid POSTPONE"); break;
	case -49: forth_type0(rctx, "search-order overflow"); break;
	case -50: forth_type0(rctx, "search-order underflow"); break;
	case -51: forth_type0(rctx, "compilation word list changed"); break;
	case -52: forth_type0(rctx, "control-flow stack overflow"); break;
	case -53: forth_type0(rctx, "exception stack overflow"); break;
	case -54: forth_type0(rctx, "floating-point underflow"); break;
	case -55: forth_type0(rctx, "floating-point unidentified fault"); break;
	case -56: forth_type0(rctx, "QUIT"); break;
	case -57: forth_type0(rctx, "exception in sending or receiving a character"); break;
	case -58: forth_type0(rctx, "[IF], [ELSE], or [THEN] exception"); break;
	default: break;
	}

	rctx->send_cr(rctx);
}

// ---------------------------------------------------------------------------------------
const char *forth_token_name(forth_cell_t token)
{
	switch(token)
	{
		case FORTH_TOKEN_NOP:		return "noop";
		case FORTH_TOKEN_ACCEPT:	return "accept";
		case FORTH_TOKEN_KEY:		return "key";
		case FORTH_TOKEN_EKEY:		return "ekey";
		case FORTH_TOKEN_KEYq:		return "key?";
		case FORTH_TOKEN_EKEYq:		return "ekey?";
		case FORTH_TOKEN_EKEY2CHAR:	return "ekey>char";
		case FORTH_TOKEN_ALIGN:		return "align";
		case FORTH_TOKEN_ALIGNED:	return "aligned";
		case FORTH_TOKEN_ALLOT:		return "allot";
		case FORTH_TOKEN_pHERE:		return "(here)";
		case FORTH_TOKEN_pTRACE:	return "(trace)";
		case FORTH_TOKEN_HERE:		return "here";
		case FORTH_TOKEN_PAD:		return "pad";
		case FORTH_TOKEN_CompileComma:	return "compile,";
		case FORTH_TOKEN_Comma:		return ",";
		case FORTH_TOKEN_CComma:	return "c,";
		case FORTH_TOKEN_Imm_Plus:	return "imm+";
		case FORTH_TOKEN_CELLS:		return "cells";
		case FORTH_TOKEN_DROP:		return "drop";
		case FORTH_TOKEN_DUP:		return "dup";
		case FORTH_TOKEN_2ROT:		return "2rot";
		case FORTH_TOKEN_2DUP:		return "2dup";
		case FORTH_TOKEN_2DROP:		return "2drop";
		case FORTH_TOKEN_2OVER:		return "2over";
		case FORTH_TOKEN_2SWAP:		return "2swap";
		case FORTH_TOKEN_qDUP:		return "?dup";
		case FORTH_TOKEN_CFetch:	return "c@";
		case FORTH_TOKEN_Fetch:		return "@";
		case FORTH_TOKEN_Store:		return "!";
		case FORTH_TOKEN_2Fetch:	return "2@";
		case FORTH_TOKEN_2Store:	return "2!";
		case FORTH_TOKEN_AT_XY:		return "at-xy";
		case FORTH_TOKEN_PAGE:		return "page";

#if defined(FORTH_INCLUDE_MS)
		case FORTH_TOKEN_MS:		return "ms";
#endif

#if defined(FORTH_INCLUDE_TIME_DATE)
		case FORTH_TOKEN_TIME_DATE:	return "time&date";
#endif

		case FORTH_TOKEN_CR:		return "cr";
		case FORTH_TOKEN_EMIT:		return "emit";
		case FORTH_TOKEN_DUMP:		return "dump";
		case FORTH_TOKEN_UNUSED:	return "unused";
		case FORTH_TOKEN_EXECUTE:	return "execute";
		case FORTH_TOKEN_SEARCH_WORDLIST: return "search-wordlist";
		case FORTH_TOKEN_FIND_WORD:	return "find-word";
		case FORTH_TOKEN_COMPARE:	return "compare";
#if !defined(FORTH_NO_DOUBLES)
		case FORTH_TOKEN_Hash:		return "#";
#endif
		case FORTH_TOKEN_LessHash:	return "<#";
		case FORTH_TOKEN_HashGreater: 	return "#>";
		case FORTH_TOKEN_HOLD:		return "hold";
		
		case FORTH_TOKEN_Hdot:		return "h.";
		case FORTH_TOKEN_Udot:		return "u.";
		case FORTH_TOKEN_DotName:	return ".name";
		case FORTH_TOKEN_Dot:		return ".";
		case FORTH_TOKEN_DotR:		return ".r";	// .R
		case FORTH_TOKEN_UdotR:		return "u.r";	// U.R
		case FORTH_TOKEN_DotS:		return ".s";
		case FORTH_TOKEN_CMOVE:		return "cmove";
		case FORTH_TOKEN_CMOVE_down:	return "cmove>";
		case FORTH_TOKEN_MOVE:		return "move";
		case FORTH_TOKEN_FILL:		return "fill";
		case FORTH_TOKEN_LATEST:	return "latest";
		case FORTH_TOKEN_pDefining:	return "(defining)";

		case FORTH_TOKEN_NIP:		return "nip";
		case FORTH_TOKEN_TUCK:		return "tuck";
		case FORTH_TOKEN_ROT:		return "rot";
		case FORTH_TOKEN_ROLL:		return "roll";
		case FORTH_TOKEN_PICK:		return "pick";
		case FORTH_TOKEN_OVER:		return "over";
		case FORTH_TOKEN_PARSE:		return "parse";
		case FORTH_TOKEN_PARSE_WORD:	return "parse-word";
		case FORTH_TOKEN_WORD:		return "word";
		case FORTH_TOKEN_Plus:		return "+";
		case FORTH_TOKEN_Subtract:	return "-";
		case FORTH_TOKEN_Divide:	return "/";
		case FORTH_TOKEN_Multiply:	return "*";
		case FORTH_TOKEN_MOD:		return "mod";
		case FORTH_TOKEN_Slash_MOD:	return "/mod";
#if !defined(FORTH_NO_DOUBLES)
		case FORTH_TOKEN_StarSlash:	return "*/";
		case FORTH_TOKEN_StarSlash_MOD:	return "*/mod";
		case FORTH_TOKEN_UM_Star:	return "um*";
		case FORTH_TOKEN_M_Star:	return "m*";
		case FORTH_TOKEN_M_Plus:	return "m+";
		case FORTH_TOKEN_UM_Slash_MOD:	return "um/mod";
#endif
		case FORTH_TOKEN_NEGATE:	return "negate";
		case FORTH_TOKEN_ABS:		return "abs";
		case FORTH_TOKEN_MIN:		return "min";
		case FORTH_TOKEN_MAX:		return "max";
		case FORTH_TOKEN_LSHIFT:	return "lshift";
		case FORTH_TOKEN_RSHIFT:	return "rshift";
		case FORTH_TOKEN_2_Star:	return "2*";
		case FORTH_TOKEN_2_Slash:	return "2/";
#if !defined(FORTH_NO_DOUBLES)
		case FORTH_TOKEN_D2_Star:	return "d2*";
		case FORTH_TOKEN_D2_Slash:	return "d2/";

		case FORTH_TOKEN_DNEGATE:	return "dnegate";
		case FORTH_TOKEN_DABS:		return "dabs";
		case FORTH_TOKEN_DMIN:		return "dmin";
		case FORTH_TOKEN_DMAX:		return "dmax";
		case FORTH_TOKEN_D_Plus:	return "d+";
		case FORTH_TOKEN_D_Subtract:	return "d-";
		case FORTH_TOKEN_D_Less:	return "d<";
		case FORTH_TOKEN_D_ULess:	return "du<";
#endif
		case FORTH_TOKEN_D_Equal:	return "d=";
	
		case FORTH_TOKEN_AND:		return "and";
		case FORTH_TOKEN_OR:		return "or";
		case FORTH_TOKEN_XOR:		return "xor";
		case FORTH_TOKEN_INVERT:	return "invert";

		case FORTH_TOKEN_toR:		return ">r";
		case FORTH_TOKEN_Rfrom:		return "r>";
		case FORTH_TOKEN_Rfetch:	return "r@";

		case FORTH_TOKEN_2toR:		return "2>r";
		case FORTH_TOKEN_2Rfrom:	return "2r>";
		case FORTH_TOKEN_2Rfetch:	return "2r@";

		case FORTH_TOKEN_NtoR:		return "n>r";
		case FORTH_TOKEN_NRfrom:	return "nr>";

		case FORTH_TOKEN_PROCESS_NUMBER: return "process-number";
#if !defined(FORTH_NO_DOUBLES)
		case FORTH_TOKEN_toNUMBER:	return ">number";
#endif
		case FORTH_TOKEN_BLK:		return "blk";
		case FORTH_TOKEN_TIB:		return "tib";
		case FORTH_TOKEN_HashTIB:	return "#tib";
		case FORTH_TOKEN_REFILL:	return "refill";
		case FORTH_TOKEN_QUERY:		return "query";
		case FORTH_TOKEN_pSOURCE_ID:	return "(source-id)";
		case FORTH_TOKEN_SOURCE:	return "source";
		case FORTH_TOKEN_SOURCE_Store:	return "source!";
		case FORTH_TOKEN_LINE_NUMBER:	return "line-number"; // LINE-NUMBER ( line number inside a file).
		case FORTH_TOKEN_toIN:		return ">in";
		case FORTH_TOKEN_SAVE_INPUT:	return "save-input";
		case FORTH_TOKEN_RESTORE_INPUT: return "restore-input";

		case FORTH_TOKEN_BASE:		return "base";
		case FORTH_TOKEN_STATE:		return "state";
		case FORTH_TOKEN_CStore:	return "c!";
		case FORTH_TOKEN_PlusStore:	return "+!";
		case FORTH_TOKEN_SWAP:		return "swap";
		case FORTH_TOKEN_THROW:		return "throw";
		case FORTH_TOKEN_DotError:	return ".error";
		case FORTH_TOKEN_TYPE:		return "type";
		case FORTH_TOKEN_BYE:		return "bye";
		case FORTH_TOKEN_WORDS:		return "words";
		case FORTH_TOKEN_ENVIRONMENTq:	return "environment?";

		case FORTH_TOKEN_pSEE:		return "(see)";
		case FORTH_TOKEN_ONLY:		return "only";
		case FORTH_TOKEN_ALSO:		return "also";
		case FORTH_TOKEN_GET_ORDER:	return "get-order";
		case FORTH_TOKEN_SET_ORDER:	return "set-order";
		case FORTH_TOKEN_CURRENT:	return "current";
		case FORTH_TOKEN_CONTEXT:	return "context";
		case FORTH_TOKEN_toBODY:	return  ">body";
		case FORTH_TOKEN_Equal:		return	"=";
		case FORTH_TOKEN_Notequal:	return "<>";
		case FORTH_TOKEN_Less:		return "<";
		case FORTH_TOKEN_Greater:	return ">";
		case FORTH_TOKEN_ULess:		return  "u<";
		case FORTH_TOKEN_UGreater:	return "u>";

		case FORTH_TOKEN_0Equal:	return "0=";
		case FORTH_TOKEN_0Notequal:	return "0<>";
		case FORTH_TOKEN_0Less:		return "0<";
		case FORTH_TOKEN_0Greater:	return "0>";

		case FORTH_TOKEN_LITERAL:	return "literal";
 
		case FORTH_TOKEN_sp0:		return "sp0";
		case FORTH_TOKEN_sp_fetch:	return "sp@";
		case FORTH_TOKEN_sp_store:	return "sp!";
		case FORTH_TOKEN_rp0:		return "rp0";
		case FORTH_TOKEN_rp_fetch:	return "rp@";
		case FORTH_TOKEN_rp_store:	return "rp!";
		case FORTH_TOKEN_handler:	return "handler";
		case FORTH_TOKEN_abort_msg:	return "(abort-msg)";	// To hold the string for ABORT".
		case FORTH_TOKEN_resolve_branch:	return "resolve-branch";
		case FORTH_TOKEN_ix2address:	return "ix>address";
		case FORTH_TOKEN_pDO:		return "(do)";		// (DO)
		case FORTH_TOKEN_pqDO:		return "(?do)"; 	// (?DO)
		case FORTH_TOKEN_UNLOOP:	return "unloop";	// UNLOOP
		case FORTH_TOKEN_LEAVE:		return "leave";		// LEAVE
		case FORTH_TOKEN_I:		return "i";		// I
		case FORTH_TOKEN_J:		return "j";		// J
		case FORTH_TOKEN_pLOOP:		return "(loop)";	// (LOOP)
		case FORTH_TOKEN_pPlusLOOP:	return "(+loop)";	// (+LOOP)
		case FORTH_TOKEN_EXIT:		return "exit";
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
		case FORTH_TOKEN_FILE_CREATE:	return "create-file";	// CREATE-FILE
		case FORTH_TOKEN_FILE_OPEN:	return "open-file";	// OPEN-FILE
		case FORTH_TOKEN_FILE_FLUSH:	return "flush-file";	// FLUSH-FILE
		case FORTH_TOKEN_FILE_DELETE:	return "delete-file";	// DELETE-FILE
		case FORTH_TOKEN_FILE_REPOSITION:return "reposition-file"; // REPOSITION-FILE
		case FORTH_TOKEN_FILE_POSITION:	return "file-position"; // FILE-POSITION
		case FORTH_TOKEN_FILE_SIZE:	return "file-size";	// FILE-SIZE
		case FORTH_TOKEN_FILE_READ:	return "read-file";	// READ-FILE
		case FORTH_TOKEN_FILE_READ_LINE:return "read-line";	// READ-LINE
		case FORTH_TOKEN_FILE_WRITE:	return "write-file";	// WRITE-FILE
		case FORTH_TOKEN_FILE_WRITE_LINE:return "write-line";	// WRITE-LINE
		case FORTH_TOKEN_FILE_CLOSE:	return "close-file";	// CLOSE-FILE
#endif
#if defined(FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS)
		case FORTH_TOKEN_ALLOCATE:	return "allocate";
		case FORTH_TOKEN_RESIZE:	return "resize";
		case FORTH_TOKEN_FREE:		return "free";
#endif

		case FORTH_TOKEN_branch:	return "branch";
		case FORTH_TOKEN_0branch:	return "0branch";
		case FORTH_TOKEN_xtlit:		return "xtlit";
		case FORTH_TOKEN_lit:		return "lit";
		case FORTH_TOKEN_uslit:		return "uslit";
		case FORTH_TOKEN_sslit:		return "sslit";
		case FORTH_TOKEN_strlit:	return "strlit";
		case FORTH_TOKEN_nest:		return "nest";
		case FORTH_TOKEN_unnest:	return "unnest";
		case FORTH_TOKEN_dovar:		return "dovar";
#if defined(FORTH_EXTERNAL_PRIMITIVES)
		case FORTH_TOKEN_doextern:	return "doextern";
#endif
		case FORTH_TOKEN_docreate:	return "docreate";
		case FORTH_TOKEN_doconst:	return "doconst";
#if defined(FORTH_USER_VARIABLES)
		case FORTH_TOKEN_USER_ALLOT:	return "user-allot";
		case FORTH_TOKEN_douser:	return "douser";
#endif
	}
	return (const char *)0;
}

static int forth_show_name(struct forth_runtime_context *rctx, forth_cell_t xt)
{
	forth_cell_t token_primitive;
	forth_cell_t name_length;
	const struct forth_header *h;
	char *end = rctx->internal_buffer + sizeof(rctx->internal_buffer);
	const char *p;

	if (FORTH_IS_TOKEN(xt))
	{
		token_primitive = FORTH_EXTRACT_TOKEN(xt);
		p = forth_token_name(token_primitive);
		if (0 != p)
		{
			return forth_type0(rctx, p);
		}		
	}
	else
	{
		h = (const struct forth_header *)(&dictionary[xt - 2]);
		name_length = (h->flags & (FORTH_HEADER_FLAGS_NAME_LENGTH_MASK));
		if (0 == name_length)
		{
			forth_type0(rctx, "noname@");
			p = forth_format_unsigned(xt, 16, FORTH_CELL_HEX_DIGITS, end);
			return rctx->write_string(rctx, p, (end - p));
		}
		else
		{
			return rctx->write_string(rctx, (const char *)(&dictionary[(xt - 2) - ((FORTH_ALIGN(name_length)) / sizeof(forth_cell_t))]), name_length);
		}
	}

	return 0;
}

static int forth_show_executing(struct forth_runtime_context *rctx, forth_cell_t w, forth_cell_t xt)
{
//	forth_cell_t token_primitive;
	// char buffer[32];
	// char *end = buffer + 31;
	char *end = rctx->internal_buffer + sizeof(rctx->internal_buffer);
	char *p = end;
	*p = ':';
	*--p = ']';
	p = forth_format_unsigned(w, 16, FORTH_CELL_HEX_DIGITS, p);
	*--p = '[';
	rctx->write_string(rctx, p, (end - p) + 1);

	// end = buffer + 31;
	p = end;
	*p = FORTH_CHAR_SPACE;
	p = forth_format_unsigned(xt, 16, FORTH_CELL_HEX_DIGITS, p);
	rctx->write_string(rctx, p, (end - p) + 1);
	forth_show_name(rctx, xt);
	rctx->write_string(rctx, end, 1);
	return forth_dots(rctx);
	// rctx->send_cr(rctx);
}

static int forth_print_next_symbol(struct forth_runtime_context *rctx, forth_cell_t dictionary[], forth_cell_t *ix)
{
	forth_cell_t xt = dictionary[(*ix)++];
	forth_scell_t offset;
	forth_cell_t count;

	if (FORTH_IS_TOKEN(xt))
	{
		switch (FORTH_EXTRACT_TOKEN(xt))
		{
			case FORTH_TOKEN_uslit:
				forth_dot(rctx, rctx->base, FORTH_PARAM_UNSIGNED(xt));
			break;

			case FORTH_TOKEN_sslit:
				forth_dot(rctx, rctx->base, FORTH_PARAM_SIGNED(xt));
			break;

			case FORTH_TOKEN_strlit:
				count = FORTH_PARAM_UNSIGNED(xt);
				forth_type0(rctx, "S\" ");
				rctx->write_string(rctx, (char *)&dictionary[*ix], count);
				forth_type0(rctx, "\"");
				*ix += FORTH_ALIGN(count) / sizeof(forth_cell_t);
			break;

			case FORTH_TOKEN_lit:
				forth_dot(rctx, rctx->base, dictionary[(*ix)++]);
			break;

			case FORTH_TOKEN_xtlit:
				forth_type0(rctx, "['] ");
				forth_show_name(rctx, dictionary[(*ix)++]);
			break;

			case FORTH_TOKEN_Imm_Plus:
				offset = FORTH_PARAM_SIGNED(xt);
				
				if (offset < 0)
				{
					forth_dot(rctx, rctx->base, -offset);
					forth_type0(rctx, " -");
				}
				else
				{
					forth_dot(rctx, rctx->base, offset);
					forth_type0(rctx, " +");
				}
			break;

			case FORTH_TOKEN_branch:
			case FORTH_TOKEN_0branch:
				forth_show_name(rctx, xt);
				offset = FORTH_PARAM_SIGNED(xt);
				if (0 <= offset)
				{
					forth_type0(rctx, " +");
				}
				else
				{
					forth_type0(rctx, " ");
				}
				forth_dot(rctx, rctx->base, offset);
			break;

			default:
				forth_show_name(rctx, xt);
			break;
				
		}
	} 
	else 
	{
		forth_show_name(rctx, xt);
	}

	return rctx->send_cr(rctx);
}

static int forth_see(struct forth_runtime_context *rctx, forth_cell_t dictionary[], forth_cell_t xt)
{
	forth_cell_t ix;

	if (FORTH_IS_TOKEN(xt))
	{
		forth_type0(rctx, "Primitive: ");
		forth_show_name(rctx, xt);
		return rctx->send_cr(rctx);
	}

	if (FORTH_IS_NOT_TOKEN(dictionary[xt]))
	{
		forth_type0(rctx, " ' ");
		forth_show_name(rctx, dictionary[xt]);
		forth_type0(rctx, " SYNONYM ");
		return forth_show_name(rctx, xt);
	}

	switch(FORTH_EXTRACT_TOKEN(dictionary[xt]))
	{
		case FORTH_TOKEN_nest:
			if (0 == (dictionary[xt - 1] & (FORTH_HEADER_FLAGS_NAME_LENGTH_MASK)))
			{
				forth_type0(rctx, ":NONAME");
			}
			else
			{
				forth_type0(rctx, ": ");
				forth_show_name(rctx, xt);
			}
			rctx->send_cr(rctx);

			ix = xt + 1;

			while (FORTH_TOKEN_unnest != FORTH_EXTRACT_TOKEN(xt = dictionary[ix]))
			{
				forth_print_next_symbol(rctx, dictionary, &ix);
			}
			forth_type0(rctx, ";");
		break;

		case FORTH_TOKEN_dovar:
			forth_type0(rctx, "VARIABLE ");
			forth_show_name(rctx, xt);
		break;
	
		case FORTH_TOKEN_doconst:
			forth_dot(rctx, rctx->base, dictionary[xt + 1]);
			forth_type0(rctx, "CONSTANT ");
			forth_show_name(rctx, xt);
		break;

#if defined(FORTH_EXTERNAL_PRIMITIVES)
		case FORTH_TOKEN_doextern:
			forth_show_name(rctx, xt);
			forth_type0(rctx, " External-primitive[ ");
			forth_dot(rctx, rctx->base, dictionary[xt + 1]);
			forth_type0(rctx, "]");
		break;
#endif
#if defined(FORTH_USER_VARIABLES)
		case FORTH_TOKEN_douser:
			forth_type0(rctx, "USER ");
			forth_show_name(rctx, xt);
		break;
#endif
		case FORTH_TOKEN_docreate:
			forth_type0(rctx, "CREATE ");
			forth_show_name(rctx, xt);
			rctx->send_cr(rctx);
			ix = xt + 1;
			xt = dictionary[ix];

			forth_type0(rctx, "...");
			rctx->send_cr(rctx);

			if (FORTH_IS_TOKEN(xt))
			{
				forth_print_next_symbol(rctx, dictionary, &ix);

			}
			else
			{
				ix = xt;
				if (FORTH_TOKEN_nest == FORTH_EXTRACT_TOKEN(dictionary[ix]))
				{
					ix++;
					forth_type0(rctx, "DOES>");
					rctx->send_cr(rctx);
					while (FORTH_TOKEN_unnest != FORTH_EXTRACT_TOKEN(xt = dictionary[ix]))
					{
						forth_print_next_symbol(rctx, dictionary, &ix);
					}
					forth_type0(rctx, ";");
				}
				else
				{
					forth_type0(rctx, "???");
				}
			}
		break;

		default:
			forth_show_name(rctx, dictionary[xt]);
			forth_type0(rctx, " ???");
		break;
	}

	return rctx->send_cr(rctx);
}

forth_scell_t forth_compare_environment(const char *qs, const char *es, forth_cell_t len)
{
	forth_cell_t el = strlen(es);
	if (len != el)
	{
		return 1;
	}
	return forth_compare_strings(qs, len, es, el);
}

static void forth_query_environment(struct forth_runtime_context *rctx)
{
	char *s = (char *)(rctx->sp[1]);	
	forth_cell_t len  = rctx->sp[0];

	rctx->sp[0] = FORTH_TRUE;

	if (0 == forth_compare_environment(s, "CORE", len))
	{
		rctx->sp[1] = FORTH_FALSE;
	}
	else if (0 == forth_compare_environment(s, "CORE-EXT", len))
	{
		rctx->sp[1] = FORTH_FALSE;
	}
	else if (0 == forth_compare_environment(s, "/COUNTED-STRING", len))
	{
		rctx->sp[1] = 255;
	}
	else if (0 == forth_compare_environment(s, "/HOLD", len))
	{
		rctx->sp[1] = FORTH_NUM_BUFF_LENGTH;
	}
	else if (0 == forth_compare_environment(s, "/PAD", len))
	{
		rctx->sp[1] = 84;
	}
/*
	else if (0 == forth_compare_environment(s, "FLOORED", len))
	{
		rctx->sp[1] = FORTH_TRUE;	// ????????
	}
*/
	else if (0 == forth_compare_environment(s, "MAX-CHAR", len))
	{
		rctx->sp[1] = 255;
	}
	else if (0 == forth_compare_environment(s, "MAX-D", len))
	{
		rctx->sp[1] = ((forth_cell_t)-1);
		rctx->sp[0] = ((forth_cell_t)-1) >> 1;
		rctx->sp--;
		rctx->sp[0] = FORTH_TRUE;
	}
	else if (0 == forth_compare_environment(s, "MAX-UD", len))
	{
		rctx->sp[1] = ((forth_cell_t)-1);
		rctx->sp[0] = ((forth_cell_t)-1);
		rctx->sp--;
		rctx->sp[0] = FORTH_TRUE;
	}
	else if (0 == forth_compare_environment(s, "MAX-N", len))
	{
		rctx->sp[1] = ((forth_cell_t)-1) >> 1;
	}
	else if (0 == forth_compare_environment(s, "MAX-U", len))
	{
		rctx->sp[1] = (forth_cell_t)-1;
	}
	else if (0 == forth_compare_environment(s, "ADDRESS-UNIT-BITS", len))
	{
		rctx->sp[1] = 8;
	}
	else if (0 == forth_compare_environment(s, "RETURN-STACK-CELLS", len))
	{
		rctx->sp[1] = rctx->rp_max - rctx->rp_min;
	}
	else if (0 == forth_compare_environment(s, "STACK-CELLS", len))
	{
		rctx->sp[1] = rctx->sp_max - rctx->sp_min;
	}
	else
	{
		rctx->sp++;
		rctx->sp[0] = 0;
	}
}

forth_cell_t forth_translate_token(forth_cell_t xt)
{
	const struct forth_header *h;

	if (!FORTH_IS_TOKEN(xt))
	{
		h = (const struct forth_header *)(&dictionary[xt - 2]);

		// If the dictionary word is just a wrapper around a token unwrap it.
		if (0 != ((FORTH_HEADER_FLAGS_TOKEN) & h->flags))
		{
			xt = dictionary[xt];
		}
	}

	return xt;
}

// =======================================================================================
int forth(struct forth_runtime_context *rctx, forth_cell_t word_to_exec)
{
	register forth_index_t ip = rctx->ip;
	register forth_cell_t  *sp = rctx->sp;
	register forth_cell_t  *rp = rctx->rp;
	register forth_cell_t  *dictionary = rctx->dictionary;
	register forth_cell_t  w = 0;
	register forth_cell_t  xt = word_to_exec;
	forth_cell_t token_primitive;
	register forth_cell_t  tos;
	forth_dcell_t dtos;
	forth_scell_t i;
	// forth_cell_t exception_handler = 0;
	char c;
	char *src;
	char *dest;
#if defined(FORTH_EXTERNAL_PRIMITIVES)
	forth_external_primitive ep;
#endif

#define POP() *(sp++)
#define PUSH(X) *(--sp) = ((forth_cell_t)(X))
#define THROW(X) PUSH(X); xt = FORTH_PACK_TOKEN(FORTH_TOKEN_THROW); continue

#define RPOP()	*(rp++)
#define RPUSH(X) *(--rp) = ((forth_cell_t)(X))

// For DO-LOOPs
#define LOOP_I rp[0]
#define LOOP_J rp[3]
#define LOOP_LIMIT   rp[1]
#define LOOP_ADDRESS_AFTER rp[2]
#define LOOP_ADJUSTMENT 3

#define SIGNED_PARAMETER   FORTH_PARAM_SIGNED(xt)
#define UNSIGNED_PARAMETER FORTH_PARAM_UNSIGNED(xt)

	while (1)
	{
#if defined(FORTH_STACK_CHECK_ENABLED)
		if (sp < rctx->sp_min)
		{
			*sp = -3;
			w = 0;
			xt = FORTH_PACK_TOKEN(FORTH_TOKEN_THROW);
		} else if (sp > rctx->sp_max)
		{
			*sp = -4;
			w = 0;
			xt = FORTH_PACK_TOKEN(FORTH_TOKEN_THROW);
		} else if (rp < rctx->rp_min)
		{
			*sp = -5;
			w = 0;
			xt = FORTH_PACK_TOKEN(FORTH_TOKEN_THROW);
		} else if (rp > rctx->rp_max)
		{
			*sp = -6;
			w = 0;
			xt = FORTH_PACK_TOKEN(FORTH_TOKEN_THROW);
		}
#endif
		if (rctx->trace)
		{
			rctx->sp = sp;
			if (0 > forth_show_executing(rctx, w, xt))
			{
				// What to do here?
				THROW(-57);
			}
		}
		// printf("xt = 0x%08x\n", xt); fflush(stdout);
		if (FORTH_IS_NOT_TOKEN(xt))
		{
			// printf("not token xt = 0x%08x\n", xt); fflush(stdout);
			w = FORTH_INDEX_EXTRACT(xt);
			xt = dictionary[w];
			// printf("new xt = 0x%08x\n", xt); fflush(stdout);
			continue;
		}

		token_primitive = FORTH_EXTRACT_TOKEN(xt);

#if 1
//		printf("\t[%08X] token xt = 0x%08x: %s [0X%04X]\n", w, xt, forth_token_name(token_primitive), UNSIGNED_PARAMETER); fflush(stdout);
#endif
		switch(token_primitive)
		{
			case FORTH_TOKEN_NOP:
			break;

			case FORTH_TOKEN_ACCEPT:
				tos = POP();
				tos = forth_accept(rctx, (char *)tos, *sp);

				if (0 > (forth_scell_t)tos)
				{
					THROW(-57);
				}

				PUSH(tos);
			break;

			case FORTH_TOKEN_KEY:
				if (0 == rctx->key)
				{
					THROW(-21);
				}

				tos = rctx->key(rctx);

				if (FORTH_TRUE == tos)
				{
					THROW(-57);
				}

				PUSH(tos);
			break;

			case FORTH_TOKEN_EKEY:
				if (0 == rctx->ekey)
				{
					THROW(-21);
				}

				tos = rctx->ekey(rctx);

				if (FORTH_TRUE == tos)
				{
					THROW(-57);
				}

				PUSH(tos);
			break;

			case FORTH_TOKEN_KEYq:		// KEY?
				if (0 == rctx->key_q)
				{
					THROW(-21);
				}

				tos = rctx->key_q(rctx);

				if (((forth_scell_t) tos) < 0)
				{
					THROW(-21);
				}

				tos = tos ? FORTH_TRUE : FORTH_FALSE;
				PUSH(tos);
			break;

			case FORTH_TOKEN_EKEYq:		// EKEY?
				if (0 == rctx->ekey_q)
				{
					THROW(-21);
				}

				tos = rctx->ekey_q(rctx);
				if (((forth_scell_t) tos) < 0)
				{
					THROW(-21);
				}

				tos = tos ? FORTH_TRUE : FORTH_FALSE;
				PUSH(tos);
			break;

			case FORTH_TOKEN_EKEY2CHAR:	// EKEY>CHAR
				if (0 == rctx->ekey_to_char)
				{
					THROW(-21);
				}

				tos = rctx->ekey_to_char(rctx, sp[0]);

				if (FORTH_TRUE == tos)
				{
					PUSH(FORTH_FALSE);
				}
				else
				{
					sp[0] = tos;
					PUSH(FORTH_TRUE);
				}
			break;

			case FORTH_TOKEN_ALIGN:
				// pctx->dp = FORTH_ALIGN((pctx->dp));
				dictionary[FORTH_DP_LOCATION] = FORTH_ALIGN(dictionary[FORTH_DP_LOCATION]);
			break;

			case FORTH_TOKEN_ALIGNED:
				*sp = FORTH_ALIGN(*sp);
			break;

			case FORTH_TOKEN_ALLOT:
				tos = POP();
				if (dictionary[FORTH_DP_MAX_LOCATION] <=  (dictionary[FORTH_DP_LOCATION] + tos))
				{
					THROW(-8);
				}
				dictionary[FORTH_DP_LOCATION] += tos;
			break;

			case FORTH_TOKEN_PAD:	// Just use HERE for now.
			case FORTH_TOKEN_HERE:
				tos = ((forth_cell_t)dictionary) + dictionary[FORTH_DP_LOCATION];
				PUSH(tos);
			break;

			case FORTH_TOKEN_pHERE:
				tos = (dictionary[FORTH_DP_LOCATION] / sizeof(forth_cell_t));
				PUSH(tos);
			break;

			case FORTH_TOKEN_pTRACE:
				tos = (forth_cell_t)(&(rctx->trace));
				PUSH(tos);
			break;

			case FORTH_TOKEN_CompileComma:	// COMPILE,
				*sp = forth_translate_token(*sp);
				// FALL THROUGH TO Comma.
			case FORTH_TOKEN_Comma:		// ,
				if (dictionary[FORTH_DP_MAX_LOCATION] <=  (dictionary[FORTH_DP_LOCATION] + sizeof(forth_cell_t)))
				{
					THROW(-8);
				}
				tos = ((forth_cell_t)dictionary) + dictionary[FORTH_DP_LOCATION];
				*(forth_cell_t *)tos = POP();
				dictionary[FORTH_DP_LOCATION] += sizeof(forth_cell_t);
			break;

			case FORTH_TOKEN_CComma:		// C,
				if (dictionary[FORTH_DP_MAX_LOCATION] <=  (dictionary[FORTH_DP_LOCATION] + sizeof(char)))
				{
					THROW(-8);
				}
				tos = ((forth_cell_t)dictionary) + dictionary[FORTH_DP_LOCATION];
				*(char *)tos = POP();
				dictionary[FORTH_DP_LOCATION] += sizeof(char);
			break;

			case FORTH_TOKEN_Imm_Plus:				// imm+
				*sp += SIGNED_PARAMETER;
			break;

			case FORTH_TOKEN_Subtract:	// -
				sp[1] -= sp[0];
				sp++;
			break;

			case FORTH_TOKEN_Divide:	// /
				tos = POP();
				sp[0] = (forth_cell_t)((forth_scell_t)sp[0] / (forth_scell_t)tos);
			break;

			case FORTH_TOKEN_MOD:
				tos = POP();
				sp[0] = (forth_cell_t)((forth_scell_t)sp[0] % (forth_scell_t)tos);
			break;

			case FORTH_TOKEN_Slash_MOD:	// /MOD
				tos = sp[0];
				sp[0] = (forth_cell_t)((forth_scell_t)sp[1] / (forth_scell_t)tos);
				sp[1] = (forth_cell_t)((forth_scell_t)sp[1] % (forth_scell_t)tos);
			break;

#if !defined(FORTH_NO_DOUBLES)
			case FORTH_TOKEN_StarSlash:	// */
				sp[2] = (forth_cell_t)((((forth_sdcell_t)sp[2]) * (forth_scell_t)sp[1]) / (forth_scell_t)sp[0]);
				sp += 2;
			break;

			case FORTH_TOKEN_StarSlash_MOD:	// */MOD
				dtos = (forth_dcell_t)(((forth_sdcell_t)sp[2]) * (forth_scell_t)sp[1]);
				tos = POP();
				sp[1] = (forth_cell_t)(((forth_sdcell_t)dtos) % (forth_scell_t)tos);
				sp[0] = (forth_cell_t)(((forth_sdcell_t)dtos) / (forth_scell_t)tos);
			break;

			case FORTH_TOKEN_UM_Star:	// UM*
				dtos = (forth_dcell_t)sp[0] * sp[1];
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;

			case FORTH_TOKEN_M_Star:	// M* ( n1 n2 -- d )
				dtos = (forth_dcell_t)((forth_sdcell_t)((forth_scell_t)sp[1]) * (forth_scell_t)(sp[0]));
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;

			case FORTH_TOKEN_M_Plus:	// M+ ( d1 n -- d2 )
				dtos = FORTH_DCELL(sp[1], sp[2]);
				dtos = dtos + POP();
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;


			case FORTH_TOKEN_UM_Slash_MOD:	// UM/MOD
				tos = POP();
				dtos = FORTH_DCELL(sp[0], sp[1]);
				sp[1] = (forth_cell_t)(dtos % tos);
				sp[0] = (forth_cell_t)(dtos / tos);
			break;
#endif
			case FORTH_TOKEN_Multiply:	// *
				tos = POP();
				sp[0] = sp[0] * tos;
			break;

			case FORTH_TOKEN_NEGATE:
				sp[0] = -(forth_scell_t)sp[0];
			break;

			case FORTH_TOKEN_INVERT:
				sp[0] = ~sp[0];
			break;

			case FORTH_TOKEN_ABS:
				if (0 > (forth_scell_t)sp[0])
				{
					sp[0] = -(forth_scell_t)sp[0];
				}
			break;

			case FORTH_TOKEN_MIN:
				tos = POP();
				if ((forth_scell_t)sp[0] > (forth_scell_t)tos)
				{
					sp[0] = tos;
				}
			break;

			case FORTH_TOKEN_MAX:
				tos = POP();
				if ((forth_scell_t)sp[0] < (forth_scell_t)tos)
				{
					sp[0] = tos;
				}
			break;

			case FORTH_TOKEN_LSHIFT:
				tos = POP();
				sp[0] <<= tos;
			break;

			case FORTH_TOKEN_RSHIFT:
				tos = POP();
				sp[0] >>= tos;
			break;

			case FORTH_TOKEN_2_Star:	// 2*
				(*sp) <<= 1;
			break;

			case FORTH_TOKEN_2_Slash:	// 2/
				(*sp) >>= 1;
			break;

#if !defined(FORTH_NO_DOUBLES)
			case FORTH_TOKEN_D2_Star:	// d2*
				dtos = FORTH_DCELL(sp[0], sp[1]);
				dtos <<= 1;
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;

			case FORTH_TOKEN_D2_Slash:	// d2/
				dtos = FORTH_DCELL(sp[0], sp[1]);
				dtos >>= 1;
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;


			case FORTH_TOKEN_DNEGATE:
				dtos = FORTH_DCELL(sp[0], sp[1]);
				dtos = -(forth_sdcell_t)dtos;
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;

			case FORTH_TOKEN_DABS:
				if (0 > (forth_scell_t)(sp[0]))
				{
					xt = FORTH_PACK_TOKEN(FORTH_TOKEN_DNEGATE);
					continue;
				}
			break;

			case FORTH_TOKEN_DMIN:
				dtos = FORTH_DCELL(sp[0], sp[1]);
				if ((forth_sdcell_t)dtos < (forth_sdcell_t)FORTH_DCELL(sp[2], sp[3]))
				{
					sp[0] = sp[2];
					sp[1] = sp[3];
				}
				sp += 2;
			break;

			case FORTH_TOKEN_DMAX:
				dtos = FORTH_DCELL(sp[0], sp[1]);
				if ((forth_sdcell_t)dtos < (forth_sdcell_t)FORTH_DCELL(sp[2], sp[3]))
				{
					sp[0] = sp[2];
					sp[1] = sp[3];
				}
				sp += 2;
			break;

			case FORTH_TOKEN_D_Plus:	// d+
				dtos = FORTH_DCELL(sp[0], sp[1]);
				sp += 2;
				dtos += FORTH_DCELL(sp[0], sp[1]);
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;

			case FORTH_TOKEN_D_Subtract:	// d-
				dtos = FORTH_DCELL(sp[2], sp[3]);
				dtos -= FORTH_DCELL(sp[0], sp[1]);
				sp += 2;
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;

			case FORTH_TOKEN_D_Less:	// D<
				sp[3] = ((forth_sdcell_t)(FORTH_DCELL(sp[0], sp[1])) > (forth_sdcell_t)(FORTH_DCELL(sp[2], sp[3]))) ? FORTH_TRUE : FORTH_FALSE;
				sp += 3;
			break;

			case FORTH_TOKEN_D_ULess:	// DU<
				sp[3] = (FORTH_DCELL(sp[0], sp[1]) > FORTH_DCELL(sp[2], sp[3])) ? FORTH_TRUE : FORTH_FALSE;
				sp += 3;
			break;
#endif
			case FORTH_TOKEN_D_Equal:	// D=
				sp[3] = ((sp[3] == sp[2]) && (sp[1] == sp[0])) ? FORTH_TRUE : FORTH_FALSE;
				sp += 3;
			break;

			case FORTH_TOKEN_AND:
				sp[1] &= sp[0];
				sp++;
			break;

			case FORTH_TOKEN_OR:
				sp[1] |= sp[0];
				sp++;
			break;

			case FORTH_TOKEN_XOR:
				sp[1] ^= sp[0];
				sp++;
			break;

			case FORTH_TOKEN_CELLS:
				*sp = sizeof(forth_cell_t) * (*sp);
			break;

			case FORTH_TOKEN_DROP:
				sp++;
			break;

			case FORTH_TOKEN_2ROT:
				tos = sp[0];
				sp[0] = sp[4];
				sp[4] = sp[2];
				sp[2] = tos;
				tos = sp[1];
				sp[1] = sp[5];
				sp[5] = sp[3];
				sp[3] = tos;
			
			break;

			case FORTH_TOKEN_2DUP:
				sp -= 2;
				sp[1] = sp[3];
				sp[0] = sp[2];
			break;

			case FORTH_TOKEN_2DROP:
				sp += 2;
			break;

			case FORTH_TOKEN_2OVER:
				sp -= 2;
				sp[1] = sp[5];
				sp[0] = sp[4];
			break;

			case FORTH_TOKEN_2SWAP:
				tos = sp[0];
				sp[0] = sp[2];
				sp[2] = tos;
				tos = sp[1];
				sp[1] = sp[3];
				sp[3] = tos;
			break;

			case FORTH_TOKEN_2Store:	// 2!
				tos = sp[0];
				((forth_cell_t *)(tos))[0] = sp[1];
				((forth_cell_t *)(tos))[1] = sp[2];
				sp += 3;
			break;

			case FORTH_TOKEN_2Fetch:	// 2@
				tos = sp[0];
				sp--;
				sp[0] = ((forth_cell_t *)(tos))[0];
				sp[1] = ((forth_cell_t *)(tos))[1];
			break;

			case FORTH_TOKEN_NtoR:		// N>R
				tos = POP();
				rp -= tos;
				for (i = 0; i < tos; i++)
				{
					rp[i] = sp[i];
				}
				sp += tos;
				RPUSH(tos);
			break;

			case FORTH_TOKEN_NRfrom:	// NR>
				tos = RPOP();
				sp -= tos;
				for (i = 0; i < tos; i++)
				{
					sp[i] = rp[i];
				}
				rp += tos;
				PUSH(tos);
			break;


			case FORTH_TOKEN_DUP:
				sp--;
				sp[0] = sp[1];
			break;

			case FORTH_TOKEN_qDUP:
				if (0 != sp[0])
				{
					sp--;
					sp[0] = sp[1];
				}
			break;

			case FORTH_TOKEN_PAGE:
				if (0 == rctx->page)
				{
					THROW(-21);
				}

				if (0 > rctx->page(rctx))
				{
					THROW(-57);
				}
			break;

#if defined(FORTH_INCLUDE_MS)
			case FORTH_TOKEN_MS:
				rctx->sp = sp;
				forth_ms(rctx);
				sp = rctx->sp;
			break;
#endif

#if defined(FORTH_INCLUDE_TIME_DATE)
		case FORTH_TOKEN_TIME_DATE:
			rctx->sp = sp;
			forth_time_date(rctx);
			sp = rctx->sp;
		break;
#endif

			case FORTH_TOKEN_AT_XY:		// AT-XY ( X Y -- )
				if (0 == rctx->at_xy)
				{
					THROW(-21);
				}

				if (0 > rctx->at_xy(rctx, sp[1], sp[0]))
				{
					THROW(-57);
				}

				sp += 2;
			break;

			case FORTH_TOKEN_CR:
				if (0 > rctx->send_cr(rctx))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_EMIT:
				tos = POP();
				c = (char) tos;
				if (0 > rctx->write_string(rctx, &c, 1))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_UNUSED:
				// tos = pctx->dp_max - pctx->dp;
				tos = dictionary[FORTH_DP_MAX_LOCATION] - dictionary[FORTH_DP_LOCATION];
				PUSH(tos);
			break;

			case FORTH_TOKEN_DUMP:
				if (0 > forth_dump(rctx, (char *)(sp[1]), sp[0]))
				{
					THROW(-57);
				}
				sp += 2;
			break;

			case FORTH_TOKEN_handler:
				tos = (forth_cell_t)&(rctx->handler);
				PUSH(tos);
			break;

			case FORTH_TOKEN_LessHash:	// <#
				rctx->numbuff_ptr = &(rctx->num_buff[FORTH_NUM_BUFF_LENGTH]);
			break;

#if !defined(FORTH_NO_DOUBLES)
			case FORTH_TOKEN_Hash:	// #
				if (0 == rctx->base)
				{
					THROW(-10);
				}

				dtos = FORTH_DCELL(sp[0], sp[1]);
				*(--(rctx->numbuff_ptr)) = forth_val2digit((forth_byte_t)(dtos % rctx->base));
				dtos = dtos / rctx->base;
				sp[1] = FORTH_CELL_LOW(dtos);
				sp[0] = FORTH_CELL_HIGH(dtos);
			break;
#endif

			case FORTH_TOKEN_HashGreater:	// #>
				sp[1] = (forth_cell_t)(rctx->numbuff_ptr);
				sp[0] = &(rctx->num_buff[FORTH_NUM_BUFF_LENGTH]) - rctx->numbuff_ptr;

			break;

			case FORTH_TOKEN_HOLD:
				*(--(rctx->numbuff_ptr)) = POP();
			break;


			case FORTH_TOKEN_Hdot:					// H.
				tos = POP();
				if (0 > forth_hdot(rctx, tos))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_Udot:
				tos = POP();
				if (0 > forth_udot(rctx, rctx->base, tos))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_Dot:
				tos = POP();
				if (0 > forth_dot(rctx, rctx->base, tos))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_DotR:		// .R
				tos = POP();
				if (0 > forth_dot_r(rctx, rctx->base, POP(), tos, 1))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_UdotR:		// U.R
				tos = POP();
				if (0 > forth_dot_r(rctx, rctx->base, POP(), tos, 0))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_DotS:
				rctx->sp = sp;
				if (0 > forth_dots(rctx))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_DotName:	// .name
				if (0 > forth_show_name(rctx, POP()))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_CMOVE:
				tos = POP();
				dest = (char *)(POP());
				src = (char *)(POP());
				for (i = 0; i < tos; i++)
				{
					dest[i] = src[i];
				}
			break;

			case FORTH_TOKEN_CMOVE_down:	// cmove>
				tos = POP();
				dest = (char *)(POP());
				src = (char *)(POP());
				for (i = tos - 1; i  >= 0; i++)
				{
					dest[i] = src[i];
				}
			break;

			case FORTH_TOKEN_FILL:	// ( addr count char -- )
				if (0 != sp[1])
				{
					memset((void *)(sp[2]), (int)sp[0], sp[1]);
				}
				sp += 3;
			break;

			case FORTH_TOKEN_MOVE:	// src dest count
				// void *memcpy(void *dest, const void *src, size_t n);
				if (0 != sp[0])
				{
					memmove((char *)(sp[1]), (char *)(sp[2]), sp[0]);
				}
				sp += 3;
			break;

			case FORTH_TOKEN_LATEST:
				tos = (forth_cell_t)(&((struct forth_wordlist *)(&dictionary[rctx->current]))->latest);
				PUSH(tos);
			break;

			case FORTH_TOKEN_pDefining:	// (DEFINING)
				PUSH(&(rctx->defining));
			break;

			case FORTH_TOKEN_TUCK:
				tos = sp[0];
				sp--;
				sp[0] = sp[1];
				sp[1] = sp[2];
				sp[2] = tos;
			break;

			case FORTH_TOKEN_PICK:
				sp[0] = sp[sp[0] + 1];
			break;

			case FORTH_TOKEN_ROLL:
				i = POP();
				if (i)
				{
					tos = sp[i];
					while(i)
					{
						sp[i] = sp[i - 1];
						i--;
					}
					sp[0] = tos;
				}
			break;

			case FORTH_TOKEN_ROT:
				tos = sp[2];
				sp[2] = sp[1];
				sp[1] = sp[0];
				sp[0] = tos;
			break;

			case FORTH_TOKEN_NIP:
				sp[1] = sp[0];
				sp++;
			break;

			case FORTH_TOKEN_OVER:
				--sp;
				sp[0] = sp[2];
			break;

			case FORTH_TOKEN_TYPE:
				tos = POP();
				if (0 > rctx->write_string(rctx, (char *)(POP()), tos))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_EXECUTE:
				xt = POP();
			continue;

			case FORTH_TOKEN_SEARCH_WORDLIST: // SEARCH-WORDLIST
				// static forth_cell_t forth_search_wordlist(forth_cell_t dictionary[], const struct forth_wordlist *wl, const char *name, forth_cell_t len)
				tos = forth_search_wordlist(dictionary, (const struct forth_wordlist *)(&dictionary[sp[0]]), (const char *)(sp[2]), sp[1]);
				sp += 3;

				if (FORTH_TRUE == tos)	// All bits '1'-s.
				{
					PUSH(0);
				}
				else
				{
					PUSH(tos + (sizeof(struct forth_header) / sizeof(forth_cell_t)));

					if (((struct forth_header *)(&dictionary[tos]))->flags & FORTH_HEADER_FLAGS_IMMEDIATE)
					{
						PUSH(1);
					}
					else
					{
						PUSH(-1);
					}
					
				}
			break;

			case FORTH_TOKEN_COMPARE:
				sp[3] = forth_compare_strings((const char *)(sp[3]), sp[2], (const char *)(sp[1]), sp[0]);
				sp += 3;
			break;

			case FORTH_TOKEN_FIND_WORD:	// FIND-WORD ( caddr count -- 0 | xt 1 | xt -1 )
				tos = POP();
				tos = forth_find_word(rctx, dictionary, (const char *)(POP()), tos);
				if (FORTH_TRUE == tos)	// All bits '1'-s.
				{
					PUSH(0);
				}
				else
				{
					PUSH(tos + (sizeof(struct forth_header) / sizeof(forth_cell_t)));

					if (((struct forth_header *)(&dictionary[tos]))->flags & FORTH_HEADER_FLAGS_IMMEDIATE)
					{
						PUSH(1);
					}
					else
					{
						PUSH(-1);
					}
					
				}
			break;

			case FORTH_TOKEN_CFetch:				// C@
				*sp = *((char *)(*sp));
			break;

			case FORTH_TOKEN_CStore:				// c!
				*((char *)(*sp)) = (char)(sp[1]);
				sp+= 2;
			break;

			case FORTH_TOKEN_PlusStore:				// +!
				tos = POP();
				*((forth_cell_t *)tos) += POP();
			break;

			case FORTH_TOKEN_Fetch:					// @
				*sp = *((forth_cell_t *)(*sp));
			break;

			case FORTH_TOKEN_Store:					// !
				*((forth_cell_t *)(*sp)) = sp[1];
				sp+= 2;
			break;

			case FORTH_TOKEN_PARSE:
				tos = POP();
				rctx->sp = sp;
				forth_parse(rctx, tos);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_PARSE_WORD:
				rctx->sp = sp;
				forth_parse_word(rctx, FORTH_CHAR_SPACE);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_WORD:
				tos = POP();
				rctx->sp = sp;
				forth_parse_word(rctx, (char)tos);
				sp = rctx->sp;
				if (sp[0] > 255)
				{
					THROW(-18);
				}
				// tos = FORTH_ALIGN(pctx->dp) + 4 * sizeof(forth_cell_t);
				tos = FORTH_ALIGN(dictionary[FORTH_DP_LOCATION]) + 4 * sizeof(forth_cell_t) + (forth_cell_t)(dictionary);
				*((unsigned char *)tos) = (unsigned char)(sp[0]);
				memmove((void *)(tos + 1), (void *)(sp[1]), sp[0]);
				sp++;
				sp[0] = tos;
			break;

			case FORTH_TOKEN_Plus:
				tos = POP();
				*sp += tos;
			break;

			case FORTH_TOKEN_toR:
				tos = POP();
				RPUSH(tos);
			break;

			case FORTH_TOKEN_Rfrom:
				tos = RPOP();
				PUSH(tos);
			break;

			case FORTH_TOKEN_Rfetch:
				tos = *rp;
				PUSH(tos);
			break;

			case FORTH_TOKEN_2toR:
				RPUSH(sp[1]);
				RPUSH(sp[0]);
				sp += 2;
			break;

			case FORTH_TOKEN_2Rfrom:
				sp -= 2;
				sp[0] = RPOP();
				sp[1] = RPOP();
			break;

			case FORTH_TOKEN_2Rfetch:
				sp -= 2;
				sp[0] = rp[0];	// Check!
				sp[1] = rp[1];
			break;

			case FORTH_TOKEN_PROCESS_NUMBER:
				tos = POP();
				rctx->sp = sp + 1;
				tos = (forth_cell_t)forth_process_number(rctx, (char *)(*sp), tos);
				sp = rctx->sp;
				if ((forth_scell_t)tos < 0)
				{
					THROW(-24);
				}
			break;

#if !defined(FORTH_NO_DOUBLES)
			case FORTH_TOKEN_toNUMBER:	// >NUMBER
				dtos = FORTH_DCELL(sp[2], sp[3]);

				while(0 != sp[0])
				{
					tos = map_digit(*((char *)(sp[1])));

					if (tos >= rctx->base)
					{
						break;
					}

					dtos = (dtos * rctx->base) + tos;
					sp[0]--;
					sp[1]++;
				}
				sp[3] = FORTH_CELL_LOW(dtos);
				sp[2] = FORTH_CELL_HIGH(dtos);
			break;
#endif

			case FORTH_TOKEN_BLK:		// BLK
				PUSH(&(rctx->blk));
			break;

			case FORTH_TOKEN_TIB:		// TIB
				PUSH(&(rctx->tib));
			
			break;

			case FORTH_TOKEN_HashTIB:	// #TIB
				PUSH(&(rctx->tib_count));
			break;

			case FORTH_TOKEN_BASE:
				PUSH(&(rctx->base));
			break;
	
			case FORTH_TOKEN_pSOURCE_ID:	// (SOURCE-ID)
				PUSH(&(rctx->source_id));
			break;
	
			case FORTH_TOKEN_SOURCE:
				PUSH(rctx->source_address);
				PUSH(rctx->source_length);
			break;

			case FORTH_TOKEN_SOURCE_Store:	// SOURCE!
				rctx->source_length = POP();
				rctx->source_address = (char *)(POP());
			break;

#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
			case FORTH_TOKEN_LINE_NUMBER:	 // LINE-NUMBER ( line number inside a file).
				PUSH(&(rctx->line_no));
			break;
#endif
			case FORTH_TOKEN_toIN:	// >IN
				PUSH(&(rctx->to_in));
			break;

			case FORTH_TOKEN_SAVE_INPUT:	// SAVE-INPUT
				if (0 != rctx->blk)
				{
					THROW(-21);	// We currently do not implement blocks, so this should never happen.
				}
				else if ((0 == rctx->source_id) || (-1 == rctx->source_id))
				{
					PUSH(rctx->to_in);
					PUSH(1);
				}
				else
				{
					PUSH(rctx->to_in);
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
					PUSH(rctx->line_no);
					PUSH(FORTH_CELL_LOW(rctx->source_file_position));
					PUSH(FORTH_CELL_HIGH(rctx->source_file_position));
					PUSH(4);
#else
					PUSH(1);
#endif
				}
			break;

			case FORTH_TOKEN_RESTORE_INPUT: // RESTORE-INPUT
				// puts("------ RESTORE-INPUT ---------");
				if (0 != rctx->blk)
				{
					THROW(-21);	// We currently do not implement blocks, so this should never happen.
				}
				else if ( ((0 == rctx->source_id) || (-1 == rctx->source_id)) && (1 == sp[0]))	// Terminal and EVALUTATE only needs >IN restored.
				{
					rctx->to_in = sp[1];
					sp++;
					sp[0] = 0;
					
				}
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
				else if (((0 != rctx->source_id) && (-1 != rctx->source_id)) && (4 == sp[0]))	// Files have postion line_no and >IN
				{
					sp[0] = rctx->source_id;
					rctx->sp = sp;
					forth_reposition_file(rctx);
					sp = rctx->sp;
					tos = POP();

					if (0 != tos)	// Can't reposition -- RESTORE-INPUT has failed.
					{
						sp[0] = FORTH_TRUE;
					}
					else
					{
						rctx->sp = sp;
						forth_refill_file(rctx);	// Bring in the correct line and set >IN = 0.
						sp = rctx->sp;
						rctx->line_no = sp[1];	// Restore line_no to its saved state.
						rctx->to_in = sp[2];	// Restore >IN to its saved value.
						sp[2] = sp[0] ? 0 : FORTH_TRUE;
						sp += 2;
					}
				}
#endif
				else	// Don't know what is going on, failed.
				{
					sp += sp[0];
					sp[0] = FORTH_TRUE;
				}

			break;


			case FORTH_TOKEN_STATE:
				PUSH(&(rctx->state));
			break;

			case FORTH_TOKEN_QUERY:
				tos = forth_query(rctx);

				if (0 > (forth_scell_t)tos)
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_REFILL:
				if (0 == rctx->source_id)
				{
					tos = forth_query(rctx);

					if (0 > (forth_scell_t)tos)
					{
						PUSH(0);
					}
					else
					{
						PUSH(FORTH_TRUE);
					}
				}
				else if (-1 == rctx->source_id)
				{
					PUSH(0);
				}
				else
				{
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
					rctx->sp = sp;
					forth_refill_file(rctx);
					sp = rctx->sp;
					if (0 != sp[0])
					{
						rctx->line_no++;
					}
#else
					PUSH(0);
#endif
				}
			break;

			case FORTH_TOKEN_SWAP:
				// printf("sp=%p\n", sp); fflush(stdout);
				tos = sp[0];
				sp[0] = sp[1];
				sp[1] = tos;
			break;

			case FORTH_TOKEN_THROW:
				tos = *sp;

				if (tos)
				{
					if (0 == rctx->handler)
					{
						rctx->sp = sp;
						rctx->rp = rp;
						rctx->ip = ip;
						return -1;
					}

					rp = (forth_cell_t *)(rctx->handler);
					rctx->handler = RPOP();
					sp = (forth_cell_t *)(RPOP());
					*sp = tos;
					ip = RPOP();
				} 
				else
				{
					sp++;
				}
			break;

			case FORTH_TOKEN_DotError:
				tos = POP();
				rctx->sp = sp;
				forth_print_error(rctx, (forth_scell_t)tos);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_BYE:
				rctx->sp = sp;
				rctx->rp = rp;
				rctx->ip = ip;
				return 0;
			break;

			case FORTH_TOKEN_WORDS:
				if (0 > forth_words(dictionary, rctx))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_ENVIRONMENTq:	// ENVIRONMENT?
				rctx->sp = sp;
				forth_query_environment(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_pSEE:	// (SEE) ( xt -- )
				if (0 > forth_see(rctx, dictionary, POP()))
				{
					THROW(-57);
				}
			break;

			case FORTH_TOKEN_ONLY:
				rctx->wordlists[rctx->wordlist_slots - 1] = FORTH_WID_Root_WORDLIST;
				rctx->wordlists[rctx->wordlist_slots - 2] = FORTH_WID_Root_WORDLIST;
				rctx->wordlist_cnt = 2;
			break;
			
			case FORTH_TOKEN_ALSO:
				if (rctx->wordlist_cnt == rctx->wordlist_slots)
				{
					THROW(-49);
				}

				rctx->wordlist_cnt++;
				rctx->wordlists[rctx->wordlist_slots - rctx->wordlist_cnt] = rctx->wordlists[(rctx->wordlist_slots - rctx->wordlist_cnt) + 1];
			break;

			case FORTH_TOKEN_GET_ORDER:
				tos = rctx->wordlist_cnt;

				for(i = 1; i <= tos; i++)
				{
					PUSH(rctx->wordlists[rctx->wordlist_slots - i]);
				}

				PUSH(rctx->wordlist_cnt);
			break;

			case FORTH_TOKEN_SET_ORDER:
				tos = POP();

				if (-1 == (forth_scell_t)tos)
				{
					xt = FORTH_TOKEN_ONLY;
					continue;
				}

				if (tos > rctx->wordlist_slots)
				{
					THROW(-49);
				}

				rctx->wordlist_cnt = tos;

				for(i = tos; i >= 1; i--)
				{
					rctx->wordlists[rctx->wordlist_slots - i] = POP();
				}
			break;

			case FORTH_TOKEN_CURRENT:
				PUSH(&(rctx->current));
			break;

			case FORTH_TOKEN_CONTEXT:
					PUSH(&(rctx->wordlists[rctx->wordlist_slots - rctx->wordlist_cnt]));
			break;

			case FORTH_TOKEN_Less:		// <
				tos = POP();
				sp[0] = (((forth_scell_t)(sp[0])) < ((forth_scell_t)tos)) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_Greater:	// >
				tos = POP();
				sp[0] = (((forth_scell_t)(sp[0])) > ((forth_scell_t)tos)) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_ULess:		// u<
				tos = POP();
				sp[0] = (sp[0] < tos) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_UGreater:	// u>
				tos = POP();
				sp[0] = (sp[0] > tos) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_Equal:		// =
				tos = POP();
				sp[0] = (sp[0] == tos) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_Notequal:	// <>
				tos = POP();
				sp[0] = (sp[0] != tos) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_0Equal:
				sp[0] = (0 == sp[0]) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_0Notequal:
				sp[0] = (0 != sp[0]) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_0Less:		// 0<
				sp[0] = (((forth_scell_t)(sp[0])) < 0) ? FORTH_TRUE : FORTH_FALSE;
			break;

			case FORTH_TOKEN_0Greater:	// 0>
				sp[0] = (((forth_scell_t)(sp[0])) > 0) ? FORTH_TRUE : FORTH_FALSE;
			break;


			case FORTH_TOKEN_LITERAL:
				tos = POP();
				// This check is for worst case.
				if (dictionary[FORTH_DP_MAX_LOCATION] <=  (dictionary[FORTH_DP_LOCATION] + (2 * sizeof(forth_cell_t))))
				{
					THROW(-8);
				}
				tos = forth_literal((forth_cell_t *)(dictionary[FORTH_DP_LOCATION] + (forth_cell_t)(dictionary)), tos);
				tos = tos * sizeof(forth_cell_t);
				dictionary[FORTH_DP_LOCATION] += tos;
			break;

			case FORTH_TOKEN_sp0:
				PUSH(rctx->sp0);
			break;

			case FORTH_TOKEN_sp_fetch:
				tos = (forth_cell_t)sp;
				PUSH(tos);
			break;

			case FORTH_TOKEN_sp_store:
				tos = POP();
				sp = (forth_cell_t *)tos;
			break;

			case FORTH_TOKEN_rp0:
				PUSH(rctx->rp0);
			break;

			case FORTH_TOKEN_rp_fetch:
				PUSH(rp);
			break;

			case FORTH_TOKEN_rp_store:
				rp = (forth_cell_t *)(POP());
			break;

			case FORTH_TOKEN_abort_msg:		// (abort-msg) -- to hold the string for ABORT".
				PUSH(&(rctx->abort_msg_len));
			break;

			// This is not in any standard. Intended to be an implementation detail to resolve all branches.
			case FORTH_TOKEN_resolve_branch:	// resolve-branch ( orig-sys dest -- )
				tos = sp[1];
				tos &= FORTH_SYS_ID_MASK;
				// printf("Before: dictionary[tos = %08x] = %08x\n", tos, dictionary[tos]);
				// dictionary[tos] |=  FORTH_PARAM_PACK((((pctx->dp - (forth_cell_t)pctx->dictionary) / sizeof(forth_cell_t)) - (tos + 1)));
				dictionary[tos] |=  FORTH_PARAM_PACK(((sp[0] & FORTH_SYS_ID_MASK) - (tos + 1)));
				// printf("After:  dictionary[tos = %08x] = %08x\n", tos, dictionary[tos]);
				sp += 2;
			break;

			case FORTH_TOKEN_ix2address:	// IX>ADDRESS
				*sp = (forth_cell_t)&(dictionary[*sp]);
			break;

			case FORTH_TOKEN_toBODY:	// >BODY
				tos = *sp;
				if (FORTH_PACK_TOKEN(FORTH_TOKEN_docreate) != dictionary[tos])
				{
					THROW(-31);
				}

				*sp = (forth_cell_t)(&dictionary[tos + 2]);
			break;
/*
// For DO-LOOPs
#define LOOP_I rp[0]
#define LOOP_J rp[3]
#define LOOP_LIMIT   rp[1]
#define LOOP_ADDRESS_AFTER rp[2]
#define LOOP_ADJUSTMENT 3
*/
			case FORTH_TOKEN_pDO:		// (DO)
				tos = ip + SIGNED_PARAMETER;
				rp -= LOOP_ADJUSTMENT;
				LOOP_I = POP();
				LOOP_LIMIT = POP();
				LOOP_ADDRESS_AFTER = tos;
			break;

			case FORTH_TOKEN_pqDO:	 	// (?DO)
				tos = ip + SIGNED_PARAMETER;

				if (sp[0] != sp[1])
				{
					rp -= LOOP_ADJUSTMENT;
					LOOP_I = POP();
					LOOP_LIMIT = POP();
					LOOP_ADDRESS_AFTER = tos;
				}
				else
				{
					sp += 2;
					ip = tos;
				}
			// printf("ip=%08x\n", ip);
			break;

			case FORTH_TOKEN_UNLOOP:	// UNLOOP
				rp += LOOP_ADJUSTMENT;
			break;

			case FORTH_TOKEN_LEAVE:		// LEAVE
				ip = LOOP_ADDRESS_AFTER;
				rp += LOOP_ADJUSTMENT;
			break;

			case FORTH_TOKEN_I:		// I
				PUSH(LOOP_I);
			break;

			case FORTH_TOKEN_J:		// J
				PUSH(LOOP_J);
			break;

			case FORTH_TOKEN_pLOOP:		// (LOOP)
				LOOP_I += 1;

				if (LOOP_I == LOOP_LIMIT)
				{
					rp += LOOP_ADJUSTMENT;
				}
				else
				{
					ip += SIGNED_PARAMETER;
				}
			break;

			case FORTH_TOKEN_pPlusLOOP:	// (+LOOP)
				tos = POP();

				LOOP_I += tos;

				if (0 > (forth_scell_t)((LOOP_I - LOOP_LIMIT) ^ tos))	// Some 2's complement's trickery.
				{
					ip += SIGNED_PARAMETER;
				}
				else
				{
					rp += LOOP_ADJUSTMENT;
				}
			break;



			case FORTH_TOKEN_branch:
				ip += SIGNED_PARAMETER;
			break;

			case FORTH_TOKEN_0branch:
				if (0 == POP())
				{
					ip += SIGNED_PARAMETER;
				}
			break;

			case FORTH_TOKEN_xtlit:			// eXecution Token literal.
			case FORTH_TOKEN_lit:			// Full sized literal.
				PUSH(dictionary[ip++]);
			break;

			case FORTH_TOKEN_sslit:			// Litaral encoded in the token, signed.
				PUSH(SIGNED_PARAMETER);
			break;

			case FORTH_TOKEN_uslit:			// Literal encoded in the token, unsigned.
				PUSH(UNSIGNED_PARAMETER);
			break;

			case FORTH_TOKEN_strlit:		// String literal, length encoded in the token.
				PUSH(&dictionary[ip]);
				tos = UNSIGNED_PARAMETER;
				PUSH(tos);
				ip += (tos + (sizeof(forth_cell_t) - 1)) / sizeof(forth_cell_t);
			break;

			case FORTH_TOKEN_nest:
				RPUSH(ip);
				ip = w + 1;
			break;

			case FORTH_TOKEN_EXIT:	// Same as unnest
			case FORTH_TOKEN_unnest:
				ip = RPOP();
			break;

#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
			case FORTH_TOKEN_FILE_CREATE:		// CREATE-FILE
				rctx->sp = sp;
				forth_create_file(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_OPEN:		// OPEN-FILE
				rctx->sp = sp;
				forth_open_file(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_FLUSH:		// FLUSH-FILE
				rctx->sp = sp;
				forth_file_flush(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_DELETE:		// DELETE-FILE
				rctx->sp = sp;
				forth_delete_file(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_REPOSITION: 	// REPOSITION-FILE
				rctx->sp = sp;
				forth_reposition_file(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_POSITION: 	// FILE-POSITION
				rctx->sp = sp;
				forth_file_position(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_SIZE:		// FILE-SIZE
				rctx->sp = sp;
				forth_file_size(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_READ:		// READ-FILE
				rctx->sp = sp;
				forth_read_file(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_READ_LINE:	// READ-LINE
				rctx->sp = sp;
				forth_read_line(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_WRITE:		// WRITE-FILE
				rctx->sp = sp;
				forth_write_file(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FILE_WRITE_LINE:	// WRITE-LINE
				rctx->sp = sp;
				forth_write_line(rctx);
				sp = rctx->sp;
			break;


			case FORTH_TOKEN_FILE_CLOSE:		// CLOSE-FILE
				rctx->sp = sp;
				forth_close_file(rctx);
				sp = rctx->sp;
			break;

#endif

#if defined(FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS)
			case FORTH_TOKEN_ALLOCATE:
				rctx->sp = sp;
				forth_allocate(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_RESIZE:
				rctx->sp = sp;
				forth_resize(rctx);
				sp = rctx->sp;
			break;

			case FORTH_TOKEN_FREE:
				rctx->sp = sp;
				forth_free(rctx);
				sp = rctx->sp;
			break;

#endif
			case FORTH_TOKEN_dovar:
				PUSH(&dictionary[w + 1]);
			break;

			case FORTH_TOKEN_doconst:
				PUSH(dictionary[w + 1]);
			break;

#if defined(FORTH_EXTERNAL_PRIMITIVES)
			case FORTH_TOKEN_doextern:
				if (0 == rctx->external_primitive_table)
				{
					THROW(-21);
				}

				ep = rctx->external_primitive_table[dictionary[w + 1]];

				if (0 == ep)
				{
					THROW(-21);
				}

				rctx->sp = sp;
				tos = ep(rctx);
				sp = rctx->sp;

				if (0 != tos)
				{
					THROW(tos);
				}
			break;
#endif

#if defined(FORTH_USER_VARIABLES)
			case FORTH_TOKEN_USER_ALLOT:	// USER-ALLOT ( n -- ix )
				tos = sp[0];

				if ((dictionary[FORTH_UP_LOCATION] + tos) >= dictionary[FORTH_MAX_UP_LOCATION])
				{
					THROW(-8);
				}

				sp[0] = dictionary[FORTH_UP_LOCATION];
				dictionary[FORTH_UP_LOCATION] += tos;
			break;

			case FORTH_TOKEN_douser:
				PUSH(&(rctx->user[UNSIGNED_PARAMETER]));
			break;
#endif

			case FORTH_TOKEN_docreate:
				PUSH(&dictionary[w + 2]);
				xt = dictionary[w + 1];
			continue;

			default:
				forth_type0(rctx, "Unknown token: ");
				forth_hdot(rctx, xt);
				rctx->send_cr(rctx);
				return -1;
			break;
		}

		w = ip++;
		// printf("w=0x%08x\n", w); fflush(stdout);
		xt = dictionary[w];
	}

}

// -----------------------------------------------------------------------------

