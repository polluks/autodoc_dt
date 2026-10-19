/*
    compilerspecific.h - Compiler specific definitions.

    Verbatim from the MorphOS ascii.datatype source release
    (www.morphos-team.net/files/src/3.20/ascii.datatype.tar.bz2), which
    carries it forward from the AROS Development Team's libdefs heritage.
    Do not edit - it tracks the ascii.datatype release.
*/

#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H

#undef SAVEDS
#undef ASM
#undef STDARGS

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifdef _AROS

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#define SAVEDS
#define ASM
#define STDARGS

#define getreg(x) 0
#define putreg(a,b)

#else

#ifdef __MORPHOS__

#define SAVEDS
#define ASM
#define STDARGS
#define register
#define __a6
#else

#define SAVEDS  __saveds
#define ASM     __asm
#define STDARGS __stdargs

typedef unsigned long IPTR;
#endif

#if !defined(_DOS_H) && defined(__SASC)
#include <dos.h>
#endif

#endif

#endif /* COMPILERSPECIFIC_H */
