/*
    libfunc.c - Library initialization functions
    Copyright (c) 2026 Amiga Developer
*/
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/gadtools.h>
#include "compilerspecific.h"
#include "autodoc_intern.h"
#include "libdefs.h"

#undef DEBUG
#define DEBUG 0

struct ExecBase         *SysBase;
struct IntuitionBase    *IntuitionBase;
struct GfxBase          *GfxBase;
#ifdef _AROS
#ifdef __MORPHOS__
struct Library          *UtilityBase;
#else
struct UtilityBase      *UtilityBase;
#endif
#else
struct Library          *UtilityBase;
#endif
struct DosLibrary       *DOSBase;
struct Library          *DataTypesBase;
struct GadtoolsBase     *GadToolsBase;

/* inside autodoc_class.c */
struct IClass *DT_MakeClass(LIBBASETYPEPTR LIBBASE);

/**************************************************************************************************/

#ifdef __MORPHOS__
int __UserLibInit(LIBBASETYPEPTR LIBBASE)
#else
ASM SAVEDS int __UserLibInit(register __a6 LIBBASETYPEPTR LIBBASE)
#endif
{
    SysBase = LIBBASE->sysbase;

    if ((DataTypesBase = OpenLibrary("datatypes.library", 37)))
    {
        if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
        {
            if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
            {
                if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)))
                {
#ifdef _AROS
                    if ((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)))
#else
                    if ((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#endif
                    {
                        if ((GadToolsBase = (struct GadtoolsBase *)OpenLibrary("gadtools.library", 37)))
                        {
                            if ((LIBBASE->class = DT_MakeClass(LIBBASE)))
                            {
                                AddClass(LIBBASE->class);

                                return 0;
                            }
                            CloseLibrary((struct Library *)GadToolsBase);
                            GadToolsBase = NULL;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

/**************************************************************************************************/

#ifdef __MORPHOS__
void __UserLibCleanup(LIBBASETYPEPTR LIBBASE)
#else
ASM SAVEDS void __UserLibCleanup(register __a6 LIBBASETYPEPTR LIBBASE)
#endif
{
    if (LIBBASE->class)
    {
        RemoveClass(LIBBASE->class);
        FreeClass(LIBBASE->class);
        LIBBASE->class = NULL;
    }

    if (GadToolsBase)  CloseLibrary((struct Library *)GadToolsBase);
    if (UtilityBase)   CloseLibrary((struct Library *)UtilityBase);
    if (DOSBase)       CloseLibrary((struct Library *)DOSBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (GfxBase)       CloseLibrary((struct Library *)GfxBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
}

/**************************************************************************************************/

SAVEDS STDARGS struct IClass *ObtainEngine(LIBBASETYPEPTR LIBBASE)
{
    return (LIBBASE->class);
}