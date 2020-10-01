#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dps8.h"
#include "dps8_cpu.h"

#include "trace_record.h"

static char * source_search_path = "listings~/MR12.6f";

// Given a component name and an offset in the component,
// find the listing file of the component and try to
// print the source code line that generated the code at
// component:offset

static void list_source (char * compname, word18 offset)
  {
    const int offset_str_len = 10;
    //char offset_str[offset_str_len + 1];
    char offset_str[17];
    sprintf (offset_str, "    %06o", offset);

    char path[(source_search_path ? strlen (source_search_path) : 1) + 
               1 + // "/"
               (compname ? strlen (compname) : 1) +
                1 + strlen (".list") + 1];
    char * searchp = source_search_path ? source_search_path : ".";
    // find <search path>/<compname>.list
    while (* searchp)
      {
        size_t pathlen = strcspn (searchp, ":");
        strncpy (path, searchp, pathlen);
        path[pathlen] = '\0';
        if (searchp[pathlen] == ':')
          searchp += pathlen + 1;
        else
          searchp += pathlen;

        if (compname)
          {
            strcat (path, "/");
            strcat (path, compname);
          }
        strcat (path, ".list");
        //sim_msg ("<%s>\n", path);
        FILE * listing = fopen (path, "r");
        if (listing)
          {
            char line[1025];
            if (feof (listing))
              goto fileDone;
            fgets (line, 1024, listing);
            if (strncmp (line, "ASSEMBLY LISTING", 16) == 0)
              {
                // Search ALM listing file
                // sim_msg ("found <%s>\n", path);

                // ALM listing files look like:
                //     000226  4a  4 00010 7421 20  \tstx2]tbootload_0$entry_stack_ptr,id
                while (! feof (listing))
                  {
                    fgets (line, 1024, listing);
                    if (strncmp (line, offset_str, (size_t) offset_str_len) == 0)
                      {
                        printf ("%s", line);
                        //break;
                      }
                    if (strcmp (line, "\fLITERALS\n") == 0)
                      break;
                  }
              } // if assembly listing
            else if (strncmp (line, "\tCOMPILATION LISTING", 20) == 0)
              {
                // Search PL/I listing file

                // PL/I files have a line location table
                //     "   LINE    LOC      LINE    LOC ...."

                bool foundTable = false;
                while (! feof (listing))
                  {
                    fgets (line, 1024, listing);
                    if (strncmp (line, "   LINE    LOC", 14) != 0)
                      continue;
                    foundTable = true;
                    // Found the table
                    // Table lines look like
                    //     "     13 000705       275 000713  ...
                    // But some times
                    //     "     10 000156   21   84 000164
                    //     "      8 000214        65 000222    4   84 000225    
                    //
                    //     "    349 001442       351 001445       353 001454    1    9 001456    1   11 001461    1   12 001463    1   13 001470
                    //     " 1   18 001477       357 001522       361 001525       363 001544       364 001546       365 001547       366 001553

                    //  I think the numbers refer to include files...
                    //   But of course the format is slightly off...
                    //    table    ".1...18
                    //    listing  ".1....18
                    int best = -1;
                    char bestLines[8] = {0, 0, 0, 0, 0, 0, 0};
                    while (! feof (listing))
                      {
                        int loc[7];
                        char linenos[7][8];
                        memset (linenos, 0, sizeof (linenos));
                        fgets (line, 1024, listing);
                        // sometimes the leading columns are blank...
                        while (strncmp (line,
                                        "                 ", 8 + 6 + 3) == 0)
                          memmove (line, line + 8 + 6 + 3,
                                   strlen (line + 8 + 6 + 3));
                        // deal with the extra numbers...

                        int cnt = sscanf (line,
                          // " %d %o %d %o %d %o %d %o %d %o %d %o %d %o", 
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o", 
                          (char *) & linenos[0], & loc[0], 
                          (char *) & linenos[1], & loc[1], 
                          (char *) & linenos[2], & loc[2], 
                          (char *) & linenos[3], & loc[3], 
                          (char *) & linenos[4], & loc[4], 
                          (char *) & linenos[5], & loc[5], 
                          (char *) & linenos[6], & loc[6]);
                        if (! (cnt == 2 || cnt == 4 || cnt == 6 ||
                               cnt == 8 || cnt == 10 || cnt == 12 ||
                               cnt == 14))
                          break; // end of table
                        int n;
                        for (n = 0; n < cnt / 2; n ++)
                          {
                            if (loc[n] > best && loc[n] <= (int) offset)
                              {
                                best = loc[n];
                                memcpy (bestLines, linenos[n],
                                        sizeof (bestLines));
                              }
                          }
                        if (best == (int) offset)
                          break;
                      }
                    if (best == -1)
                      goto fileDone; // Not found in table

                    //   But of course the format is slightly off...
                    //    table    ".1...18
                    //    listing  ".1....18
                    // bestLines "21   84 "
                    // listing   " 21    84 "
                    char searchPrefix[10];
                    searchPrefix[0] = ' ';
                    searchPrefix[1] = bestLines[0];
                    searchPrefix[2] = bestLines[1];
                    searchPrefix[3] = ' ';
                    searchPrefix[4] = bestLines[2];
                    searchPrefix[5] = bestLines[3];
                    searchPrefix[6] = bestLines[4];
                    searchPrefix[7] = bestLines[5];
                    searchPrefix[8] = bestLines[6];
                    // ignore trailing space; some times its a tab
                    // searchPrefix[ 9] = bestLines[ 7];
                    searchPrefix[9] = '\0';

                    // Look for the line in the listing
                    rewind (listing);
                    while (! feof (listing))
                      {
                        fgets (line, 1024, listing);
                        if (strncmp (line, "\f\tSOURCE", 8) == 0)
                          goto fileDone; // end of source code listing
                        char prefix[10];
                        strncpy (prefix, line, 9);
                        prefix[9] = '\0';
                        if (strcmp (prefix, searchPrefix) != 0)
                          continue;
                        // Got it
                        printf ("%s", line);
                        //break;
                      }
                    goto fileDone;
                  } // if table start
                if (! foundTable)
                  {
                    // Can't find the LINE/LOC table; look for listing
                    rewind (listing);
                    while (! feof (listing))
                      {
                        fgets (line, 1024, listing);
                        if (strncmp (line,
                                     offset_str + 4,
                                     offset_str_len - 4) == 0)
                          {
                            printf ("%s", line);
                            //break;
                          }
                        //if (strcmp (line, "\fLITERALS\n") == 0)
                          //break;
                      }
                  } // if ! tableFound
              } // if PL/I listing
                        
fileDone:
            fclose (listing);
          } // if (listing)
      }
  }
#define BOOT_SEGMENTS_MAX 1024
#define BOOT_COMPONENTS_MAX 4096
#define BOOK_SEGMENT_NAME_LEN 33

static struct book_segment
  {
    char * segname;
    int segno;
  } book_segments[BOOT_SEGMENTS_MAX];

static int n_book_segments = 0;

static struct book_component
  {
    char * compname;
    int book_segment_number;
    uint txt_start, txt_length;
    int intstat_start, intstat_length, symbol_start, symbol_length;
  } book_components[BOOT_COMPONENTS_MAX];

static int n_book_components = 0;


static int lookup_book_segment (char * name)
  {
    for (int i = 0; i < n_book_segments; i ++)
      if (strcmp (name, book_segments[i].segname) == 0)
        return i;
    return -1;
  }

static int add_book_segment (char * name, int segno)
  {
    int n = lookup_book_segment (name);
    if (n >= 0)
      return n;
    if (n_book_segments >= BOOT_SEGMENTS_MAX)
      return -1;
    book_segments[n_book_segments].segname = strdup (name);
    book_segments[n_book_segments].segno = segno;
    n = n_book_segments;
    n_book_segments ++;
    return n;
  }
 
static int add_book_component (int segnum, char * name, uint txt_start,
                               uint txt_length, int intstat_start, 
                               int intstat_length, int symbol_start,
                               int symbol_length)
  {
    if (n_book_components >= BOOT_COMPONENTS_MAX)
      return -1;
    book_components[n_book_components].compname = strdup (name);
    book_components[n_book_components].book_segment_number = segnum;
    book_components[n_book_components].txt_start = txt_start;
    book_components[n_book_components].txt_length = txt_length;
    book_components[n_book_components].intstat_start = intstat_start;
    book_components[n_book_components].intstat_length = intstat_length;
    book_components[n_book_components].symbol_start = symbol_start;
    book_components[n_book_components].symbol_length = symbol_length;
    int n = n_book_components;
    n_book_components ++;
    return n;
  }
 
// Warning: returns ptr to static buffer
static char * lookup_system_book_address (word18 segno, word18 offset,
                                         char * * compname, word18 * compoffset)
  {
    static char buf[129];
    int i;

    for (i = 0; i < n_book_segments; i ++)
      if (book_segments[i].segno == (int) segno)
        break;

    if (i >= n_book_segments)
      return NULL;

    int best = -1;
    uint bestoffset = 0;

    for (int j = 0; j < n_book_components; j ++)
      {
        if (book_components[j].book_segment_number != i)
          continue;
        if (book_components[j].txt_start <= offset &&
            book_components[j].txt_start + book_components[j].txt_length > offset)
          {
            sprintf (buf, "%s:%s+0%0o", book_segments[i].segname,
              book_components[j].compname,
              offset - book_components[j].txt_start);
            if (compname)
              * compname = book_components[j].compname;
            if (compoffset)
              * compoffset = offset - book_components[j].txt_start;
            return buf;
          }
        if (book_components[j].txt_start <= offset &&
            book_components[j].txt_start > bestoffset)
          {
            best = j;
            bestoffset = book_components[j].txt_start;
          }
      }

    if (best != -1)
      {
        // Didn't find a component track bracketed the offset; return the
        // component that was before the offset
        if (compname)
          * compname = book_components[best].compname;
        if (compoffset)
          * compoffset = offset - book_components[best].txt_start;
        sprintf (buf, "%s:%s+0%0o", book_segments[i].segname,
          book_components[best].compname,
          offset - book_components[best].txt_start);
        return buf;
      }

    // Found a segment, but it had no components. Return the segment name
    // as the component name

    if (compname)
      * compname = book_segments[i].segname;
    if (compoffset)
      * compoffset = offset;
    sprintf (buf, "%s:+0%0o", book_segments[i].segname,
             offset);
    return buf;
  }

static char * lookup_address (word18 segno, word18 offset, char * * compname,
                       word18 * compoffset)
  {
    if (compname)
      * compname = NULL;
    if (compoffset)
      * compoffset = 0;

    // Magic numbers!
    // Multics seems to have a copy of hpchs_ (segno 0162) in segment 0322;
    // This little tweak allows source code level tracing for segment 0322,
    // and has no operational significance to the emulator
    // Hmmm. What is happening is that these segments are being loaded into
    // ring 4, and assigned segment #'s; the assigned number will vary 
    // depending on the exact sequence of events.
    if (segno == 0322)
      segno = 0162;
    if (segno == 0310)
      segno = 041;
    if (segno == 0314)
      segno = 041;
    if (segno == 0313)
      segno = 040;
    if (segno == 0317)
      segno = 0161;

#if 0
    // Hack to support formline debugging
#define IOPOS 02006 // interpret_op_ptr_ offset
    if (segno == 0371)
      {
        if (offset < IOPOS)
          {
            if (compname)
              * compname = "find_condition_info_";
            if (compoffset)
              * compoffset = offset;
            static char buf[129];
            sprintf (buf, "bound_debug_util_:find_condition_info_+0%0o", 
                  offset - 0);
            return buf;
          }
        else
          {
            if (compname)
              * compname = "interpret_op_ptr_";
            if (compoffset)
              * compoffset = offset - IOPOS;
            static char buf[129];
            sprintf (buf, "bound_debug_util_:interpret_op_ptr_+0%0o", 
                  offset - IOPOS);
            return buf;
          }

      }
#endif

    char * ret = lookup_system_book_address (segno, offset, compname, compoffset);
    return ret;
  }

static t_stat load_system_book (const char * buf)
  {
// Quietly ignore if not debug enabled
    // Multics 12.5 assigns segment number to collection 3 starting at 0244.
    uint c3 = 0244;

#define bufSz 257
    char filebuf[bufSz];
    int current = -1;

    FILE * fp = fopen (buf, "r");
    if (! fp)
      {
        printf ("error opening file %s\n", buf);
        return SCPE_ARG;
      }
    for (;;)
      {
        char * bufp = fgets (filebuf, bufSz, fp);
        if (! bufp)
          break;
        //sim_msg ("<%s\n>", filebuf);
        char name[BOOK_SEGMENT_NAME_LEN];
        int segno, p0, p1, p2;

        // 32 is BOOK_SEGMENT_NAME_LEN - 1
        int cnt = sscanf (filebuf, "%32s %o  (%o, %o, %o)", name, & segno, 
          & p0, & p1, & p2);
        if (filebuf[0] != '\t' && cnt == 5)
          {
            //sim_msg ("A: %s %d\n", name, segno);
            int rc = add_book_segment (name, segno);
            if (rc < 0)
              {
                printf ("error adding segment name\n");
                fclose (fp);
                return SCPE_ARG;
              }
            continue;
          }
        else
          {
            // Check for collection 3 segment
            // 32 is BOOK_SEGMENT_NAME_LEN - 1
            cnt = sscanf (filebuf, "%32s  (%o, %o, %o)", name, 
              & p0, & p1, & p2);
            if (filebuf[0] != '\t' && cnt == 4)
              {
                if (strstr (name, "fw.") || strstr (name, ".ec"))
                  continue;
                //sim_msg ("A: %s %d\n", name, segno);
                int rc = add_book_segment (name, (int) (c3 ++));
                if (rc < 0)
                  {
                    printf ("error adding segment name\n");
                    fclose (fp);
                    return SCPE_ARG;
                  }
                continue;
              }
          }
        cnt = sscanf (filebuf, "Bindmap for >ldd>h>e>%32s", name);
        if (cnt != 1)
          cnt = sscanf (filebuf, "Bindmap for >ldd>hard>e>%32s", name);
        if (cnt == 1)
          {
            //sim_msg ("B: %s\n", name);
            //int rc = add_book_segment (name);
            int rc = lookup_book_segment (name);
            if (rc < 0)
              {
                // The collection 3.0 segments do not have segment numbers,
                // and the 1st digit of the 3-tuple is 1, not 0. Ignore
                // them for now.
                current = -1;
                continue;
                //sim_warn ("error adding segment name\n");
                //return SCPE_ARG;
              }
            current = rc;
            continue;
          }

        uint txt_start, txt_length;
        int intstat_start, intstat_length, symbol_start, symbol_length;
        cnt = sscanf (filebuf, "%32s %o %o %o %o %o %o", name, & txt_start,
                      & txt_length, & intstat_start, & intstat_length,
                      & symbol_start, & symbol_length);

        if (cnt == 7)
          {
            //sim_msg ("C: %s\n", name);
            if (current >= 0)
              {
                add_book_component (current, name, txt_start, txt_length,
                                    intstat_start, intstat_length, symbol_start,
                                    symbol_length);
              }
            continue;
          }

        cnt = sscanf (filebuf, "%32s %o  (%o, %o, %o)", name, & segno, 
          & p0, & p1, & p2);
        if (filebuf[0] == '\t' && cnt == 5)
          {
            //sim_msg ("D: %s %d\n", name, segno);
            int rc = add_book_segment (name, segno);
            if (rc < 0)
              {
                printf ("error adding segment name\n");
                fclose (fp);
                return SCPE_ARG;
              }
            continue;
          }

      }
    fclose (fp);
#if 0
    for (int i = 0; i < n_book_segments; i ++)
      { 
        sim_msg ("  %-32s %6o\n", book_segments[i].segname,
                    book_segments[i].segno);
        for (int j = 0; j < n_book_components; j ++)
          {
            if (book_components[j].book_segment_number == i)
              {
                printf ("    %-32s %6o %6o %6o %6o %6o %6o\n",
                  book_components[j].compname, 
                  book_components[j].txt_start, 
                  book_components[j].txt_length, 
                  book_components[j].intstat_start, 
                  book_components[j].intstat_length, 
                  book_components[j].symbol_start, 
                  book_components[j].symbol_length);
              }
          }
      }
#endif
    return SCPE_OK;
  }


static char last_compname [129] = "";

void traceInstruction (int cnt, struct record cpu, uint cpun)
  {
    char * compname;
    word18 compoffset;
    char * where = lookup_address (cpu.PPR.PSR, cpu.PPR.IC, & compname,
                                   & compoffset);
    if (!where)
      where = "";

    if (!compname)
      compname = "";

    if (strcmp (last_compname, compname))
      {
        printf ("\n%s\n\n", compname);
        strncpy (last_compname, compname, sizeof (last_compname));
      }

    printf ("%6d %d %06o %s\n", cnt, cpun, cpu.PPR.IC, where);
    list_source (compname, compoffset);
  }

int main (int argc, char * argv[])
  {
    load_system_book ("listings~/MR12.6f/system_book_12.6f");


    struct record rec;
    for (;;)
      {
//       2 00312:000110 1 000200
        uint cpun;
        int cnt;

        int rc = scanf ("%d %ho:%o %d %o", & cnt, & rec.PPR.PSR, & rec.PPR.IC, & cpun, & rec.cu.IR);
        if (rc != 5)
          break;
        //printf ("%u %05o:%08o\n", rec.cpun, rec.PPR.PSR, rec.PPR.IC);
        traceInstruction (cnt, & rec, cpun);
      }
    return 0;
  }


