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

#include <stdio.h>
#include <string.h>
#include "forth_config.h"
#include "forth_features.h"
#include "forth.h"
#include "forth_internal.h"


const char *autoHeader ="// Auto generated file.\n// Do NOT edit.\n//To make changes edit gen_dict.c\n\n";

uint32_t ip = 0;
uint32_t latest = 0;

#if defined(FORTH_32BIT)
#define CELL_FORMAT "0x%08x"
#endif

uint16_t gen_str(FILE* f, const char *str, size_t len)
{
	size_t l;
	forth_cell_t chunk;
	size_t byte_index;
#if defined(FORTH_32BIT)
	size_t bytes_per_cell = 4;
#endif

	if ('\\' != str[len - 1])
	{
		fprintf(f, "// %s\n", str);
	}

	for (l = 0; l < len; l++)
	{
		byte_index = (l % bytes_per_cell);

		if (0 == byte_index)
		{
			if (0 != l)
			{
				fprintf(f, "  " CELL_FORMAT ",\n", chunk);
				ip++;
			}
			chunk = str[l];
		}
		else
		{
			chunk |= ((forth_cell_t)str[l]) << (8 * byte_index);
		}
		
	}

//	if (0 != (len % bytes_per_cell))
	{
			fprintf(f, "  " CELL_FORMAT ",\n", chunk);
			ip++;
	}
}

void gen_entry(FILE* f, const char *name, uint32_t flags)
{
	uint16_t len = strlen(name);

	gen_str(f, name, len);

	fprintf(f, "  " CELL_FORMAT ",\t// link\n", latest );
	latest = ip;
	ip++;

	// fprintf(f, "  " CELL_FORMAT ",\t// flags\n", len | (((uint32_t)flags) << 16) );
	fprintf(f, "  " CELL_FORMAT ",\t// flags\n", len | flags);
	ip++;
}

void output_cell(FILE *f, const char *data)
{
	fprintf(f, "\t %s\t,\t// 0x%08x\n", data, ip);
	ip++;
}

void output_token(FILE *f, const char *token)
{
	fprintf(f, "\t FORTH_PACK_TOKEN( %s)\t,\t// 0x%08x\n", token, ip);
	ip++;
}

forth_cell_t stack[256];

forth_cell_t *sp = &stack[255];

#define PUSH(X) *--sp = (X)
#define POP()   *sp++

forth_cell_t target_ix = 0;

static const char *forth_label(forth_cell_t ix)
{
	static char label_buffer[64];
	sprintf(label_buffer, "FORTH_internal_label_%08x", ix);
	return label_buffer;
}

void If(FILE *fc, FILE *fh) // -- Orig
{
	char if_buffer[256];
	PUSH(target_ix);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_0branch) | FORTH_PARAM_PACK(%s - " CELL_FORMAT "),\n", forth_label(target_ix++), ip + 1);
	ip++;
}

void Then(FILE *fc, FILE *fh) // Orig --
{
	fprintf(fh, "#define %s\t" CELL_FORMAT " // THEN\n", forth_label(POP()), ip);
}


void Else(FILE *fc, FILE *fh) // Orig1 -- Orig2
{
	forth_cell_t target = target_ix;
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_branch) | FORTH_PARAM_PACK(%s - " CELL_FORMAT "),\n", forth_label(target_ix++), ip + 1);
	ip++;
	Then(fc, fh);
	PUSH(target);
}

void Begin(FILE *fc, FILE *fh) // -- Dest
{
	fprintf(fc, "//\tBEGIN\n");
	fprintf(fh, "#define %s\t" CELL_FORMAT " // BEGIN\n", forth_label(target_ix), ip);
	PUSH(target_ix++);
}

void While(FILE *fc, FILE *fh) // Dest -- Orig Dest
{
	forth_cell_t tmp = POP();
	fprintf(fc, "//\tWHILE\n");
	If(fc, fh);
	PUSH(tmp);
}

void Until(FILE *fc, FILE *fh) // Dest --
{
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_0branch) | FORTH_PARAM_PACK(%s - " CELL_FORMAT "),\n", forth_label(POP()), ip + 1);
	ip++;
	fprintf(fc, "//\tUNTIL\n");
}

void Again(FILE *fc, FILE *fh) // Dest --
{
	fprintf(fh, "// AGAIN\n");
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_branch) | FORTH_PARAM_PACK(%s - " CELL_FORMAT "),\n", forth_label(POP()), ip + 1);
	ip++;
	fprintf(fc, "//\tAGAIN\n");
}

void Repeat(FILE *fc, FILE *fh) // Orig Dest -- 
{
	Again(fc, fh);
	Then(fc, fh);
}

void Lit(FILE *fc, FILE *fh, forth_cell_t value)
{
	char buffer[256];
	forth_cell_t packed = FORTH_PARAM_PACK(value);
	if (FORTH_PARAM_EXTRACT(packed) == value)
	{
		fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_uslit) |  " CELL_FORMAT ",\n", packed);
		ip++;
	}
	else
	{
		output_token(fc, "FORTH_TOKEN_lit");
		fprintf(fc, "\t" CELL_FORMAT ",\n", value);
		ip++;
	}

	fprintf(fc, "//\tLITERAL value %u %d " CELL_FORMAT "\n", value, value, value);
}

void StrLit(FILE *fc, FILE *fh, char *str)
{
	uint16_t len = strlen(str);
	forth_cell_t packed = FORTH_PARAM_PACK(len);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_strlit) |  " CELL_FORMAT ",\n", packed);
	ip++;
	gen_str(fc, str, len);
}

int main()
{
	FILE *fc;
	FILE *fh;
	FILE *fih;
	fc = fopen("forth_dict.c","w");
	fh = fopen("forth_dict.h", "w");
	fih = fh;
	fputs(autoHeader, fc);
	fputs("#ifndef FORTH_DICT_H\n#define FORTH_DICT_H\n", fh);
	fputs(autoHeader, fh);

	fputs("#include \"forth_config.h\"\n", fh);
	fputs("#include \"forth.h\"\n\n", fh);
	fputs("extern forth_cell_t dictionary[FORTH_DICTIONARY_SIZE];\n\n", fh);	

	fputs("#include \"forth_features.h\"\n\n",fh);
	fputs("#include \"forth_internal.h\"\n", fh);

	fputs("#include \"forth_dict.h\"\n\n", fc);

	fputs("forth_cell_t dictionary[FORTH_DICTIONARY_SIZE] =\n{\n", fc);	
	output_token(fc, "FORTH_TOKEN_BYE");
	
	fprintf(fh, "#define FORTH_DP_LOCATION\t" CELL_FORMAT "\n", ip);
	output_cell(fc, "((FORTH_DP_VALUE) * sizeof(forth_cell_t))");		// BYTES
	fprintf(fh, "#define FORTH_DP_MAX_LOCATION\t" CELL_FORMAT "\n", ip);
	output_cell(fc, "((FORTH_DICTIONARY_SIZE) * sizeof(forth_cell_t))");	// BYTES
// -----------------------------------------------------------------------------------
	fprintf(fh, "#define FORTH_WID_Root_WORDLIST\t" CELL_FORMAT "\n", ip);
	output_cell(fc, "FORTH_Root_LATEST_VALUE");	// LATEST
	output_cell(fc, "0");				// PARENT
	output_cell(fc, "0");				// LINK

	gen_entry(fc, "ROOT-WORDLIST", 0);
	fprintf(fh, "#define FORTH_XT_Root_WORDLIST\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_doconst");
	output_cell(fc, "FORTH_WID_Root_WORDLIST");

	gen_entry(fc, "ONLY", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ONLY");

	gen_entry(fc, "ALSO", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ALSO");

	gen_entry(fc, "GET-ORDER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_GET_ORDER");

	gen_entry(fc, "SET-ORDER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_SET_ORDER");

	gen_entry(fc, "PREVIOUS", 0);			// : PREVIOUS ( -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_GET_ORDER");	// GET-ORDER
	output_token(fc, "FORTH_TOKEN_DUP");		// DUP
	Lit(fc, fih, 1);				// 1
	output_token(fc, "FORTH_TOKEN_Greater");	// >
	If(fc, fih);					// IF
		output_token(fc, "FORTH_TOKEN_NIP");	//	NIP
		output_cell(fc, "FORTH_XT_1_Minus");	//	1-
		output_token(fc, "FORTH_TOKEN_SET_ORDER"); // 	SET-ORDER
	Else(fc, fih);					// ELSE
		Begin(fc, fih);				//	BEGIN
			output_token(fc, "FORTH_TOKEN_qDUP"); //	?DUP
		While(fc, fih);				//	WHILE
			output_token(fc, "FORTH_TOKEN_NIP"); //		NIP
			output_cell(fc, "FORTH_XT_1_Minus"); //		1-
		Repeat(fc, fih);			//	REPEAT	
	Then(fc, fih);					// THEN
	output_token(fc, "FORTH_TOKEN_unnest");		// ;

	fprintf(fh, "#define FORTH_WID_FORTH_WORDLIST\t" CELL_FORMAT "\n", ip);
	output_cell(fc, "FORTH_LATEST_VALUE");		// LATEST
	output_cell(fc, "FORTH_WID_Root_WORDLIST");	// PARENT
	output_cell(fc, "FORTH_WID_Root_WORDLIST");	// LINK

	gen_entry(fc, "FORTH-WORDLIST", 0);
	fprintf(fh, "#define FORTH_XT_FORTH_WORDLIST\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_doconst");
	output_cell(fc, "FORTH_WID_FORTH_WORDLIST");

	gen_entry(fc, "FORTH", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_FORTH_WORDLIST");
	output_token(fc, "FORTH_TOKEN_CONTEXT");
	output_token(fc, "FORTH_TOKEN_Store");
	output_token(fc, "FORTH_TOKEN_unnest");
// ------------------------------------------------------------------
	gen_entry(fc, "ORDER", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_GET_ORDER");
	Begin(fc, fih);
		output_token(fc, "FORTH_TOKEN_DUP");
	While(fc, fih);
		output_token(fc, "FORTH_TOKEN_SWAP");
		output_token(fc, "FORTH_TOKEN_DUP");
		output_cell(fc, "FORTH_XT_FORTH_WORDLIST");
		output_token(fc, "FORTH_TOKEN_Equal");
		If(fc, fih);
			output_token(fc, "FORTH_TOKEN_DROP");
			StrLit(fc, fih, "FORTH ");
			output_token(fc, "FORTH_TOKEN_TYPE");
		Else(fc, fih);
			output_token(fc, "FORTH_TOKEN_DUP");
			output_cell(fc, "FORTH_XT_Root_WORDLIST");
			output_token(fc, "FORTH_TOKEN_Equal");
			If(fc, fih);
				output_token(fc, "FORTH_TOKEN_DROP");
				StrLit(fc, fih, "Root ");
				output_token(fc, "FORTH_TOKEN_TYPE");
			Else(fc, fih);
				Lit(fc, fih, '#');
				output_token(fc, "FORTH_TOKEN_EMIT");
				output_token(fc, "FORTH_TOKEN_Hdot");
			Then(fc, fih);
		Then(fc, fih);
		Lit(fc, fih, -1);
		output_token(fc, "FORTH_TOKEN_Plus");
	Repeat(fc, fih);
		output_token(fc, "FORTH_TOKEN_DROP");
		output_token(fc, "FORTH_TOKEN_CR");
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "WORDS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_WORDS");

	fprintf(fh, "#define FORTH_Root_LATEST_VALUE\t" CELL_FORMAT "\n", latest);
	latest = 0;

	gen_entry(fc, "FORTH-ENGINE-VERSION", 0);
	output_token(fc, "FORTH_TOKEN_doconst");
	output_cell(fc, "FORTH_ENGINE_VERSION");
	
	gen_entry(fc, "WID-LINK", 0);
	fprintf(fh, "#define FORTH_XT_WID_LINK\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_dovar");
	output_cell(fc, "FORTH_WID_FORTH_WORDLIST");

	gen_entry(fc, "CONTEXT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CONTEXT");

	gen_entry(fc, "CURRENT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CURRENT");
// -----------------------------------------------------------------------------------
#if defined(FORTH_USER_VARIABLES)
	gen_entry(fc, "MAX-USER-VARIABLES", 0);			// MAX-USER-VARIABLES ( -- #max-user )
	output_token(fc, "FORTH_TOKEN_doconst");		// CONSTANT
	fprintf(fh, "#define FORTH_MAX_UP_LOCATION\t" CELL_FORMAT "\n", ip);
	output_cell(fc, "FORTH_USER_VARIABLES");		// CELLS
	
	gen_entry(fc, "USER-VARIABLES", 0);			// USER-VARIABLES ( -- #user )
	output_token(fc, "FORTH_TOKEN_doconst");		// CONSTANT
	fprintf(fh, "#define FORTH_UP_LOCATION\t" CELL_FORMAT "\n", ip);
	output_cell(fc, "0");					// CELLS
#endif
// -----------------------------------------------------------------------------------
	gen_entry(fc, "FIND-WORD", FORTH_HEADER_FLAGS_TOKEN);		// ( caddr len -- 0 | xt 1 | xt -1 )
	output_token(fc, "FORTH_TOKEN_FIND_WORD");

	gen_entry(fc, "SEARCH-WORDLIST", FORTH_HEADER_FLAGS_TOKEN);	// ( caddr len wid -- 0 | xt 1 | xt -1 )
	output_token(fc, "FORTH_TOKEN_SEARCH_WORDLIST");


	gen_entry(fc, "DEFINITIONS", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_CONTEXT");	// CONTEXT
	output_token(fc, "FORTH_TOKEN_Fetch");		// @
	output_token(fc, "FORTH_TOKEN_CURRENT");	// CURRENT
	output_token(fc, "FORTH_TOKEN_Store");		// !
	output_token(fc, "FORTH_TOKEN_unnest");		// ;
	
	gen_entry(fc, "GET-CURRENT", 0);		// : GET-CURRENT ( -- wid )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_CURRENT");	// CURRENT
	output_token(fc, "FORTH_TOKEN_Fetch");		// @
	output_token(fc, "FORTH_TOKEN_unnest");		// ;

	gen_entry(fc, "SET-CURRENT", 0);		// : SET-CURRENT ( wid -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_CURRENT");	// CURRENT
	output_token(fc, "FORTH_TOKEN_Store");		// !
	output_token(fc, "FORTH_TOKEN_unnest");		// ;

	gen_entry(fc, "WORDLIST", 0);			// : WORDLIST ( -- wid )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_ALIGN");		// ALIGN
	output_token(fc, "FORTH_TOKEN_pHERE");		// (HERE) -- wid
	Lit(fc, fih, 0);				// 0
	output_token(fc, "FORTH_TOKEN_Comma");		// , -- latest
	output_token(fc, "FORTH_TOKEN_CURRENT");	// CURRENT
	output_token(fc, "FORTH_TOKEN_Fetch");		// @
	output_token(fc, "FORTH_TOKEN_Comma");		// , -- parent
	output_cell(fc,  "FORTH_XT_WID_LINK");		// WID-LINK
	output_token(fc, "FORTH_TOKEN_Fetch");		// @
	output_token(fc, "FORTH_TOKEN_Comma");		// , -- link
	output_token(fc, "FORTH_TOKEN_DUP");		// DUP
	output_cell(fc,  "FORTH_XT_WID_LINK");		// WID-LINK
	output_token(fc, "FORTH_TOKEN_Store");		// !
	output_token(fc, "FORTH_TOKEN_unnest");		// ;
	
	gen_entry(fc, "1", FORTH_HEADER_FLAGS_TOKEN);
	Lit(fc, fih, 1);

	gen_entry(fc, "0", FORTH_HEADER_FLAGS_TOKEN);
	Lit(fc, fih, 0);

	gen_entry(fc, "FALSE", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_uslit) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(0));
	ip++;

	gen_entry(fc, "TRUE", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_sslit) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(-1));
	ip++;

	gen_entry(fc, "CELL+", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fh, "#define FORTH_XT_CELL_Plus\t" CELL_FORMAT "\n", ip);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_Imm_Plus) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(sizeof(forth_cell_t)));
	ip++;

	gen_entry(fc, "1+", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fh, "#define FORTH_XT_1_Plus\t" CELL_FORMAT "\n", ip);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_Imm_Plus) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(1));
	ip++;

	gen_entry(fc, "1-", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fh, "#define FORTH_XT_1_Minus\t" CELL_FORMAT "\n", ip);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_Imm_Plus) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(-1));
	ip++;

	gen_entry(fc, "CHAR+", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fh, "#define FORTH_XT_CHAR_Plus\t" CELL_FORMAT "\n", ip);
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_Imm_Plus) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(sizeof(char)));
	ip++;

	gen_entry(fc, "CHARS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_NOP");

// __________________________________________________________________
	gen_entry(fc, "NOOP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_NOP");

	gen_entry(fc, "ACCEPT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ACCEPT");

	gen_entry(fc, "KEY", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_KEY");

	gen_entry(fc, "KEY?", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_KEYq");

	gen_entry(fc, "EKEY", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_EKEY");

	gen_entry(fc, "EKEY?", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_EKEYq");

	gen_entry(fc, "EKEY>CHAR", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_EKEY2CHAR");

	gen_entry(fc, "ALIGN", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ALIGN");

	gen_entry(fc, "ALIGNED", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ALIGNED");

	gen_entry(fc, "ALLOT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ALLOT");

	gen_entry(fc, "CELLS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CELLS");

	gen_entry(fc, "2!", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2Store");

	gen_entry(fc, "2@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2Fetch");

	gen_entry(fc, "2SWAP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2SWAP");

	gen_entry(fc, "2OVER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2OVER");

	gen_entry(fc, "2DROP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2DROP");

	gen_entry(fc, "2DUP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2DUP");

	gen_entry(fc, "2ROT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2ROT");

	gen_entry(fc, "DROP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DROP");

	gen_entry(fc, "?DUP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_qDUP");

	gen_entry(fc, "DUP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DUP");

	gen_entry(fc, "PICK", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PICK");

	gen_entry(fc, "ROLL", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ROLL");

	gen_entry(fc, "ROT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ROT");

	gen_entry(fc, "TUCK", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_TUCK");

	gen_entry(fc, "NIP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_NIP");

	gen_entry(fc, "OVER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_OVER");

	gen_entry(fc, "CMOVE>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CMOVE_down");

	gen_entry(fc, "CMOVE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CMOVE");

	gen_entry(fc, "MOVE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_MOVE");

	gen_entry(fc, "FILL", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILL");

	gen_entry(fc, "ERASE", 0);				// ERASE ( caddr count -- )
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_FILL");			// FILL
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "UNUSED", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_UNUSED");

	gen_entry(fc, "DUMP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DUMP");

	gen_entry(fc, "CR", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CR");

	gen_entry(fc, "TYPE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_TYPE");

	gen_entry(fc, "EMIT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_EMIT");

	gen_entry(fc, "PAGE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PAGE");

	gen_entry(fc, "AT-XY", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_AT_XY");

#if defined(FORTH_INCLUDE_MS)
	gen_entry(fc, "MS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_MS");
#endif

#if defined(FORTH_INCLUDE_TIME_DATE)
	gen_entry(fc, "TIME&DATE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_TIME_DATE");
#endif

	gen_entry(fc, "EXECUTE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_EXECUTE");

	gen_entry(fc, "H.", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Hdot");

	gen_entry(fc, ".", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Dot");

	gen_entry(fc, ".S", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DotS");

	gen_entry(fc, "U.R", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_UdotR");

	gen_entry(fc, ".R", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DotR");

	gen_entry(fc, "U.", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Udot");

	gen_entry(fc, "PARSE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PARSE");

	gen_entry(fc, "PARSE-WORD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PARSE_WORD");

	gen_entry(fc, "WORD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_WORD");

#if !defined(FORTH_NO_DOUBLES)
	gen_entry(fc, "DMAX", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DMAX");

	gen_entry(fc, "DMIN", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DMIN");

	gen_entry(fc, "DABS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DABS");

	gen_entry(fc, "DNEGATE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DNEGATE");

	gen_entry(fc, "DU<", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D_ULess");

	gen_entry(fc, "D<", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D_Less");

	gen_entry(fc, "D-", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D_Subtract");

	gen_entry(fc, "D+", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D_Plus");

	gen_entry(fc, "D2/", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D2_Slash");

	gen_entry(fc, "D2*", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D2_Star");
#endif

	gen_entry(fc, "D=", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_D_Equal");

	gen_entry(fc, "2/", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2_Slash");

	gen_entry(fc, "2*", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2_Star");

	gen_entry(fc, "RSHIFT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_RSHIFT");

	gen_entry(fc, "LSHIFT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_LSHIFT");

	gen_entry(fc, "MAX", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_MAX");

	gen_entry(fc, "MIN", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_MIN");

	gen_entry(fc, "ABS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ABS");

	gen_entry(fc, "NEGATE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_NEGATE");

	gen_entry(fc, "*", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Multiply");

	gen_entry(fc, "/", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Divide");

	gen_entry(fc, "MOD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_MOD");

#if !defined(FORTH_NO_DOUBLES)
	gen_entry(fc, "UM/MOD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_UM_Slash_MOD");

	gen_entry(fc, "M+", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_M_Plus");

	gen_entry(fc, "M*", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_M_Star");

	gen_entry(fc, "UM*", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_UM_Star");

	gen_entry(fc, "/MOD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Slash_MOD");

	gen_entry(fc, "*/", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_StarSlash");

	gen_entry(fc, "*/MOD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_StarSlash_MOD");
#endif

	gen_entry(fc, "-", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Subtract");

	gen_entry(fc, "+", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Plus");

	gen_entry(fc, "XOR", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_XOR");

	gen_entry(fc, "OR", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_OR");

	gen_entry(fc, "AND", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_AND");

	gen_entry(fc, "INVERT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_INVERT");

	gen_entry(fc, "PROCESS-NUMBER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PROCESS_NUMBER");

#if !defined(FORTH_NO_DOUBLES)
	gen_entry(fc, ">NUMBER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_toNUMBER");
#endif
	gen_entry(fc, "ENVIRONMENT?", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ENVIRONMENTq");

	gen_entry(fc, "RESTORE-INPUT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_RESTORE_INPUT");

	gen_entry(fc, "SAVE-INPUT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_SAVE_INPUT");

	gen_entry(fc, "REFILL", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_REFILL");

	gen_entry(fc, "QUERY", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_QUERY");

	gen_entry(fc, "C!", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CStore");

	gen_entry(fc, "C@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CFetch");

	gen_entry(fc, "!", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Store");

	gen_entry(fc, "+!", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PlusStore");

	gen_entry(fc, "@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Fetch");

	gen_entry(fc, "(SOURCE-ID)", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");

	gen_entry(fc, "SOURCE-ID", FORTH_HEADER_FLAGS_TOKEN);
	fprintf(fh, "#define FORTH_XT_SOURCE_ID\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");	// (SOURCE-ID)
	output_token(fc, "FORTH_TOKEN_Fetch");		// @
	output_token(fc, "FORTH_TOKEN_unnest");		// ;

	gen_entry(fc, "SOURCE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_SOURCE");

	gen_entry(fc, ">IN", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_toIN");

	gen_entry(fc, "#>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_HashGreater");
	gen_entry(fc, "HOLD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_HOLD");

#if !defined(FORTH_NO_DOUBLES)
	gen_entry(fc, "#", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Hash");
#endif

	gen_entry(fc, "<#", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_LessHash");

	gen_entry(fc, "TIB", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_TIB");

	gen_entry(fc, "#TIB", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_HashTIB");

	gen_entry(fc, "BASE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_BASE");

	gen_entry(fc, "BLK", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_BLK");

#if !defined(FORTH_DISABLE_COMPILER)
	gen_entry(fc, "STATE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_STATE");
#endif /* FORTH_DISABLE_COMPILER */	

	gen_entry(fc, "SWAP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_SWAP");

	gen_entry(fc, "THROW", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_THROW");

	gen_entry(fc, ".ERROR", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DotError");

	gen_entry(fc, "BYE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_BYE");

	gen_entry(fc, "(SEE)", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_pSEE");

	gen_entry(fc, "WORDS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_WORDS");

	gen_entry(fc, "HANDLER", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_handler");

	gen_entry(fc, "COMPARE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_COMPARE");

	gen_entry(fc, "0=", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_0Equal");

	gen_entry(fc, "0<", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_0Less");

	gen_entry(fc, "0>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_0Greater");

	gen_entry(fc, "0<>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_0Notequal");

	gen_entry(fc, "=", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Equal");

	gen_entry(fc, "<>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Notequal");

	gen_entry(fc, "<", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Less");

	gen_entry(fc, ">", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Greater");

	gen_entry(fc, "U<", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ULess");

	gen_entry(fc, "U>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_UGreater");

	gen_entry(fc, "NR>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_NRfrom");

	gen_entry(fc, "N>R", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_NtoR");

	gen_entry(fc, "2R@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2Rfetch");

	gen_entry(fc, "2>R", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2toR");

	gen_entry(fc, "2R>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_2Rfrom");

	gen_entry(fc, "R@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Rfetch");

	gen_entry(fc, ">R", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_toR");

	gen_entry(fc, "R>", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Rfrom");

	gen_entry(fc, "SP0", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_sp0");

	gen_entry(fc, "SP@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_sp_fetch");

	gen_entry(fc, "SP!", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_sp_store");

	gen_entry(fc, "RP0", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_rp0");

	gen_entry(fc, "RP@", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_rp_fetch");

	gen_entry(fc, "RP!", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_rp_store");

	gen_entry(fc, "C,", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CComma");

	gen_entry(fc, ",", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_Comma");

	gen_entry(fc, "IX>ADDRESS", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ix2address");

	gen_entry(fc, "PAD", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_PAD");

	gen_entry(fc, "HERE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_HERE");

	gen_entry(fc, "(TRACE)", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_pTRACE");

	gen_entry(fc, ".NAME", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_DotName");


// __________________________________________________________________
	gen_entry(fc, "TRACE-ON", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fh, FORTH_TRUE);				// TRUE
	output_token(fc, "FORTH_TOKEN_pTRACE");			// TRACE
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "TRACE-OFF", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fh, 0);						// 0
	output_token(fc, "FORTH_TOKEN_pTRACE");			// (TRACE)
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

#if !defined(FORTH_DISABLE_COMPILER)
	gen_entry(fc, "CS-PICK", 0);				// No token flag, so it will not become pick when viewed.
	output_token(fc, "FORTH_TOKEN_PICK");

	gen_entry(fc, "CS-ROLL", 0);				// No token flag, so it will not become roll when viewed.
	output_token(fc, "FORTH_TOKEN_ROLL");

	gen_entry(fc, "LITERAL", FORTH_HEADER_FLAGS_TOKEN | FORTH_HEADER_FLAGS_IMMEDIATE);
	output_token(fc, "FORTH_TOKEN_LITERAL");

	gen_entry(fc, "2LITERAL", FORTH_HEADER_FLAGS_IMMEDIATE); // : 2LITERAL
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_LITERAL");		// POSTPONE LITERAL
	output_token(fc, "FORTH_TOKEN_LITERAL");		// POSTPONE LITERAL
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "FIND", 0); // : FIND ( caddr -- caddr 0 | xt -1 | xt 1 )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_cell(fc, "FORTH_XT_COUNT");			// COUNT
	output_token(fc, "FORTH_TOKEN_FIND_WORD");		// FIND-WORD
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER
	If(fc, fih);						// IF
		output_token(fc, "FORTH_TOKEN_DROP");		// 	DROP
	Else(fc, fih);						// ELSE
		output_token(fc, "FORTH_TOKEN_SWAP");		//	SWAP
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "COMPILE,", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_CompileComma");

	gen_entry(fc, "EXIT", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_EXIT");

	gen_entry(fc, "(HERE)", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_pHERE");

	gen_entry(fc, "LATEST", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_LATEST");

	gen_entry(fc, "(DEFINING)", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_pDefining");

	gen_entry(fc, "IMMEDIATE", 0);				// IMMEDIATE
	fprintf(fh, "#define FORTH_XT_IMMEDIATE\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_ix2address");		// IX>ADDRESS
	output_cell(fc, "FORTH_XT_CELL_Plus");			// CELL+
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	Lit(fc, fih, FORTH_HEADER_FLAGS_IMMEDIATE);		// Immagediate flag
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, ";", FORTH_HEADER_FLAGS_IMMEDIATE);	// ;
	fprintf(fh, "#define FORTH_XT_SemiColon\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, FORTH_COLON_SYS);				// colon-sys
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
	Lit(fc, fih, -22);					// -22
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_unnest");			// unnest
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_qDUP");			// ?DUP
	If(fc, fih);						// IF
		output_token(fc, "FORTH_TOKEN_DUP");		// 	DUP
		output_cell(fc, "FORTH_XT_1_Plus");		//	1+ // link -> flags
		output_token(fc, "FORTH_TOKEN_ix2address");	// 	IX>ADDRESS
		output_token(fc, "FORTH_TOKEN_Fetch");		// 	@
		Lit(fc, fih, FORTH_HEADER_FLAGS_NAME_LENGTH_MASK); //	name_length_mask
		output_token(fc, "FORTH_TOKEN_AND");		//	AND
		If(fc, fih);					//	IF
			output_token(fc, "FORTH_TOKEN_LATEST");	// 		LATEST
			output_token(fc, "FORTH_TOKEN_Store");	// 		!
		Else(fc, fih);					//	ELSE
			output_token(fc, "FORTH_TOKEN_DROP");	// 		DROP
		Then(fc, fih);					//	THEN
	Then(fc, fih);						// THEN
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_STATE");			// STATE
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");			// ;


	gen_entry(fc, "]", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, -1);					// -1
	output_token(fc, "FORTH_TOKEN_STATE");
	output_token(fc, "FORTH_TOKEN_Store");
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "ABORT\"", FORTH_HEADER_FLAGS_IMMEDIATE);
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_IF");				// POSTPONE IF
	output_cell(fc, "FORTH_XT_Squot");			// POSTPONE S"
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_abort_msg");		// (abort-msg)
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_2Store");			// 2!
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	Lit(fc, fih, -2);					// -2
	output_token(fc, "FORTH_TOKEN_LITERAL");		// POSTPONE LITERAL
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_cell(fc,  "FORTH_XT_THEN");			// POSTPONE THEN
	output_token(fc, "FORTH_TOKEN_unnest");
	
#endif /* FORTH_DISABLE_COMPILER */	

	gen_entry(fc, "[", FORTH_HEADER_FLAGS_IMMEDIATE);
	fprintf(fh, "#define FORTH_XT_interpreting\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, FORTH_FALSE);
	output_token(fc, "FORTH_TOKEN_STATE");
	output_token(fc, "FORTH_TOKEN_Store");
	output_token(fc, "FORTH_TOKEN_unnest");

#if !defined(FORTH_DISABLE_COMPILER)
	gen_entry(fc, "THEN", FORTH_HEADER_FLAGS_IMMEDIATE);	// THEN ( orig1 -- orig2 )
	fprintf(fh, "#define FORTH_XT_THEN\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	Lit(fc, fih, ~(FORTH_SYS_ID_MASK));			// Orig-sys-ID-mask
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	Lit(fc, fih, FORTH_ORIG_SYS_ID);			// Orig-sys-ID
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S
	Lit(fc, fih, -22);					// -22
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S
	output_token(fc, "FORTH_TOKEN_AND");			// AND
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_unnest");


	gen_entry(fc, "ELSE", FORTH_HEADER_FLAGS_IMMEDIATE);
	fprintf(fh, "#define FORTH_XT_ELSE\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	Lit(fc, fih, FORTH_ORIG_SYS_ID);			// Orig-sys-ID
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_branch");			// BRANCH
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_cell(fc,  "FORTH_XT_THEN");			// POSTPONE THEN
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "IF", FORTH_HEADER_FLAGS_IMMEDIATE);	// IF ( -- orig ) exec: ( flag -- )
	fprintf(fh, "#define FORTH_XT_IF\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	Lit(fc, fih, FORTH_ORIG_SYS_ID);			// Orig-sys-ID
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_0branch");		// 0BRANCH
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "CASE", FORTH_HEADER_FLAGS_IMMEDIATE);	// CASE C:( -- #of-s )
	output_token(fc, "FORTH_TOKEN_doconst");		// CONSTANT
	output_cell(fc, "0");					// 0

	gen_entry(fc, "OF", FORTH_HEADER_FLAGS_IMMEDIATE);	// OF  C:( --  of-sys) R: ( x1 x2 -- |x1 )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_1_Plus");			// 1+	Increment #of-s.
	output_token(fc, "FORTH_TOKEN_toR");			// >R	Shiffle #of-s out of the way for IF.
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']  -- This is POSTPONE OVER
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER R: Copy x1 to top.
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']  -- This is POSTPONE =
	output_token(fc, "FORTH_TOKEN_Equal");			// = R: Compare x1 and x2.
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_cell(fc,  "FORTH_XT_IF");			// POSTPONE IF
	output_token(fc, "FORTH_TOKEN_xtlit");			// ['] -- This is POSTPONE DROP
	output_token(fc, "FORTH_TOKEN_DROP");			// DROP R: Discard x1 if x1 == x2.
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R> Shuffle #of-s back.
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "ENDOF", FORTH_HEADER_FLAGS_IMMEDIATE);	// ENDOF ( orig1 #of-s -- orig2 #of-s )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_toR");			// >R	Shiffle #of-s out of the way for ELSE.
	output_cell(fc,  "FORTH_XT_ELSE");			// POSTPONE ELSE
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R> Shuffle #of-s back.
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "ENDCASE", FORTH_HEADER_FLAGS_IMMEDIATE);	// ENDCASE ( orig1 .. orign #of-s -- ) RT: ( x -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_xtlit");			// ['] -- This is POSTPONE DROP
	output_token(fc, "FORTH_TOKEN_DROP");			// DROP R: Discard x.
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	Begin(fc, fih);						// BEGIN
		output_token(fc, "FORTH_TOKEN_qDUP");		//	?DUP 
	While(fc, fih);						// WHILE	-- Resolve orig-s if #of-s is non-zero.
		output_token(fc, "FORTH_TOKEN_SWAP");		//	SWAP ( orig #of-s -- #of-s orig )
		output_cell(fc,  "FORTH_XT_THEN");		// 	POSTPONE THEN
		output_cell(fc, "FORTH_XT_1_Minus");		// 	1- Decrement #of-s.
	Repeat(fc, fih);					// REPEAT
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
	
	gen_entry(fc, "AHEAD", FORTH_HEADER_FLAGS_IMMEDIATE);	// AHEAD ( -- orig )
	fprintf(fh, "#define FORTH_XT_AHEAD\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	Lit(fc, fih, FORTH_ORIG_SYS_ID);			// Orig-sys-ID
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_branch");			// BRANCH
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "BEGIN", FORTH_HEADER_FLAGS_IMMEDIATE);	// BEGIN ( -- dest )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	Lit(fc, fih, FORTH_DEST_SYS_ID);			// Dest-sys-ID
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "WHILE", FORTH_HEADER_FLAGS_IMMEDIATE);	// WHILE (Dest -- Orig Dest )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_IF");				// POSTPONE IF
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "UNTIL", FORTH_HEADER_FLAGS_IMMEDIATE);	// UNTIL (dest -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	Lit(fc, fih, ~(FORTH_SYS_ID_MASK));			// Sys-ID-mask
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	Lit(fc, fih, FORTH_DEST_SYS_ID);			// Dest-sys-ID
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
	Lit(fc, fih, -22);					// -22
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_0branch");		// 0BRANCH
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "AGAIN", FORTH_HEADER_FLAGS_IMMEDIATE);	// AGAIN (dest -- )
	fprintf(fh, "#define FORTH_XT_AGAIN\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	Lit(fc, fih, ~(FORTH_SYS_ID_MASK));			// Sys-ID-mask
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	Lit(fc, fih, FORTH_DEST_SYS_ID);			// Dest-sys-ID
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
	Lit(fc, fih, -22);					// -22
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_branch");			// BRANCH
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "REPEAT", FORTH_HEADER_FLAGS_IMMEDIATE);	// REPEAT ( Orig Dest -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_AGAIN");			// AGAIN
	output_cell(fc, "FORTH_XT_THEN");			// THEN
	output_token(fc, "FORTH_TOKEN_unnest");


	gen_entry(fc, "+LOOP", FORTH_HEADER_FLAGS_IMMEDIATE);	// +LOOP ( do-sys -- )
	fprintf(fh, "#define FORTH_XT_pLOOP\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	Lit(fc, fih, ~(FORTH_SYS_ID_MASK));			// Sys-ID-mask
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	Lit(fc, fih, FORTH_DO_SYS_ID);				// do-sys-ID
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
	Lit(fc, fih, -22);					// -22
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_pPlusLOOP");		// (+LOOP)
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER
	Lit(fc, fih, 1);					// 1
	output_token(fc, "FORTH_TOKEN_Plus");			// +
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "LOOP", FORTH_HEADER_FLAGS_IMMEDIATE);	// LOOP ( do-sys -- )
	fprintf(fh, "#define FORTH_XT_LOOP\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	Lit(fc, fih, ~(FORTH_SYS_ID_MASK));			// Sys-ID-mask
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	Lit(fc, fih, FORTH_DO_SYS_ID);				// do-sys-ID
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
	Lit(fc, fih, -22);					// -22
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_pLOOP");			// (LOOP)
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER
	Lit(fc, fih, 1);					// 1
	output_token(fc, "FORTH_TOKEN_Plus");			// +
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_resolve_branch");		// resolve-branch
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "I", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_I");

	gen_entry(fc, "J", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_J");

	gen_entry(fc, "LEAVE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_LEAVE");

	gen_entry(fc, "UNLOOP", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_UNLOOP");

	gen_entry(fc, "DO", FORTH_HEADER_FLAGS_IMMEDIATE);	// DO ( -- do-sys )
	fprintf(fh, "#define FORTH_XT_DO\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	Lit(fc, fih, FORTH_DO_SYS_ID);				// do-sys-ID
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_pDO");			// (DO)
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "?DO", FORTH_HEADER_FLAGS_IMMEDIATE);	// ?DO ( -- do-sys )
	fprintf(fh, "#define FORTH_XT_qDO\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	Lit(fc, fih, FORTH_DO_SYS_ID);				// do-sys-ID
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_xtlit");			// xtlit
	output_token(fc, "FORTH_TOKEN_pqDO");			// (?DO)
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");


	gen_entry(fc, "POSTPONE", FORTH_HEADER_FLAGS_IMMEDIATE);	// POPSTPONE ( <<Name>> -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_pTICK");			// (')
	Lit(fc, fih, -1);					// -1	-- Non Immediate word
	output_token(fc, "FORTH_TOKEN_Equal");			// =
	If(fc, fih);						// IF
		output_token(fc, "FORTH_TOKEN_xtlit");		// [']
		output_token(fc, "FORTH_TOKEN_xtlit");		// xtlit
		output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
		output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
		output_token(fc, "FORTH_TOKEN_xtlit");		// [']
		output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
		output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
	Else(fc, fih);						// ELSE		Immediate word
		output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
	Then(fc, fih);						// THEN
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "[CHAR]", FORTH_HEADER_FLAGS_IMMEDIATE);	// [CHAR] ( <<str>> -- ) Exec: ( -- c )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CHAR");			// CHAR
	output_token(fc, "FORTH_TOKEN_LITERAL");		// LITERAL
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "[']", FORTH_HEADER_FLAGS_IMMEDIATE);	// ['] ( --  ) Exec: ( -- xt )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_TICK");			// '
	output_token(fc, "FORTH_TOKEN_xtlit");			// xlit
	output_token(fc, "FORTH_TOKEN_xtlit");			// xlit
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "(", FORTH_HEADER_FLAGS_IMMEDIATE);	// ( <<TEXT>> )
	output_token(fc, "FORTH_TOKEN_nest");
	Begin(fc, fih);						// BEGIN
		Lit(fc, fih, ')');				//	[CHAR] )
		output_token(fc, "FORTH_TOKEN_PARSE");		//	PARSE
		output_token(fc, "FORTH_TOKEN_NIP");		//	NIP
		output_token(fc, "FORTH_TOKEN_0Equal");		//	0=
	While(fc, fih);						// WHILE
		output_cell(fc, "FORTH_XT_SOURCE_ID");		//	SOURCE-ID
		output_token(fc, "FORTH_TOKEN_0Notequal");	//	0<>
		output_cell(fc, "FORTH_XT_SOURCE_ID");		//	SOURCE-ID
		Lit(fc, fih, -1);				//	-1
		output_token(fc, "FORTH_TOKEN_Notequal");	//	<>
		output_token(fc, "FORTH_TOKEN_AND");		//	AND
		If(fc, fih);					//	IF
			output_token(fc, "FORTH_TOKEN_REFILL");	//		REFILL
			output_token(fc, "FORTH_TOKEN_0Equal");	//		0=
			If(fc, fih);				//		IF
				output_token(fc, "FORTH_TOKEN_EXIT");	//		EXIT
			Then(fc, fih);				//		THEN
		Then(fc, fih);					//	THEN
	Repeat(fc, fih);					// REPEAT
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
#endif /* FORTH_DISABLE_COMPILER */	

	gen_entry(fc, "\\", FORTH_HEADER_FLAGS_IMMEDIATE);	// COMMENT \ <<TEXT>>EOL
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_SOURCE");			//
	output_token(fc, "FORTH_TOKEN_toIN");			// >IN
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_DROP");			// DROP
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "[THEN]", FORTH_HEADER_FLAGS_IMMEDIATE);	// [THEN]
	output_token(fc, "FORTH_TOKEN_NOP");

	gen_entry(fc, "[ELSE]", FORTH_HEADER_FLAGS_IMMEDIATE);	// [ELSE]
	fprintf(fh, "#define FORTH_XT_bELSE\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, 1);					// 1
	Begin(fc, fih);						// BEGIN
		Begin(fc, fih);						// BEGIN
			output_token(fc, "FORTH_TOKEN_PARSE_WORD");	//	PARSE-WORD
			output_token(fc, "FORTH_TOKEN_DUP");		//	DUP
		While(fc, fih);						// WHILE
			output_token(fc, "FORTH_TOKEN_2DUP");		//	2DUP
			StrLit(fc, fih, "[IF]");			// S" [IF]"
			output_token(fc, "FORTH_TOKEN_COMPARE");	// COMPARE
			output_token(fc, "FORTH_TOKEN_0Equal");		// 0=
			If(fc, fih);					// IF
				output_token(fc, "FORTH_TOKEN_2DROP");	//	2DROP
				Lit(fc, fih, 1);			//	1
				output_token(fc, "FORTH_TOKEN_Plus");	//	+
			Else(fc, fih);					// ELSE
				output_token(fc, "FORTH_TOKEN_2DUP");		//	2DUP
				StrLit(fc, fih, "[ELSE]");			// 	S" [ELSE]"
				output_token(fc, "FORTH_TOKEN_COMPARE");	// 	COMPARE
				output_token(fc, "FORTH_TOKEN_0Equal");		// 	0=
				If(fc, fih);					// 	IF
					output_token(fc, "FORTH_TOKEN_2DROP");		//	2DROP
					Lit(fc, fih, 1);					//	1
					output_token(fc, "FORTH_TOKEN_Subtract");	//	-
					output_token(fc, "FORTH_TOKEN_DUP");		//	DUP
					If(fc, fih);					// 	IF
						Lit(fc, fih, 1);			//		1
						output_token(fc, "FORTH_TOKEN_Plus");	//		+
					Then(fc, fih);					// 	THEN
				Else(fc, fih);					// 	ELSE
					StrLit(fc, fih, "[THEN]");			// 	S" [THEN]"
					output_token(fc, "FORTH_TOKEN_COMPARE");	// 	COMPARE
					output_token(fc, "FORTH_TOKEN_0Equal");		// 	0=
					If(fc, fih);					// 	IF
						Lit(fc, fih, 1);			//		1
						output_token(fc, "FORTH_TOKEN_Subtract"); //		-
					Then(fc, fih);					// 	THEN
				Then(fc, fih);					// 	THEN
			Then(fc, fih);					// THEN
			output_token(fc, "FORTH_TOKEN_qDUP");		// ?DUP
			output_token(fc, "FORTH_TOKEN_0Equal");		// 0=
			If(fc, fih);					// IF
				output_token(fc, "FORTH_TOKEN_EXIT");	// 	EXIT
			Then(fc, fih);					// THEN
		Repeat(fc, fih);					// REPEAT
		output_token(fc, "FORTH_TOKEN_2DROP");		//	2DROP
		output_token(fc, "FORTH_TOKEN_REFILL");		//	REFILL
		output_token(fc, "FORTH_TOKEN_0Equal");		//	0=
	Until(fc, fih);						// UNTIL
	output_token(fc, "FORTH_TOKEN_DROP");			// DROP
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "[IF]", FORTH_HEADER_FLAGS_IMMEDIATE);	// [IF] ( f -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_0Equal");			// 0=
	If(fc, fih);						// IF
	output_cell(fc, "FORTH_XT_bELSE");			// 	POSTPONE [ELSE]
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

#if !defined(FORTH_DISABLE_COMPILER)
	gen_entry(fc, ":", 0);					// :
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CREATE_NAME_colon");		// CREATE-NAME:
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Store");			// !
	Lit(fc, fih, FORTH_COLON_SYS);				// colon-sys
	Lit(fc, fih, -1);					// -1
	output_token(fc, "FORTH_TOKEN_STATE");			// STATE
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_nest");			// nest
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, ":NONAME", 0);				// :NONAME ( -- xt colon-sys )
	fprintf(fh, "#define FORTH_XT_Colon_NONAME\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_ALIGN");			// ALIGN
	// Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE) 
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Store");			// !
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_Comma");			// ,	Link
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_Comma");			// ,	Flags
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE) Xt
	Lit(fc, fih, FORTH_COLON_SYS);				// colon-sys
	Lit(fc, fih, -1);					// -1
	output_token(fc, "FORTH_TOKEN_STATE");			// STATE
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_nest");			// nest
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	
	gen_entry(fc, "(does>)", 0);				// (does>) ( xt -- ) patches the last definition's DOES> address to point to XT
	fprintf(fh, "#define FORTH_XT_pDOES\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	Lit(fc, fih, 3);					// 3
	output_token(fc, "FORTH_TOKEN_Plus");			// +
	output_token(fc, "FORTH_TOKEN_ix2address");		// IX>ADDRESS
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "DOES>", FORTH_HEADER_FLAGS_IMMEDIATE);	// DOES>	// Review (DEFINING), althought the standard appears to allow this behevior.
	fprintf(fh, "#define FORTH_XT_DOES\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_xtlit");			// xlit
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_NOP");			// NOOP	// Place holder
	output_token(fc, "FORTH_TOKEN_HERE");			// HERE
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_cell(fc,  "FORTH_XT_pDOES");			// (does>)
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_cell(fc, "FORTH_XT_SemiColon");			// POSTPONE ;
	output_cell(fc, "FORTH_XT_Colon_NONAME");		// :NONAME
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "RECURSE", FORTH_HEADER_FLAGS_IMMEDIATE);	// RECURSE ( -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_pDefining");		// (DEFINING)
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	Lit(fc, fih, 2);					// 2
	output_token(fc, "FORTH_TOKEN_Plus");			// +
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "SLITERAL", FORTH_HEADER_FLAGS_IMMEDIATE); // Comp ( c-addr len --  ) ex: ( -- caddr1 len )
	fprintf(fh, "#define FORTH_XT_SLITERAL\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	Lit(fc, fih, FORTH_PARAM_MAX);				// FORTH_PARAM_MAX
	output_token(fc, "FORTH_TOKEN_UGreater");		// U>
	Lit(fc, fih, -18);					// -18 This really is parsed string overflow, but I have no better idea.
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_xtlit");			// xlit
	output_token(fc, "FORTH_TOKEN_strlit");			// strlit
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_HERE");			// HERE
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER
	output_token(fc, "FORTH_TOKEN_ALLOT");			// ALLOT
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_MOVE");			// MOVE
	output_token(fc, "FORTH_TOKEN_ALIGN");			// ALIGN
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
#endif /* FORTH_DISABLE_COMPILER */	

	gen_entry(fc, "COUNT", 0);				// : COUNT ( c-addr1 -- caddr2 count )
	fprintf(fh, "#define FORTH_XT_COUNT\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	fprintf(fc, "FORTH_PACK_TOKEN(FORTH_TOKEN_Imm_Plus) |  " CELL_FORMAT ",\n", FORTH_PARAM_PACK(1));	// +1
	ip++;
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_CFetch");			// C@
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, ".(",  FORTH_HEADER_FLAGS_IMMEDIATE);	// : .(
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, ')');					// CHAR )
	output_token(fc, "FORTH_TOKEN_PARSE");			// PARSE
	output_token(fc, "FORTH_TOKEN_TYPE");			// TYPE
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "\"", 0);					// : " ( <<String>>" -- addr cnt)
	fprintf(fh, "#define FORTH_XT_quot\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, '"');					// CHAR "
	output_token(fc, "FORTH_TOKEN_PARSE");			// PARSE
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "S\"",  FORTH_HEADER_FLAGS_IMMEDIATE);	// S" String"
	fprintf(fh, "#define FORTH_XT_Squot\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_quot");			// "
#if !defined(FORTH_DISABLE_COMPILER)
	output_token(fc, "FORTH_TOKEN_STATE");			// STATE
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	If(fc, fih);						// IF
		output_cell(fc, "FORTH_XT_SLITERAL");		// SLITERAL
	Then(fc, fih);						// THEN
#endif /* FORTH_DISABLE_COMPILER */	
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, ".\"",  FORTH_HEADER_FLAGS_IMMEDIATE);	// ." String"
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_quot");			// "
#if !defined(FORTH_DISABLE_COMPILER)
	output_token(fc, "FORTH_TOKEN_STATE");			// STATE
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	If(fc, fih);						// IF
		output_cell(fc, "FORTH_XT_SLITERAL");		// SLITERAL
		output_token(fc, "FORTH_TOKEN_xtlit");		// [']
		output_token(fc, "FORTH_TOKEN_TYPE");		// TYPE
		output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
	Else(fc, fih);						// ELSE
#endif /* FORTH_DISABLE_COMPILER */	
		output_token(fc, "FORTH_TOKEN_TYPE");		// TYPE -- Implementation choice to make ." " work in interpretation state --
								//	    such use is non-standard though.
#if !defined(FORTH_DISABLE_COMPILER)
	Then(fc, fih);						// THEN
#endif /* FORTH_DISABLE_COMPILER */	
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	
#if !defined(FORTH_DISABLE_COMPILER)
	gen_entry(fc, "CREATE-NAME", 0);			// Create Name ( c-addr len -- ix )
	fprintf(fh, "#define FORTH_XT_CREATE_NAME\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_0Equal");			// 0=
	Lit(fc, fih, -16);					// -16
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW

	output_token(fc, "FORTH_TOKEN_2DUP");			// 2DUP
	output_token(fc, "FORTH_TOKEN_FIND_WORD");		// FIND-WORD
	If(fc, fih);						// IF
		output_token(fc, "FORTH_TOKEN_DROP");		//	DROP
		output_cell(fc, "FORTH_XT_SPACE");		//	SPACE
		output_token(fc, "FORTH_TOKEN_2DUP");		// 	2DUP
		output_token(fc, "FORTH_TOKEN_TYPE");		//	TYPE
		StrLit(fc, fih, " is being redefined.");	//      S" "
		output_token(fc, "FORTH_TOKEN_TYPE");		//	TYPE
		output_token(fc, "FORTH_TOKEN_CR");		//	CR
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_ALIGN");			// ALIGN
	output_token(fc, "FORTH_TOKEN_HERE");			// HERE
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER
	output_token(fc, "FORTH_TOKEN_ALLOT");			// ALLOT
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_MOVE");			// MOVE
	output_token(fc, "FORTH_TOKEN_ALIGN");			// ALIGN
	output_token(fc, "FORTH_TOKEN_pHERE");			// (HERE)
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_Comma");			// ,
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_Comma");			// ,
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "CREATE-NAME:", 0);			// CREATE-NAME: ( -- ix )
	fprintf(fh, "#define FORTH_XT_CREATE_NAME_colon\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_PARSE_WORD");		// PARSE-WORD
	output_cell(fc, "FORTH_XT_CREATE_NAME");		// CREATE-NAME
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, ">BODY", FORTH_HEADER_FLAGS_TOKEN);	// : >BODY ( xt -- body)
	output_token(fc, "FORTH_TOKEN_toBODY");
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_ix2address");		// IX>ADDRESS
	output_cell(fc, "FORTH_XT_CELL_Plus");			// CELL+
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_docreate");		// docreate
	output_token(fc, "FORTH_TOKEN_Notequal");		// <>
	Lit(fc, fih, -31);					// -31
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_cell(fc, "FORTH_XT_CELL_Plus");			// CELL+
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "CREATE", 0);				// CREATE
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CREATE_NAME_colon");		// CREATE-NAME:
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_docreate");
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_NOP");
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");

#if defined(FORTH_USER_VARIABLES)
	gen_entry(fc, "USER", 0);				// USER <<name>>
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CREATE_NAME_colon");		// CREATE-NAME:
	Lit(fc, fih, 1);					// 1
	output_token(fc, "FORTH_TOKEN_USER_ALLOT");		// user-allot
	output_token(fc, "FORTH_TOKEN_xtlit");			// [']
	output_token(fc, "FORTH_TOKEN_douser");			// douser
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
#endif	
	gen_entry(fc, "VARIABLE", 0);				// VARIABLE <<name>>
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CREATE_NAME_colon");		// CREATE-NAME:
	output_token(fc, "FORTH_TOKEN_xtlit");
	output_token(fc, "FORTH_TOKEN_dovar");
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	Lit(fc, fih, 0);
	output_token(fc, "FORTH_TOKEN_Comma");			// ,
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_unnest");
	
	gen_entry(fc, "CONSTANT", 0);				// CONSTANT <<name>> ( val -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CREATE_NAME_colon");		// CREATE-NAME:
	output_token(fc, "FORTH_TOKEN_xtlit");
	output_token(fc, "FORTH_TOKEN_doconst");
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_Comma");			// ,
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "SYNONYM", 0);				// SYNONYM <<name>> ( xt -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_CREATE_NAME_colon");		// CREATE-NAME:
	output_token(fc, "FORTH_TOKEN_LATEST");			// LATEST
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_CompileComma");		// COMPILE,
	output_cell(fc, "FORTH_XT_1_Minus");			// 1-
	output_token(fc, "FORTH_TOKEN_ix2address");		// IX>ADDRESS
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	Lit(fc, fih, FORTH_HEADER_FLAGS_IMMEDIATE);		// FORTH_HEADER_FLAGS_IMMADIATE
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	If(fc, fih);						// IF
	output_cell(fc, "FORTH_XT_IMMEDIATE");			//	IMMEDIATE
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
#endif /* FORTH_DISABLE_COMPILER */	

	gen_entry(fc, "SEE", 0);				// SEE <<name>>
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_TICK");			// '
	output_token(fc, "FORTH_TOKEN_pSEE");			// (SEE)
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "WITHIN", 0);				// : WITHIN ( test low high -- flag )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_OVER");			// OVER
	output_token(fc, "FORTH_TOKEN_Subtract");		// -
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_Subtract");		// -
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_ULess");			// U<
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "D0=", 0);				// : D0= ( d -- flag )
	fprintf(fh, "#define FORTH_XT_D0Equal\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_0Equal");			// 0=
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

#if !defined(FORTH_NO_DOUBLES)
	gen_entry(fc, "D0<", 0);				// : D0< ( d -- flag )
	fprintf(fh, "#define FORTH_XT_D0Less\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_NIP");			// NIP
	output_token(fc, "FORTH_TOKEN_0Less");			// 0<
	output_token(fc, "FORTH_TOKEN_unnest");			// ;


	gen_entry(fc, "#S", 0);					// : #S ( d -- 0 )
	fprintf(fh, "#define FORTH_XT_HashS\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Begin(fc, fih);						// BEGIN
		output_token(fc, "FORTH_TOKEN_Hash");		//	#
		output_token(fc, "FORTH_TOKEN_2DUP");		//	2DUP
		output_cell(fc,  "FORTH_XT_D0Equal");		//	D0=
	Until(fc, fih);						// UNTIL
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	
	gen_entry(fc, "SIGN", 0);				// : SIGN ( n --  )
	fprintf(fh, "#define FORTH_XT_SIGN\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_0Less");			// 0<
	If(fc, fih);						// IF
		Lit(fc, fih, '-');				// 	CHAR -
		output_token(fc, "FORTH_TOKEN_HOLD");		//	HOLD
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "S>D", 0);				// : S>D ( n -- d )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_0Less");			// 0<
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "D>S", 0);				// : D>S ( d -- n )
	output_token(fc, "FORTH_TOKEN_DROP");

	gen_entry(fc, "D.", 0);					// : D. ( d --  )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_DABS");			// DABS
	output_token(fc, "FORTH_TOKEN_LessHash");		// <#
	Lit(fc, fih, FORTH_CHAR_SPACE);				// BL
	output_token(fc, "FORTH_TOKEN_HOLD");			// HOLD
	output_cell(fc,  "FORTH_XT_HashS");			// #S
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_cell(fc, "FORTH_XT_SIGN");			// SIGN
	output_token(fc, "FORTH_TOKEN_HashGreater");		// #>
	output_token(fc, "FORTH_TOKEN_TYPE");			// TYPE
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
#endif
	gen_entry(fc, "DEPTH", 0);				// : DEPTH ( -- n )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_sp_fetch");		// SP@
	output_token(fc, "FORTH_TOKEN_sp0");			// SP0
	output_token(fc, "FORTH_TOKEN_SWAP");			// SWAP
	output_token(fc, "FORTH_TOKEN_Subtract");		// -
	Lit(fc, fih, sizeof(forth_cell_t));			// Size of a cell.
	output_token(fc, "FORTH_TOKEN_Divide");			// /
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "HEX", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, 16);
	output_token(fc, "FORTH_TOKEN_BASE");
	output_token(fc, "FORTH_TOKEN_Store");
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "DECIMAL", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, 10);
	output_token(fc, "FORTH_TOKEN_BASE");
	output_token(fc, "FORTH_TOKEN_Store");
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "BL", 0);
	output_token(fc, "FORTH_TOKEN_doconst");
	output_cell(fc, "FORTH_CHAR_SPACE");

	gen_entry(fc, "SPACE", 0);
	fprintf(fh, "#define FORTH_XT_SPACE\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fh, FORTH_CHAR_SPACE);
	output_token(fc, "FORTH_TOKEN_EMIT");
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "SPACES", 0);
	output_token(fc, "FORTH_TOKEN_nest");
	Begin(fc, fih);						// BEGIN
		output_token(fc, "FORTH_TOKEN_DUP");		// 	DUP
	While(fc, fih);						// WHILE
		output_cell(fc, "FORTH_XT_SPACE");		// 	SPACE
		Lit(fc, fih, 1);				// 	1
		output_token(fc, "FORTH_TOKEN_Subtract");	//	-
	Repeat(fc, fih);					// REPEAT
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
	

	gen_entry(fc, "?", 0);					// : ? ( n -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_Dot");			// .
	output_token(fc, "FORTH_TOKEN_unnest");			// !

	gen_entry(fc, "CHAR", 0);
	fprintf(fh, "#define FORTH_XT_CHAR\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_PARSE_WORD");		// PARSE-WORD
	If(fc, fih);						// IF length <> 0
		output_token(fc, "FORTH_TOKEN_CFetch");		// C@
	Else(fc, fih);						// ELSE
		output_token(fc, "FORTH_TOKEN_DROP");		// DROP
		Lit(fc, fih, 0);				// 0
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_unnest");			// !

	gen_entry(fc, "[DEFINED]", FORTH_HEADER_FLAGS_IMMEDIATE); // [DEFINED] ( <<Name>> -- flag )
	fprintf(fh, "#define FORTH_XT_bDEFINED\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_PARSE_WORD");		// PARSE-WORD
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	If(fc, fih);						// IF
		output_token(fc, "FORTH_TOKEN_FIND_WORD");	// FIND-WORD
		output_token(fc, "FORTH_TOKEN_NIP");		// 	NIP
		output_token(fc, "FORTH_TOKEN_0Notequal");	// 	0<>
	Else(fc, fih);						// ELSE
		output_token(fc, "FORTH_TOKEN_NIP");		// 	NIP
	Then(fc, fih);						// THEN
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "[UNDEFINED]", FORTH_HEADER_FLAGS_IMMEDIATE); // [UNDEFINED] ( <<Name>> -- flag )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_bDEFINED");			// POSTPONE [DEFINED]
	output_token(fc, "FORTH_TOKEN_0Equal");			// 0=
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "(')", 0);				// (') ( <<Name>> -- xt 1|-1 )
	fprintf(fh, "#define FORTH_XT_pTICK\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_PARSE_WORD");		// PARSE-WORD
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_0Equal");			// 0=
	Lit(fc, fih, -16);					// -16
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_FIND_WORD");		// FIND-WORD
	output_token(fc, "FORTH_TOKEN_qDUP");			// ?DUP
	output_token(fc, "FORTH_TOKEN_0Equal");			// 0=
	Lit(fc, fih, -13);					// -13
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "'", 0);					// ' Tick ( <<Name>> -- xt )
	fprintf(fh, "#define FORTH_XT_TICK\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_pTICK");			// (')
	output_token(fc, "FORTH_TOKEN_DROP");			// DROP
	output_token(fc, "FORTH_TOKEN_unnest");

	gen_entry(fc, "CATCH", 0);				// : CATCH
	fprintf(fh, "#define FORTH_XT_CATCH\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
//	output_token(fc, "FORTH_TOKEN_DotS");
	output_token(fc, "FORTH_TOKEN_sp_fetch");		// SP@
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_handler");		// HANDLER
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_rp_fetch");		// RP@
	output_token(fc, "FORTH_TOKEN_handler");		// HANDLER
	output_token(fc, "FORTH_TOKEN_Store");			// !
//	output_token(fc, "FORTH_TOKEN_DotS");
	output_token(fc, "FORTH_TOKEN_EXECUTE");		// EXECUTE
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_handler");		// HANDLER
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_DROP");			// DROP
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_unnest");			// ;
	

	gen_entry(fc, "ABORT", 0);				// : ABORT
	output_token(fc, "FORTH_TOKEN_nest");
	Lit(fc, fih, -1);					// -1
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_nest");			// ;

	gen_entry(fc, "INTERPRET", 0);				// : INTERPET
	fprintf(fh, "#define FORTH_XT_INTERPRET\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	Begin(fc, fih);
#if 1
		output_token(fc, "FORTH_TOKEN_PARSE_WORD");	// PARSE-WORD
#else
		Lit(fc, fih, 0x20);
		output_token(fc, "FORTH_TOKEN_PARSE");
#endif
		output_token(fc, "FORTH_TOKEN_DUP");
	While(fc, fih);
		// output_token(fc, "FORTH_TOKEN_DotS");
		output_token(fc, "FORTH_TOKEN_2toR");		// 2>R
//		output_token(fc, "FORTH_TOKEN_2Rfetch");
//		output_token(fc, "FORTH_TOKEN_TYPE");
		output_token(fc, "FORTH_TOKEN_2Rfetch");	// 2R@
		output_token(fc, "FORTH_TOKEN_FIND_WORD");	// FIND-WORD
//		output_token(fc, "FORTH_TOKEN_CR");
//		output_token(fc, "FORTH_TOKEN_DotS");
//		output_token(fc, "FORTH_TOKEN_CR");
		output_token(fc, "FORTH_TOKEN_DUP");		// DUP
		If(fc, fih);
			output_token(fc, "FORTH_TOKEN_STATE");	// STATE
			output_token(fc, "FORTH_TOKEN_Fetch");	// @
			output_token(fc, "FORTH_TOKEN_XOR");	// XOR
			If(fc, fih);
				output_token(fc, "FORTH_TOKEN_EXECUTE");	// EXECUTE
			Else(fc, fih);
				output_token(fc, "FORTH_TOKEN_CompileComma");	// COMPILE,
			Then(fc, fh);
		Else(fc, fih);
			output_token(fc, "FORTH_TOKEN_DROP");			// DROP
			output_token(fc, "FORTH_TOKEN_2Rfetch");		// 2R@
			output_token(fc, "FORTH_TOKEN_xtlit");			// -- [']
			output_token(fc, "FORTH_TOKEN_PROCESS_NUMBER");		// PROCESS-NUMBER
			output_cell(fc, "FORTH_XT_CATCH");			// CATCH
			If(fc, fih);
				output_token(fc, "FORTH_TOKEN_CR");
				output_token(fc, "FORTH_TOKEN_2Rfetch");		// 2R>
				output_token(fc, "FORTH_TOKEN_TYPE");			// TYPE
				output_cell(fc, "FORTH_XT_SPACE");			// SPACE
				Lit(fc, fih, -13);					// -13
				output_token(fc, "FORTH_TOKEN_THROW");			// THROW
			Then(fc, fih);
			output_token(fc, "FORTH_TOKEN_STATE");				// STATE
			output_token(fc, "FORTH_TOKEN_Fetch");				// @
			If(fc, fih);
				If(fc, fih);
					output_token(fc, "FORTH_TOKEN_SWAP");		// SWAP
					output_token(fc, "FORTH_TOKEN_LITERAL");	// LITERAL
				Then(fc, fih);
				output_token(fc, "FORTH_TOKEN_LITERAL");		// LITERAL
			Else(fc, fih);
				output_token(fc, "FORTH_TOKEN_DROP");			// DROP
			Then(fc, fih);
		Then(fc, fih);
		output_token(fc, "FORTH_TOKEN_2Rfrom");				// 2R>
		output_token(fc, "FORTH_TOKEN_DROP");				// DROP
		output_token(fc, "FORTH_TOKEN_DROP");				// DROP
	Repeat(fc, fih);
 	output_token(fc, "FORTH_TOKEN_DROP");					// DROP
	output_token(fc, "FORTH_TOKEN_DROP");					// DROP
	output_token(fc, "FORTH_TOKEN_unnest");					// ;

	gen_entry(fc, "EVALUATE", 0);						// : EVALUATE ( caddr cnt -- ??? )
	output_token(fc, "FORTH_TOKEN_nest");
	output_cell(fc, "FORTH_XT_SOURCE_ID");					// SOURCE-ID
	output_token(fc, "FORTH_TOKEN_toR");					// >R
	output_token(fc, "FORTH_TOKEN_SOURCE");					// SOURCE
	output_token(fc, "FORTH_TOKEN_2toR");					// 2>R
	output_token(fc, "FORTH_TOKEN_toIN");					// >IN
	output_token(fc, "FORTH_TOKEN_toR");					// >R
	output_token(fc, "FORTH_TOKEN_SOURCE_Store");				// SOURCE!
	output_token(fc, "FORTH_TOKEN_BLK");					// BLK
	output_token(fc, "FORTH_TOKEN_Fetch");					// @
	output_token(fc, "FORTH_TOKEN_toR");					// >R
	Lit(fc, fih, 0);							// 0
	output_token(fc, "FORTH_TOKEN_BLK");					// BLK
	output_token(fc, "FORTH_TOKEN_Store");					// !
	Lit(fc, fih, 0);							// 0
	output_token(fc, "FORTH_TOKEN_toIN");					// >IN	
	output_token(fc, "FORTH_TOKEN_Store");					// !
	Lit(fc, fih, -1);							// -1
	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");				// (SOURCE-ID)
	output_token(fc, "FORTH_TOKEN_Store");					// !
	output_token(fc, "FORTH_TOKEN_xtlit");					// [']
	output_cell(fc, "FORTH_XT_INTERPRET");					// INTERPRET
	output_cell(fc, "FORTH_XT_CATCH");					// CATCH
	output_token(fc, "FORTH_TOKEN_Rfrom");					// R>
	output_token(fc, "FORTH_TOKEN_BLK");					// BLK
	output_token(fc, "FORTH_TOKEN_Store");					// !
	output_token(fc, "FORTH_TOKEN_Rfrom");					// R>
	output_token(fc, "FORTH_TOKEN_toIN");					// >IN	
	output_token(fc, "FORTH_TOKEN_Store");					// !
	output_token(fc, "FORTH_TOKEN_2Rfrom");					// 2R>
	output_token(fc, "FORTH_TOKEN_SOURCE_Store");				// SOURCE!
	output_token(fc, "FORTH_TOKEN_Rfrom");					// R>
	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");				// (SOURCE-ID)
	output_token(fc, "FORTH_TOKEN_Store");					// !
	output_token(fc, "FORTH_TOKEN_THROW");					// THROW
	output_token(fc, "FORTH_TOKEN_unnest");					// ;

#if 0
	gen_entry(fc, "QUIT", 0);						// : QUIT
	fprintf(fh, "#define FORTH_XT_QUIT\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_rp0");					// RP0
	output_token(fc, "FORTH_TOKEN_rp_store");				// RP!
	Lit(fc, fih, 0);							// 0
	output_token(fc, "FORTH_TOKEN_handler");				// HANDLER
	output_token(fc, "FORTH_TOKEN_Store");					// !
	output_cell(fc, "FORTH_XT_interpreting");				// [
	Begin(fc, fih);								// BEGIN
		StrLit(fc, fih, " OK");						// 	S"  OK"
		output_token(fc, "FORTH_TOKEN_TYPE");				// 	TYPE
		output_token(fc, "FORTH_TOKEN_CR");				// 	CR
		output_token(fc, "FORTH_TOKEN_REFILL");				// 	REFILL
		If(fc, fih);							//	IF
			output_token(fc, "FORTH_TOKEN_xtlit");			// 		[']
			output_cell(fc, "FORTH_XT_INTERPRET");			// 		INTERPRET
			output_cell(fc, "FORTH_XT_CATCH");			// 		CATCH
			output_token(fc, "FORTH_TOKEN_qDUP");			// 		?DUP
			If(fc, fih);						//		IF
				output_token(fc, "FORTH_TOKEN_DotError");	// 			.ERROR
				output_token(fc, "FORTH_TOKEN_CR");		// 			CR
				output_token(fc, "FORTH_TOKEN_sp0");		// 			SP0
				output_token(fc, "FORTH_TOKEN_sp_store");	// 			SP!
				output_cell(fc, "FORTH_XT_QUIT");		// 			QUIT
			Then(fc, fih);						//		THEN
		Else(fc, fih);							//	ELSE
			output_cell(fc, "FORTH_XT_QUIT");			// 		QUIT -- ??????
		Then(fc, fih);							//	THEN
	Again(fc, fih);								// AGAIN
	output_token(fc, "FORTH_TOKEN_unnest");					// ;
#else
	gen_entry(fc, "QUIT", 0);						// : QUIT
	fprintf(fh, "#define FORTH_XT_QUIT\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_rp0");					// RP0
	output_token(fc, "FORTH_TOKEN_rp_store");				// RP!
	Lit(fc, fih, 0);							// 0
	output_token(fc, "FORTH_TOKEN_handler");				// HANDLER
	output_token(fc, "FORTH_TOKEN_Store");					// !
	output_cell(fc, "FORTH_XT_interpreting");				// [
	Begin(fc, fih);								// BEGIN
		output_token(fc, "FORTH_TOKEN_STATE");				// 	STATE
		output_token(fc, "FORTH_TOKEN_Fetch");				// 	@
		output_token(fc, "FORTH_TOKEN_0Equal");				//	0=
		If(fc, fih);							// 	IF
			StrLit(fc, fih, " OK");					// 		S"  OK"
			output_token(fc, "FORTH_TOKEN_TYPE");			// 		TYPE
			output_token(fc, "FORTH_TOKEN_CR");			// 		CR
		Then(fc, fih);							// 	THEN
		output_token(fc, "FORTH_TOKEN_REFILL");				// 	REFILL
	While(fc, fih);								// WHILE
		output_token(fc, "FORTH_TOKEN_xtlit");				// 	[']
		output_cell(fc, "FORTH_XT_INTERPRET");				// 	INTERPRET
		output_cell(fc, "FORTH_XT_CATCH");				// 	CATCH
		output_token(fc, "FORTH_TOKEN_qDUP");				// 	?DUP
		If(fc, fih);							//	IF
			output_token(fc, "FORTH_TOKEN_DotError");		// 		.ERROR
			output_token(fc, "FORTH_TOKEN_CR");			// 		CR
			output_token(fc, "FORTH_TOKEN_sp0");			// 		SP0
			output_token(fc, "FORTH_TOKEN_sp_store");		// 		SP!
			output_cell(fc, "FORTH_XT_QUIT");			// 		QUIT
		Then(fc, fih);							//	THEN
	Repeat(fc, fih);							// REPEAT
//	output_cell(fc, "FORTH_XT_QUIT");					// QUIT 
	output_token(fc, "FORTH_TOKEN_BYE");					// BYE
	output_token(fc, "FORTH_TOKEN_unnest");					// ;

#endif	
// -----------------------------------------------------------------------------------
#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)
	gen_entry(fc, "R/O", 0);				// R/O	( -- fam )
	output_token(fc, "FORTH_TOKEN_doconst");		// doconst
	output_cell(fc, "FORTH_FAM_READ");			// R/O fam

	gen_entry(fc, "W/O", 0);				// W/O ( -- fam )
	output_token(fc, "FORTH_TOKEN_doconst");		// doconst
	output_cell(fc, "FORTH_FAM_WRITE");			// W/O fam

	gen_entry(fc, "R/W", 0);				// R/W ( -- fam )
	output_token(fc, "FORTH_TOKEN_doconst");		// doconst
	output_cell(fc, "((FORTH_FAM_READ) | (FORTH_FAM_WRITE))");		// R/W fam

	gen_entry(fc, "BIN", 0);				// BIN ( fam1 -- fam2 )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_lit");			// lit
	output_cell(fc, "FORTH_FAM_BINARY");			// BIN flag in fam.
	output_token(fc, "FORTH_TOKEN_OR");			// OR
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "INCLUDE-FILE", 0);			// : INCLUDE-FILE ( fid -- )
	fprintf(fh, "#define FORTH_XT_INCLUDE_FILE\t" CELL_FORMAT "\n", ip);
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_DUP");			// DUP
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_SAVE_INPUT");		// SAVE-INPUT
	output_token(fc, "FORTH_TOKEN_NtoR");			// N>R
	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");		// (SOURCE_ID)
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	output_token(fc, "FORTH_TOKEN_BLK");			// BLK
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_BLK");			// BLK
	output_token(fc, "FORTH_TOKEN_Store");			// !

	output_token(fc, "FORTH_TOKEN_LINE_NUMBER");		// LINE-NUMBER
	output_token(fc, "FORTH_TOKEN_Fetch");			// @
	output_token(fc, "FORTH_TOKEN_toR");			// >R
	Lit(fc, fih, 0);					// 0
	output_token(fc, "FORTH_TOKEN_LINE_NUMBER");		// LINE-NUMBER
	output_token(fc, "FORTH_TOKEN_Store");			// !

	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");		// (SOURCE_ID)
	output_token(fc, "FORTH_TOKEN_Store");			// !
/*
	Begin(fc, fih);						// BEGIN
		output_token(fc, "FORTH_TOKEN_REFILL");		//	REFILL
	While(fc, fih);						// WHILE
		output_cell(fc, "FORTH_XT_INTERPRET");		// 	INTERPRET
	Repeat(fc, fih);					// REPEAT
*/
	Begin(fc, fih);						// BEGIN
		output_token(fc, "FORTH_TOKEN_REFILL");		//	REFILL
//		output_token(fc, "FORTH_TOKEN_DotS");		// 	.S
		If(fc, fih);					//	IF
//			output_token(fc, "FORTH_TOKEN_DotS");	// 		.S
			output_token(fc,"FORTH_TOKEN_xtlit");	//		[']
			output_cell(fc, "FORTH_XT_INTERPRET");	// 		INTERPRET
			output_cell(fc, "FORTH_XT_CATCH");	//		CATCH

			output_token(fc,"FORTH_TOKEN_DUP");	//		DUP
			If(fc, fih);				//		IF
				output_token(fc, "FORTH_TOKEN_DUP");	//		DUP
				output_token(fc, "FORTH_TOKEN_DotError"); //		.ERROR
			Then(fc, fih);				//		THEN

			output_token(fc,"FORTH_TOKEN_qDUP");	//		?DUP
		Else(fc, fih);					//	ELSE
			Lit(fc, fih, 0);			//		0
			Lit(fc, fih, FORTH_TRUE);		//		TRUE
		Then(fc, fih);					//	THEN
	Until(fc, fih);						// UNTIL
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S

	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_LINE_NUMBER");		// LINE-NUMBER
	output_token(fc, "FORTH_TOKEN_Store");			// !

	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_BLK");			// BLK
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_pSOURCE_ID");		// (SOURCE_ID)
	output_token(fc, "FORTH_TOKEN_Store");			// !
	output_token(fc, "FORTH_TOKEN_NRfrom");			// NR>
	// output_token(fc, "FORTH_TOKEN_DotS");			// .S
	output_token(fc, "FORTH_TOKEN_RESTORE_INPUT");		// RESTORE-INPUT
	// output_token(fc, "FORTH_TOKEN_DotS");			// .S
	Lit(fc, fih, -1);					// This is ABORT -- need to invent a better one.
	output_token(fc, "FORTH_TOKEN_AND");			// AND
	output_token(fc, "FORTH_TOKEN_Rfrom");			// R>
	output_token(fc, "FORTH_TOKEN_FILE_CLOSE");		// CLOSE-FILE
//	output_token(fc, "FORTH_TOKEN_DotS");			// .S
	output_token(fc, "FORTH_TOKEN_ROT");			// ROT
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_THROW");			// THROW
	output_token(fc, "FORTH_TOKEN_unnest");			// ;

	gen_entry(fc, "CLOSE-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_CLOSE");

	gen_entry(fc, "OPEN-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_OPEN");

	gen_entry(fc, "CREATE-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_CREATE");

	gen_entry(fc, "INCLUDED", 0);			// : INCLUDED ( caddr len -- )
	output_token(fc, "FORTH_TOKEN_nest");
	output_token(fc, "FORTH_TOKEN_lit");		// Lit
	output_cell(fc, "FORTH_FAM_READ");		// R/O fam
	output_token(fc, "FORTH_TOKEN_FILE_OPEN");	// OPEN-FILE
	output_token(fc, "FORTH_TOKEN_THROW");		// THROW
	output_cell(fc, "FORTH_XT_INCLUDE_FILE");		// INCLUDE-FILE
	output_token(fc, "FORTH_TOKEN_unnest");		// ;

	gen_entry(fc, "FLUSH-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_FLUSH");

	gen_entry(fc, "DELETE-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_DELETE");

	gen_entry(fc, "REPOSITION-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_REPOSITION");

	gen_entry(fc, "FILE-POSITION", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_POSITION");

	gen_entry(fc, "FILE-SIZE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_SIZE");

	gen_entry(fc, "READ-LINE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_READ_LINE");

	gen_entry(fc, "READ-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_READ");

	gen_entry(fc, "WRITE-LINE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_WRITE_LINE");

	gen_entry(fc, "WRITE-FILE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FILE_WRITE");
#endif

#if defined(FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS)
	gen_entry(fc, "ALLOCATE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_ALLOCATE");

	gen_entry(fc, "RESIZE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_RESIZE");

	gen_entry(fc, "FREE", FORTH_HEADER_FLAGS_TOKEN);
	output_token(fc, "FORTH_TOKEN_FREE");

#endif
// -----------------------------------------------------------------------------------

	fputs("};\n",fc);

	fprintf(fh, "#define FORTH_DP_VALUE\t%u\n", ip);
	fprintf(fh, "#define FORTH_LATEST_VALUE\t" CELL_FORMAT "\n", latest);
	fputs("#endif\n", fh);
	fclose(fc);
	fclose(fh);
}

