/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2018 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

// Interface for cfg_parse

typedef struct config_value_list_s
  {
    const char * value_name;
    int64_t value;
  } config_value_list_t;

typedef struct config_list_s
  {
    const char * name; // opt name
    int64_t min, max; // value limits
    config_value_list_t * value_list;
  } config_list_t;

typedef struct config_state_s
  {
    char * copy;
    char * statement_save;
  } config_state_t;

int cfg_parse (const char * tag, const char * cptr, config_list_t * clist, config_state_t * state, int64_t * result);
void cfg_parse_done (config_state_t * state);

struct opcode_s * get_iwb_info (DCDstruct *i);
char * dump_flags(char * buffer, word18 flags);
char *disassemble(char * result, word36 instruction);
char *get_mod_string(char * msg, word6 tag);
word72 convert_to_word72 (word36 even, word36 odd);
void convert_to_word36 (word72 src, word36 *even, word36 *odd);

word36 compl36(word36 op1, word18 *flags, bool * ovf);
word18 compl18(word18 op1, word18 *flags, bool * ovf);

void copyBytes(int posn, word36 src, word36 *dst);
void copyChars(int posn, word36 src, word36 *dst);

void putByte(word36 *dst, word9 data, int posn);
void putChar(word36 *dst, word6 data, int posn);

void cmp36(word36 op1, word36 op2, word18 *flags);
void cmp36wl(word36 A, word36 Y, word36 Q, word18 *flags);
void cmp18(word18 op1, word18 op2, word18 *flags);
void cmp72(word72 op1, word72 op2, word18 *flags);

char *strlower(char *q);
int strmask(char *str, char *mask);
char *Strtok(char *, char *);
char *stripquotes(char *s);
char *trim(char *s);
char *ltrim(char *s);
char *rtrim(char *s);

//
// getbitsNN/setbitsNN/putbitsNN
//
//   Manipluate bitfields.
//     NN      the word size (18, 36, 72).
//     data    the incoming word
//     i       the starting bit number (DPS8 notation, 0 is the MSB)
//     n       the number of bits, starting at i
//     val     the bits
//
//   val = getbitsNN (data, i, n)
//
//       extract n bits from data, starting at i.
//
//           val = getbits36 (data, 0, 1) --> the sign bit
//           val = getbits36 (data, 18, 18) --> the low eighteen bits
//
//   newdata = setbitsNN (data, i, n, val)
//
//       return 'data' with n bits of val inserted.
// 
//           newdata = setbits36 (data, 0, 18, 1) --> move '1' into the high
//                                                    18 bits.
//
//   putbitsNN (& data, i, n, val)
//
//        put val into data (equivalent to 'data = setbitsNN (data, ....)'
//
//           putbits (& data, 0, 18, 1) --> set the high 18 bits to '1'.
//

#if 0
static inline word72 getbits72 (word72 x, uint i, uint n)
  {
    // bit 71 is right end, bit zero is 72nd from the right
    int shift = 71 - (int) i - (int) n + 1;
    if (shift < 0 || shift > 71)
      {
        sim_printf ("getbits72: bad args (i=%d,n=%d)\n", i, n);
        return 0;
      }
     else
      return (x >> (unsigned) shift) & ~ (~0U << n);
  }
#endif

static inline word36 getbits36(word36 x, uint i, uint n) {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)n+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36: bad args (%012"PRIo64",i=%d,n=%d)\n", x, i, n);
        return 0;
    } else
        return (x >> (unsigned) shift) & ~ (~0U << n);
}

static inline word1 getbits36_1 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_1: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 01;
  }

static inline word2 getbits36_2 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)2+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_2: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 03;
  }

static inline word3 getbits36_3 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)3+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_3: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 07;
  }

static inline word4 getbits36_4 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)4+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_4: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 017;
  }

static inline word5 getbits36_5 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)5+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_5: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 037;
  }

static inline word6 getbits36_6 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)6+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_6: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 077;
  }

static inline word7 getbits36_7 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)7+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_7: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 0177;
  }

static inline word8 getbits36_8 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)8+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_8: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 0377;
  }

static inline word9 getbits36_9 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)9+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_9: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 0777;
  }

static inline word10 getbits36_10 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)10+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_10: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 01777;
  }

static inline word12 getbits36_12 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)12+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_12: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 07777;
  }

static inline word14 getbits36_14 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)14+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_14: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 037777;
  }

static inline word15 getbits36_15 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)15+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_15: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 077777;
  }

static inline word16 getbits36_16 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)16+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_16: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 0177777;
  }

static inline word18 getbits36_18 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)18+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_18: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 0777777;
  }

static inline word24 getbits36_24 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)24+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_24: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & MASK24;
  }

static inline word28 getbits36_28 (word36 x, uint i)
  {
    // bit 35 is right end, bit zero is 36th from the right
    int shift = 35-(int)i-(int)28+1;
    if (shift < 0 || shift > 35) {
        sim_printf ("getbits36_28: bad args (%012"PRIo64",i=%d)\n", x, i);
        return 0;
    } else
        return (x >> (unsigned) shift) & 01777777777;
  }

static inline word36 setbits36(word36 x, uint p, uint n, word36 val)
{
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36: bad args (%012"PRIo64",pos=%d,n=%d)\n", x, p, n);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    mask <<= (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ mask) | ((val&MASKBITS(n)) << (36 - p - n));
    return result;
}

static inline word36 setbits36_1 (word36 x, uint p, word1 val)
{
    const int n = 1;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_1: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_4 (word36 x, uint p, word4 val)
{
    const int n = 4;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_4: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_5 (word36 x, uint p, word5 val)
{
    const int n = 5;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_5: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_6 (word36 x, uint p, word6 val)
{
    const int n = 6;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_6: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_8 (word36 x, uint p, word8 val)
{
    const int n = 8;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_8: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_9 (word36 x, uint p, word9 val)
{
    const int n = 9;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_9: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_16 (word36 x, uint p, word16 val)
{
    const int n = 16;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_16: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline word36 setbits36_24 (word36 x, uint p, word24 val)
{
    const int n = 24;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("setbits36_24: bad args (%012"PRIo64",pos=%d)\n", x, p);
        return 0;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    word36 result = (x & ~ smask) | (((word36) val & mask) << shift);
    return result;
}

static inline void putbits36 (word36 * x, uint p, uint n, word36 val)
  {
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35)
      {
        sim_printf ("putbits36: bad args (%012"PRIo64",pos=%d,n=%d)\n", * x, p, n);
        return;
      }
    word36 mask = ~ (~0U << n);  // n low bits on
    mask <<= (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~mask) | ((val & MASKBITS (n)) << (36 - p - n));
    return;
  }

static inline void putbits36_1 (word36 * x, uint p, word1 val)
{
    const int n = 1;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_1: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_2 (word36 * x, uint p, word2 val)
{
    const int n = 2;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_2: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_3 (word36 * x, uint p, word3 val)
{
    const int n = 3;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_3: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_4 (word36 * x, uint p, word4 val)
{
    const int n = 4;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_4: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_5 (word36 * x, uint p, word5 val)
{
    const int n = 5;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_5: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_6 (word36 * x, uint p, word6 val)
{
    const int n = 6;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_6: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_7 (word36 * x, uint p, word7 val)
{
    const int n = 7;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_7: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_8 (word36 * x, uint p, word8 val)
{
    const int n = 8;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_8: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_9 (word36 * x, uint p, word9 val)
{
    const int n = 9;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_9: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_10 (word36 * x, uint p, word10 val)
{
    const int n = 10;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_10: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_12 (word36 * x, uint p, word12 val)
{
    const int n = 12;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_12: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_13 (word36 * x, uint p, word13 val)
{
    const int n = 13;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_13: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_14 (word36 * x, uint p, word14 val)
{
    const int n = 14;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_14: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_15 (word36 * x, uint p, word15 val)
{
    const int n = 15;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_15: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_16 (word36 * x, uint p, word16 val)
{
    const int n = 16;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_16: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_17 (word36 * x, uint p, word17 val)
{
    const int n = 17;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_17: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_18 (word36 * x, uint p, word18 val)
{
    const int n = 18;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_18: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_23 (word36 * x, uint p, word23 val)
{
    const int n = 23;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_23: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_24 (word36 * x, uint p, word24 val)
{
    const int n = 24;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_24: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits36_28 (word36 * x, uint p, word28 val)
{
    const int n = 28;
    int shift = 36 - (int) p - (int) n;
    if (shift < 0 || shift > 35) {
        sim_printf ("putbits36_28: bad args (%012"PRIo64",pos=%d)\n", *x, p);
        return;
    }
    word36 mask = ~ (~0U<<n);  // n low bits on
    word36 smask = mask << (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~ smask) | (((word36) val & mask) << shift);
}

static inline void putbits72 (word72 * x, uint p, uint n, word72 val)
  {
    int shift = 72 - (int) p - (int) n;
    if (shift < 0 || shift > 71)
      {
        sim_printf ("putbits72: bad args (pos=%d,n=%d)\n", p, n);
        return;
      }
#ifdef NEED_128
// n low bits on
    uint64_t lowmask = 0;
    uint64_t highmask = 0;
    if (n >= 64)
      {
        lowmask = MASK64;
        highmask = ~ ((~(uint64_t)0) << n);
        highmask &= 0377U;
      }
    else
      {
        lowmask = ~ ((~(uint64_t)0) << n);
      }
    word72 mask = construct_128 (highmask, lowmask);
    mask = lshift_128 (mask, (uint) shift);
    word72 notmask = complement_128 (mask);
    * x = or_128 (and_128 (* x, notmask), and_128 (lshift_128 (val, 72 - p - n), mask));
#else
    word72 mask = ~ ((~(word72)0) << n);  // n low bits on
    mask <<= (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~mask) | ((val & MASKBITS72 (n)) << (72 - p - n));
#endif
    return;
  }


//  getbits18 (data, starting bit, number of bits)

static inline word18 getbits18 (word18 x, uint i, uint n)
  {
    // bit 17 is right end, bit zero is 18th from the right
    int shift = 17 - (int) i - (int) n + 1;
    if (shift < 0 || shift > 17)
      {
        sim_printf ("getbits18: bad args (%06o,i=%d,n=%d)\n", x, i, n);
        return 0;
      }
    else
      return (x >> (unsigned) shift) & ~ (~0U << n);
  }

//  putbits18 (data, starting bit, number of bits, bits)

static inline void putbits18 (word18 * x, uint p, uint n, word18 val)
  {
    int shift = 18 - (int) p - (int) n;
    if (shift < 0 || shift > 17)
      {
        sim_printf ("putbits18: bad args (%06o,pos=%d,n=%d)\n", * x, p, n);
        return;
      }
    word18 mask = ~ (~0U << n);  // n low bits on
    mask <<= (unsigned) shift;  // shift 1s to proper position; result 0*1{n}0*
    // caller may provide val that is too big, e.g., a word with all bits
    // set to one, so we mask val
    * x = (* x & ~mask) | ((val & MASKBITS18 (n)) << (18 - p - n));
    return;
  }

//
// setmask -- set bits from mask
//

static inline void setmask (word36 * v, word36 mask)
  {
    * v |= mask;
  }

//
// clrmask -- clear bits from mask
//

static inline void clrmask (word36 * v, word36 mask)
  {
    * v &= ~mask;
  }

char * strdupesc (const char * str);
word36 extr36 (uint8 * bits, uint woffset);
void put36 (word36 val, uint8 * bits, uint woffset);
int extractASCII36FromBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 *wordp);
int extractWord36FromBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, uint64 *wordp);
int insertASCII36toBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 word);
int insertWord36toBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 word);
void print_int128 (int128 n, char * p);
word36 Add36b (word36 op1, word36 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf);
word36 Sub36b (word36 op1, word36 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf);
word18 Add18b (word18 op1, word18 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf);
word18 Sub18b (word18 op1, word18 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf);
word72 Add72b (word72 op1, word72 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf);
word72 Sub72b (word72 op1, word72 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf);

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result);

#if defined(THREADZ) || defined(LOCKLESS)
void currentTR (word27 * trunits, bool * ovf);
#endif
