#ifndef AUTODOC_INTERN_H
#define AUTODOC_INTERN_H

#include <dos/dos.h>
#include <exec/lists.h>

struct AutodocBase_intern
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR              seglist;
    struct IClass    *class;
};

#define IPB(ipb)        ((struct AutodocBase_intern *)ipb)

#define G(o) ((struct Gadget *)(o))

/* Line flags used by our layout (the struct Line itself is defined in
 * datatypes/textclass.h) */
#define LNF_SECTION     2
#define LNF_BOLD        4
#define LNF_ITALIC      8
#define LNF_CODE        16

/* Maximum text buffer size */
#define MAX_TEXT_SIZE   (1024 * 1024)
#define MAX_LINE_LEN    512

/* Autodoc section types */
#define AD_SECT_NONE       0
#define AD_SECT_NAME       1
#define AD_SECT_SYNOPSIS   2
#define AD_SECT_FUNCTION   3
#define AD_SECT_INPUTS     4
#define AD_SECT_RESULT     5
#define AD_SECT_EXAMPLE    6
#define AD_SECT_NOTES      7
#define AD_SECT_BUGS       8
#define AD_SECT_SEEALSO    9

/* Instance data */
struct AutodocData
{
    APTR    Pool;
    ULONG   Flags;
};

#endif /* AUTODOC_INTERN_H */