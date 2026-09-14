/*
 * test_autodoc.c - Host-side validation of Autodoc parsing logic.
 * Compiles the pure parsing helpers (no Amiga libraries needed) and
 * exercises them against the sample.autodoc file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FS_NORMAL 0
#define FS_BOLD   1

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

/* --- copy of helpers from autodoc_class.c --- */

static const char * const section_names[] =
{
    "NAME", "SYNOPSIS", "FUNCTION", "INPUTS", "RESULT",
    "EXAMPLE", "NOTES", "BUGS", "SEE ALSO", NULL
};

static const int section_types[] =
{
    AD_SECT_NAME, AD_SECT_SYNOPSIS, AD_SECT_FUNCTION, AD_SECT_INPUTS,
    AD_SECT_RESULT, AD_SECT_EXAMPLE, AD_SECT_NOTES, AD_SECT_BUGS,
    AD_SECT_SEEALSO
};

static char *SkipAutodocDecoration(char *line)
{
    char *p = line;

    while (*p == ' ' || *p == '\t')
        p++;

    while (*p == '*' || *p == '/')
        p++;

    while (*p == ' ' || *p == '\t')
        p++;

    if (*p == '-' && (p[1] == ' ' || p[1] == '\t' || p[1] == '\0'))
    {
        p++;
        while (*p == ' ' || *p == '\t')
            p++;
    }

    return p;
}

static int TrimTrailingDecoration(char *p, int len)
{
    while (len > 0 && (p[len - 1] == '*' || p[len - 1] == ' ' || p[len - 1] == '\t'))
        len--;
    return len;
}

static int IsAutodocStart(const char *line, int len)
{
    const char *p = line;
    int stars = 0;

    if (len < 8)
        return 0;

    if (strncmp(line, "/****i* ", 8) == 0) return 1;
    if (strncmp(line, "/****o* ", 8) == 0) return 1;

    if (*p == '/')
        p++;

    while (*p == '*') { stars++; p++; }

    if (stars < 6)
        return 0;

    if (*p != ' ')
        return 0;

    p++;
    while (*p == ' ' || *p == '\t' || *p == '*')
        p++;

    return (*p != '\0');
}

static int IsAutodocEnd(const char *line, int len)
{
    const char *p = line;
    int stars = 0;

    while (*p == '*') { stars++; p++; }

    if (stars < 3)
        return 0;

    while (*p == ' ' || *p == '\t' || *p == '/')
        p++;

    return (*p == '\0');
}

static int ClassifyHeading(const char *text)
{
    int s;
    if (text[0] == '\0')
        return AD_SECT_NONE;
    for (s = 0; section_names[s] != NULL; s++)
        if (strcmp(text, section_names[s]) == 0)
            return section_types[s];
    return AD_SECT_NONE;
}

/* --- test driver --- */

static int failures = 0;

#define CHECK(cond, msg) \
    do { if (!(cond)) { printf("FAIL: %s (line %d)\n", msg, __LINE__); failures++; } } while(0)

static void test_start_markers(void)
{
    CHECK(IsAutodocStart("/****** exec/AddTail", 18), "start: /******");
    CHECK(IsAutodocStart("****** exec/AddTail", 17), "start: ******");
    CHECK(IsAutodocStart("/****i* internal/Thing", 20), "start internal");
    CHECK(IsAutodocStart("/****o* obselete/Thing", 20), "start obsolete");
    CHECK(!IsAutodocStart("   *   NAME", 10), "not start: indented");
    CHECK(!IsAutodocStart("*   NAME", 8), "not start: plain *");
    CHECK(!IsAutodocStart("", 0), "not start: empty");
}

static void test_end_markers(void)
{
    CHECK(IsAutodocEnd("***", 3), "end: ***");
    CHECK(IsAutodocEnd("*********************", 8), "end: long stars");
    CHECK(!IsAutodocEnd("*   NAME", 8), "not end: * NAME");
    CHECK(!IsAutodocEnd("** ", 3), "not end: 2 stars");
    CHECK(!IsAutodocEnd("", 0), "not end: empty");
}

static void test_decoration(void)
{
    char buf[128];
    char *r;

    strcpy(buf, "*   NAME");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "NAME") == 0, "decor: *   NAME -> NAME");

    strcpy(buf, "*\tbody text");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "body text") == 0, "decor: *\\tbody text");

    strcpy(buf, "**  text after");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "text after") == 0, "decor: **  text");

    strcpy(buf, "*-  decor");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "decor") == 0, "decor: *-  decor");

    strcpy(buf, "* - decor too");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "decor too") == 0, "decor: * - decor too");

    strcpy(buf, "*\t-- background --");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "-- background --") == 0, "decor: preserves -- text");

    strcpy(buf, "  *   SYNOPSIS");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "SYNOPSIS") == 0, "decor: indented *   SYNOPSIS");

    strcpy(buf, "/****** autodoc.datatype/std/DT_MakeClass *****************");
    r = SkipAutodocDecoration(buf);
    CHECK(strncmp(r, "autodoc.datatype/std/DT_MakeClass", 33) == 0, "decor: header line leading");

    strcpy(buf, "plain text");
    r = SkipAutodocDecoration(buf);
    CHECK(strcmp(r, "plain text") == 0, "decor: plain text preserved");
}

static void test_headings(void)
{
    CHECK(ClassifyHeading("NAME") == AD_SECT_NAME, "heading NAME");
    CHECK(ClassifyHeading("SYNOPSIS") == AD_SECT_SYNOPSIS, "heading SYNOPSIS");
    CHECK(ClassifyHeading("FUNCTION") == AD_SECT_FUNCTION, "heading FUNCTION");
    CHECK(ClassifyHeading("INPUTS") == AD_SECT_INPUTS, "heading INPUTS");
    CHECK(ClassifyHeading("RESULT") == AD_SECT_RESULT, "heading RESULT");
    CHECK(ClassifyHeading("EXAMPLE") == AD_SECT_EXAMPLE, "heading EXAMPLE");
    CHECK(ClassifyHeading("NOTES") == AD_SECT_NOTES, "heading NOTES");
    CHECK(ClassifyHeading("BUGS") == AD_SECT_BUGS, "heading BUGS");
    CHECK(ClassifyHeading("SEE ALSO") == AD_SECT_SEEALSO, "heading SEE ALSO");
    CHECK(ClassifyHeading("StealMoney -- steal") == AD_SECT_NONE, "not heading: content");
    CHECK(ClassifyHeading("") == AD_SECT_NONE, "not heading: empty");
    CHECK(ClassifyHeading("NOTE") == AD_SECT_NONE, "not heading: NOTE plural");
}

static void test_line_walker(void)
{
    const char *doc =
        "/****** exec.library/AddTail *************************************"
        "\n*\n*   NAME\n*"
        "\tAddTail -- Add a node to the tail of a list. (V36)\n"
        "*\n*   SYNOPSIS\n"
        "*\tVOID AddTail(struct List *list, struct Node *node);\n"
        "*****************************************************************\n";

    const char *p = doc;
    int inAutodoc = 0;
    int cur = AD_SECT_NONE;
    int lineNo = 0;
    int sawName = 0;
    int sawSynopsis = 0;
    int sawTitle = 0;

    while (*p)
    {
        const char *eol = strchr(p, '\n');
        int len = eol ? (int)(eol - p) : (int)strlen(p);
        char *line = malloc(len + 1);
        char *content;
        int sect;

        memcpy(line, p, len);
        line[len] = '\0';
        lineNo++;

        if (IsAutodocStart(line, len)) {
            /* module/function title lives on the marker line */
            char *title = SkipAutodocDecoration(line);
            int tlen = TrimTrailingDecoration(title, len - (int)(title - line));
            inAutodoc = 1; cur = AD_SECT_NONE;
            if (tlen > 0) {
                sawTitle = 1;
                printf("  -> title: '%.*s'\n", tlen, title);
            }
        } else if (inAutodoc && IsAutodocEnd(line, len)) {
            inAutodoc = 0; cur = AD_SECT_NONE;
        } else if (inAutodoc) {
            content = SkipAutodocDecoration(line);
            sect = ClassifyHeading(content);
            if (sect != AD_SECT_NONE) {
                cur = sect;
                if (sect == AD_SECT_NAME) sawName = 1;
                if (sect == AD_SECT_SYNOPSIS) sawSynopsis = 1;
            }
        }

        printf("line %d: %-4s inAutodoc=%d sect=%d content='%s'\n",
               lineNo, inAutodoc ? "IN" : "OUT", inAutodoc, cur, line);

        free(line);
        if (!eol) break;
        p = eol + 1;
    }

    CHECK(sawTitle, "walker: saw module title before headings");
    CHECK(sawName, "walker: saw NAME heading");
    CHECK(sawSynopsis, "walker: saw SYNOPSIS heading");
    printf("walker: title=%d NAME=%d SYNOPSIS=%d\n", sawTitle, sawName, sawSynopsis);
}

static void test_sample_file(void)
{
    FILE *f = fopen("sample.autodoc", "rb");
    char *doc = NULL;
    long flen;
    const char *p;
    int inAutodoc = 0;
    int cur = AD_SECT_NONE;
    int lineNo = 0;
    int sections[10] = {0};
    const char *sect_names[] = { "NONE", "NAME", "SYNOPSIS", "FUNCTION", "INPUTS",
                                 "RESULT", "EXAMPLE", "NOTES", "BUGS", "SEE ALSO" };

    if (!f)
    {
        printf("SKIP: sample.autodoc not found\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    flen = ftell(f);
    fseek(f, 0, SEEK_SET);
    doc = malloc(flen + 1);
    if (fread(doc, 1, flen, f) != (size_t)flen)
    {
        printf("SKIP: could not read sample.autodoc\n");
        free(doc);
        fclose(f);
        return;
    }
    fclose(f);
    doc[flen] = '\0';

    p = doc;
    while (*p)
    {
        const char *eol = strchr(p, '\n');
        int len = eol ? (int)(eol - p) : (int)strlen(p);
        char *line = malloc(len + 1);
        char *content;
        int sect;
        int headers = 0;

        memcpy(line, p, len);
        line[len] = '\0';
        lineNo++;

        if (IsAutodocStart(line, len)) {
            char *title = SkipAutodocDecoration(line);
            int tlen = TrimTrailingDecoration(title, len - (int)(title - line));
            inAutodoc = 1; cur = AD_SECT_NONE;
            if (tlen > 0) {
                headers++;
                if (strncmp(title, "autodoc.datatype", 16) == 0) {
                    printf("  sample title OK: '%.*s'\n", tlen, title);
                }
            }
        } else if (inAutodoc && IsAutodocEnd(line, len)) {
            inAutodoc = 0; cur = AD_SECT_NONE;
        } else if (inAutodoc) {
            content = SkipAutodocDecoration(line);
            sect = ClassifyHeading(content);
            if (sect != AD_SECT_NONE) {
                cur = sect;
                sections[sect]++;
                headers++;
            }
        }

        if (!headers && inAutodoc && cur != AD_SECT_NONE && line[0])
            sections[cur] = sections[cur]; /* body line, nothing to do */

        free(line);
        if (!eol) break;
        p = eol + 1;
    }

    printf("sample: %d lines, sections found:\n", lineNo);
    for (int s = AD_SECT_NAME; s <= AD_SECT_SEEALSO; s++)
        if (sections[s])
            printf("  %-8s x%d\n", sect_names[s], sections[s]);

    CHECK(sections[AD_SECT_NAME] >= 1, "sample: NAME heading found");
    CHECK(sections[AD_SECT_SYNOPSIS] >= 1, "sample: SYNOPSIS heading found");
    CHECK(sections[AD_SECT_FUNCTION] >= 1, "sample: FUNCTION heading found");
    CHECK(sections[AD_SECT_INPUTS] >= 1, "sample: INPUTS heading found");
    CHECK(sections[AD_SECT_RESULT] >= 1, "sample: RESULT heading found");
    CHECK(sections[AD_SECT_EXAMPLE] >= 1, "sample: EXAMPLE heading found");
    CHECK(sections[AD_SECT_NOTES] >= 1, "sample: NOTES heading found");
    CHECK(sections[AD_SECT_BUGS] >= 1, "sample: BUGS heading found");
    CHECK(sections[AD_SECT_SEEALSO] >= 1, "sample: SEE ALSO heading found");

    free(doc);
}

int main(void)
{
    printf("=== Autodoc parsing logic tests ===\n");
    test_start_markers();
    test_end_markers();
    test_decoration();
    test_headings();
    test_line_walker();
    test_sample_file();

    if (failures == 0)
        printf("\nAll tests passed.\n");
    else
        printf("\n%d test(s) FAILED.\n", failures);

    return failures ? 1 : 0;
}