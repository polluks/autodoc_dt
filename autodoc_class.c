/*
    Copyright (c) 2026 Amiga Developer
    autodoc_class.c - Autodoc datatype class implementation.

    Rendering of Amiga Autodoc documentation files, based on the
    Autodoc Style Guide (https://wiki.amigaos.net/wiki/Autodoc_Style_Guide).

    Autodoc format summary (per the style guide):
      - An autodoc begins with a line matching "/****** " or "****** "
        (six asterisks preceded by '/' or '*', then a space).
      - "/****i* " marks an internal-only autodoc (extracted only when the
        "internal" flag is enabled, V53.1).
      - "/****o* " marks an obsolete autodoc (extracted only when the
        "obsolete" flag is enabled, V53.2).
      - An autodoc ends with at least three asterisks at the start of a line.
      - Headings are: NAME, SYNOPSIS, FUNCTION, INPUTS, RESULT, EXAMPLE,
        NOTES, BUGS, SEE ALSO.
      - The module/function line is formatted: modulename.type/FunctionName
        (the special name "--background--" is used for background info).
      - Body text is tab-indented after the leading '*'.

    Credits: AmigaOS Documentation Wiki - "Autodoc Style Guide",
    Copyright (c) Hyperion Entertainment and contributors.
    Structure modeled after the AROS ascii.datatype (The AROS
    Development Team, 1995-2001).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <graphics/text.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/textclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/datatypes.h>

#include "autodoc_intern.h"

#undef DEBUG
#define DEBUG 0

#ifndef __varargs68k
#define __varargs68k
#endif

/* Autodoc section keyword table (from the style guide) */
static const char * const section_names[] =
{
    "NAME",
    "SYNOPSIS",
    "FUNCTION",
    "INPUTS",
    "RESULT",
    "EXAMPLE",
    "NOTES",
    "BUGS",
    "SEE ALSO",
    NULL
};

static const ULONG section_types[] =
{
    AD_SECT_NAME,
    AD_SECT_SYNOPSIS,
    AD_SECT_FUNCTION,
    AD_SECT_INPUTS,
    AD_SECT_RESULT,
    AD_SECT_EXAMPLE,
    AD_SECT_NOTES,
    AD_SECT_BUGS,
    AD_SECT_SEEALSO
};

/**************************************************************************************************/

#ifdef __MORPHOS__
static __varargs68k IPTR NotifyAttrChanges(Object *o, VOID *ginfo, ULONG flags, ...)
{
    va_list va;
    ULONG result;
    va_start(va, flags);

    result = DoMethod(o, OM_NOTIFY, (ULONG)((struct TagItem *)va->overflow_arg_area), (ULONG)(ginfo), flags);
    va_end(va);

    return (result);
}
#else
static VARARGS IPTR NotifyAttrChanges(Object *o, VOID *ginfo, ULONG flags, ULONG tag1, ...)
{
    return DoMethod(o, OM_NOTIFY, &tag1, ginfo, flags);
}
#endif

/**************************************************************************************************/

/* Skip the C-comment / asterisk decoration at the start of an Autodoc
 * content line. Handles the forms:
 *   "* text", "*   HEADING", "*\tbody", "**  text", "*- text", "* - text".
 * Returns a pointer to the actual content within the line.
 */
static STRPTR SkipAutodocDecoration(STRPTR line)
{
    STRPTR p = line;

    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t')
        p++;

    /* Skip the comment opener: '*' and '/' */
    while (*p == '*' || *p == '/')
        p++;

    /* Skip whitespace */
    while (*p == ' ' || *p == '\t')
        p++;

    /* An optional '-' decoration marker, only when followed by whitespace
     * (so "-- background --" or "-foo" text is preserved) */
    if (*p == '-' && (p[1] == ' ' || p[1] == '\t' || p[1] == '\0'))
    {
        p++;
        while (*p == ' ' || *p == '\t')
            p++;
    }

    return p;
}

/**************************************************************************************************/

/* Trim trailing whitespace and comment decoration ('*') from a segment
 * that points into the Autodoc text buffer. Returns the new length.
 */
static LONG TrimTrailingDecoration(STRPTR p, LONG len)
{
    while (len > 0 && (p[len - 1] == '*' || p[len - 1] == ' ' || p[len - 1] == '\t'))
        len--;
    return len;
}

/**************************************************************************************************/

/* Determine if a line marks the start of an Autodoc block.
 * Per the style guide: "/****** " or "****** " (six asterisks preceded
 * by '/' or not, then a space). Internal "/****i* " (V53.1) and
 * obsolete "/****o* " (V53.2) markers. Any run of six or more asterisks
 * followed by a space and content is accepted, so "****** title" works
 * both as the opening marker and as the immediate start of the next
 * block after a previous one.
 */
static BOOL IsAutodocStart(CONST_STRPTR line, ULONG len)
{
    CONST_STRPTR p = line;
    ULONG stars = 0;

    if (len < 8)
        return FALSE;

    /* Internal / obsolete forms */
    if (strncmp(line, "/****i* ", 8) == 0)
        return TRUE;
    if (strncmp(line, "/****o* ", 8) == 0)
        return TRUE;

    /* Optional leading '/' then a run of asterisks */
    if (*p == '/')
        p++;

    while (*p == '*')
    {
        stars++;
        p++;
    }

    if (stars < 6)
        return FALSE;

    /* Must be followed by a space and some content */
    if (*p != ' ')
        return FALSE;

    p++;
    while (*p == ' ' || *p == '\t' || *p == '*')
        p++;

    return (*p != '\0');
}

/**************************************************************************************************/

/* Determine if a line marks the end of an Autodoc block.
 * Per the style guide: at least three asterisks at the start of a line.
 * To distinguish from the "****** title" start marker, the line must
 * consist only of asterisks, an optional trailing '/' and whitespace.
 */
static BOOL IsAutodocEnd(CONST_STRPTR line, ULONG len)
{
    CONST_STRPTR p = line;
    ULONG stars = 0;

    while (*p == '*')
    {
        stars++;
        p++;
    }

    if (stars < 3)
        return FALSE;

    while (*p == ' ' || *p == '\t' || *p == '/')
        p++;

    return (*p == '\0');
}

/**************************************************************************************************/

/* Look up a decoded heading in the section keyword table.
 * Returns the section type, or AD_SECT_NONE if not a heading.
 */
static ULONG ClassifyHeading(CONST_STRPTR text)
{
    LONG len = strlen(text);

    /* Allow an optional trailing '(' etc., but require no other chars */
    if (len == 0)
        return AD_SECT_NONE;

    for (int s = 0; section_names[s] != NULL; s++)
    {
        if (strcmp(text, section_names[s]) == 0)
            return section_types[s];
    }

    return AD_SECT_NONE;
}

/**************************************************************************************************/

static IPTR Autodoc_New(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    if ((retval = DoSuperMethodA(cl, o, (Msg)msg)))
    {
        struct AutodocData *data;
        IPTR len, estlines, poolsize;
        BOOL success = FALSE;
        STRPTR buffer;

        data = INST_DATA(cl, (Object *)retval);

        GetDTAttrs((Object *)retval, TDTA_Buffer, (IPTR)&buffer, TDTA_BufferLen, (IPTR)&len, TAG_DONE);

        if (buffer && len)
        {
            estlines = (len / 80) + 1;
            estlines = (estlines > 200) ? 200 : estlines;
            poolsize = sizeof(struct Line) * estlines;

            if ((data->Pool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, poolsize, poolsize)))
                success = TRUE;
            else
                SetIoErr(ERROR_NO_FREE_STORE);
        }
        else
        {
            SetIoErr(ERROR_REQUIRED_ARG_MISSING);
        }

        if (!success)
        {
            CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
            retval = 0;
        }
    }

    return retval;
}

/**************************************************************************************************/

static IPTR Autodoc_Dispose(Class *cl, Object *o, Msg msg)
{
    struct AutodocData *data;
    struct List *linelist = NULL;
    IPTR retval;

    data = INST_DATA(cl, o);

    if (GetDTAttrs(o, TDTA_LineList, (IPTR)&linelist, TAG_DONE) && linelist)
        NewList(linelist);

    DeletePool(data->Pool);

    retval = DoSuperMethodA(cl, o, msg);

    return retval;
}

/**************************************************************************************************/

static IPTR Autodoc_Set(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    if ((retval = DoSuperMethodA(cl, o, (Msg)msg)) && (OCLASS(o) == cl))
    {
        struct RastPort *rp;

        if ((rp = ObtainGIRPort(msg->ops_GInfo)))
        {
            struct gpRender gpr;

            gpr.MethodID = GM_RENDER;
            gpr.gpr_GInfo = msg->ops_GInfo;
            gpr.gpr_RPort = rp;
            gpr.gpr_Redraw = GREDRAW_UPDATE;
            DoMethodA(o, (Msg)&gpr);

            ReleaseGIRPort(rp);
        }
        retval = 0;
    }

    return retval;
}

/**************************************************************************************************/

static IPTR Autodoc_Layout(Class *cl, Object *o, struct gpLayout *msg)
{
    IPTR retval;

    NotifyAttrChanges(o, msg->gpl_GInfo, 0, GA_ID, G(o)->GadgetID, DTA_Busy, TRUE, TAG_DONE);

    retval = (IPTR)DoSuperMethodA(cl, o, (Msg)msg);

    retval += DoAsyncLayout(o, msg);

    return retval;
}

/**************************************************************************************************/

static IPTR Autodoc_ProcLayout(Class *cl, Object *o, struct gpLayout *msg)
{
    IPTR retval;

    NotifyAttrChanges(o, ((struct gpLayout *)msg)->gpl_GInfo, 0, GA_ID, G(o)->GadgetID, DTA_Busy, TRUE, TAG_DONE);

    retval = (IPTR)DoSuperMethodA(cl, o, (Msg)msg);

    return retval;
}

/**************************************************************************************************/

/* The main layout routine.
 *
 * Walks the Autodoc text buffer and builds a line list.
 *  - Inside an Autodoc block, section headings (NAME, SYNOPSIS, ...)
 *    and the module/function title line are rendered bold.
 *  - "/****** ... ***" block decorations and the leading "* " of each
 *    content line are removed by pointing ln_Text past the decoration.
 *  - Body, synopsis and example text is rendered as normal text.
 *  - Text outside any Autodoc block is rendered as plain text.
 *
 * Line segments point directly into the text buffer (like ascii.datatype
 * does), so no per-line copies are needed.
 */
static IPTR Autodoc_AsyncLayout(Class *cl, Object *o, struct gpLayout *gpl)
{
    struct DTSpecialInfo *si = (struct DTSpecialInfo *)G(o)->SpecialInfo;
    struct AutodocData *data = INST_DATA(cl, o);
    ULONG visible = 0, total = 0;
    struct RastPort trp;
    ULONG bsig = 0;

    BOOL abort = FALSE;
    BOOL inAutodoc = FALSE;
    ULONG currentSection = AD_SECT_NONE;

    struct TextAttr *tattr;
    struct TextFont *font;
    struct List *linelist;
    struct IBox *domain;
    IPTR wrap = FALSE;
    IPTR bufferlen;
    STRPTR buffer;
    STRPTR title;

    ULONG offset = 0;
    ULONG swidth;
    struct Line *line;
    ULONG yoffset = 0;
    ULONG linelength = 0;
    ULONG max_linelength = 0;
    UBYTE fgpen = 1;
    UBYTE bgpen = 0;
    ULONG i;
    LONG lineStart;

    ULONG nomwidth, nomheight;
    ULONG tabulator = 8;
    LONG len;
    STRPTR p;

    {
        TEXT tmpbuf[3];

        if (GetVar("TABSIZE", tmpbuf, sizeof(tmpbuf), 0) > 0)
        {
            tabulator = atoi(tmpbuf);
            if (tabulator < 1)  tabulator = 1;
            if (tabulator > 32) tabulator = 32;
        }
    }

    if (GetDTAttrs(o, DTA_TextAttr, (IPTR)&tattr, DTA_TextFont, (IPTR)&font,
                   DTA_Domain, (IPTR)&domain, DTA_ObjName, (IPTR)&title,
                   TDTA_Buffer, (IPTR)&buffer, TDTA_BufferLen, (IPTR)&bufferlen,
                   TDTA_LineList, (IPTR)&linelist, TDTA_WordWrap, (IPTR)&wrap,
                   TAG_DONE) == 12)
    {
        ObtainSemaphore(&(si->si_Lock));

        nomheight = (ULONG)(24 * font->tf_YSize);
        nomwidth  = (ULONG)(80 * font->tf_XSize);

        if (buffer)
        {
            InitRastPort(&trp);
            SetFont(&trp, font);

            if (wrap || gpl->gpl_Initial)
            {
                while ((line = (struct Line *)RemHead(linelist)))
                    FreePooled(data->Pool, line, sizeof(struct Line));

                offset     = 0;
                lineStart  = 0;
                total      = 0;
                yoffset    = 0;
                inAutodoc  = FALSE;
                currentSection = AD_SECT_NONE;

                for (i = 0; (i < (ULONG)bufferlen) && (bsig == 0) && !abort; i++)
                {
                    if (buffer[i] != '\n')
                        continue;

                    /* One complete physical line: buffer[lineStart..i) */
                    len = (LONG)(i - lineStart);
                    if (len > 0 && buffer[i - 1] == '\r')
                        len--;

                    p = &buffer[lineStart];

                    /* Start of a new Autodoc block? The module/function
                     * title is on this marker line, so render it bold.
                     * Checked first so a "****** title" line can both
                     * close the previous block and open the next one. */
                    if (IsAutodocStart(p, len))
                    {
                        STRPTR content = SkipAutodocDecoration(p);
                        LONG titleLen;

                        inAutodoc = TRUE;
                        currentSection = AD_SECT_NONE;

                        titleLen = (LONG)(len - (content - p));
                        if (titleLen < 0)
                            titleLen = 0;
                        titleLen = TrimTrailingDecoration(content, titleLen);

                        if (titleLen > 0)
                        {
                            line = AllocPooled(data->Pool, sizeof(struct Line));
                            if (line)
                            {
                                swidth = TextLength(&trp, content, titleLen);

                                line->ln_Text    = content;
                                line->ln_TextLen = titleLen;
                                line->ln_XOffset = 0;
                                line->ln_YOffset = yoffset + font->tf_Baseline;
                                line->ln_Width   = swidth;
                                line->ln_Height  = font->tf_YSize;
                                line->ln_Flags   = LNF_LF;
                                line->ln_FgPen   = fgpen;
                                line->ln_BgPen   = bgpen;
                                line->ln_Style   = FS_BOLD;
                                line->ln_Data    = NULL;

                                linelength = line->ln_Width;
                                if (linelength > max_linelength)
                                    max_linelength = linelength;

                                AddTail(linelist, (struct Node *)line);

                                yoffset += font->tf_YSize;
                                total++;
                            }
                            else
                            {
                                abort = TRUE;
                            }
                        }

                        lineStart = i + 1;
                        offset = 0;
                        continue;
                    }

                    /* End of an Autodoc block (a line of only asterisks) */
                    if (inAutodoc && IsAutodocEnd(p, len))
                    {
                        inAutodoc = FALSE;
                        currentSection = AD_SECT_NONE;
                        lineStart = i + 1;
                        offset = 0;
                        continue;
                    }

                    if (inAutodoc)
                    {
                        /* Point at the content, past the decoration */
                        STRPTR content = SkipAutodocDecoration(p);
                        ULONG styleFlags = FS_NORMAL;
                        ULONG sect;
                        LONG contentLen;

                        /* Length of the content on this physical line */
                        contentLen = (LONG)(i - lineStart);
                        if (contentLen > 0 && buffer[i - 1] == '\r')
                            contentLen--;
                        contentLen -= (LONG)(content - p);
                        if (contentLen < 0)
                            contentLen = 0;
                        contentLen = TrimTrailingDecoration(content, contentLen);

                        /* Classify heading lines */
                        sect = ClassifyHeading(content);
                        if (sect != AD_SECT_NONE)
                        {
                            currentSection = sect;
                            styleFlags = FS_BOLD;
                        }

                        /* Allocate a line segment pointing into buffer */
                        if (contentLen > 0)
                        {
                            line = AllocPooled(data->Pool, sizeof(struct Line));
                            if (line)
                            {
                                swidth = TextLength(&trp, content, contentLen);

                                line->ln_Text    = content;
                                line->ln_TextLen = contentLen;
                                line->ln_XOffset = offset;
                                line->ln_YOffset = yoffset + font->tf_Baseline;
                                line->ln_Width   = swidth;
                                line->ln_Height  = font->tf_YSize;
                                line->ln_Flags   = LNF_LF;
                                line->ln_FgPen   = fgpen;
                                line->ln_BgPen   = bgpen;
                                line->ln_Style   = styleFlags;
                                line->ln_Data    = NULL;

                                linelength = line->ln_Width + line->ln_XOffset;
                                if (linelength > max_linelength)
                                    max_linelength = linelength;

                                AddTail(linelist, (struct Node *)line);

                                yoffset += font->tf_YSize;
                                offset = 0;
                                total++;
                            }
                            else
                            {
                                abort = TRUE;
                            }
                        }
                        else
                        {
                            /* Blank content line inside autodoc */
                            yoffset += font->tf_YSize;
                            total++;
                            offset = 0;
                        }
                    }
                    else
                    {
                        /* Plain text outside any autodoc */
                        if (len > 0)
                        {
                            line = AllocPooled(data->Pool, sizeof(struct Line));
                            if (line)
                            {
                                swidth = TextLength(&trp, p, len);

                                line->ln_Text    = p;
                                line->ln_TextLen = len;
                                line->ln_XOffset = offset;
                                line->ln_YOffset = yoffset + font->tf_Baseline;
                                line->ln_Width   = swidth;
                                line->ln_Height  = font->tf_YSize;
                                line->ln_Flags   = LNF_LF;
                                line->ln_FgPen   = fgpen;
                                line->ln_BgPen   = bgpen;
                                line->ln_Style   = FS_NORMAL;
                                line->ln_Data    = NULL;

                                linelength = line->ln_Width + line->ln_XOffset;
                                if (linelength > max_linelength)
                                    max_linelength = linelength;

                                AddTail(linelist, (struct Node *)line);

                                yoffset += font->tf_YSize;
                                offset = 0;
                                total++;
                            }
                            else
                            {
                                abort = TRUE;
                            }
                        }
                        else
                        {
                            yoffset += font->tf_YSize;
                            total++;
                        }
                    }

                    lineStart = i + 1;
                    offset = 0;

                    bsig = CheckSignal(SIGBREAKF_CTRL_C);
                }

                /* Handle any trailing content without a newline */
                if (lineStart < (LONG)bufferlen)
                {
                    len = (LONG)(bufferlen - lineStart);
                    p = &buffer[lineStart];

                    if (inAutodoc)
                    {
                        STRPTR content = SkipAutodocDecoration(p);
                        ULONG sect;
                        ULONG styleFlags = FS_NORMAL;

                        len -= (LONG)(content - p);
                        if (len < 0)
                            len = 0;
                        len = TrimTrailingDecoration(content, len);

                        sect = ClassifyHeading(content);
                        if (sect != AD_SECT_NONE)
                        {
                            currentSection = sect;
                            styleFlags = FS_BOLD;
                        }

                        if (len > 0)
                        {
                            line = AllocPooled(data->Pool, sizeof(struct Line));
                            if (line)
                            {
                                swidth = TextLength(&trp, content, len);

                                line->ln_Text    = content;
                                line->ln_TextLen = len;
                                line->ln_XOffset = 0;
                                line->ln_YOffset = yoffset + font->tf_Baseline;
                                line->ln_Width   = swidth;
                                line->ln_Height  = font->tf_YSize;
                                line->ln_Flags   = LNF_LF;
                                line->ln_FgPen   = fgpen;
                                line->ln_BgPen   = bgpen;
                                line->ln_Style   = styleFlags;
                                line->ln_Data    = NULL;

                                AddTail(linelist, (struct Node *)line);
                                total++;
                            }
                        }
                    }
                    else
                    {
                        if (len > 0)
                        {
                            line = AllocPooled(data->Pool, sizeof(struct Line));
                            if (line)
                            {
                                swidth = TextLength(&trp, p, len);

                                line->ln_Text    = p;
                                line->ln_TextLen = len;
                                line->ln_XOffset = 0;
                                line->ln_YOffset = yoffset + font->tf_Baseline;
                                line->ln_Width   = swidth;
                                line->ln_Height  = font->tf_YSize;
                                line->ln_Flags   = LNF_LF;
                                line->ln_FgPen   = fgpen;
                                line->ln_BgPen   = bgpen;
                                line->ln_Style   = FS_NORMAL;
                                line->ln_Data    = NULL;

                                AddTail(linelist, (struct Node *)line);
                                total++;
                            }
                        }
                    }
                }
            }
            else
            {
                total          = si->si_TotVert;
                max_linelength = si->si_TotHoriz * si->si_HorizUnit;
            }

#ifndef __MORPHOS__
            DeinitRastPort(&trp);
#endif
        }

        si->si_VertUnit = font->tf_YSize;
        si->si_VisVert  = visible = domain->Height / si->si_VertUnit;
        si->si_TotVert  = total;

        si->si_HorizUnit = font->tf_XSize;
        si->si_VisHoriz  = domain->Width / si->si_HorizUnit;
        si->si_TotHoriz  = max_linelength / si->si_HorizUnit;

        ReleaseSemaphore(&si->si_Lock);

        if (bsig == 0)
        {
            NotifyAttrChanges(o, gpl->gpl_GInfo, 0, GA_ID, G(o)->GadgetID,
                              DTA_VisibleVert, visible,
                              DTA_TotalVert, total,
                              DTA_NominalVert, nomheight,
                              DTA_VertUnit, font->tf_YSize,
                              DTA_VisibleHoriz, (domain->Width / si->si_HorizUnit),
                              DTA_TotalHoriz, max_linelength / si->si_HorizUnit,
                              DTA_NominalHoriz, nomwidth,
                              DTA_HorizUnit, si->si_HorizUnit,
                              DTA_TopHoriz, si->si_TopHoriz,
                              DTA_Title, (IPTR)title,
                              DTA_Busy, FALSE,
                              DTA_Sync, TRUE,
                              TAG_DONE);
        }
    }

    return (IPTR)total;
}

/**************************************************************************************************/

#if defined(_AROS) || defined(__MORPHOS__)
AROS_UFH3S(IPTR, DT_Dispatcher, AROS_UFHA(Class *, cl, A0), AROS_UFHA(Object *, o, A2), AROS_UFHA(Msg, msg, A1))
{
#else
IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *o, register __a1 Msg msg)
{
#endif

    AROS_USERFUNC_INIT IPTR retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = Autodoc_New(cl, o, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            retval = Autodoc_Dispose(cl, o, msg);
            break;

        case OM_SET:
        case OM_UPDATE:
            retval = Autodoc_Set(cl, o, (struct opSet *)msg);
            break;

        case GM_LAYOUT:
            retval = Autodoc_Layout(cl, o, (struct gpLayout *)msg);
            break;

        case DTM_PROCLAYOUT:
            retval = Autodoc_ProcLayout(cl, o, (struct gpLayout *)msg);
            /* fall through */

        case DTM_ASYNCLAYOUT:
            retval = Autodoc_AsyncLayout(cl, o, (struct gpLayout *)msg);
            break;

        default:
            retval = DoSuperMethodA(cl, o, msg);
            break;
    }

    return retval;

    AROS_USERFUNC_EXIT}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *autodocbase)
{
    struct IClass *cl = MakeClass("autodoc.datatype", "text.datatype", 0,
                                  sizeof(struct AutodocData), 0);

    if (cl)
    {
#if defined(_AROS) || defined(__MORPHOS__)
        cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
        cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
        cl->cl_UserData = (IPTR)autodocbase;
    }

    return cl;
}