/*
    obtainengine.c - ObtainEngine function for autodoc.datatype
    Copyright (c) 2026 Amiga Developer
*/
#include <exec/libraries.h>
#include <proto/exec.h>
#include <intuition/classes.h>
#include <aros/libcall.h>
#include "autodoc_intern.h"
#include "libdefs.h"
#include "compilerspecific.h"

extern SAVEDS STDARGS struct IClass *ObtainEngine(LIBBASETYPEPTR LIBBASE);

/***************************************************************************************************/

AROS_LH0(struct IClass *, ObtainEngine,
         LIBBASETYPEPTR, LIBBASE, 5, BASENAME)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    return ObtainEngine(LIBBASE);

    AROS_LIBFUNC_EXIT
}