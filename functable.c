/*
    functable.c - Function table for autodoc.datatype
    Copyright (c) 2026 Amiga Developer
*/
#ifndef LIBCORE_COMPILER_H
#include <exec/libraries.h>
#include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

#include "libdefs.h"

extern void AROS_SLIB_ENTRY(open, BASENAME) (void);
extern void AROS_SLIB_ENTRY(close, BASENAME) (void);
extern void AROS_SLIB_ENTRY(expunge, BASENAME) (void);
extern void AROS_SLIB_ENTRY(null, BASENAME) (void);
extern void AROS_SLIB_ENTRY(ObtainEngine, BASENAME) (void);

void *const LIBFUNCTABLE[] =
{
    (void *const) FUNCARRAY_32BIT_NATIVE,
    AROS_SLIB_ENTRY(open, BASENAME),
    AROS_SLIB_ENTRY(close, BASENAME),
    AROS_SLIB_ENTRY(expunge, BASENAME),
    AROS_SLIB_ENTRY(null, BASENAME),
    AROS_SLIB_ENTRY(ObtainEngine, BASENAME), /* 5 */
    (void *)-1L
};