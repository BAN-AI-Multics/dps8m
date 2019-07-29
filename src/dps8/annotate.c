#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef unsigned int uint;
typedef uint32_t word18;

// LOAD_SYSTEM_BOOK <filename>
#define BK "listings~/MR12.6f/system_book_12.6f"
#define PATH "listings~/MR12.6f/"
#define bookSegmentsMax 1024
#define bookComponentsMax 4096
#define bookSegmentNameLen 33
static struct bookSegment
  {
    char * segname;
    int segno;
  } bookSegments [bookSegmentsMax];

static int nBookSegments = 0;

static struct bookComponent
  {
    char * compname;
    int bookSegmentNum;
    uint txt_start, txt_length;
    int intstat_start, intstat_length, symbol_start, symbol_length;
  } bookComponents [bookComponentsMax];

static int nBookComponents = 0;

static int lookupBookSegment (char * name)
  {
    for (int i = 0; i < nBookSegments; i ++)
      if (strcmp (name, bookSegments [i] . segname) == 0)
        return i;
    return -1;
  }

static int addBookSegment (char * name, int segno)
  {
    int n = lookupBookSegment (name);
    if (n >= 0)
      return n;
    if (nBookSegments >= bookSegmentsMax)
      return -1;
    bookSegments [nBookSegments] . segname = strdup (name);
    bookSegments [nBookSegments] . segno = segno;
    n = nBookSegments;
    nBookSegments ++;
    return n;
  }

static int addBookComponent (int segnum, char * name, uint txt_start, uint txt_length, int intstat_start, int intstat_length, int symbol_start, int symbol_length)
  {
    if (nBookComponents >= bookComponentsMax)
      return -1;
    bookComponents [nBookComponents] . compname = strdup (name);
    bookComponents [nBookComponents] . bookSegmentNum = segnum;
    bookComponents [nBookComponents] . txt_start = txt_start;
    bookComponents [nBookComponents] . txt_length = txt_length;
    bookComponents [nBookComponents] . intstat_start = intstat_start;
    bookComponents [nBookComponents] . intstat_length = intstat_length;
    bookComponents [nBookComponents] . symbol_start = symbol_start;
    bookComponents [nBookComponents] . symbol_length = symbol_length;
    int n = nBookComponents;
    nBookComponents ++;
    return n;
  }
 
// Warning: returns ptr to static buffer
static char * lookupSystemBookAddress (word18 segno, word18 offset, char * * compname, word18 * compoffset)
  {
    static char buf [129];
    int i;
    for (i = 0; i < nBookSegments; i ++)
      if (bookSegments [i] . segno == (int) segno)
        break;
    if (i >= nBookSegments)
      return NULL;

    int best = -1;
    uint bestoffset = 0;

    for (int j = 0; j < nBookComponents; j ++)
      {
        if (bookComponents [j] . bookSegmentNum != i)
          continue;
        if (bookComponents [j] . txt_start <= offset &&
            bookComponents [j] . txt_start + bookComponents [j] . txt_length > offset)
          {
            sprintf (buf, "%s:%s+0%0o", bookSegments [i] . segname,
              bookComponents [j].compname,
              offset - bookComponents [j] . txt_start);
            if (compname)
              * compname = bookComponents [j].compname;
            if (compoffset)
              * compoffset = offset - bookComponents [j] . txt_start;
            return buf;
          }
        if (bookComponents [j] . txt_start <= offset &&
            bookComponents [j] . txt_start > bestoffset)
          {
            best = j;
            bestoffset = bookComponents [j] . txt_start;
          }
      }

    if (best != -1)
      {
        // Didn't find a component track bracketed the offset; return the
        // component that was before the offset
        if (compname)
          * compname = bookComponents [best].compname;
        if (compoffset)
          * compoffset = offset - bookComponents [best] . txt_start;
        sprintf (buf, "%s:%s+0%0o", bookSegments [i] . segname,
          bookComponents [best].compname,
          offset - bookComponents [best] . txt_start);
        return buf;
      }

    // Found a segment, but it had no components. Return the segment name
    // as the component name

    if (compname)
      * compname = bookSegments [i] . segname;
    if (compoffset)
      * compoffset = offset;
    sprintf (buf, "%s:+0%0o", bookSegments [i] . segname,
             offset);
    return buf;
 }

char * lookupAddress (word18 segno, word18 offset, char * * compname, word18 * compoffset)
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
            static char buf [129];
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
            static char buf [129];
            sprintf (buf, "bound_debug_util_:interpret_op_ptr_+0%0o", 
                  offset - IOPOS);
            return buf;
          }

      }
#endif

    char * ret = lookupSystemBookAddress (segno, offset, compname, compoffset);
//#ifndef SCUMEM
//    if (ret)
//      return ret;
//    ret = lookupSegmentAddress (segno, offset, compname, compoffset);
//#endif
    return ret;
  }


#if 0 
// Warning: returns ptr to static buffer
static int lookupSystemBookName (char * segname, char * compname, long * segno, long * offset)
  {
    int i;
    for (i = 0; i < nBookSegments; i ++)
      if (strcmp (bookSegments [i] . segname, segname) == 0)
        break;
    if (i >= nBookSegments)
      return -1;

    for (int j = 0; j < nBookComponents; j ++)
      {
        if (bookComponents [j] . bookSegmentNum != i)
          continue;
        if (strcmp (bookComponents [j] . compname, compname) == 0)
          {
            * segno = bookSegments [i] . segno;
            * offset = (long) bookComponents[j].txt_start;
            return 0;
          }
      }

   return -1;
 }
#endif

static char * sourceSearchPath = NULL;

// search path is path:path:path....

static void setSearchPath (const char * buf)
  {
    if (sourceSearchPath)
      free (sourceSearchPath);
    sourceSearchPath = strdup (buf);
  }

#if 0
static t_stat listSourceAt (UNUSED int32 arg, UNUSED const char *  buf)
  {
    // list seg:offset
    int segno;
    uint offset;
    if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
      return SCPE_ARG;
    char * compname;
    word18 compoffset;
    char * where = lookupAddress ((word18) segno, offset,
                                  & compname, & compoffset);
    if (where)
      {
        printf ("%05o:%06o %s\n", segno, offset, where);
        listSource (compname, compoffset, 0);
      }
    return SCPE_OK;
  }
#endif

void listSource (char * compname, word18 offset, uint dflag)
  {
    const int offset_str_len = 10;
    //char offset_str [offset_str_len + 1];
    char offset_str [17];
    sprintf (offset_str, "    %06o", offset);

    char path [(sourceSearchPath ? strlen (sourceSearchPath) : 1) + 
               1 + // "/"
               (compname ? strlen (compname) : 1) +
                1 + strlen (".list") + 1];
    char * searchp = sourceSearchPath ? sourceSearchPath : ".";
    // find <search path>/<compname>.list
    while (* searchp)
      {
        size_t pathlen = strcspn (searchp, ":");
        strncpy (path, searchp, pathlen);
        path [pathlen] = '\0';
        if (searchp [pathlen] == ':')
          searchp += pathlen + 1;
        else
          searchp += pathlen;

        if (compname)
          {
            strcat (path, "/");
            strcat (path, compname);
          }
        strcat (path, ".list");
        //printf ("<%s>\n", path);
        FILE * listing = fopen (path, "r");
        if (listing)
          {
            char line [1025];
            if (feof (listing))
              goto fileDone;
            fgets (line, 1024, listing);
            if (strncmp (line, "ASSEMBLY LISTING", 16) == 0)
              {
                // Search ALM listing file
                // printf ("found <%s>\n", path);

                // ALM listing files look like:
                //     000226  4a  4 00010 7421 20  \tstx2]tbootload_0$entry_stack_ptr,id
                while (! feof (listing))
                  {
                    fgets (line, 1024, listing);
                    if (strncmp (line, offset_str, (size_t) offset_str_len) == 0)
                      {
                        printf ("%s", line);
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
                    char bestLines [8] = {0, 0, 0, 0, 0, 0, 0};
                    while (! feof (listing))
                      {
                        int loc [7];
                        char linenos [7] [8];
                        memset (linenos, 0, sizeof (linenos));
                        fgets (line, 1024, listing);
                        // sometimes the leading columns are blank...
                        while (strncmp (line, "                 ", 8 + 6 + 3) == 0)
                          memmove (line, line + 8 + 6 + 3, strlen (line + 8 + 6 + 3));
                        // deal with the extra numbers...

                        int cnt = sscanf (line,
                          // " %d %o %d %o %d %o %d %o %d %o %d %o %d %o", 
                          "%8c%o%*3c%8c%o%*3c%8c%o%*3c%8c%o%*3c%8c%o%*3c%8c%o%*3c%8c%o", 
                          (char *) & linenos [0], & loc [0], 
                          (char *) & linenos [1], & loc [1], 
                          (char *) & linenos [2], & loc [2], 
                          (char *) & linenos [3], & loc [3], 
                          (char *) & linenos [4], & loc [4], 
                          (char *) & linenos [5], & loc [5], 
                          (char *) & linenos [6], & loc [6]);
                        if (! (cnt == 2 || cnt == 4 || cnt == 6 ||
                               cnt == 8 || cnt == 10 || cnt == 12 ||
                               cnt == 14))
                          break; // end of table
                        int n;
                        for (n = 0; n < cnt / 2; n ++)
                          {
                            if (loc [n] > best && loc [n] <= (int) offset)
                              {
                                best = loc [n];
                                memcpy (bestLines, linenos [n], sizeof (bestLines));
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
                    char searchPrefix [10];
                    searchPrefix [ 0] = ' ';
                    searchPrefix [ 1] = bestLines [ 0];
                    searchPrefix [ 2] = bestLines [ 1];
                    searchPrefix [ 3] = ' ';
                    searchPrefix [ 4] = bestLines [ 2];
                    searchPrefix [ 5] = bestLines [ 3];
                    searchPrefix [ 6] = bestLines [ 4];
                    searchPrefix [ 7] = bestLines [ 5];
                    searchPrefix [ 8] = bestLines [ 6];
                    // ignore trailing space; some times its a tab
                    // searchPrefix [ 9] = bestLines [ 7];
                    searchPrefix [9] = '\0';

                    // Look for the line in the listing
                    rewind (listing);
                    while (! feof (listing))
                      {
                        fgets (line, 1024, listing);
                        if (strncmp (line, "\f\tSOURCE", 8) == 0)
                          goto fileDone; // end of source code listing
                        char prefix [10];
                        strncpy (prefix, line, 9);
                        prefix [9] = '\0';
                        if (strcmp (prefix, searchPrefix) != 0)
                          continue;
                        // Got it
                        printf ("%s", line);
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
                        if (strncmp (line, offset_str + 4, offset_str_len - 4) == 0)
                          {
                            printf ("%s", line);
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
#if 0

// LSB n:n   given a segment number and offset, return a segment name,
//           component and offset in that component
//     sname:cname+offset
//           given a segment name, component name and offset, return
//           the segment number and offset
   
static t_stat lookupSystemBook (UNUSED int32  arg, const char * buf)
  {
    char w1 [strlen (buf)];
    char w2 [strlen (buf)];
    char w3 [strlen (buf)];
    long segno, offset;

    size_t colon = strcspn (buf, ":");
    if (buf [colon] != ':')
      return SCPE_ARG;

    strncpy (w1, buf, colon);
    w1 [colon] = '\0';
    //printf ("w1 <%s>\n", w1);

    size_t plus = strcspn (buf + colon + 1, "+");
    if (buf [colon + 1 + plus] == '+')
      {
        strncpy (w2, buf + colon + 1, plus);
        w2 [plus] = '\0';
        strcpy (w3, buf + colon + 1 + plus + 1);
      }
    else
      {
        strcpy (w2, buf + colon + 1);
        strcpy (w3, "");
      }
    //printf ("w1 <%s>\n", w1);
    //printf ("w2 <%s>\n", w2);
    //printf ("w3 <%s>\n", w3);

    char * end1;
    segno = strtol (w1, & end1, 8);
    char * end2;
    offset = strtol (w2, & end2, 8);

    if (* end1 == '\0' && * end2 == '\0' && * w3 == '\0')
      { 
        // n:n
        char * ans = lookupAddress ((word18) segno, (word18) offset, NULL, NULL);
        printf ("%s\n", ans ? ans : "not found");
      }
    else
      {
        if (* w3)
          {
            char * end3;
            offset = strtol (w3, & end3, 8);
            if (* end3 != '\0')
              return SCPE_ARG;
          }
        else
          offset = 0;
        long comp_offset;
        int rc = lookupSystemBookName (w1, w2, & segno, & comp_offset);
        if (rc)
          {
            printf ("not found\n");
            return SCPE_OK;
          }
        printf ("0%o:0%o\n", (uint) segno, (uint) (comp_offset + offset));
        absAddrN  ((int) segno, (uint) (comp_offset + offset));
      }
/*
    if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
      return SCPE_ARG;
    char * ans = lookupAddress (segno, offset);
    printf ("%s\n", ans ? ans : "not found");
*/
    return SCPE_OK;
  }

static t_stat addSystemBookEntry (UNUSED int32 arg, const char * buf)
  {
    // asbe segname compname seg txt_start txt_len intstat_start intstat_length symbol_start symbol_length
    char segname [bookSegmentNameLen];
    char compname [bookSegmentNameLen];
    uint segno;
    uint txt_start, txt_len;
    uint  intstat_start, intstat_length;
    uint  symbol_start, symbol_length;

    // 32 is bookSegmentNameLen - 1
    if (sscanf (buf, "%32s %32s %o %o %o %o %o %o %o", 
                segname, compname, & segno, 
                & txt_start, & txt_len, & intstat_start, & intstat_length, 
                & symbol_start, & symbol_length) != 9)
      return SCPE_ARG;

    int idx = addBookSegment (segname, (int) segno);
    if (idx < 0)
      return SCPE_ARG;

    if (addBookComponent (idx, compname, txt_start, txt_len, (int) intstat_start, (int) intstat_length, (int) symbol_start, (int) symbol_length) < 0)
      return SCPE_ARG;

    return SCPE_OK;
  }
#endif

static void loadSystemBook (const char * buf)
  {
    // Multics 12.5 assigns segment number to collection 3 starting at 0244.
    uint c3 = 0244;

#define bufSz 257
    char filebuf [bufSz];
    int current = -1;

    FILE * fp = fopen (buf, "r");
    if (! fp)
      {
        printf ("error opening file %s\n", buf);
        exit (2);
      }
    for (;;)
      {
        char * bufp = fgets (filebuf, bufSz, fp);
        if (! bufp)
          break;
        //printf ("<%s\n>", filebuf);
        char name [bookSegmentNameLen];
        int segno, p0, p1, p2;

        // 32 is bookSegmentNameLen - 1
        int cnt = sscanf (filebuf, "%32s %o  (%o, %o, %o)", name, & segno, 
          & p0, & p1, & p2);
        if (filebuf [0] != '\t' && cnt == 5)
          {
            //printf ("A: %s %d\n", name, segno);
            int rc = addBookSegment (name, segno);
            if (rc < 0)
              {
                printf ("error adding segment name\n");
                fclose (fp);
                exit (2);
              }
            continue;
          }
        else
          {
            // Check for collection 3 segment
            // 32 is bookSegmentNameLen - 1
            cnt = sscanf (filebuf, "%32s  (%o, %o, %o)", name, 
              & p0, & p1, & p2);
            if (filebuf [0] != '\t' && cnt == 4)
              {
                if (strstr (name, "fw.") || strstr (name, ".ec"))
                  continue;
                //printf ("A: %s %d\n", name, segno);
                int rc = addBookSegment (name, (int) (c3 ++));
                if (rc < 0)
                  {
                    printf ("error adding segment name\n");
                    fclose (fp);
                    exit (2);
                  }
                continue;
              }
          }
        cnt = sscanf (filebuf, "Bindmap for >ldd>h>e>%32s", name);
        if (cnt != 1)
          cnt = sscanf (filebuf, "Bindmap for >ldd>hard>e>%32s", name);
        if (cnt == 1)
          {
            //printf ("B: %s\n", name);
            //int rc = addBookSegment (name);
            int rc = lookupBookSegment (name);
            if (rc < 0)
              {
                // The collection 3.0 segments do not have segment numbers,
                // and the 1st digit of the 3-tuple is 1, not 0. Ignore
                // them for now.
                current = -1;
                continue;
                //printf ("error adding segment name\n");
                //exit (2);
              }
            current = rc;
            continue;
          }

        uint txt_start, txt_length;
        int intstat_start, intstat_length, symbol_start, symbol_length;
        cnt = sscanf (filebuf, "%32s %o %o %o %o %o %o", name, & txt_start, & txt_length, & intstat_start, & intstat_length, & symbol_start, & symbol_length);

        if (cnt == 7)
          {
            //printf ("C: %s\n", name);
            if (strcmp (name, "Start") == 0 || strcmp (name, "Length") == 0)
              continue;
            if (current >= 0)
              {
                
                addBookComponent (current, name, txt_start, txt_length, intstat_start, intstat_length, symbol_start, symbol_length);
              }
            continue;
          }

        cnt = sscanf (filebuf, "%32s %o  (%o, %o, %o)", name, & segno, 
          & p0, & p1, & p2);
        if (filebuf [0] == '\t' && cnt == 5)
          {
            //printf ("D: %s %d\n", name, segno);
            int rc = addBookSegment (name, segno);
            if (rc < 0)
              {
                printf ("error adding segment name\n");
                fclose (fp);
                exit (2);
              }
            continue;
          }

      }
    fclose (fp);
    fprintf (stderr, "system book: %d segments, %d components\n", nBookSegments, nBookComponents);
#if 0
    for (int i = 0; i < nBookSegments; i ++)
      { 
        printf ("  %-32s %6o\n", bookSegments [i] . segname, bookSegments [i] . segno);
        for (int j = 0; j < nBookComponents; j ++)
          {
            if (bookComponents [j] . bookSegmentNum == i)
              {
                printf ("    %-32s %6o %6o %6o %6o %6o %6o\n",
                  bookComponents [j] . compname, 
                  bookComponents [j] . txt_start, 
                  bookComponents [j] . txt_length, 
                  bookComponents [j] . intstat_start, 
                  bookComponents [j] . intstat_length, 
                  bookComponents [j] . symbol_start, 
                  bookComponents [j] . symbol_length);
              }
          }
      }
#endif
  }


int main (int argc, char * argv [])
  {
    if (argc == 3)
      {
        setSearchPath (argv[1]);
        loadSystemBook (argv[2]);
      }
    else if (argc == 1)
      {
        setSearchPath (PATH);
        loadSystemBook (BK);
      }
    else
      {
        fprintf (stderr, "annotate [search_path book] < raw > annotated\n");
        exit (1);
      }
// DBG(108398614)> CPU TRACE: 00042:010324 0 036717707000 (TSX7 036717)
    char * buf = NULL;
    size_t size;
    ssize_t ret;
    while ((ret = getline (& buf, & size, stdin)) != -1)
      {
        static unsigned long long last_cycles = 0;
        unsigned long long cycles;
        int cnt = sscanf (buf, "DBG(%llu)", & cycles);
        if (cnt && cycles != last_cycles)
          printf ("\n");
        last_cycles = cycles;
        printf ("%s", buf);
        if (! cnt)
          continue;

        //if (ret && buf [0] != 'D')
          //continue;
#if 1
#define KW " CPU "
        char * p = strstr (buf, KW);
        if (! p)
          continue;
        uint cpun, segno, offset;
        cnt = sscanf (p + strlen (KW), "%o TRACE: %o:%o", & cpun, & segno, & offset);
        if (cnt != 3)
          continue;
        char * compname;
        word18 compoffset;
        char * where = lookupAddress ((word18) segno, offset,
                                      & compname, & compoffset);
        if (where)
          {
            printf ("%05o:%06o %s\n", segno, offset, where);
            listSource (compname, compoffset, 0);
          }
#else
#define KW "CPU TRACE "
#define KW2 "CPU FAULT "
        char * p = strstr (buf, KW);
        if (! p)
          p = strstr (buf, KW2);
        if (p)
          {
            uint cpun, segno, offset;
            int cnt = sscanf (p + strlen (KW), "%o: %o:%o", & cpun, & segno, & offset);
            if (cnt == 3)
              {
                char * compname;
                word18 compoffset;
                char * where = lookupAddress ((word18) segno, offset,
                                              & compname, & compoffset);
                if (where)
                  {
                    printf ("%05o:%06o %s\n", segno, offset, where);
                    listSource (compname, compoffset, 0);
                  }
              }
          }
#endif
      }
  }
