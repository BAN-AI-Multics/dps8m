#undef sim_debug
#define sim_debug(dbits, dptr, ...) \
  if (sim_timell () >= sim_deb_start && \
      (sim_deb_stop == 0 || sim_timell () < sim_deb_stop) && \
      sim_deb && \
      ((dptr)->dctrl & dbits)) \
        _sim_debug (dbits, dptr, __VA_ARGS__); \
  else \
    (void) 0
#define if_sim_debug(dbits, dptr) \
  if (sim_timell () >= sim_deb_start && \
      (sim_deb_stop == 0 || sim_timell () < sim_deb_stop) && \
      sim_deb && \
     ((dptr)->dctrl & dbits))

/* scp Debug flags */

#define DBG_TRACE       (1U << 0)    ///< instruction trace
#define DBG_MSG         (1U << 1)    ///< misc output

#define DBG_REGDUMPAQI  (1U << 2)    ///< A/Q/IR register dump
#define DBG_REGDUMPIDX  (1U << 3)    ///< index register dump
#define DBG_REGDUMPPR   (1U << 4)    ///< pointer registers dump
#define DBG_REGDUMPADR  (1U << 5)    ///< address registers dump
#define DBG_REGDUMPPPR  (1U << 6)    ///< PPR register dump
#define DBG_REGDUMPDSBR (1U << 7)    ///< descritptor segment base register dump
#define DBG_REGDUMPFLT  (1U << 8)    ///< C(EAQ) floating-point register dump

#define DBG_REGDUMP     (DBG_REGDUMPAQI | DBG_REGDUMPIDX | DBG_REGDUMPPR | DBG_REGDUMPADR | DBG_REGDUMPPPR | DBG_REGDUMPDSBR | DBG_REGDUMPFLT)

#define DBG_ADDRMOD     (1U << 9)    ///< follow address modifications
#define DBG_APPENDING   (1U << 10)   ///< follow appending unit operations
#define DBG_TRACEEXT    (1U << 11)   ///< extended instruction trace
#define DBG_WARN        (1U << 12)   
#define DBG_DEBUG       (1U << 13)   
#define DBG_INFO        (1U << 14)   
#define DBG_NOTIFY      (1U << 15)   
#define DBG_SIM_USES_16 (1U << 16)
#define DBG_SIM_USES_17 (1U << 17)
#define DBG_SIM_USES_18 (1U << 18)
#define DBG_ERR         (1U << 19)   
#define DBG_ALL (DBG_NOTIFY | DBG_INFO | DBG_ERR | DBG_DEBUG | DBG_WARN | \
                 DBG_ERR )
#define DBG_FAULT       (1U << 20)  ///< follow fault handling
#define DBG_INTR        (1U << 21)  // follow interrupt handling
#define DBG_CORE        (1U << 22)
#define DBG_CYCLE       (1U << 23)
#define DBG_CAC         (1U << 24)

// Abort codes, used to sort out longjmp's back to the main loop.
// Codes > 0 are simulator stop codes
// Codes < 0 are internal aborts
// Code  = 0 stops execution for an interrupt check (XXX Don't know if I like 
// this or not)
// XXX above is not entirely correct (anymore).


#define STOP_UNK    0
#define STOP_UNIMP  1
#define STOP_DIS    2
#define STOP_BKPT   3
#define STOP_BUG    4
#define STOP_FLT_CASCADE   5
#define STOP_HALT   6


// not really STOP codes, but get returned from instruction loops
#define CONT_TRA    -1  ///< encountered a transfer instruction dont bump PPR.IC
#define CONT_FAULT  -2  ///< instruction encountered some kind of fault
#define CONT_INTR   -3  ///< instruction saw interrupt go up

extern uint32 sim_brk_summ, sim_brk_types, sim_brk_dflt;
extern FILE *sim_deb;
void _sim_debug (uint32 dbits, DEVICE* dptr, const char* fmt, ...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 3, 4)))
#endif
;

