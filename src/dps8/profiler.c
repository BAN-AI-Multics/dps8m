#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>

#include "dps8.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_state.h"
#include "shm.h"
//#include "threadz.h"

struct system_state_s * system_state;
vol word36 * M = NULL;                                          // memory
cpu_state_t * cpus;
vol cpu_state_t * cpup_;
#undef cpu
#define cpu (* cpup_)

// Sampling loop
//   Sample rate: 1000 Hz
#define USLEEP_1KHz 1000 // 1000 usec is 1 msec
//   Display update rate: 1 Hz
#define UPDATE_RATE 1000  // Every 1000 samples is 1 sec.

int main (int argc, char * argv[])
  {
    for (;;)
      {
        system_state = (struct system_state_s *)
          open_shm ("state", sizeof (struct system_state_s));

        if (system_state)
          break;

        printf ("No state file found; retry in 1 second\n");
        sleep (1);
      }

    initscr ();

    M = system_state->M;
    cpus = system_state->cpus;
// We don't have access to the thread state data structure, so we
// can't easily tell how many or which CPUs are running. We can watch
// the cycle count, tho. If it is 0, then the CPU hasn't been started;
// If it isn't changing, then it has probably been stopped.

//#define for_cpus for (uint cpun = 0; cpun < N_CPU_UNITS_MAX; cpun ++)
#define for_cpus for (uint cpun = 0; cpun < 2; cpun ++)

    unsigned long long last_cycle_cnt[N_CPU_UNITS_MAX] = {0,0,0,0,0,0,0,0};
    int dis_cnt[N_CPU_UNITS_MAX] = {0,0,0,0,0,0,0,0};

    for_cpus
      last_cycle_cnt[cpun] = cpus[cpun].cycleCnt;

    //for_cpus last_cycle_cnt[cpun] = cpus[cpun].cycleCnt;

    long sample_ctr = 0;
    for (;;)
      {
// Once a second, update display
        if (sample_ctr && sample_ctr % UPDATE_RATE == 0)
          {
            clear ();
            unsigned long disk_seeks = __atomic_exchange_n (& system_state->profiler.disk_seeks, 0, __ATOMIC_SEQ_CST);
            unsigned long disk_reads = __atomic_exchange_n (& system_state->profiler.disk_reads, 0, __ATOMIC_SEQ_CST);
            unsigned long disk_writes = __atomic_exchange_n (& system_state->profiler.disk_writes, 0, __ATOMIC_SEQ_CST);
            unsigned long disk_read = __atomic_exchange_n (& system_state->profiler.disk_read, 0, __ATOMIC_SEQ_CST);
            unsigned long disk_written = __atomic_exchange_n (& system_state->profiler.disk_written, 0, __ATOMIC_SEQ_CST);
            printw ("DISK S %06d R %06d W %06d MB/S %4d\n", disk_seeks, disk_reads, disk_writes, (disk_read + disk_written) * 9 / 2 / 1048576);

            for_cpus
              {
                cpup_ = (cpu_state_t *) & cpus[cpun];
                if (! cpu.run)
                  continue;
                printw ("CPU %c\n", 'A' + cpun);
                unsigned long long cnt = __atomic_load_n (& cpu.cycleCnt, __ATOMIC_ACQUIRE);
                float dis_pct = ((float) (dis_cnt[cpun] * 100)) / UPDATE_RATE;
                printw ("Cycles %10lld Idle %5.1f%%\n", cnt - last_cycle_cnt [cpun], dis_pct);
                dis_cnt[cpun] = 0;
                last_cycle_cnt [cpun] = cnt;

                uint cioc_iom = __atomic_exchange_n (& cpu.cioc_iom, 0, __ATOMIC_SEQ_CST);
                uint cioc_cpu = __atomic_exchange_n (& cpu.cioc_cpu, 0, __ATOMIC_SEQ_CST);
                uint intrs = __atomic_exchange_n (& cpu.intrs, 0, __ATOMIC_SEQ_CST);
                printw ("CIOC IOM %4u CPU %4u INTRS %4u\n", cioc_iom, cioc_cpu, intrs);


                for (uint fn = 0; fn < 32; fn ++)
                  {
                    if (fn == 0)
                      printw ("     SDF     STR     MME     F1      TRO     CMD     DRL     LUF\n");
                    else if (fn == 8)
                      printw ("     CON     PAR     IPR     ONC     SUF     OFL     DIV     EXF\n");
                    else if (fn == 16)
                      printw ("     DF0     DF1     DF2     DF3     ACV     MME2    MME3    MME4\n");
                    else if (fn == 24)
                      printw ("     F2      F3      UN1     UN2     UN3     UN4     UN5     TRB\n");
                    unsigned long fcnt = __atomic_exchange_n (& cpu.faults[fn], 0, __ATOMIC_SEQ_CST);
                    printw ("%8u", fcnt);
                    if (fn % 8 == 7)
                      printw ("\n");
                  }



                printw ("\n");
              }

            refresh ();
          }

        for_cpus
          {
            cpup_ = (cpu_state_t *) & cpus[cpun];
            if (! cpu.run)
              continue;
            if (cpu.currentInstruction.opcode10 == 00616)
              dis_cnt[cpun] ++;
          }

        usleep (USLEEP_1KHz); // 1 ms
        sample_ctr ++;
     }

    return 0;
  }


