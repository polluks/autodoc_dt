/*
    autodoc_init.c - Autodoc datatype initialization code.
    Copyright (c) 2026 Amiga Developer
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <utility/utility.h>

#ifdef __MORPHOS__
#include <intuition/classusr.h>
#define CLIB_ALIB_PROTOS_H
#endif

#include "initstruct.h"
#include "autodoc_intern.h"
#include "libdefs.h"
#include <clib/alib_protos.h>

#include "autodoc.datatype_VERSION.h"

#define INIT AROS_SLIB_ENTRY(init,Autodoc)

#ifdef __MORPHOS__
unsigned long __abox__ = 1;
#endif

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct AutodocBase_intern *INIT();
extern struct AutodocBase_intern *AROS_SLIB_ENTRY(open,Autodoc)();
extern BPTR AROS_SLIB_ENTRY(close,Autodoc)();
extern BPTR AROS_SLIB_ENTRY(expunge,Autodoc)();
extern int AROS_SLIB_ENTRY(null,Autodoc)();
extern const char LIBEND;

#ifdef __MORPHOS__
extern int __UserLibInit(LIBBASETYPEPTR);
extern void __UserLibCleanup(LIBBASETYPEPTR);
#endif

#undef SysBase
#define SysBase (IPB(AutodocBase)->sysbase)

int entry(void)
{
    return -1;
}

const struct Resident resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&resident + 1,
#ifdef __MORPHOS__
    RTF_PPC | RTF_EXTENDED | RTF_AUTOINIT,
#else
    RTF_AUTOINIT,
#endif
    VERSION,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl,
#ifdef __MORPHOS__
    REVISION,
    NULL
#endif
};

const char name[] = "autodoc.datatype";

const char version[] = VERSTAG;

const APTR inittabl[4] =
{
    (APTR)sizeof(struct AutodocBase_intern),
    (APTR)LIBFUNCTABLE,
#ifdef __MORPHOS__
    NULL,
#else
    (APTR)&datatable,
#endif
    &INIT
};

#ifndef __MORPHOS__
struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (LIBEND);
};

#define O(n) offsetof(struct AutodocBase_intern,n)

const struct inittable datatable =
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { VERSION_NUMBER } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { REVISION_NUMBER } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};

#undef O

#endif /* ! __MORPHOS__ */

#undef SysBase

#ifdef __MORPHOS__
struct AutodocBase_intern *LIB_init(struct AutodocBase_intern *LIBBASE, BPTR segList, struct ExecBase *SysBase)
#else
AROS_LH2(struct AutodocBase_intern *, init,
 AROS_LHA(struct AutodocBase_intern *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
     struct ExecBase *, SysBase, 0, BASENAME)
#endif
{
    AROS_LIBFUNC_INIT

#ifdef __MORPHOS__
    LIBBASE->library.lib_Revision = REVISION;
#endif

    LIBBASE->sysbase = SysBase;
    LIBBASE->seglist = segList;

    if (__UserLibInit(LIBBASE) == 0)
    {
        return LIBBASE;
    }
    else
    {
        return NULL;
    }
#ifndef __MORPHOS__
    AROS_LIBFUNC_EXIT
#endif
}

#define SysBase LIBBASE->sysbase

AROS_LH1(struct AutodocBase_intern *, open,
 AROS_LHA(ULONG, version, D0),
     struct AutodocBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT

    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags &= ~LIBF_DELEXP;

    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct AutodocBase_intern *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT

    if (!--LIBBASE->library.lib_OpenCnt)
    {
        if (LIBBASE->library.lib_Flags & LIBF_DELEXP)
        {
#ifdef __MORPHOS__
            return LIB_expunge();
#else
            return expunge();
#endif
        }
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct AutodocBase_intern *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;

    if (LIBBASE->library.lib_OpenCnt)
    {
        LIBBASE->library.lib_Flags |= LIBF_DELEXP;
        return 0;
    }

    __UserLibCleanup(LIBBASE);

    Remove(&LIBBASE->library.lib_Node);

    ret = LIBBASE->seglist;

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct AutodocBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}