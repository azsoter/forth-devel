#ifndef FORTH_INTERFACE_H
#define FORTH_INTERFACE_H
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
#define FORTH_POP(RCTX) *((RCTX)->sp++)
#define FORTH_PUSH(RCTX, X) *(--((RCTX)->sp)) = (forth_cell_t)(X)

#if defined(FORTH_EXTERNAL_PRIMITIVES)
/*
* External primitives are C functions of the following prototype:
* forth_cell_t func(forth_runtime_context_p rctx);
*
* This is to allow application specific extensions to the Forth system without editing core files in the Forth engine and more importantly without
* the need to regenerate the entire dictionary which can be impractical if e.g. working inside a target compiler IDE since compiling and running
* gen_dict needs a host compiler.
*
* An external primitive's return value is fed to THROW,
* i.e. it should be 0 on success and some meaningful exception code if there was an error.
*
* The parameter is the address of the run time context of the calling Forth system, Forth's stacks can be accessed through it.
*
* The external primitives must be listed in an array and the address of the array must be stored in the external_primitive_table field
* of the run time time context (see forth.h).
* This allows the Forth system to call the external primitives by index from an already compiled program.
* This allows reloading a compiled dictionary into an application even after the application has been recompiled and addresses change
* since external primitives are not referred to by address inside the dictionary.
*
* In order to access external primitives from Forth they must be registered. That is a Forth word must be created whose name is the Forth
* name for the external primitive. The other parameter of the registration function is the index of the primitive in the table.
*
* This is a 'compile time' operation, i.e. it has to be done once when the Forth program is compiled / system is started up.
*
* If the dictionary is backed up and then reloaded at a later time only the table has to be provided in the external_primitive_table field
* of the run time context, but the existing external primitives should not be re-registered.
*
* Also the registration is stored inside the dictionary, so if multiple threads of Forth are running e.g. on top of an RTOS 
* the registration, just like any other compilation, should only be run on one of them.
*
* However the run time context of all threads must have the external_primitive_table field set properly.
*
* Note: The previous statement does allow threads to use different implementations for the same primitive -- i.e. all threads need not
* necessarily use the same external_primitive_table so long as the functions listed at the same index have compatible semantics on each thread.
* This allows some creativity on the application programmer's behalf, but as with any powerful tool due caution must be exercised.
*/
extern forth_cell_t forth_register_external_primitive(struct forth_runtime_context *rctx, const char *name, forth_cell_t index);
#endif
#endif

