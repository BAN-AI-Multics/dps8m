#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dps8.h"
#include "dps8_cpu.h"

#include "trace_record.h"

int main (int argc, char * argv[])
  {
    struct record rec;

    int fd = open ("tracer.dat", O_RDONLY, 0);

    for (;;)
      {
        ssize_t rc = read (fd, & rec, sizeof (rec));
        if (rc <= 0)
          break;
        printf ("%05o:%06o %u %06o\n", rec.PPR.PSR, rec.PPR.IC, rec.cpun, rec.cu.IR);
      }
    return 0;
  }


