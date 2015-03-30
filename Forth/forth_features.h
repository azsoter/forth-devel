#ifndef FORTH_FEATURES_H
#define FORTH_FEATURES_H
// Feature definitions for the Embeddable Forth Command Interpreter.

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


#define TIB_SIZE 256
#define FORTH_DICTIONARY_SIZE 4096 /* CELLs. */

#define FORTH_USER_VARIABLES 256 // Number of CELL sized user variables allowed.

//#undef FORTH_INCLUDE_FILE_ACCESS_WORDS 
#define FORTH_INCLUDE_FILE_ACCESS_WORDS 1

// #undef FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS 
#define FORTH_INCLUDE_MEMORY_ALLOCATION_WORDS 1

#if defined(FORTH_INCLUDE_FILE_ACCESS_WORDS)

#	define FORTH_FILE_INPUT_BUFFER_LENGTH 256

#	if 0 && defined(__GNUC__)
#		define FORTH_ALLOCATE_CNAME(PTR, LEN) (strndupa)((PTR), (LEN))
#		define FORTH_FREE_CNAME(X)
#	else
#		define FORTH_ALLOCATE_CNAME(PTR, LEN) (strndup)((PTR), (LEN))
#		define FORTH_FREE_CNAME(X) free((X))
#	endif
#endif

#undef FORTH_DISABLE_COMPILER
// #define FORTH_DISABLE_COMPILER 1

// To disable *ALL* double number and mixed arithmetic, no D+ -es, M+ -es or */-s, not even #.
#undef FORTH_NO_DOUBLES
// #define FORTH_NO_DOUBLES 1

#define FORTH_INCLUDE_MS 1
#define FORTH_INCLUDE_TIME_DATE 1

// #undef FORTH_ALLOW_0X_HEX 
#define FORTH_ALLOW_0X_HEX 1	/* Allow C-style hex numbers starting with 0x. */

// External primitives implemented as separate C function in the application -- not as tokens in the Forth core.
// #undef FORTH_EXTERNAL_PRIMITIVES
#define FORTH_EXTERNAL_PRIMITIVES 1

#define FORTH_STACK_CHECK_ENABLED

#undef FORTH_APPLICATION_DEFINED_CONTEXT_FIELDS 

#endif

