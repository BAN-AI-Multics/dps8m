/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 563cbc3d-f631-11ec-a429-80ee73e9b8e7
 *
 * =================================================================================================================================
 *
 * Copyright (c) 1998 P. G. Womack, Diss, Norfolk, UK.
 * Copyright (c) 2006 John S. Urban <urbanjost@comcast.net>
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * =================================================================================================================================
 */

/* ============================================================================================================================== */

/*
         1         2         3         4         5         6         7         8         9        10        11        12        13
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
*/

/*
 * ---------------------------------------------------------------------------
 *
 * Derived from asa2pdf - John S. Urban - Apr 30, 2006
 *
 * Needed to emulate an old ASA 60 line by 132 column lineprinter quickly with
 * output as a PDF file.
 *
 * Tested with xpdf, gv/ghostview, and acroread (PC version) PDF interpreters.
 *
 * ---------------------------------------------------------------------------
 *
 * V1:
 *
 *   *  Began with txt2pdf; Copyright 1998; P. G. Womack, Diss, Norfolk, UK.
 *   *  "Do what you like, but don't claim you wrote it."
 *   *  Added bar shading.
 *   *  user-settable gray scale value via environment variable IMPACT_GRAY
 *   *  placed Adobe-recommended "binary" flag at top of PDF file.
 *   *  changed sizes to simulate a 60 line by 132 column lineprinter.
 *   *  changed so obeys ASA carriage control (' 01+' in column 1).
 *
 * ---------------------------------------------------------------------------
 *
 * V2:
 *
 *   * added command line options for
 *     * margins, lines per page, page size
 *     * gray scale, dash code pattern, shade spacing
 *     * margin messages, font
 *
 * ---------------------------------------------------------------------------
 */

/* ============================================================================================================================== */

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#if !defined(_AIX)
# include <getopt.h>
#endif
#if defined(__APPLE__)
# include <xlocale.h>
#endif
#include <locale.h>

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

/* ============================================================================================================================== */
#define MAX(x, y)       ((x) > (y) ?  (x) : (y))
/* #define MIN(x, y)       ((x) < (y) ?  (x) : (y)) */
/* #define ABS(x)          ((x) <  0  ? -(x) : (x)) */
/* ============================================================================================================================== */
/* size of printable area */
/* Default unit is 72 points per inch */
 static float GLOBAL_PAGE_DEPTH;
 static float GLOBAL_PAGE_WIDTH;
 static float GLOBAL_PAGE_MARGIN_LEFT;
 static float GLOBAL_PAGE_MARGIN_RIGHT;
 static float GLOBAL_PAGE_MARGIN_TOP;
 static float GLOBAL_PAGE_MARGIN_BOTTOM;
 static float GLOBAL_UNIT_MULTIPLIER=72.0;
 static int   GLOBAL_SHADE_STEP=2;
 static char  GLOBAL_DASHCODE[256];
 static char  GLOBAL_FONT[256];
 static int   GLOBAL_SHIFT = 0;
 static int   GLOBAL_LINECOUNT=0;
 static int   GLOBAL_PAGES=0;
 static int   GLOBAL_PAGECOUNT=0;
 static int   GLOBAL_LINENUMBERS = 0;

 static float GLOBAL_LINES_PER_PAGE;

 static float GLOBAL_GRAY_SCALE;
 static int   GLOBAL_GREEN_BAR;

 static char  GLOBAL_CENTER_TITLE[256];
 static char  GLOBAL_LEFT_TITLE[256];
 static float GLOBAL_TITLE_SIZE;

/* ============================================================================================================================== */
 static int   GLOBAL_ADD=0;
 static int   GLOBAL_VERSION = 3;
 static int   GLOBAL_VERSION_PATCH = 1;
 static float GLOBAL_LEAD_SIZE;
 static float GLOBAL_FONT_SIZE;
 static int   GLOBAL_OBJECT_ID = 1;
 static int   GLOBAL_PAGE_TREE_ID;
 static int   GLOBAL_NUM_PAGES = 0;
 static int   GLOBAL_NUM_XREFS = 0;
 static long *GLOBAL_XREFS = NULL;
 static int   GLOBAL_STREAM_ID, GLOBAL_STREAM_LEN_ID;
 static long  GLOBAL_STREAM_START;
 static float GLOBAL_YPOS;

 typedef struct _PageList {
    struct _PageList *next;

        int  page_id;
#if !defined(__MINGW64__)
        char pad[sizeof(void(*)(void))-sizeof(int)];
#endif /* if !defined(__MINGW64__) */

 } PageList;

 static PageList *GLOBAL_PAGE_LIST = NULL;
 static PageList **GLOBAL_INSERT_PAGE = &GLOBAL_PAGE_LIST;

/* ============================================================================================================================== */
 static void store_page(int id){
   PageList *n = (PageList *)malloc(sizeof(*n));

   if(n == NULL) {
      (void)fprintf(stderr,"\rERROR: Unable to allocate array for page %d.\r\n",
                    GLOBAL_NUM_PAGES + 1);
      (void)fprintf(stderr,"\r\n\rFATAL: Bugcheck! Aborting at %s[%s:%d]\r\n",
                    __func__, __FILE__, __LINE__);
      abort();
   }
   n->next             = NULL;
   n->page_id          = id;
   *GLOBAL_INSERT_PAGE = n;
   GLOBAL_INSERT_PAGE  = &n->next;

   GLOBAL_NUM_PAGES++;
 }
/* ============================================================================================================================== */
 static void start_object(int id){
        if(id >= GLOBAL_NUM_XREFS) {
           long *new_xrefs;
           int delta, new_num_xrefs;
           delta = GLOBAL_NUM_XREFS / 5;

           if(delta < 1000) {
              delta += 1000;
           }

           new_num_xrefs = GLOBAL_NUM_XREFS + delta;
           new_xrefs = (long *)malloc(new_num_xrefs * sizeof(*new_xrefs));

           if(new_xrefs == NULL) {
              (void)fprintf(stderr, "\r\nERROR: Unable to allocate array for object %d.\r\n", id);
              (void)fprintf(stderr, "\rFATAL: Bugcheck! Aborting at %s[%s:%d]\r\n",
                            __func__, __FILE__, __LINE__);
              abort();
           }

           if(GLOBAL_XREFS == NULL) {
              (void)fprintf(stderr, "\r\nERROR: GLOBAL_XREFS == NULL!\r\n");
              (void)fprintf(stderr, "\rFATAL: Bugcheck! Aborting at %s[%s:%d]\r\n",
                            __func__, __FILE__, __LINE__);
              abort();
           }

           (void)memcpy(new_xrefs, GLOBAL_XREFS, GLOBAL_NUM_XREFS * sizeof(*GLOBAL_XREFS));
           FREE(GLOBAL_XREFS);
           GLOBAL_XREFS = new_xrefs;
           GLOBAL_NUM_XREFS = new_num_xrefs;
        }

        GLOBAL_XREFS[id] = ftell(stdout);
        (void)printf("%d 0 obj\n", id);

 }
/* ============================================================================================================================== */
 static void print_bars(void){
        float x1;
        float yyy1;
        float height;
        float width;
        float step;

        if (GLOBAL_GREEN_BAR)
          (void)fprintf(stdout,"0.60 0.82 0.60 rg\n"); /* green */
        else
          (void)fprintf(stdout,"%f g\n",GLOBAL_GRAY_SCALE); /* gray-scale value */
        /*
        * If you want to add color,
        * R G B rg where R G B are red, green, blue components
        * in range 0.0 to 1.0 sets fill color, "RG" sets line
        * color instead of fill color.
        *
        * 0.60 0.82 0.60 rg
        *
        * */

        (void)fprintf(stdout,"%d i\n",1); /*  */

        x1=GLOBAL_PAGE_MARGIN_LEFT-0.1*GLOBAL_FONT_SIZE;
        height=GLOBAL_SHADE_STEP*GLOBAL_LEAD_SIZE;
        yyy1 = GLOBAL_PAGE_DEPTH - GLOBAL_PAGE_MARGIN_TOP - height- 0.22*GLOBAL_FONT_SIZE;
        width=GLOBAL_PAGE_WIDTH-GLOBAL_PAGE_MARGIN_LEFT-GLOBAL_PAGE_MARGIN_RIGHT;
        step=1.0;
        if(GLOBAL_DASHCODE[0] != '\0'){
           (void)fprintf(stdout, "0 w [%s] 0 d\n",GLOBAL_DASHCODE); /* dash code array plus offset */
        }
         /*
         8.4.3.6       Line Dash Pattern

         The line dash pattern shall control the pattern of dashes and gaps used to stroke paths. It shall be specified by
         a dash array and a dash phase. The dash array's elements shall be numbers that specify the lengths of
         alternating dashes and gaps; the numbers shall be nonnegative and not all zero. The dash phase shall specify
         the distance into the dash pattern at which to start the dash. The elements of both the dash array and the dash
         phase shall be expressed in user space units.

         Before beginning to stroke a path, the dash array shall be cycled through, adding up the lengths of dashes and
         gaps. When the accumulated length equals the value specified by the dash phase, stroking of the path shall
         begin, and the dash array shall be used cyclically from that point onward. Table 56 shows examples of line
         dash patterns. As can be seen from the table, an empty dash array and zero phase can be used to restore the
         dash pattern to a solid line.

                                               Table 56 - Examples of Line Dash Patterns

                              Dash Array       Appearance                   Description
                              and Phase

                              [] 0                                          No dash; solid, unbroken lines

                              [3] 0                                         3 units on, 3 units off, ...

                              [2] 1                                         1 on, 2 off, 2 on, 2 off, ...

                              [2 1] 0                                       2 on, 1 off, 2 on, 1 off, ...

                              [3 5] 6                                       2 off, 3 on, 5 off, 3 on, 5 off, ...

                              [ 2 3 ] 11                                    1 on, 3 off, 2 on, 3 off, 2 on, ...

         Dashed lines shall wrap around curves and corners just as solid stroked lines do. The ends of each dash shall
         be treated with the current line cap style, and corners within dashes shall be treated with the current line join
         style. A stroking operation shall take no measures to coordinate the dash pattern with features of the path; it
         simply shall dispense dashes and gaps along the path in the pattern defined by the dash array.

         When a path consisting of several subpaths is stroked, each subpath shall be treated independently--that is,
         the dash pattern shall be restarted and the dash phase shall be reapplied to it at the beginning of each subpath.
         */

        while ( yyy1 >= (GLOBAL_PAGE_MARGIN_BOTTOM-height) ){
           if(GLOBAL_DASHCODE[0] ==  '\0'){
                /* a shaded bar */
                 (void)fprintf(stdout,"%f %f %f %f re f\n",x1,yyy1,width,height);
                 step=2.0;
                /*
                 * x1 yyy1 m x2 y2 l S
                 * xxx w  # line width
                 (void)fprintf(stdout,"0.6 0.8 0.6 RG\n %f %f m %f %f l S\n",x1,yyy1,x1+width,yyy1);
                 */
           }else{
                  (void)fprintf(stdout, "%f %f m ", x1 ,yyy1);
                  (void)fprintf(stdout, "%f %f l s\n",x1+width,yyy1);
           }
           yyy1=yyy1-step*height;
        }
        if(GLOBAL_DASHCODE[0] != '\0'){
           (void)fprintf(stdout, "[] 0 d\n"); /* set dash pattern to solid line */
        }

        (void)fprintf(stdout,"%d G\n",0); /* */
        (void)fprintf(stdout,"%d g\n",0); /* gray-scale value */

 }
/* ============================================================================================================================== */
 static void printstring(char *buffer){
 /* Print string as (escaped_string) where ()\ have a preceding \ character added */
        char c;
        (void)putchar('(');
        if(GLOBAL_LINENUMBERS != 0){
        (void)fprintf(stdout,"%6d ",GLOBAL_LINECOUNT);
        }
        while((c = *buffer++) != '\0') {
              switch(c+GLOBAL_ADD) {
                 case '(':
                 case ')':
                 case '\\':
                    (void)putchar('\\');
              }
              (void)putchar(c+GLOBAL_ADD);
        }
        (void)putchar(')');
 }
/* ============================================================================================================================== */
 static void printme(float xvalue,float yvalue,char *string){
        //float charwidth;
        //float start;
        (void)fprintf(stdout,"BT /F2 %f Tf %f %f Td",GLOBAL_TITLE_SIZE,xvalue,yvalue);
        printstring(string);
        (void)fprintf(stdout," Tj ET\n");
 }
/* ============================================================================================================================== */
 static void printme_top(void){
        char *varname;
        if( (varname=getenv("IMPACT_TOP")) != (char *)NULL ) {
           char IMPACT_TOP[256];
           float text_size=20.0;
           float charwidth;
           float xvalue;
           float yvalue;
           (void)strncpy(IMPACT_TOP,varname,255);
           charwidth=text_size*0.60; /* assuming fixed-space font Courier-Bold */
           (void)fprintf(stdout,"1.0 0.0 0.0 rg\n"); /* gray-scale value */
           yvalue=GLOBAL_PAGE_DEPTH-text_size;
           xvalue=GLOBAL_PAGE_MARGIN_LEFT
              +((GLOBAL_PAGE_WIDTH-GLOBAL_PAGE_MARGIN_LEFT-GLOBAL_PAGE_MARGIN_RIGHT)/2.0)
              -(strlen(IMPACT_TOP)*charwidth/2.0);

           (void)fprintf(stdout,"BT /F2 %f Tf %f %f Td",text_size,xvalue,yvalue);
           printstring(IMPACT_TOP);
           (void)fprintf(stdout," Tj ET\n");

           (void)fprintf(stdout,"0.0 0.0 0.0 rg\n"); /* gray-scale value */
        }
 }
/* ============================================================================================================================== */
 static void print_margin_label(void){
     float charwidth;
     float start;
     int hold;
     hold=GLOBAL_LINENUMBERS;
     GLOBAL_LINENUMBERS=0;

     printme_top();

     if(GLOBAL_CENTER_TITLE[0] != 0 /*NULL*/ ){
        /* assuming fixed-space font Courier-Bold */
        charwidth=GLOBAL_TITLE_SIZE*0.60;
        start=GLOBAL_PAGE_MARGIN_LEFT
           +((GLOBAL_PAGE_WIDTH-GLOBAL_PAGE_MARGIN_LEFT-GLOBAL_PAGE_MARGIN_RIGHT)/2.0)
           -(strlen(GLOBAL_CENTER_TITLE)*charwidth/2.0);

        printme(start,GLOBAL_PAGE_DEPTH-GLOBAL_PAGE_MARGIN_TOP+0.12*GLOBAL_TITLE_SIZE,GLOBAL_CENTER_TITLE);
        printme(start,GLOBAL_PAGE_MARGIN_BOTTOM-GLOBAL_TITLE_SIZE,GLOBAL_CENTER_TITLE);
     }

     if(GLOBAL_PAGES != 0 ) {
        char line[80];
        charwidth=GLOBAL_TITLE_SIZE*0.60; /* assuming fixed-space font Courier-Bold */
        (void)sprintf(line,"Page %4d",GLOBAL_PAGECOUNT);
        start=((GLOBAL_PAGE_WIDTH-GLOBAL_PAGE_MARGIN_RIGHT)-(strlen(line)*charwidth)); /* Right Justified */
        printme(start,GLOBAL_PAGE_DEPTH-GLOBAL_PAGE_MARGIN_TOP+0.12*GLOBAL_TITLE_SIZE,line);
        printme(start,GLOBAL_PAGE_MARGIN_BOTTOM-GLOBAL_TITLE_SIZE,line);
     }

     if(GLOBAL_LEFT_TITLE[0] != '\0' ){
        start=GLOBAL_PAGE_MARGIN_LEFT; /* Left justified */
        printme(start,GLOBAL_PAGE_DEPTH-GLOBAL_PAGE_MARGIN_TOP+0.12*GLOBAL_TITLE_SIZE,GLOBAL_LEFT_TITLE);
        printme(start,GLOBAL_PAGE_MARGIN_BOTTOM-GLOBAL_TITLE_SIZE,GLOBAL_LEFT_TITLE);
     }
     GLOBAL_LINENUMBERS=hold;

 }
/* ============================================================================================================================== */
 static void start_page(void) {
   GLOBAL_STREAM_ID = GLOBAL_OBJECT_ID++;
   GLOBAL_STREAM_LEN_ID = GLOBAL_OBJECT_ID++;
   GLOBAL_PAGECOUNT++;
   start_object(GLOBAL_STREAM_ID);
   (void)printf("<< /Length %d 0 R >>\n", GLOBAL_STREAM_LEN_ID);
   (void)printf("stream\n");
   GLOBAL_STREAM_START = ftell(stdout);
   print_bars();
   print_margin_label();
   (void)printf("BT\n/F0 %g Tf\n", GLOBAL_FONT_SIZE);
   GLOBAL_YPOS = GLOBAL_PAGE_DEPTH - GLOBAL_PAGE_MARGIN_TOP;
   (void)printf("%g %g Td\n", GLOBAL_PAGE_MARGIN_LEFT, GLOBAL_YPOS);
   (void)printf("%g TL\n", GLOBAL_LEAD_SIZE);
 }
/* ============================================================================================================================== */
 static void end_page(void){
    long stream_len;
    int page_id = GLOBAL_OBJECT_ID++;

    store_page(page_id);
    (void)printf("ET\n");
    stream_len = ftell(stdout) - GLOBAL_STREAM_START;
    (void)printf("endstream\nendobj\n");
    start_object(GLOBAL_STREAM_LEN_ID);
    (void)printf("%ld\nendobj\n", (long) stream_len);
    start_object(page_id);
    (void)printf("<</Type/Page/Parent %d 0 R/Contents %d 0 R>>\nendobj\n", GLOBAL_PAGE_TREE_ID, GLOBAL_STREAM_ID);
 }
/* ============================================================================================================================== */
 static void increment_ypos(float mult){
   if (GLOBAL_YPOS < GLOBAL_PAGE_DEPTH - GLOBAL_PAGE_MARGIN_TOP ){  /* if not at top of page */
      GLOBAL_YPOS += GLOBAL_LEAD_SIZE*mult;
   }
}
/* ============================================================================================================================== */
 static void do_text (void)
  {
    char buffer [8192];
    //char ASA;
    int c;
    int i, ic;

    start_page ();

    for(i = 0; i < GLOBAL_SHIFT; i ++)
      {
        buffer [i] = ' ';
      }

    while ((ic = getchar ()) != EOF)
      {
        c = ic;
        if (c == '\r') // print the buffer, do not advance
          {
            (void)fprintf (stdout, "0 %f Td\n", GLOBAL_LEAD_SIZE);
            increment_ypos (1.0);
            goto printline;
          }
        if (c == '\n') // print the buffer. advance
          {
            goto printline;
          }
        if (c == '\f') // print the buffer, advance to next page
          {
            goto printline;
          }
        // add the character to the buffer
        if (i < 8190)
          buffer [i ++] = (char) c;
        continue;

printline:
        GLOBAL_LINECOUNT ++;
        GLOBAL_ADD=0;

        buffer [i ++] = 0;
        printstring (buffer);
        (void)printf ("'\n");

        if (c == '\f')
          {
            if (GLOBAL_YPOS < GLOBAL_PAGE_DEPTH - GLOBAL_PAGE_MARGIN_TOP )
              {
                end_page();
                start_page();
              }
          }
        GLOBAL_YPOS -= GLOBAL_LEAD_SIZE;
        i = GLOBAL_SHIFT;
      }
    end_page();
}
/* ============================================================================================================================== */
 static void dopages(void){
        int i, catalog_id, font_id0, font_id1;
        long start_xref;

        (void)printf("%%PDF-1.0\n");

        /*
           Note: If a PDF file contains binary data, as most do, it is
           recommended that the header line be immediately followed by a
           comment line containing at least four binary characters - that
           is, characters whose codes are 128 or greater. This will ensure
           proper behavior of file transfer applications that inspect data
           near the beginning of a file to determine whether to treat the
           file's contents as text or as binary.
        */

        (void)fprintf(stdout,"%%%c%c%c%c\n",128,129,130,131);
        (void)fprintf(stdout,"%% PDF: Adobe Portable Document Format\n");

        GLOBAL_LEAD_SIZE=(GLOBAL_PAGE_DEPTH-GLOBAL_PAGE_MARGIN_TOP-GLOBAL_PAGE_MARGIN_BOTTOM)/GLOBAL_LINES_PER_PAGE;
        GLOBAL_FONT_SIZE=GLOBAL_LEAD_SIZE;

        GLOBAL_OBJECT_ID = 1;
        GLOBAL_PAGE_TREE_ID = GLOBAL_OBJECT_ID++;

        do_text();

        font_id0 = GLOBAL_OBJECT_ID++;
        start_object(font_id0);
        (void)printf("<</Type/Font/Subtype/Type1/BaseFont/%s/Encoding/WinAnsiEncoding>>\nendobj\n",GLOBAL_FONT);

        font_id1 = GLOBAL_OBJECT_ID++;
        start_object(font_id1);
        (void)printf("<</Type/Font/Subtype/Type1/BaseFont/%s/Encoding/WinAnsiEncoding>>\nendobj\n",GLOBAL_FONT);

        start_object(GLOBAL_PAGE_TREE_ID);

        (void)printf("<</Type /Pages /Count %d\n", GLOBAL_NUM_PAGES);

        {
           PageList *ptr = GLOBAL_PAGE_LIST;
           (void)printf("/Kids[\n");
           while(ptr != NULL) {
              (void)printf("%d 0 R\n", ptr->page_id);
              ptr = ptr->next;
           }
           (void)printf("]\n");
        }

        (void)printf("/Resources<</ProcSet[/PDF/Text]/Font<</F0 %d 0 R\n", font_id0);
        (void)printf("/F1 %d 0 R\n", font_id1);
        (void)printf(" /F2<</Type/Font/Subtype/Type1/BaseFont/Courier-Bold/Encoding/WinAnsiEncoding >> >>\n");
        (void)printf(">>/MediaBox [ 0 0 %g %g ]\n", GLOBAL_PAGE_WIDTH, GLOBAL_PAGE_DEPTH);
        (void)printf(">>\nendobj\n");
        catalog_id = GLOBAL_OBJECT_ID++;
        start_object(catalog_id);
        (void)printf("<</Type/Catalog/Pages %d 0 R>>\nendobj\n", GLOBAL_PAGE_TREE_ID);
        start_xref = ftell(stdout);
        (void)printf("xref\n");
        (void)printf("0 %d\n", GLOBAL_OBJECT_ID);
        (void)printf("0000000000 65535 f \n");

        for(i = 1; i < GLOBAL_OBJECT_ID; i++){
           (void)printf("%010ld 00000 n \n", GLOBAL_XREFS[i]);
        }

        (void)printf("trailer\n<<\n/Size %d\n/Root %d 0 R\n>>\n", GLOBAL_OBJECT_ID, catalog_id);
        (void)printf("startxref\n%ld\n%%%%EOF\n", (long) start_xref);
 }
/* ============================================================================================================================== */
static void showhelp(int itype){
switch (itype){
case 1:
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |NAME:                                                                         |\n");
   (void)fprintf(stderr," |prt2pdf: A filter to convert text files with ASA carriage control to a PDF.   |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |SYNOPSIS:                                                                     |\n");
   (void)fprintf(stderr," |   prt2pdf(1) reads input from standard input. The first character            |\n");
   (void)fprintf(stderr," |   of each line is interpreted as a control character. Lines beginning with   |\n");
   (void)fprintf(stderr," |   any character other than those listed in the ASA carriage-control          |\n");
   (void)fprintf(stderr," |   characters table are interpreted as if they began with a blank,            |\n");
   (void)fprintf(stderr," |   and an appropriate diagnostic appears on standard error. The first         |\n");
   (void)fprintf(stderr," |   character of each line is not printed.                                     |\n");
   (void)fprintf(stderr," |     +------------+-----------------------------------------------+           |\n");
   (void)fprintf(stderr," |     | Character  |                                               |           |\n");
   (void)fprintf(stderr," |     +------------+-----------------------------------------------+           |\n");
   (void)fprintf(stderr," |     | +          | Do not advance; overstrike previous line.     |           |\n");
   (void)fprintf(stderr," |     | blank      | Advance one line.                             |           |\n");
   (void)fprintf(stderr," |     | null lines | Treated as if they started with a blank       |           |\n");
   (void)fprintf(stderr," |     | 0          | Advance two lines.                            |           |\n");
   (void)fprintf(stderr," |     | -          | Advance three lines (IBM extension).          |           |\n");
   (void)fprintf(stderr," |     | 1          | Advance to top of next page.                  |           |\n");
   (void)fprintf(stderr," |     | all others | Discarded (except for extensions listed below)|           |\n");
   (void)fprintf(stderr," |     +------------+-----------------------------------------------+           |\n");
   (void)fprintf(stderr," | Extensions                                                                   |\n");
   (void)fprintf(stderr," |    H  Advance one-half line.                                                 |\n");
   (void)fprintf(stderr," |    R  Do not advance; overstrike previous line. Use red text color           |\n");
   (void)fprintf(stderr," |    G  Do not advance; overstrike previous line. Use green text color         |\n");
   (void)fprintf(stderr," |    B  Do not advance; overstrike previous line. Use blue text color          |\n");
   (void)fprintf(stderr," |    r  Advance one line. Use red text color                                   |\n");
   (void)fprintf(stderr," |    g  Advance one line. Use green text color                                 |\n");
   (void)fprintf(stderr," |    b  Advance one line. Use blue text color                                  |\n");
   (void)fprintf(stderr," |    ^  Overprint but add 127 to the ADE value of the character                |\n");
   (void)fprintf(stderr," |       (ie., use ASCII extended character set)                                |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |PRINTABLE PAGE AREA                                                           |\n");
   (void)fprintf(stderr," !  The page size may be specified using -H for height, -W for width, and -u    |\n");
   (void)fprintf(stderr," !  to indicate the points per unit (72 makes H and W in inches,                |\n");
   (void)fprintf(stderr," !  1 is used when units are in font points). For example:                      |\n");
   (void)fprintf(stderr," |    -u 72 -H 8.5 -W 11   # page Height and Width in inches                    |\n");
   (void)fprintf(stderr," |    -u 72 -B 0.5 -L 0.5 -R 0.5 -T 0.5 # margins (Top, Bottom, Left, Right)    |\n");
   (void)fprintf(stderr," |  common media sizes with -u 1:                                               |\n");
   (void)fprintf(stderr," |    +-------------------+------+------------+                                 |\n");
   (void)fprintf(stderr," |    | name              |  W   |        H   |                                 |\n");
   (void)fprintf(stderr," |    +-------------------+------+------------+                                 |\n");
   (void)fprintf(stderr," |    | Letterdj (11x8.5) | 792  |       612  | (LandScape)                     |\n");
   (void)fprintf(stderr," |    | A4dj              | 842  |       595  |                                 |\n");
   (void)fprintf(stderr," |    | Letter (8.5x11)   | 612  |       792  | (Portrait)                      |\n");
   (void)fprintf(stderr," |    | Legal             | 612  |       1008 |                                 |\n");
   (void)fprintf(stderr," |    | A5                | 420  |       595  |                                 |\n");
   (void)fprintf(stderr," |    | A4                | 595  |       842  |                                 |\n");
   (void)fprintf(stderr," |    | A3                | 842  |       1190 |                                 |\n");
   (void)fprintf(stderr," |    +-------------------+------+------------+                                 |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |SHADING                                                                       |\n");
   (void)fprintf(stderr," |    -g 0.800781     # gray-scale value  for shaded bars ( 0 < g 1 )           |\n");
   (void)fprintf(stderr," |                    # 0 is black, 1 is white.                                 |\n");
   (void)fprintf(stderr," |    -i 2            # repeat shade pattern every N lines                      |\n");
   (void)fprintf(stderr," |    -d ' '          # dashcode pattern (seems buggy)                          |\n");
   (void)fprintf(stderr," |    -G              # green bar                                               |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |MARGIN LABELS                                                                 |\n");
   (void)fprintf(stderr," |   -s ''            # top middle page label.                                  |\n");
   (void)fprintf(stderr," |   -t ''            # top left page label.                                    |\n");
   (void)fprintf(stderr," |   -P               # add page numbers to right corners                       |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |TEXT OPTIONS                                                                  |\n");
   (void)fprintf(stderr," |   -l 60            # lines per page                                          |\n");
   (void)fprintf(stderr," |   -f Courier       # font names: Courier, Courier-Bold,Courier-Oblique       |\n");
   (void)fprintf(stderr," |                      Helvetica, Symbol, Times-Bold, Helvetica-Bold,          |\n");
   (void)fprintf(stderr," |                      ZapfDingbats, Times-Italic, Helvetica-Oblique,          |\n");
   (void)fprintf(stderr," |                      Times-BoldItalic, Helvetica-BoldOblique,                |\n");
   (void)fprintf(stderr," |                      Times-Roman, Courier-BoldOblique                        |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |   -S 0             # right shift 1 for non-ASA files                         |\n");
   (void)fprintf(stderr," |   -N               # add line numbers                                        |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |   -v 2             # version number                                          |\n");
   (void)fprintf(stderr," |   -h               # display this help                                       |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |ENVIRONMENT VARIABLES:                                                        |\n");
   (void)fprintf(stderr," | $IMPACT_TOP Will be printed in large red letters across the page top.        |\n");
   (void)fprintf(stderr," | $IMPACT_GRAY sets the default gray-scale value, same as the -g switch.       |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");
   (void)fprintf(stderr," |EXAMPLES:                                                                     |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # create non-ASA file in portrait mode with a dashed line under every line   |\n");
   (void)fprintf(stderr," | prt2pdf -S 1 -W 8.5 -H 11 -i 1 -d '2 4 1' -T 1 -B .75 < INFILE > junko.pdf   |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # banner on top                                                              |\n");
   (void)fprintf(stderr," | env IMPACT_GRAY=1 IMPACT_TOP=CONFIDENTIAL prt2pdf < test.txt > test.pdf      |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # 132 landscape                                                              |\n");
   (void)fprintf(stderr," |  prt2pdf -s LANDSCAPE < prt2pdf.c > junko.A.pdf                              |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # 132 landscape with line numbers with dashed lines                          |\n");
   (void)fprintf(stderr," |  prt2pdf -s 'LANDSCAPE LINE NUMBERS' -d '3 1 2' \\                            |\n");
   (void)fprintf(stderr," |  -N -T .9 < prt2pdf.c > test.pdf                                             |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # portrait 80 non-ASA file with dashed lines                                 |\n");
   (void)fprintf(stderr," |  prt2pdf -s PORTRAIT -S 1 -W 8.5 -H 11 -i 1 -d '2 4 1' \\                     |\n");
   (void)fprintf(stderr," |  -T 1 -B .75 < prt2pdf.c > test.pdf                                          |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # portrait 80 with line numbers , non-ASA                                    |\n");
   (void)fprintf(stderr," |  prt2pdf -s 'PORTRAIT LINE NUMBERS' -l 66 -S 1 -W 8.5 -H 11 \\                |\n");
   (void)fprintf(stderr," |  -i 1 -T 1 -B .75 -N < prt2pdf.c > test.pdf                                  |\n");
   (void)fprintf(stderr," !-----------------                                                             |\n");
   (void)fprintf(stderr," | # titling                                                                    |\n");
   (void)fprintf(stderr," |  prt2pdf -d '1 0 1' -t \"$USER\" -i 1 -P -N -T 1 \\                             |\n");
   (void)fprintf(stderr," |  -s \"prt2pdf.c\" < prt2pdf.c > test.pdf                                       |\n");
   (void)fprintf(stderr," +------------------------------------------------------------------------------+\n");

break;
case 2:
(void)fprintf (stderr,"-T %f # Top margin\n", GLOBAL_PAGE_MARGIN_TOP/GLOBAL_UNIT_MULTIPLIER);
(void)fprintf (stderr,"-B %f # Bottom margin\n", GLOBAL_PAGE_MARGIN_BOTTOM/GLOBAL_UNIT_MULTIPLIER);
(void)fprintf (stderr,"-L %f # Left margin\n", GLOBAL_PAGE_MARGIN_LEFT/GLOBAL_UNIT_MULTIPLIER);
(void)fprintf (stderr,"-R %f # Right margin\n", GLOBAL_PAGE_MARGIN_RIGHT/GLOBAL_UNIT_MULTIPLIER);

(void)fprintf (stderr,"-W %f # page Width\n", GLOBAL_PAGE_WIDTH/GLOBAL_UNIT_MULTIPLIER);
(void)fprintf (stderr,"-H %f # page Height\n", GLOBAL_PAGE_DEPTH/GLOBAL_UNIT_MULTIPLIER);

(void)fprintf (stderr,"-u %f # unit multiplier\n", GLOBAL_UNIT_MULTIPLIER);

(void)fprintf (stderr,"-g %f # shading gray scale value ([black]0 <= g <= 1[white]\n", GLOBAL_GRAY_SCALE);
(void)fprintf (stderr,"-i %d # shading line increment\n", GLOBAL_SHADE_STEP);
(void)fprintf (stderr,"-d %s # shading line dashcode\n", GLOBAL_DASHCODE);
(void)fprintf (stderr,"-G %d # green bar\n", GLOBAL_GREEN_BAR);

(void)fprintf (stderr,"-l %f # lines per page\n", GLOBAL_LINES_PER_PAGE);
(void)fprintf (stderr,"-f %s # font name\n", GLOBAL_FONT);

(void)fprintf (stderr,"-s %s # margin label\n", GLOBAL_CENTER_TITLE);
(void)fprintf (stderr,"-t %s # margin left label\n", GLOBAL_LEFT_TITLE);
(void)fprintf (stderr,"-S %d # right shift\n", GLOBAL_SHIFT);

(void)fprintf (stderr,"-N [flag=%d]   # add line numbers \n", GLOBAL_LINENUMBERS);
(void)fprintf (stderr,"-P [flag=%d]   # add page numbers\n", GLOBAL_PAGES);

(void)fprintf (stderr,"-v    # display version number\n");
(void)fprintf (stderr,"-V    # display build info\n");
(void)fprintf (stderr,"-h    # display help\n");
break;
}
}
/* ============================================================================================================================== */
int main(int argc, char **argv) {
/*
   How getopt is typically used. The key points to notice are:
     * Normally, getopt is called in a loop. When getopt returns -1,
       indicating no more options are present, the loop terminates.
     * A switch statement is used to dispatch on the return value from
       getopt. In typical use, each case just sets a variable that is
       used later in the program.
     * A second loop is used to process the remaining non-option
       arguments.
*/
   (void)setlocale(LC_ALL, "");

   char *varname;

   int prindex;
   int c;
   GLOBAL_PAGE_DEPTH         = 612.0;
   GLOBAL_PAGE_WIDTH         = 792.0;      /* Default is 72 points per inch */
   GLOBAL_PAGE_MARGIN_TOP    = 36.0 -24.0;
   GLOBAL_PAGE_MARGIN_BOTTOM = 36.0 -24.0;
   GLOBAL_PAGE_MARGIN_LEFT   = 40.0 -14.0;
   GLOBAL_PAGE_MARGIN_RIGHT  = 39.0 -14.0;
   GLOBAL_LINES_PER_PAGE     = 64.0;
   GLOBAL_GRAY_SCALE         = 0.800781; /* gray-scale value */
   GLOBAL_GREEN_BAR          = 0;

   varname=getenv("IMPACT_GRAY");
   if (varname == (char*)NULL ){
       GLOBAL_GRAY_SCALE=0.800781; /* gray-scale value */
   }else if (varname[0] == '\0'){
       GLOBAL_GRAY_SCALE=0.800781; /* gray-scale value */
   }else{
      (void)sscanf(varname,"%f",&GLOBAL_GRAY_SCALE);
      if(GLOBAL_GRAY_SCALE < 0 ){
          GLOBAL_GRAY_SCALE=0.800781; /* gray-scale value */
      }
   }

   GLOBAL_LEFT_TITLE[0]='\0';
   GLOBAL_CENTER_TITLE[0]='\0';

   opterr = 0;

   (void)strncpy(GLOBAL_DASHCODE,"",255);
   (void)strncpy(GLOBAL_FONT,"Courier",255);
   GLOBAL_TITLE_SIZE=20.0;

   while ((c = getopt (argc, argv, "B:d:f:g:H:hi:L:l:GNPR:s:S:t:T:u:W:vVX")) != -1)
         switch (c) {
           case 'L': GLOBAL_PAGE_MARGIN_LEFT =    strtod(optarg,NULL)*GLOBAL_UNIT_MULTIPLIER; break; /* Left margin              */
           case 'R': GLOBAL_PAGE_MARGIN_RIGHT =   strtod(optarg,NULL)*GLOBAL_UNIT_MULTIPLIER; break; /* Right margin             */
           case 'B': GLOBAL_PAGE_MARGIN_BOTTOM =  strtod(optarg,NULL)*GLOBAL_UNIT_MULTIPLIER; break; /* Bottom margin            */
           case 'T': GLOBAL_PAGE_MARGIN_TOP =     strtod(optarg,NULL)*GLOBAL_UNIT_MULTIPLIER; break; /* Top margin               */
           case 'H': GLOBAL_PAGE_DEPTH =          strtod(optarg,NULL)*GLOBAL_UNIT_MULTIPLIER; break; /* Height                   */
           case 'W': GLOBAL_PAGE_WIDTH =          strtod(optarg,NULL)*GLOBAL_UNIT_MULTIPLIER; break; /* Width                    */

           case 'g': GLOBAL_GRAY_SCALE =          strtod(optarg,NULL);                        break; /* grayscale value for bars */
           case 'G': GLOBAL_GREEN_BAR =           1;                                          break; /* green bars               */
           case 'l': GLOBAL_LINES_PER_PAGE=       strtod(optarg,NULL);                        break; /* lines per page           */
           case 'u': GLOBAL_UNIT_MULTIPLIER =     strtod(optarg,NULL);                        break; /* unit_divisor             */
           case 'i': GLOBAL_SHADE_STEP =          strtod(optarg,NULL);                        break; /* increment for bars       */
           case 'S': GLOBAL_SHIFT =               MAX(0,strtod(optarg,NULL));                 break; /* right shift              */

           case 's': (void)strncpy(GLOBAL_CENTER_TITLE,optarg,255);                           break; /* special label            */
           case 't': (void)strncpy(GLOBAL_LEFT_TITLE,optarg,255);                             break; /* margin left label        */

           case 'd': (void)strncpy(GLOBAL_DASHCODE,optarg,255);                               break; /* dash code                */
           case 'f': (void)strncpy(GLOBAL_FONT,optarg,255);                                   break; /* font                     */

           case 'N': GLOBAL_LINENUMBERS=1;                                                    break; /* number lines             */
           case 'P': GLOBAL_PAGES=1;                                                          break; /* number pages             */
           case 'h': showhelp(1);
                     exit(0);
                     /*NOTREACHED*/ /* unreachable */
                                                                                              break; /* help                     */
           case 'X': showhelp(2);
                                                                                              break;
           case 'V': ;
#if defined(__VERSION__)
# if defined(__GNUC__)
#  if !defined (__clang_version__) || defined(__INTEL_COMPILER)
                     char xcmp[2];
                     (void)sprintf(xcmp, "%.1s", __VERSION__ );
                     if (!isdigit((int)xcmp[0]))
                     {
                         (void)fprintf (stderr, "Compiler: %s\n", __VERSION__ );
                     } else {
                         (void)fprintf (stderr, "Compiler: GCC %s\n", __VERSION__ );
                     }
#  else
                     (void)fprintf (stderr, "Compiler: Clang %s\n", __clang_version__ );
#  endif /* if !defined(__clang_version__) || defined(__INTEL_COMPILER)  */
# else
                     (void)fprintf (stderr, "Compiler: %s\n", __VERSION__ );
# endif /* if defined(__GNUC__) */
#endif /* if defined(__VERSION__) */
                     exit(0);
                     /*NOTREACHED*/ /* unreachable */
                                                                                              break; /* build info               */
           case 'v': (void)fprintf (stderr, "prt2pdf version %d.0.%d\n",GLOBAL_VERSION,GLOBAL_VERSION_PATCH);
                     exit(0);
                     /*NOTREACHED*/ /* unreachable */
                                                                                              break; /* version                  */
           case '?':
             (void)fprintf(stderr," SWITCH IS %c\n",c);
             if (isprint (optopt)){
               (void)fprintf (stderr, "Unknown option `-%c'.\n", optopt);
             }else{
               (void)fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
             }
             showhelp(2);
             _Exit(1);
             /*NOTREACHED*/ /* unreachable */
#if !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) && !defined(__SUNPRO_CC_COMPAT)
             return 1;
#endif /* if !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) && !defined(__SUNPRO_CC_COMPAT) */
             /*NOTREACHED*/ /* unreachable */
           default:
             (void)fprintf (stderr, "\rFATAL: Bugcheck! Aborting at %s[%s:%d]\r\n",
                            __func__, __FILE__, __LINE__);
             abort();
             /*NOTREACHED*/ /* unreachable */
#if !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) && !defined(__SUNPRO_CC_COMPAT)
             return 1;
#endif /* if !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) && !defined(__SUNPRO_CC_COMPAT) */
             /*NOTREACHED*/ /* unreachable */
           }

           if(GLOBAL_SHADE_STEP < 1 ){
              (void)fprintf(stderr,"W-A-R-N-I-N-G: prt2pdf(1) resetting -i %d to -i 1\n",GLOBAL_SHADE_STEP);
              GLOBAL_SHADE_STEP=1;
           }

           for (prindex = optind; prindex < argc; prindex++){
              (void)fprintf (stderr,"Non-option argument %s\n", argv[prindex]);
           }
           dopages();
           exit(0);
}
/* ============================================================================================================================== */
