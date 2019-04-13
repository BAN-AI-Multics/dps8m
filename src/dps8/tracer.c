#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dps8.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_state.h"
#include "shm.h"

struct system_state_s * system_state;
vol word36 * M = NULL;                                          // memory
cpu_state_t * cpus;
vol cpu_state_t * cpup_;
#undef cpu
#define cpu (* cpup_)

// Sampling loop
//   Sample rate: 1000 Hz
#define USLEEP_1KHz 1000 // 1000 usec is 1 msec

#include "trace_record.h"

int main (int argc, char * argv[])
  {
    struct record rec;
    for (;;)
      {
        system_state = (struct system_state_s *)
          open_shm ("state", sizeof (struct system_state_s));

        if (system_state)
          break;

        printf ("No state file found; retry in 1 second\n");
        sleep (1);
      }

    int fd = creat ("tracer.dat", S_IRUSR | S_IWUSR);

    M = system_state->M;
    cpus = system_state->cpus;
// We don't have access to the thread state data structure, so we
// can't easily tell how many or which CPUs are running. We can watch
// the cycle count, tho. If it is 0, then the CPU hasn't been started;
// If it isn't changing, then it has probably been stopped.

#define for_cpus for (uint cpun = 0; cpun < N_CPU_UNITS_MAX; cpun ++)

    //unsigned long long last_cycle_cnt[N_CPU_UNITS_MAX] = {0,0,0,0,0,0,0,0};
    //int dis_cnt[N_CPU_UNITS_MAX] = {0,0,0,0,0,0,0,0};

    //for_cpus
      //last_cycle_cnt[cpun] = cpus[cpun].cycleCnt;

    long sample_ctr = 0;
    for (;;)
      {

        for_cpus
          {
            cpup_ = (cpu_state_t *) & cpus[cpun];
            if (! cpu.run)
              continue;
            rec.cpun = (uint8_t) cpun;
            rec.PPR = cpu.PPR;
            rec.cu.IR = cpu.cu.IR;
            if (rec.PPR.PSR == 061 && rec.PPR.IC == 0307)
              continue; // Don't trace idle loop DIS
            write (fd, & rec, sizeof (rec));
          }

        usleep (USLEEP_1KHz); // 1 ms
        sample_ctr ++;
     }

    return 0;
  }


