/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 9ca9a790-f62e-11ec-a677-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2016 Michal Tomek
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 *
 * This source file may contain code comments that adapt, include, and/or
 * incorporate Multics program code and/or documentation distributed under
 * the Multics License.  In the event of any discrepancy between code
 * comments herein and the original Multics materials, the original Multics
 * materials should be considered authoritative unless otherwise noted.
 * For more details and historical background, see the LICENSE.md file at
 * the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

//-V::536

#include <stdio.h>
#include <math.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_ins.h"
#include "dps8_math.h"
#include "dps8_utils.h"

#define DBG_CTR cpu.cycleCnt

#if defined(__CYGWIN__)
long double ldexpl(long double x, int n) {
       return __builtin_ldexpl(x, n);
}

long double exp2l (long double e) {
       return __builtin_exp2l(e);
}
#endif

#if defined(NEED_128)
# define ISZERO_128(m) iszero_128 (m)
# define ISEQ_128(a,b) iseq_128 (a, b)
#else
# define ISZERO_128(m) ((m) == 0)
# define ISEQ_128(a,b) ((a) == (b))
#endif

#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma global novector
#endif

/*
 * convert floating point quantity in C(EAQ) to a IEEE long double ...
 */
long double EAQToIEEElongdouble(void)
{
    // mantissa
    word72 Mant = convert_to_word72 (cpu.rA, cpu.rQ);

    if (ISZERO_128 (Mant))
        return (long double) 0;
#if defined(NEED_128)

    bool S = isnonzero_128 (and_128 (Mant, SIGN72)); // sign of mantissa
    if (S)
        Mant = and_128 (negate_128 (Mant), MASK71); // 71 bits (not 72!)
#else
    bool S = Mant & SIGN72; // sign of mantissa
    if (S)
        Mant = (-Mant) & MASK71; // 71 bits (not 72!)
#endif

    long double m = 0;  // mantissa value;
    int e = SIGNEXT8_int (cpu . rE & MASK8); // make signed

    if (S && ISZERO_128 (Mant))// denormalized -1.0*2^e
        return -exp2l(e);

    long double v = 0.5;
    for(int n = 70 ; n >= 0 ; n -= 1) // this also normalizes the mantissa
    {
#if defined(NEED_128)
        if (isnonzero_128 (and_128 (Mant, lshift_128 (construct_128 (0, 1), (unsigned int) n))))
        {
            m += v;
        }
#else
        if (Mant & ((word72)1 << n))
        {
            m += v;
        }
#endif
        v /= 2.0;
    }

    /*if (m == 0 && e == -128)    // special case - normalized 0
        return 0;
    if (m == 0)
        return (S ? -1 : 1) * exp2l(e); */

    return (S ? -1 : 1) * ldexpl(m, e);
}

#if defined(__MINGW32__) || defined(__MINGW64__)

// MINGW doesn't have long double support, convert to IEEE double instead
double EAQToIEEEdouble(void)
{
    // mantissa
    word72 Mant = convert_to_word72 (cpu.rA, cpu.rQ);

    if (ISZERO_128 (Mant))
        return 0;

# if defined(NEED_128)
    bool S = isnonzero_128 (and_128 (Mant, SIGN72)); // sign of mantissa
    if (S)
        Mant = and_128 (negate_128 (Mant), MASK71); // 71 bits (not 72!)
# else
    bool S = Mant & SIGN72; // sign of mantissa
    if (S)
        Mant = (-Mant) & MASK71; // 71 bits (not 72!)
# endif

    double m = 0;  // mantissa value
    int e = SIGNEXT8_int (cpu . rE & MASK8); // make signed

    if (S && ISZERO_128 (Mant))// denormalized -1.0*2^e
        return -exp2(e);

    double v = 0.5;
    for(int n = 70 ; n >= 0 ; n -= 1) // this also normalizes the mantissa
    {
# if defined(NEED_128)
        if (isnonzero_128 (and_128 (Mant, lshift_128 (construct_128 (0, 1), (unsigned int) n))))
        {
            m += v;
        }
# else
        if (Mant & ((word72)1 << n))
        {
            m += v;
        }
# endif
        v /= 2.0;
    }

    return (S ? -1 : 1) * ldexp(m, e);
}
#endif

#if !defined(QUIET_UNUSED)
/*
 * return normalized dps8 representation of IEEE double f0 ...
 */
float72 IEEElongdoubleToFloat72(long double f0)
{
    if (f0 == 0)
        return (float72)((float72)0400000000000LL << 36);

    bool sign = f0 < 0 ? true : false;
    long double f = fabsl(f0);

    int exp;
    long double mant = frexpl(f, &exp);
    //sim_printf (sign=%d f0=%Lf mant=%Lf exp=%d\n", sign, f0, mant, exp);

    word72 result = 0;

    // now let's examine the mantissa and assign bits as necessary...

    if (sign && mant == 0.5)
    {
        //result = bitfieldInsert72(result, 1, 63, 1);
        putbits72 (& result, 71-62, 1, 1);
        exp -= 1;
        mant -= 0.5;
    }

    long double bitval = 0.5;    ///< start at 1/2 and go down .....
    for(int n = 62 ; n >= 0 && mant > 0; n -= 1)
    {
        if (mant >= bitval)
        {
            //result = bitfieldInsert72(result, 1, n, 1);
            putbits72 (& result, 71-n, 1, 1);
            mant -= bitval;
            //sim_printf ("Inserting a bit @ %d %012"PRIo64" %012"PRIo64"\n",
            //            n, (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
        }
        bitval /= 2.0;
    }
    //sim_printf ("n=%d mant=%f\n", n, mant);
    //sim_printf ("result=%012"PRIo64" %012"PRIo64"\n",
    //            (word36)((result >> 36) & DMASK), (word36)(result & DMASK));

    // if f is < 0 then take 2-comp of result ...
    if (sign)
    {
        result = -result & (((word72)1 << 64) - 1);
        //sim_printf ("-result=%012"PRIo64" %012"PRIo64"\n",
        //            (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
    }
    // insert exponent ...
    int e = (int)exp;
    //result = bitfieldInsert72(result, e & 0377, 64, 8);    ///< & 0777777777777LL;
    putbits72 (& result, 71-64, 8, e & 0377);

    // XXX TODO test for exp under/overflow ...

    return result;
}
#endif

#if !defined(QUIET_UNUSED)
static long double MYfrexpl(long double x, int *exp)
{
    long double exponents[20], *next;
    int exponent, bit;

    /* Check for zero, nan and infinity. */
    if (x != x || x + x == x )
    {
        *exp = 0;
        return x;
    }

    if (x < 0)
        return -MYfrexpl(-x, exp);

    exponent = 0;
    if (x > 1.0)
    {
        for (next = exponents, exponents[0] = 2.0L, bit = 1;
             *next <= x + x;
             bit <<= 1, next[1] = next[0] * next[0], next++);

        for (; next >= exponents; bit >>= 1, next--)
            if (x + x >= *next)
            {
                x /= *next;
                exponent |= bit;
            }

    }

    else if (x < 0.5)
    {
        for (next = exponents, exponents[0] = 0.5L, bit = 1;
             *next > x;
             bit <<= 1, next[1] = next[0] * next[0], next++);

        for (; next >= exponents; bit >>= 1, next--)
            if (x < *next)
            {
                x /= *next;
                exponent |= bit;
            }

        exponent = -exponent;
    }

    *exp = exponent;
    return x;
}
#endif

#if !defined(QUIET_UNUSED)
/*
 * Store EAQ with normalized dps8 representation of IEEE double f0 ...
 */
void IEEElongdoubleToEAQ(long double f0)
{
    if (f0 == 0)
    {
        cpu . rA = 0;
# if defined(TESTING)
        HDBGRegAW ("IEEEld2EAQ");
# endif
        cpu . rQ = 0;
        cpu . rE = 0200U; /*-128*/
        return;
    }

    bool sign = f0 < 0 ? true : false;
    long double f = fabsl(f0);
    int exp;
    long double mant = MYfrexpl(f, &exp);
    word72 result = 0;

    // now let's examine the mantissa and assign bits as necessary...
    if (sign && mant == 0.5)
    {
        //result = bitfieldInsert72(result, 1, 63, 1);
        result = putbits72 (& result, 71-63, 1, 1);
        exp -= 1;
        mant -= 0.5;
    }

    long double bitval = 0.5;    ///< start at 1/2 and go down .....
    for(int n = 70 ; n >= 0 && mant > 0; n -= 1)
    {
        if (mant >= bitval)
        {
            //result = bitfieldInsert72(result, 1, n, 1);
            putbits72 (& result 71-n, 1, 1);
            mant -= bitval;
            //sim_printf ("Inserting a bit @ %d %012"PRIo64" %012"PRIo64"\n",
            //            n, (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
        }
        bitval /= 2.0;
    }

    // if f is < 0 then take 2-comp of result ...
    if (sign)
        result = -result & (((word72)1 << 72) - 1);

    cpu . rE = exp & MASK8;
    cpu . rA = (result >> 36) & MASK36;
# if defined(TESTING)
    HDBGRegAW ("IEEEld2EAQ");
# endif
    cpu . rQ = result & MASK36;
}
#endif

#if 0
/*
 * return IEEE double version dps8 single-precision number ...
 */
static double float36ToIEEEdouble(word36 f36)
{
    unsigned char E;    ///< exponent
    uint64 Mant;         ///< mantissa
    E = (f36 >> 28) & 0xff;
    Mant = f36 & 01777777777LL;
    if (Mant == 0)
        return 0;

    bool S = Mant & 01000000000LL; ///< sign of mantissa
    if (S)
        Mant = (-Mant) & 0777777777; // 27 bits (not 28!)

    double m = 0;       ///< mantissa value;
    int e = (char)E;  ///< make signed

    if (S && Mant == 0) // denormalized -1.0*2^e
        return -exp2(e);

    double v = 0.5;
    for(int n = 26 ; n >= 0 ; n -= 1) // this also normalizes the mantissa
    {
        if (Mant & ((uint64)1 << n))
        {
            m += v;
        }   //else
        v /= 2.0;
    }

    return (S ? -1 : 1) * ldexp(m, e);
}
#endif

#if 0
/*
 * return normalized dps8 representation of IEEE double f0 ...
 */
float36 IEEEdoubleTofloat36(double f0)
{
    if (f0 == 0)
        return 0400000000000LL;

    double f = f0;
    bool sign = f0 < 0 ? true : false;
    if (sign)
        f = fabs(f0);

    int exp;
    double mant = frexp(f, &exp);

    //sim_printf (sign=%d f0=%f mant=%f exp=%d\n", sign, f0, mant, exp);

    word36 result = 0;

    // now let's examine the mantissa and assign bits as necessary...

    double bitval = 0.5;    ///< start at 1/2 and go down .....
    for(int n = 26 ; n >= 0 && mant > 0; n -= 1)
    {
        if (mant >= bitval)
        {
            //result = bitfieldInsert36(result, 1, n, 1);
            setbits36_1 (& result, 35-n, 1);
            mant -= bitval;
            //sim_printf ("Inserting a bit @ %d result=%012"PRIo64"\n", n, result);
        }
        bitval /= 2.0;
    }
    //sim_printf ("result=%012"PRIo64"\n", result);

    // if f is < 0 then take 2-comp of result ...
    if (sign)
    {
        result = -result & 001777777777LL;
        //sim_printf ("-result=%012"PRIo64"\n", result);
    }
    // insert exponent ...
    int e = (int)exp;
    //result = bitfieldInsert36(result, e, 28, 8) & 0777777777777LL;
    putbits36_8 (& result, 0, e);

    // XXX TODO test for exp under/overflow ...

    return result;
}
#endif

/*
 * single-precision arithmetic routines ...
 */

#if defined(HEX_MODE)
//#define HEX_SIGN (SIGN72 | BIT71 | BIT70 | BIT69 | BIT68)
//#define HEX_MSB  (         BIT71 | BIT70 | BIT69 | BIT68)
# if defined(NEED_128)
#  define HEX_SIGN construct_128 (0xF0, 0)
#  define HEX_MSB  construct_128 (0x70, 0)
#  define HEX_NORM construct_128 (0x78, 0)
# else
#  define HEX_SIGN (SIGN72 | BIT71 | BIT70 | BIT69)
#  define HEX_MSB  (         BIT71 | BIT70 | BIT69)
#  define HEX_NORM (         BIT71 | BIT70 | BIT69 | BIT68)
# endif

static inline bool isHex (void) {
    return (! cpu.tweaks.l68_mode) && (!! cpu.options.hex_mode_installed) &&  (!! cpu.MR.hexfp) && (!! TST_I_HEX);
}
#endif

/*!
 * Unnormalized floating single-precision add
 */
void ufa (bool sub, bool normalize) {
  // C(EAQ) + C(Y) → C(EAQ)
  // The ufa instruction is executed as follows:
  //
  // The mantissas are aligned by shifting the mantissa of the operand having
  // the algebraically smaller exponent to the right the number of places
  // equal to the absolute value of the difference in the two exponents. Bits
  // shifted beyond the bit position equivalent to AQ71 are lost.
  //
  // The algebraically larger exponent replaces C(E). The sum of the
  // mantissas replaces C(AQ).
  //
  // If an overflow occurs during addition, then;
  // * C(AQ) are shifted one place to the right.
  // * C(AQ)0 is inverted to restore the sign.
  // * C(E) is increased by one.

  CPTUR (cptUseE);
#if defined(HEX_MODE)
  uint shift_amt = isHex () ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  word72 m1 = convert_to_word72 (cpu.rA, cpu.rQ);
  // 28-bit mantissa (incl sign)
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.CY, 8)), 44u); // 28-bit mantissa (incl sign)
#else
  word72 m2 = ((word72) getbits36_28 (cpu.CY, 8)) << 44; // 28-bit mantissa (incl sign)
#endif

  int e1 = SIGNEXT8_int (cpu.rE & MASK8);
  int e2 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));

  // RJ78: The two's complement of the subtrahend is first taken and the
  // smaller value is then right-shifted to equalize it (i.e. ufa).

  int m2zero = 0;
  if (sub) {
    // ISOLTS-735 08i asserts no carry for (-1.0 * 2^96) - (-1.0 * 2^2) but 08g
    // asserts carry for -0.5079365*2^78-0.0
    // I assume zero subtrahend is handled in a special way.

    if (ISZERO_128 (m2))
      m2zero = 1;
#if defined(NEED_128)
    if (iseq_128 (m2, SIGN72)) {  // -1.0 -> 0.5, increase exponent, ISOLTS-735 08i,j
      m2 = rshift_128 (m2, shift_amt);
      e2 += 1;
    } else {
       m2 = and_128 (negate_128 (m2), MASK72);
    }
#else // ! NEED_128
    if (m2 == SIGN72) {  // -1.0 -> 0.5, increase exponent, ISOLTS-735 08i,j
      m2 >>= shift_amt;
      e2 += 1;
    } else {
      //m2 = (-m2) & MASK72;
      // ubsan
      m2 = ((word72) (- (word72s) m2)) & MASK72;
    }
#endif // ! NEED_128
  } // if sub

  int e3 = -1;

  // which exponent is smaller?
  L68_ (cpu.ou.cycle |= ou_GOE;)
  int shift_count = -1;
  word1 allones = 1;
  word1 notallzeros = 0;
  //word1 last = 0;
  (void)allones;
  if (e1 == e2) {
    shift_count = 0;
    e3 = e1;
  } else if (e1 < e2) {
    shift_count = abs (e2 - e1) * (int) shift_amt;
#if defined(NEED_128)
    bool sign = isnonzero_128 (and_128 (m1, SIGN72)); // mantissa negative?
    for (int n = 0 ; n < shift_count ; n += 1) {
      //last = m1.l & 1;
      allones &= m1.l & 1;
      notallzeros |= m1.l & 1;
      m1 = rshift_128 (m1, 1);
      if (sign)
        m1 = or_128 (m1, SIGN72);
    } // for n
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = construct_128 (0, 0);
    m1 = and_128 (m1, MASK72);
    e3 = e2;
#else // ! NEED_128
    bool sign = m1 & SIGN72;   // mantissa negative?
    for (int n = 0 ; n < shift_count ; n += 1) {
      //last = m1 & 1;
      allones &= m1 & 1;
      notallzeros |= m1 & 1;
      m1 >>= 1;
      if (sign)
        m1 |= SIGN72;
    } // for n

    if (m1 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = 0;
    m1 &= MASK72;
    e3 = e2;
#endif // NEED_128
  } else {
    // e2 < e1;
    shift_count = abs (e1 - e2) * (int) shift_amt;
#if defined(NEED_128)
    bool sign = isnonzero_128 (and_128 (m2, SIGN72)); // mantissa negative?
    for (int n = 0 ; n < shift_count ; n += 1) {
      //last = m2.l & 1;
      allones &= m2.l & 1;
      notallzeros |= m2.l & 1;
      m2 = rshift_128 (m2, 1);
      if (sign)
        m2 = or_128 (m2, SIGN72);
    } // for n
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = construct_128 (0, 0);
    m2 = and_128 (m2, MASK72);
    e3 = e1;
#else // ! NEED_128
    bool sign = m2 & SIGN72;   // mantissa negative?
    for (int n = 0 ; n < shift_count ; n += 1) {
      //last = m2 & 1;
      allones &= m2 & 1;
      notallzeros |= m2 & 1;
      m2 >>= 1;
      if (sign)
        m2 |= SIGN72;
    }
    if (m2 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = 0;
    m2 &= MASK72;
    e3 = e1;
#endif // ! NEED_128
  }

  bool ovf;
  word72 m3;
  m3 = Add72b (m1, m2, 0, I_CARRY, & cpu.cu.IR, & ovf);
  // ISOLTS-735 08g
  // if 2's complement carried, OR it in now.
  if (m2zero)
    SET_I_CARRY;

  if (ovf) {
#if defined(NEED_128)
# if defined(HEX_MODE)
//    word72 signbit = m3 & sign_msk;
//    m3 >>= shift_amt;
//    m3 = (m3 & MASK71) | signbit;
//    m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
//    e3 += 1;
    word72 s = and_128 (m3, SIGN72); // save the sign bit
    if (isHex ()) {
      m3 = rshift_128 (m3, shift_amt); // renormalize the mantissa
      if (isnonzero_128 (s))
        // Sign is set, number should be positive; clear the sign bit and the 3 MSBs
        m3 = and_128 (m3, MASK68);
      else
        // Sign is clr, number should be negative; set the sign bit and the 3 MSBs
        m3 = or_128 (m3, HEX_SIGN);
    } else {
      word72 signbit = and_128 (m3, SIGN72);
      m3 = rshift_128 (m3, 1);
      m3 = and_128 (m3, MASK71);
      m3 = or_128 (m3, signbit);
      m3 = xor_128 (m3, SIGN72); // C(AQ)0 is inverted to restore the sign
    }
    e3 += 1;
# else // ! HEX_MODE
    word72 signbit = and_128 (m3, SIGN72);
    m3 = rshift_128 (m3, 1);
    m3 = and_128 (m3, MASK71);
    m3 = or_128 (m3, signbit);
    m3 = xor_128 (m3, SIGN72); // C(AQ)0 is inverted to restore the sign
    e3 += 1;
# endif // ! HEX_MODE
#else // ! NEED_128
# if defined(HEX_MODE)
//    word72 signbit = m3 & sign_msk;
//    m3 >>= shift_amt;
//    m3 = (m3 & MASK71) | signbit;
//    m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
//    e3 += 1;
    word72 s = m3 & SIGN72; // save the sign bit
    if (isHex ()) {
      m3 >>= shift_amt; // renormalize the mantissa
      if (s)
        // Sign is set, number should be positive; clear the sign bit and the 3 MSBs
        m3 &= MASK68;
      else
        // Sign is clr, number should be negative; set the sign bit and the 3 MSBs
        m3 |=  HEX_SIGN;
    } else {
      word72 signbit = m3 & SIGN72;
      m3 >>= 1;
      m3 = (m3 & MASK71) | signbit;
      m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
    }
    e3 += 1;
# else // ! HEX_MODE
    word72 signbit = m3 & SIGN72;
    m3 >>= 1;
    m3 = (m3 & MASK71) | signbit;
    m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
    e3 += 1;
# endif // ! HEX_MODE
#endif // ! NEED_128
  }

  convert_to_word36 (m3, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("ufa");
#endif
  cpu.rE = e3 & 0377;

  SC_I_NEG (cpu.rA & SIGN36); // Do this here instead of in Add72b because
                                // of ovf handling above
  if (cpu.rA == 0 && cpu.rQ == 0) {
    SET_I_ZERO;
    cpu.rE = 0200U; /*-128*/
  } else {
    CLR_I_ZERO;
  }

  if (normalize) {
    fno_ext (& e3, & cpu.rE, & cpu.rA, & cpu.rQ);
  }

  // EOFL: If exponent is greater than +127, then ON
  if (e3 > 127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "ufa exp overflow fault");
  }

  // EUFL: If exponent is less than -128, then ON
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "ufa exp underflow fault");
  }
} // ufa

/*
 * floating normalize ...
 */

void fno (word8 * E, word36 * A, word36 * Q) {
  int e0 = SIGNEXT8_int ((* E) & MASK8);
  fno_ext (& e0, E, A, Q);
}

void fno_ext (int * e0, word8 * E, word36 * A, word36 * Q) {
  // The fno instruction normalizes the number in C(EAQ) if C(AQ) ≠ 0 and the
  // overflow indicator is OFF.
  //
  // A normalized floating number is defined as one whose mantissa lies in
  // the interval [0.5,1.0) such that
  //     0.5<= |C(AQ)| <1.0 which, in turn, requires that C(AQ)0 ≠ C(AQ)1.
  //
  // If the overflow indicator is ON, then C(AQ) is shifted one place to the
  // right, C(AQ)0 is inverted to reconstitute the actual sign, and the
  // overflow indicator is set OFF. This action makes the fno instruction
  // useful in correcting overflows that occur with fixed point numbers.
  //
  // Normalization is performed by shifting C(AQ)1,71 one place to the left
  // and reducing C(E) by 1, repeatedly, until the conditions for C(AQ)0 and
  // C(AQ)1 are met. Bits shifted out of AQ1 are lost.
  //
  // If C(AQ) = 0, then C(E) is set to -128 and the zero indicator is set ON.

  L68_ (cpu.ou.cycle |= ou_GON;)
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#endif
  * A &= DMASK;
  * Q &= DMASK;
  float72 m = convert_to_word72 (* A, * Q);
  if (TST_I_OFLOW) {
    CLR_I_OFLOW;
#if defined(NEED_128)
    word72 s = and_128 (m, SIGN72); // save the sign bit
# if defined(HEX_MODE)
    if (isHex ()) {
      m = rshift_128 (m, shift_amt); // renormalize the mantissa
      if (isnonzero_128 (s)) // sign of mantissa
        // Sign is set, number should be positive; clear the sign bit and the 3 MSBs
        m = and_128 (m, MASK68);
      else
        // Sign is clr, number should be negative; set the sign bit and the 3 MSBs
        m = or_128 (m, HEX_SIGN);
    } else {
      m = rshift_128 (m, 1); // renormalize the mantissa
      m = or_128 (m, SIGN72); // set the sign bit
      m = xor_128 (m, s); // if the was 0, leave it 1; if it was 1, make it 0
    }
# else // ! HEX_MODE
  m = rshift_128 (m, 1); // renormalize the mantissa
  m = or_128 (m, SIGN72); // set the sign bit
  m = xor_128 (m, s); // if the was 0, leave it 1; if it was 1, make it 0
# endif // ! HEX_MODE

  // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
  if (iszero_128 (m)) {
    * E = 0200U; /* -128 */
    SET_I_ZERO;
  } else {
    CLR_I_ZERO;
    if (* E == 127) {
//sim_printf ("overflow 1 E %o \r\n", * E);
      SET_I_EOFL;
      if (tstOVFfault ())
        dlyDoFault (FAULT_OFL, fst_zero, "fno exp overflow fault");
    }
    (* E) ++;
    * E &= MASK8;
  }
#else // ! NEED_128
  word72 s = m & SIGN72; // save the sign bit
# if defined(HEX_MODE)
  if (isHex ()) {
    m >>= shift_amt; // renormalize the mantissa
    if (s)
      // Sign is set, number should be positive; clear the sign bit and the 3 MSBs
      m &= MASK68;
    else
      // Sign is clr, number should be negative; set the sign bit and the 3 MSBs
      m |=  HEX_SIGN;
    } else { // ! isHex
      m >>= 1; // renormalize the mantissa
      m |= SIGN72; // set the sign bit
      m ^= s; // if the was 0, leave it 1; if it was 1, make it 0
    } // ! isHex
# else // ! HEX_MODE
    m >>= 1; // renormalize the mantissa
    m |= SIGN72; // set the sign bit
    m ^= s; // if the was 0, leave it 1; if it was 1, make it 0
# endif // ! HEX_MODE

    // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
    if (m == 0) {
      * E = 0200U; /* -128 */
      SET_I_ZERO;
    } else {
      CLR_I_ZERO;
      if (* E == 127) {
        SET_I_EOFL;
        if (tstOVFfault ())
          dlyDoFault (FAULT_OFL, fst_zero, "fno exp overflow fault");
      }
      (* E) ++;
      * E &= MASK8;
    }
#endif // ! NEED_128
    convert_to_word36 (m, A, Q);
    SC_I_NEG ((* A) & SIGN36);

    return;
  }

  // only normalize C(EAQ) if C(AQ) ≠ 0 and the overflow indicator is OFF
#if defined(NEED_128)
  if (iszero_128 (m)) { /// C(AQ) == 0.
    //* A = (m >> 36) & MASK36;
    //* Q = m & MASK36;
    * E = 0200U; /* -128 */
    SET_I_ZERO;
    CLR_I_NEG;
    return;
  }
#else // ! NEED_128
  if (m == 0) { // C(AQ) == 0.
    //* A = (m >> 36) & MASK36;
    //* Q = m & MASK36;
    * E = 0200U; /* -128 */
    SET_I_ZERO;
    CLR_I_NEG;
    return;
  }
#endif // ! NEED_128

  //int e = SIGNEXT8_int ((* E) & MASK8);
  int e = * e0;
#if defined(NEED_128)
  bool s = isnonzero_128 (and_128 (m, SIGN72));
#else
  bool s = (m & SIGN72) != (word72)0;    ///< save sign bit
#endif

#if defined(HEX_MODE)
// Normalized in Hex Mode: If sign is 0, bits 1-4 != 0; if sign is 1,
// bits 1-4 != 017.
  if (isHex ()) {
    if (s) {
      // Negative
      // Until bits 1-4 != 014
      // Termination guarantee: Zeros are being shifted into the right
      // end, so the loop will terminate when the first shifted
      // zero enters bits 1-4.
# if defined(NEED_128)
      while (iseq_128 (and_128 (m, HEX_NORM), HEX_NORM)) {
        m = lshift_128 (m, 4);
        e -= 1;
      }
      m = and_128 (m, MASK71);
      m = or_128 (m, SIGN72);
# else // ! NEED_128
      while ((m & HEX_NORM) == HEX_NORM) {
        m <<= 4;
        e -= 1;
      }
      m &= MASK71;
      m |= SIGN72;
# endif // ! NEED_128
    } else { // ! s
# if defined(NEED_128)
      // Positive
      // Until bits 1-4 != 0
      // Termination guarantee: m is known to be non-zero; a non-zero
      // bit will eventually be shifted into bits 1-4.
      while (iszero_128 (and_128 (m, HEX_NORM))) {
        m = lshift_128 (m, 4);
        e -= 1;
      }
      m = and_128 (m, MASK71);
# else // ! NEED_128
      // Positive
      // Until bits 1-4 != 0
      // Termination guarantee: m is known to be non-zero; a non-zero
      // bit will eventually be shifted into bits 1-4.
      while ((m & HEX_NORM) == 0) {
        m <<= 4;
        e -= 1;
      }
      m &= MASK71;
# endif // ! NEED_128
    } // ! s
  } else { // ! isHex
# if defined(NEED_128)
    while (s == isnonzero_128 (and_128 (m, BIT71))) { // until C(AQ)0 != C(AQ)1?
      m = lshift_128 (m, 1);
      e -= 1;
    }

    m = and_128 (m, MASK71);

    if (s)
      m = or_128 (m, SIGN72);
# else // ! NEED_128
    while (s == !! (m & BIT71)) { // until C(AQ)0 != C(AQ)1?
      m <<= 1;
      e -= 1;
    }

    m &= MASK71;

    if (s)
      m |= SIGN72;
# endif // ! NEED_128
  } // ! isHex
#else // ! HEX_MODE
# if defined(NEED_128)
  while (s == isnonzero_128 (and_128 (m, BIT71))) { // until C(AQ)0 != C(AQ)1?
    m = lshift_128 (m, 1);
    e -= 1;
  }

  m = and_128 (m, MASK71);

  if (s)
    m = or_128 (m, SIGN72);
# else // ! NEED_128
  while (s == !! (m & BIT71)) { // until C(AQ)0 != C(AQ)1?
    m <<= 1;
    e -= 1;
  }

  m &= MASK71;

  if (s)
    m |= SIGN72;
# endif // ! NEED_128
#endif // ! HEX_MODE

  if (e < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "fno exp underflow fault");
  }

  * E = (word8) e & MASK8;
  convert_to_word36 (m, A, Q);

  // EAQ is normalized, so if A is 0, so is Q, and the check can be elided
  if (* A == 0)    // set to normalized 0
    * E = 0200U; /* -128 */

  // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
  SC_I_ZERO (* A == 0 && * Q == 0);

  // Neg: If C(AQ)0 = 1, then ON; otherwise OFF
  SC_I_NEG ((* A) & SIGN36);

  * e0 = e;
}

#if 0
// XXX eventually replace fno() with fnoEAQ()
void fnoEAQ(word8 *E, word36 *A, word36 *Q)
{
    // The fno instruction normalizes the number in C(EAQ) if C(AQ) ≠ 0 and the overflow indicator is OFF.
    //    A normalized floating number is defined as one whose mantissa lies in the interval [0.5,1.0) such that
    //    0.5<= |C(AQ)| <1.0 which, in turn, requires that C(AQ)0 ≠ C(AQ)1.
    //    If the overflow indicator is ON, then C(AQ) is shifted one place to the right, C(AQ)0 is inverted to
    //    reconstitute the actual sign, and the overflow indicator is set OFF. This action makes the fno instruction
    //    useful in correcting overflows that occur with fixed point numbers.
    //  Normalization is performed by shifting C(AQ)1,71 one place to the left and reducing C(E) by 1, repeatedly,
    //  until the conditions for C(AQ)0 and C(AQ)1 are met. Bits shifted out of AQ1 are lost.
    //  If C(AQ) = 0, then C(E) is set to -128 and the zero indicator is set ON.

    float72 m = ((word72)*A << 36) | (word72)*Q;
    if (TST_I_OFLOW)
    {
        m >>= 1;
        m &= MASK72;

        m ^= ((word72)1 << 71);

        CLR_I_OFLOW;

        // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
        //SC_I_ZERO (*E == -128 && m == 0);
        //SC_I_ZERO (*E == 0200U /*-128*/ && m == 0);
        if (m == 0)
        {
            *E = -128;
            SET_I_ZERO;
        }
        // Neg:
        CLR_I_NEG;
        return; // XXX: ???
    }

    // only normalize C(EAQ) if C(AQ) ≠ 0 and the overflow indicator is OFF
    if (m == 0) // C(AQ) == 0.
    {
        *A = (m >> 36) & MASK36;
        *Q = m & MASK36;
        *E = 0200U; /*-128*/

        // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
        //SC_I_ZERO(*E == -128 && m == 0);
        //SC_I_ZERO(*E == 0200U /*-128*/ && m == 0);
        SET_I_ZERO;
        // Neg:
        CLR_I_NEG;

        return;
    }
    int8   e = (int8)*E;

    bool s = m & SIGN72;    ///< save sign bit
    while ((bool)(m & SIGN72) == (bool)(m & (SIGN72 >> 1))) // until C(AQ)0 ≠ C(AQ)1?
    {
        m <<= 1;
        m &= MASK72;

        if (s)
            m |= SIGN72;

        if ((e - 1) < -128)
            SET_I_EUFL;
        else    // XXX: my interpretation
            e -= 1;

        if (m == 0) // XXX: necessary??
        {
            *E = (word8)-128;
            break;
        }
    }

    *E = e & 0377;
    *A = (m >> 36) & MASK36;
    *Q = m & MASK36;

    // EAQ is normalized, so if A is 0, so is Q, and the check can be elided
    if (*A == 0)    // set to normalized 0
        *E = (word8)-128;

    // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
    SC_I_ZERO (*A == 0);

    // Neg: If C(AQ)0 = 1, then ON; otherwise OFF
    SC_I_NEG (*A & SIGN36);

}
#endif

/*
 * floating negate ...
 */
void fneg (void) {
  // This instruction changes the number in C(EAQ) to its normalized negative
  // (if C(AQ) ≠ 0). The operation is executed by first forming the twos
  // complement of C(AQ), and then normalizing C(EAQ).
  //
  // Even if originally C(EAQ) were normalized, an exponent overflow can
  // still occur, namely when C(E) = +127 and C(AQ) = 100...0 which is the
  // twos complement approximation for the decimal value -1.0.

  CPTUR (cptUseE);
  // Form the mantissa from AQ

  word72 m = convert_to_word72 (cpu.rA, cpu.rQ);

  // If the mantissa is 4000...00 (least negative value, it is negable in
  // two's complement arithmetic. Divide it by 2, losing a bit of precision,
  // and increment the exponent.
  if (ISEQ_128 (m, SIGN72)) {
    // Negation of 400..0 / 2 is 200..0; we can get there shifting; we know
    // that a zero will be shifted into the sign bit because of the masking
    // in 'm='.
#if defined(NEED_128)
    m = rshift_128 (m, 1);
#else
    m >>= 1;
#endif
    // Increment the exp, checking for overflow.
    if (cpu.rE == 127) {
      SET_I_EOFL;
      if (tstOVFfault ())
        dlyDoFault (FAULT_OFL, fst_zero, "fneg exp overflow fault");
    }
    cpu.rE ++;
    cpu.rE &= MASK8;
  } else {
    // Do the negation
#if defined(NEED_128)
    m = negate_128 (m);
#else
    // m = -m;
    // ubsan
    m = (word72) (- (word72s) m);
#endif
  }
  convert_to_word36 (m, & cpu.rA, & cpu.rQ);
  fno (& cpu.rE, & cpu.rA, & cpu.rQ);  // normalize
#if defined(TESTING)
  HDBGRegAW ("fneg");
#endif
}

/*
 * Unnormalized Floating Multiply ...
 */
void ufm (bool normalize) {
  // The ufm instruction is executed as follows:
  //      C(E) + C(Y)0,7 → C(E)
  //      ( C(AQ) × C(Y)8,35 )0,71 → C(AQ)

  // Zero: If C(AQ) = 0, then ON; otherwise OFF
  // Neg: If C(AQ)0 = 1, then ON; otherwise OFF
  // Exp Ovr: If exponent is greater than +127, then ON
  // Exp Undr: If exponent is less than -128, then ON

  CPTUR (cptUseE);
  word72 m1 = convert_to_word72 (cpu.rA, cpu.rQ);
  int    e1 = SIGNEXT8_int (cpu.rE & MASK8);
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.CY, 8)), 44u); // 28-bit mantissa (incl sign)
#else
  word72 m2 = ((word72) getbits36_28 (cpu.CY, 8)) << 44; ///< 28-bit mantissa (incl sign)
#endif
  int    e2 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));

  if (ISZERO_128 (m1) || ISZERO_128 (m2)) {
    SET_I_ZERO;
    CLR_I_NEG;

    cpu.rE = 0200U; /*-128*/
    cpu.rA = 0;
#if defined(TESTING)
    HDBGRegAW ("ufm");
#endif
    cpu.rQ = 0;

    return; // normalized 0
  }

  int e3 = e1 + e2;

#if 0
  if (e3 >  127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "ufm exp overflow fault");
  }
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "ufm exp underflow fault");
  }
#endif

  // RJ78: This multiplication is executed in the following way:
  // C(E) + C(Y)(0-7) -> C(E)
  // C(AQ) * C(Y)(8-35) results in a 98-bit product plus sign,
  // the leading 71 bits plus sign of which -> C(AQ).

  // shift the CY mantissa to get 98 bits precision
#if defined(NEED_128)
  int128 t = SIGNEXT72_128(m2);
  uint128 ut = rshift_128 (cast_128 (t), 44);
  int128 m3 = multiply_s128 (SIGNEXT72_128(m1), cast_s128 (ut));
//sim_debug (DBG_TRACEEXT, & cpu_dev, "m3 %016"PRIx64"%016"PRIx64"\n", m3.h, m3.l);
#else
  int128 m3 = (SIGNEXT72_128(m1) * (SIGNEXT72_128(m2) >> 44));
//sim_debug (DBG_TRACEEXT, & cpu_dev, "m3 %016"PRIx64"%016"PRIx64"\n", (uint64_t) (m3>>64), (uint64_t) m3);
#endif
  // realign to 72bits
#if defined(NEED_128)
  word72 m3a = and_128 (rshift_128 (cast_128 (m3), 98u - 71u), MASK72);
//sim_debug (DBG_TRACEEXT, & cpu_dev, "m3a %016"PRIx64"%016"PRIx64"\n", m3a.h, m3a.l);
#else
  word72 m3a = ((word72) (m3 >> (98-71))) & MASK72;
//sim_debug (DBG_TRACEEXT, & cpu_dev, "m3a %016"PRIx64"%016"PRIx64"\n", (uint64_t) (m3a>>64), (uint64_t) m3a);
#endif

  // A normalization is performed only in the case of both factor mantissas being 100...0
  // which is the twos complement approximation to the decimal value -1.0.
  if (ISEQ_128 (m1, SIGN72) && ISEQ_128 (m2, SIGN72)) {
    if (e3 == 127) {
      SET_I_EOFL;
      if (tstOVFfault ())
        dlyDoFault (FAULT_OFL, fst_zero, "ufm exp overflow fault");
    }
#if defined(NEED_128)
    m3a = rshift_128 (m3a, 1);
#else
    m3a >>= 1;
#endif
    e3 += 1;
  }

  convert_to_word36 (m3a, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("ufm");
#endif
  cpu.rE = (word8) e3 & MASK8;
sim_debug (DBG_TRACEEXT, & cpu_dev, "fmp A %012"PRIo64" Q %012"PRIo64" E %03o\n", cpu.rA, cpu.rQ, cpu.rE);
  SC_I_NEG (cpu.rA & SIGN36);

  if (cpu.rA == 0 && cpu.rQ == 0) {
    SET_I_ZERO;
    cpu.rE = 0200U; /*-128*/
  } else {
    CLR_I_ZERO;
  }

  if (normalize) {
    fno_ext (& e3, & cpu.rE, & cpu.rA, & cpu.rQ);
  }

  if (e3 >  127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "ufm exp overflow fault");
  }
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "ufm exp underflow fault");
  }
}

/*
 * floating divide ...
 */
// CANFAULT
static void fdvX (bool bInvert) {
  // C(EAQ) / C (Y) → C(EA)
  // C(Y) / C(EAQ) → C(EA) (Inverted)

  // 00...0 → C(Q)

  // The fdv instruction is executed as follows:
  // The dividend mantissa C(AQ) is shifted right and the dividend exponent
  // C(E) increased accordingly until | C(AQ)0,27 | < | C(Y)8,35 |
  // C(E) - C(Y)0,7 → C(E)
  // C(AQ) / C(Y)8,35 → C(A)
  // 00...0 → C(Q)

  CPTUR (cptUseE);
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  word72 m1;
  int    e1;

  word72 m2;
  int    e2;

  bool roundovf = 0;

  if (! bInvert) {
    m1 = convert_to_word72 (cpu.rA, cpu.rQ);
    e1 = SIGNEXT8_int (cpu.rE & MASK8);

#if defined(NEED_128)
    m2 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.CY, 8)), 44u); // 28-bit mantissa (incl sign)
#else
    m2 = ((word72) getbits36_28 (cpu.CY, 8)) << 44; ///< 28-bit mantissa (incl sign)
#endif
    e2 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));

  } else { // invert

    m2 = convert_to_word72 (cpu.rA, cpu.rQ);
    e2 = SIGNEXT8_int (cpu.rE & MASK8);

    // round divisor per RJ78
    // If AQ(28-71) is not equal to 0 and A(0) = 0, then 1 is added to AQ(27).
    // 0 -> AQ(28-71) unconditionally. AQ(0-27) is then used as the divisor mantissa.
#if defined(NEED_128)
    if ((iszero_128 (and_128 (m2, SIGN72))) &&
        (isnonzero_128 (and_128 (m2, construct_128 (0, 0377777777777777LL))))) {
      m2  = add_128 (m2, construct_128 (0, 0400000000000000LL));
      // I surmise that the divisor is taken as unsigned 28 bits in this case
      roundovf = 1;
    }
    m2 = and_128 (m2, lshift_128 (construct_128 (0, 0777777777400), 36));

    m1 = lshift_128 (construct_128 (0, getbits36_28 (cpu.CY, 8)), 44); ///< 28-bit mantissa (incl sign)
    e1 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));
#else
    if (!(m2 & SIGN72) && m2 & 0377777777777777LL) {
        m2 += 0400000000000000LL;
        // I surmise that the divisor is taken as unsigned 28 bits in this case
      roundovf = 1;
    }
    m2 &= (word72)0777777777400 << 36;

    m1 = ((word72) getbits36_28 (cpu.CY, 8)) << 44; ///< 28-bit mantissa (incl sign)
    e1 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));
#endif
  }

  if (ISZERO_128 (m1)) {
    SET_I_ZERO;
    CLR_I_NEG;

    cpu.rE = 0200U; /*-128*/
    cpu.rA = 0;
#if defined(TESTING)
    HDBGRegAW ("fdvX");
#endif
    cpu.rQ = 0;

    return; // normalized 0
  }

  // make everything positive, but save sign info for later....
  int sign = 1;
#if defined(NEED_128)
  if (isnonzero_128 (and_128 (m1, SIGN72)))
#else
  if (m1 & SIGN72)
#endif
  {
    SET_I_NEG; // in case of divide fault
#if defined(NEED_128)
    if (iseq_128 (m1, SIGN72)) {
      m1 = rshift_128 (m1, shift_amt);
      e1 += 1;
    } else {
      m1 = and_128 (negate_128 (m1), MASK72);
    }
#else
    if (m1 == SIGN72) {
      m1 >>= shift_amt;
      e1 += 1;
    } else {
      m1 = (~m1 + 1) & MASK72;
    }
#endif
    sign = -sign;
  } else {
    CLR_I_NEG; // in case of divide fault
  }

#if defined(NEED_128)
  if ((isnonzero_128 (and_128 (m2, SIGN72))) && !roundovf) {
    if (iseq_128 (m2, SIGN72)) {
      m2 = rshift_128 (m2, shift_amt);
      e2 += 1;
    } else {
      m2 = and_128 (negate_128 (m2), MASK72);
    }
    sign = -sign;
  }
#else
  if ((m2 & SIGN72) && !roundovf) {
    if (m2 == SIGN72) {
      m2 >>= shift_amt;
      e2 += 1;
    } else {
      m2 = (~m2 + 1) & MASK72;
    }
    sign = -sign;
  }
#endif

  if (ISZERO_128 (m2)) {
    // NB: If C(Y)8,35 ==0 then the alignment loop will never exit! That's why it been moved before the alignment

    SET_I_ZERO;
    // NEG already set

    // FDV: If the divisor mantissa C(Y)8,35 is zero after alignment (HWR: why after?), the division does
    // not take place. Instead, a divide check fault occurs, C(AQ) contains the dividend magnitude, and
    // the negative indicator reflects the dividend sign.
    // FDI: If the divisor mantissa C(AQ) is zero, the division does not take place.
    // Instead, a divide check fault occurs and all the registers remain unchanged.
    if (! bInvert) {
      convert_to_word36 (m1, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
      HDBGRegAW ("fdvX");
#endif
    }

    doFault (FAULT_DIV, fst_zero, "FDV: divide check fault");
  }

#if defined(NEED_128)
  while (isge_128 (m1, m2)) {
    // DH02 (equivalent but perhaps clearer description):
    // dividend exponent C(E) increased accordingly until | C(AQ)0,71 | < | C(Y)8,35 with zero fill |
    // We have already taken the absolute value so just shift it
    m1 = rshift_128 (m1, shift_amt);
    e1 += 1;
  }
#else
  while (m1 >= m2) {
    // DH02 (equivalent but perhaps clearer description):
    // dividend exponent C(E) increased accordingly until | C(AQ)0,71 | < | C(Y)8,35 with zero fill |
    // We have already taken the absolute value so just shift it
    m1 >>= shift_amt;
    e1 += 1;
  }
#endif

  int e3 = e1 - e2;

  if (e3 > 127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "fdvX exp overflow fault");
  }
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "fdvX exp underflow fault");
  }

  // We need 35 bits quotient + sign. Divisor is at most 28 bits.
  // Do a 63(28+35) by 35 fractional divide
  // lo 44bits are always zero
#if defined(NEED_128)
  uint32_t divisor = rshift_128 (m2, 44).l & MASK28;
  word72 m3;
  if (divisor > MASK16)
    m3 = divide_128_32 (rshift_128 (m1, (44u-35u)), divisor, NULL);
  else
    m3 = divide_128_16 (rshift_128 (m1, (44u-35u)), (uint16_t) divisor, NULL);
#else
  word72 m3 = (m1 >> (44-35)) / (m2 >> 44);
#endif

#if defined(NEED_128)
  m3 = lshift_128 (m3, 36);
  if (sign == -1)
    m3 = and_128 (negate_128 (m3), MASK72);
#else
  m3 <<= 36; // convert back to float
  if (sign == -1)
    m3 = (~m3 + 1) & MASK72;
#endif

  cpu.rE = (word8) e3 & MASK8;
#if defined(NEED_128)
  cpu.rA = rshift_128 (m3, 36u).l & MASK36;
#else
  cpu.rA = (m3 >> 36) & MASK36;
#endif
#if defined(TESTING)
  HDBGRegAW ("fdvX");
#endif
  cpu.rQ = 0;

  SC_I_ZERO (cpu . rA == 0);
  SC_I_NEG (cpu . rA & SIGN36);

  if (cpu.rA == 0)    // set to normalized 0
    cpu.rE = 0200U; /*-128*/
} // fdvX

void fdv (void) {
  fdvX (false);    // no inversion
}

void fdi (void) {
  fdvX (true);      // invert
}

/*
 * single precision floating round ...
 */
void frd (void) {
  // If C(AQ) != 0, the frd instruction performs a true round to a precision
  // of 28 bits and a normalization on C(EAQ).
  // A true round is a rounding operation such that the sum of the result of
  // applying the operation to two numbers of equal magnitude but opposite
  // sign is exactly zero.

  // The frd instruction is executed as follows:
  // C(AQ) + (11...1)29,71 -> C(AQ)
  // If C(AQ)0 = 0, then a carry is added at AQ71
  // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is
  // increased by 1.
  // If overflow does not occur, C(EAQ) is normalized.
  // If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.

  // I believe AL39 is incorrect; bits 28-71 should be set to 0, not 29-71.
  // DH02-01 & Bull 9000 is correct.

  // test case 15.5
  //                 rE                     rA                                     rQ
  // 014174000000 00000110 000111110000000000000000000000000000 000000000000000000000000000000000000
  // +                                                  1111111 111111111111111111111111111111111111
  // =            00000110 000111110000000000000000000001111111 111111111111111111111111111111111111
  // If C(AQ)0 = 0, then a carry is added at AQ71
  // =            00000110 000111110000000000000000000010000000 000000000000000000000000000000000000
  // 0 → C(AQ)29,71
  //              00000110 000111110000000000000000000010000000 000000000000000000000000000000000000
  // after normalization .....
  // 010760000002 00000100 011111000000000000000000001000000000 000000000000000000000000000000000000
  // I think this is wrong

  // 0 -> C(AQ)28,71
  //              00000110 000111110000000000000000000000000000 000000000000000000000000000000000000
  // after normalization .....
  // 010760000000 00000100 011111000000000000000000000000000000 000000000000000000000000000000000000
  // which I think is correct

  //
  // GE CPB1004F, DH02-01 (DPS8/88) & Bull DPS9000 assembly ... have this ...

  // The rounding operation is performed in the following way.
  // -  a) A constant (all 1s) is added to bits 29-71 of the mantissa.
  // -  b) If the number being rounded is positive, a carry is inserted into
  // the least significant bit position of the adder.
  // -  c) If the number being rounded is negative, the carry is not inserted.
  // -  d) Bits 28-71 of C(AQ) are replaced by zeros.
  // If the mantissa overflows upon rounding, it is shifted right one place
  // and a corresponding correction is made to the exponent.
  // If the mantissa does not overflow and is nonzero upon rounding,
  // normalization is performed.

  // If the resultant mantissa is all zeros, the exponent is forced to -128
  // and the zero indicator is set.
  // If the exponent resulting from the operation is greater than +127, the
  // exponent Overflow indicator is set.
  // If the exponent resulting from the operation is less than -128, the
  // exponent Underflow indicator is set.
  // The definition of normalization is located under the description of the FNO instruction.

  // So, Either AL39 is wrong or the DPS8m did it wrong. (Which was fixed in
  // later models.) I'll assume AL39 is wrong.

  CPTUR (cptUseE);
  L68_ (cpu.ou.cycle |= ou_GOS;)

  word72 m = convert_to_word72 (cpu.rA, cpu.rQ);
  if (ISZERO_128 (m)) {
    cpu.rE = 0200U; /*-128*/
    SET_I_ZERO;
    CLR_I_NEG;
    return;
  }

  // C(AQ) + (11...1)29,71 → C(AQ)
  bool ovf;
  word18 flags1 = 0;
  word1 carry = 0;
  // If C(AQ)0 = 0, then a carry is added at AQ71
#if defined(NEED_128)
  if (iszero_128 (and_128 (m, SIGN72)))
#else
  if ((m & SIGN72) == 0)
#endif
  {
    carry = 1;
  }
#if defined(NEED_128)
  m = Add72b (m, construct_128 (0, 0177777777777777LL), carry, I_OFLOW, & flags1, & ovf);
#else
  m = Add72b (m, 0177777777777777LL, carry, I_OFLOW, & flags1, & ovf);
#endif

  // 0 -> C(AQ)28,71  (per. RJ78)
#if defined(NEED_128)
  m = and_128 (m, lshift_128 (construct_128 (0, 0777777777400), 36));
#else
  m &= ((word72)0777777777400 << 36);
#endif

  // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is
  // increased by 1.
  // If overflow does not occur, C(EAQ) is normalized.
  // All of this is done by fno, we just need to save the overflow flag

  bool savedovf = TST_I_OFLOW;
  SC_I_OFLOW(ovf);
  convert_to_word36 (m, & cpu.rA, & cpu.rQ);

  fno (& cpu.rE, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("frd");
#endif
  SC_I_OFLOW(savedovf);
}

void fstr (word36 *Y)
  {
    // The fstr instruction performs a true round and normalization on C(EAQ)
    // as it is stored.  The definition of true round is located under the
    // description of the frd instruction.  The definition of normalization is
    // located under the description of the fno instruction.  Attempted
    // repetition with the rpl instruction causes an illegal procedure fault.

    L68_ (cpu.ou.cycle |= ou_GOS;)
    word36 A = cpu . rA, Q = cpu . rQ;
    word8 E = cpu . rE;
    //A &= DMASK;
    //Q &= DMASK;
    //E &= (int) MASK8;

    float72 m = convert_to_word72 (A, Q);
#if defined(NEED_128)
    if (iszero_128 (m))
#else
    if (m == 0)
#endif
      {
        E = (word8)-128;
        SET_I_ZERO;
        CLR_I_NEG;
        *Y = 0;
        putbits36_8 (Y, 0, (word8) E & MASK8);
        return;
      }

    // C(AQ) + (11...1)29,71 → C(AQ)
    bool ovf;
    word18 flags1 = 0;
    word1 carry = 0;
    // If C(AQ)0 = 0, then a carry is added at AQ71
#if defined(NEED_128)
    if (iszero_128 (and_128 (m, SIGN72)))
#else
    if ((m & SIGN72) == 0)
#endif
      {
        carry = 1;
      }
#if defined(NEED_128)
    m = Add72b (m, construct_128 (0, 0177777777777777LL), carry, I_OFLOW, & flags1, & ovf);
#else
    m = Add72b (m, 0177777777777777LL, carry, I_OFLOW, & flags1, & ovf);
#endif

    // 0 -> C(AQ)28,71  (per. RJ78)
#if defined(NEED_128)
    m = and_128 (m, lshift_128 (construct_128 (0, 0777777777400), 36));
#else
    m &= ((word72)0777777777400 << 36);
#endif

    // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is
    // increased by 1.
    // If overflow does not occur, C(EAQ) is normalized.
    // All of this is done by fno, we just need to save the overflow flag

    bool savedovf = TST_I_OFLOW;
    SC_I_OFLOW(ovf);
    convert_to_word36 (m, & A, & Q);
    fno (& E, & A, & Q);
    SC_I_OFLOW(savedovf);

    * Y = setbits36_8 (A >> 8, 0, (word8) E);
  }

/*
 * single precision Floating Compare ...
 */
void fcmp (void) {
  // C(E) :: C(Y)0,7
  // C(AQ)0,27 :: C(Y)8,35

  // Zero: If C(EAQ) = C(Y), then ON; otherwise OFF
  // Neg: If C(EAQ) < C(Y), then ON; otherwise OFF

  // Notes: The fcmp instruction is executed as follows:
  // The mantissas are aligned by shifting the mantissa of the operand with
  // the algebraically smaller exponent to the right the number of places
  // equal to the difference in the two exponents.
  // The aligned mantissas are compared and the indicators set accordingly.

  CPTUR (cptUseE);
#if defined(NEED_128)
  word72 m1= lshift_128 (construct_128 (0, cpu.rA & 0777777777400), 36);
#else
  word72 m1 = ((word72)cpu.rA & 0777777777400LL) << 36;
#endif
  int    e1 = SIGNEXT8_int (cpu.rE & MASK8);

  // 28-bit mantissa (incl sign)
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, getbits36_28 (cpu.CY, 8)), 44);
#else
  word72 m2 = ((word72) getbits36_28 (cpu.CY, 8)) << 44;
#endif
  int    e2 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));

  // Which exponent is smaller???

  L68_ (cpu.ou.cycle = ou_GOE;)
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  int shift_count = -1;
  word1 notallzeros = 0;

  if (e1 == e2) {
    shift_count = 0;
  } else if (e1 < e2) {
    L68_ (cpu.ou.cycle = ou_GOA;)
    shift_count = abs (e2 - e1) * (int) shift_amt;
    // mantissa negative?
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m1, SIGN72));
#else
    bool s = (m1 & SIGN72) != (word72)0;
#endif
    for(int n = 0; n < shift_count; n += 1) {
#if defined(NEED_128)
      notallzeros |= m1.l & 1;
      m1 = rshift_128 (m1, 1);
#else
      notallzeros |= m1 & 1;
      m1 >>= 1;
#endif
      if (s)
#if defined(NEED_128)
        m1 = or_128 (m1, SIGN72);
#else
        m1 |= SIGN72;
#endif
    }
#if defined(NEED_128)
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = construct_128 (0, 0);
    m1 = and_128 (m1, MASK72);
#else // NEED_128
    if (m1 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = 0;
    m1 &= MASK72;
#endif // NEED_128
  } else { // e2 < e1;
    L68_ (cpu.ou.cycle = ou_GOA;)
    shift_count = abs (e1 - e2) * (int) shift_amt;
    // mantissa negative?
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m2, SIGN72));
#else
    bool s = (m2 & SIGN72) != (word72)0;
#endif
    for (int n = 0 ; n < shift_count ; n += 1) {
#if defined(NEED_128)
      notallzeros |= m2.l & 1;
      m2 = rshift_128 (m2, 1);
#else
      notallzeros |= m2 & 1;
      m2 >>= 1;
#endif
      if (s)
#if defined(NEED_128)
        m2 = or_128 (m2, SIGN72);
#else
        m2 |= SIGN72;
#endif
    } // for n
#if defined(NEED_128)
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = construct_128 (0, 0);
    m2 = and_128 (m2, MASK72);
    //e3 = e1;
#else
    if (m2 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = 0;
    m2 &= MASK72;
    //e3 = e1;
#endif
  }

  // need to do algebraic comparisons of mantissae
#if defined(NEED_128)
  SC_I_ZERO (iseq_128 (m1, m2));
  SC_I_NEG (islt_s128 (SIGNEXT72_128(m1), SIGNEXT72_128(m2)));
#else
  SC_I_ZERO (m1 == m2);
  SC_I_NEG ((int128)SIGNEXT72_128(m1) < (int128)SIGNEXT72_128(m2));
#endif
} // fcmp

/*
 * single precision Floating Compare magnitude ...
 */
void fcmg (void) {
  // C(E) :: C(Y)0,7
  // | C(AQ)0,27 | :: | C(Y)8,35 |
  // * Zero: If | C(EAQ)| = | C(Y) |, then ON; otherwise OFF
  // * Neg : If | C(EAQ)| < | C(Y) |, then ON; otherwise OFF

  // Notes: The fcmp instruction is executed as follows:
  // The mantissas are aligned by shifting the mantissa of the operand with
  // the algebraically smaller exponent to the right the number of places
  // equal to the difference in the two exponents.
  // The aligned mantissas are compared and the indicators set accordingly.

  // The fcmg instruction is identical to the fcmp instruction except that the
  // magnitudes of the mantissas are compared instead of the algebraic values.

  // ISOLTS-736 01u asserts that |0.0*2^64|<|1.0| in 28bit precision
  // this implies that all shifts are 72 bits long
  // RJ78 also comments: If the number of shifts equals or exceeds 72, the
  // number with the lower exponent is defined as zero.

  CPTUR (cptUseE);
  L68_ (cpu.ou.cycle = ou_GOS;)
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  // C(AQ)0,27
#if defined(NEED_128)
  word72 m1 = lshift_128 (construct_128 (0, cpu.rA & 0777777777400), 36);
#else
  word72 m1 = ((word72)cpu.rA & 0777777777400LL) << 36;
#endif
  int    e1 = SIGNEXT8_int (cpu.rE & MASK8);

  // C(Y)0,7
  // 28-bit mantissa (incl sign)
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, getbits36_28 (cpu.CY, 8)), 44);
#else
  word72 m2 = ((word72) getbits36_28 (cpu.CY, 8)) << 44;
#endif
  int    e2 = SIGNEXT8_int (getbits36_8 (cpu.CY, 0));

  // Which exponent is smaller???

  L68_ (cpu.ou.cycle = ou_GOE;)
  int shift_count = -1;
  word1 notallzeros = 0;

  if (e1 == e2) {
    shift_count = 0;
  } else if (e1 < e2) {
    L68_ (cpu.ou.cycle = ou_GOA;)
    shift_count = abs (e2 - e1) * (int) shift_amt;
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m1, SIGN72));
#else
    bool s = (m1 & SIGN72) != (word72)0;    ///< save sign bit
#endif
    for (int n = 0; n < shift_count; n += 1) {
#if defined(NEED_128)
      notallzeros |= m1.l & 1;
      m1 = rshift_128 (m1, 1);
#else
      notallzeros |= m1 & 1;
      m1 >>= 1;
#endif
    if (s)
#if defined(NEED_128)
      m1 = or_128 (m1, SIGN72);
#else
      m1 |= SIGN72;
#endif
    }

#if defined(NEED_128)
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = construct_128 (0, 0);
    m1 = and_128 (m1, MASK72);
#else
    if (m1 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = 0;
    m1 &= MASK72;
#endif
  } else { // e2 < e1;
    L68_ (cpu.ou.cycle = ou_GOA;)
    shift_count = abs(e1 - e2) * (int) shift_amt;
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m2, SIGN72));
#else
    bool s = (m2 & SIGN72) != (word72)0;    ///< save sign bit
#endif
    for (int n = 0; n < shift_count; n += 1) {
#if defined(NEED_128)
      notallzeros |= m2.l & 1;
      m2 = rshift_128 (m2, 1);
#else
      notallzeros |= m2 & 1;
      m2 >>= 1;
#endif
      if (s)
#if defined(NEED_128)
        m2 = or_128 (m2, SIGN72);
#else
        m2 |= SIGN72;
#endif
    }
#if defined(NEED_128)
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = construct_128 (0, 0);
    m2 = and_128 (m2, MASK72);
#else
    if (m2 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = 0;
    m2 &= MASK72;
#endif
  }

  SC_I_ZERO (ISEQ_128 (m1, m2));
#if defined(NEED_128)
  int128 sm1 = SIGNEXT72_128 (m1);
  if (islt_s128 (sm1, construct_s128 (0, 0)))
    sm1 = negate_s128 (sm1);
  int128 sm2 = SIGNEXT72_128 (m2);
  if (islt_s128 (sm2, construct_s128 (0, 0)))
    sm2 = negate_s128 (sm2);
  SC_I_NEG (islt_s128 (sm1, sm2));
#else
  int128 sm1 = SIGNEXT72_128 (m1);
  if (sm1 < 0)
    sm1 = - sm1;
  int128 sm2 = SIGNEXT72_128 (m2);
  if (sm2 < 0)
    sm2 = - sm2;
  SC_I_NEG (sm1 < sm2);
#endif
}

/*
 * double-precision arithmetic routines ....
 */

#if 0
// extract mantissa + exponent from a YPair ....
static void YPairToExpMant(word36 Ypair[], word72 *mant, int *exp)
{
    //*mant = (word72)bitfieldExtract36(Ypair[0], 0, 28) << 44;   // 28-bit mantissa (incl sign)
    *mant = ((word72) getbits36_28 (Ypair[0], 8)) << 44;   // 28-bit mantissa (incl sign)
    *mant |= (((word72) Ypair[1]) & DMASK) << 8;
    //*exp = SIGNEXT8_int (bitfieldExtract36(Ypair[0], 28, 8) & 0377U);           // 8-bit signed integer (incl sign)
    *exp = SIGNEXT8_int (getbits36_8 (Ypair[0], 0) & 0377U);           // 8-bit signed integer (incl sign)
}

//! combine mantissa + exponent into a YPair ....
static void ExpMantToYpair(word72 mant, int exp, word36 *yPair)
{
    yPair[0] = ((word36)exp & 0377) << 28;
    yPair[0] |= (mant >> 44) & 01777777777LL;
    yPair[1] = (mant >> 8) & 0777777777777LL;   //400LL;
}
#endif

/*!
 * unnormalized floating double-precision add
 */
void dufa (bool subtract, bool normalize) {
  // Except for the precision of the mantissa of the operand from main
  // memory, the dufa instruction is identical to the ufa instruction.

  // C(EAQ) + C(Y) → C(EAQ)
  // The ufa instruction is executed as follows:
  // The mantissas are aligned by shifting the mantissa of the operand having
  // the algebraically smaller exponent to the right the number of places
  // equal to the absolute value of the difference in the two exponents. Bits
  // shifted beyond the bit position equivalent to AQ71 are lost.
  // The algebraically larger exponent replaces C(E). The sum of the
  // mantissas replaces C(AQ).
  // If an overflow occurs during addition, then;
  // *  C(AQ) are shifted one place to the right.
  // *  C(AQ)0 is inverted to restore the sign.
  // *  C(E) is increased by one.

  // The dufs instruction is identical to the dufa instruction with the
  // exception that the twos complement of the mantissa of the operand from
  // main memory (op2) is used.

  CPTUR (cptUseE);
  L68_ (cpu.ou.cycle = ou_GOS;)
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#else
  uint shift_amt = 1;
#endif

  word72 m1 = convert_to_word72 (cpu.rA, cpu.rQ);
  int e1 = SIGNEXT8_int (cpu.rE & MASK8);

  // 64-bit mantissa (incl sign)
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.Ypair[0], 8)), 44u); // 28-bit mantissa (incl sign)
         m2 = or_128 (m2, lshift_128 (construct_128 (0, cpu.Ypair[1]), 8u));
#else
  word72 m2 = ((word72) getbits36_28 (cpu.Ypair[0], 8)) << 44;
         m2 |= (word72) cpu.Ypair[1] << 8;
#endif

  int e2 = SIGNEXT8_int (getbits36_8 (cpu.Ypair[0], 0));

  // see ufs
  int m2zero = 0;
  if (subtract) {

#if defined(NEED_128)
    if (iszero_128 (m2))
      m2zero = 1;
    if (iseq_128 (m2, SIGN72)) {
# if defined(HEX_MODE)
      m2 = rshift_128 (m2, shift_amt);
      e2 += 1;
# else
      m2 = rshift_128 (m2, 1);
      e2 += 1;
# endif
    } else {
      m2 = and_128 (negate_128 (m2), MASK72);
    }
#else
    if (m2 == 0)
      m2zero = 1;
    if (m2 == SIGN72) {
# if defined(HEX_MODE)
      m2 >>= shift_amt;
      e2 += 1;
# else
      m2 >>= 1;
      e2 += 1;
# endif
    } else {
      // m2 = (-m2) & MASK72;
      // ubsan
      m2 = ((word72) (- (word72s) m2)) & MASK72;
    }
#endif
  }

  int e3 = -1;

  // Which exponent is smaller?

  L68_ (cpu.ou.cycle |= ou_GOE;)
  int shift_count = -1;
  word1 notallzeros = 0;

  if (e1 == e2) {
    shift_count = 0;
    e3 = e1;
  } else if (e1 < e2) {
    L68_ (cpu.ou.cycle |= ou_GOA;)
    shift_count = abs(e2 - e1) * (int) shift_amt;
    // mantissa negative?
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m1, SIGN72));
#else
    bool s = (m1 & SIGN72) != (word72)0;
#endif
    for (int n = 0; n < shift_count; n += 1) {
#if defined(NEED_128)
      notallzeros |= m1.l & 1;
      m1 = rshift_128 (m1, 1);
#else
      notallzeros |= m1 & 1;
      m1 >>= 1;
#endif
      if (s)
#if defined(NEED_128)
        m1 = or_128 (m1, SIGN72);
#else
        m1 |= SIGN72;
#endif
    }
#if defined(NEED_128)
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = construct_128 (0, 0);
    m1 = and_128 (m1, MASK72);
#else
    if (m1 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = 0;
    m1 &= MASK72;
#endif
    e3 = e2;
  } else { // e2 < e1;
    L68_ (cpu.ou.cycle |= ou_GOA;)
    shift_count = abs(e1 - e2) * (int) shift_amt;
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m2, SIGN72));
#else
    bool s = (m2 & SIGN72) != (word72)0;    ///< save sign bit
#endif
    for (int n = 0; n < shift_count; n += 1) {
#if defined(NEED_128)
      notallzeros |= m2.l & 1;
      m2 = rshift_128 (m2, 1);
#else
      notallzeros |= m2 & 1;
      m2 >>= 1;
#endif
      if (s)
#if defined(NEED_128)
        m2 = or_128 (m2, SIGN72);
#else
        m2 |= SIGN72;
#endif
    }
#if defined(NEED_128)
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = construct_128 (0, 0);
    m2 = and_128 (m2, MASK72);
#else
    if (m2 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = 0;
    m2 &= MASK72;
#endif
    e3 = e1;
  }

  bool ovf;
  word72 m3 = Add72b (m1, m2, 0, I_CARRY, & cpu.cu.IR, & ovf);
  if (m2zero)
    SET_I_CARRY;

  if (ovf) {
#if defined(HEX_MODE)
//    word72 signbit = m3 & sign_msk;
//    m3 >>= shift_amt;
//    m3 = (m3 & MASK71) | signbit;
//    m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
//    e3 += 1;
    // save the sign bit
# if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m3, SIGN72));
# else
    bool s = (m3 & SIGN72) != (word72)0;
# endif
    if (isHex ()) {
# if defined(NEED_128)
      m3 = rshift_128 (m3, shift_amt); // renormalize the mantissa
      if (s)
        // Sign is set, number should be positive; clear the sign bit and the 3 MSBs
        m3 = and_128 (m3, MASK68);
      else
        // Sign is clr, number should be negative; set the sign bit and the 3 MSBs
        m3 = or_128 (m3, HEX_SIGN);
# else
      m3 >>= shift_amt; // renormalize the mantissa
      if (s)
        // Sign is set, number should be positive; clear the sign bit and the 3 MSBs
        m3 &= MASK68;
      else
        // Sign is clr, number should be negative; set the sign bit and the 3 MSBs
        m3 |=  HEX_SIGN;
# endif
    } else {
# if defined(NEED_128)
      word72 signbit = and_128 (m3, SIGN72);
      m3 = rshift_128 (m3, 1); // renormalize the mantissa
      m3 = or_128 (and_128 (m3, MASK71), signbit);
      m3 = xor_128 (m3, SIGN72); // C(AQ)0 is inverted to restore the sign
# else
      word72 signbit = m3 & SIGN72;
      m3 >>= 1;
      m3 = (m3 & MASK71) | signbit;
      m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
# endif
    }
    e3 += 1;
#else
# if defined(NEED_128)
    word72 signbit = and_128 (m3, SIGN72);
    m3 = rshift_128 (m3, 1); // renormalize the mantissa
    m3 = or_128 (and_128 (m3, MASK71), signbit);
    m3 = xor_128 (m3, SIGN72); // C(AQ)0 is inverted to restore the sign
# else
    word72 signbit = m3 & SIGN72;
    m3 >>= 1;
    m3 = (m3 & MASK71) | signbit;
    m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
# endif
    e3 += 1;
#endif
  }

  convert_to_word36 (m3, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("dufa");
#endif
  cpu.rE = e3 & 0377;

  SC_I_NEG (cpu.rA & SIGN36); // Do this here instead of in Add72b because
                                // of ovf handling above
  if (cpu.rA == 0 && cpu.rQ == 0) {
    SET_I_ZERO;
    cpu.rE = 0200U; /*-128*/
  } else {
    CLR_I_ZERO;
  }

  if (normalize) {
    fno_ext (& e3, & cpu.rE, & cpu.rA, & cpu.rQ);
  }

  // EOFL: If exponent is greater than +127, then ON
  if (e3 > 127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dufa exp overflow fault");
  }

  // EUFL: If exponent is less than -128, then ON
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
        dlyDoFault (FAULT_OFL, fst_zero, "dufa exp underflow fault");
  }
} // dufa

#if 0
/*
 * unnormalized floating double-precision subtract
 */
void dufs (void)
{
    // Except for the precision of the mantissa of the operand from main memory,
    // the dufs instruction is identical with the ufs instruction.

    // The ufs instruction is identical to the ufa instruction with the
    // exception that the twos complement of the mantissa of the operand from
    // main memory (op2) is used.

    // They're probably a few gotcha's here but we'll see.
    // Yup ... when mantissa 1 000 000 .... 000 we can't do 2'c comp.

    float72 m2 = 0;
    int e2 = -128;

    YPairToExpMant(cpu.Ypair, &m2, &e2);

    word72 m2c = ~m2 + 1;     ///< & 01777777777LL;     ///< take 2-comp of mantissa
    m2c &= MASK72;

    /*
     * When signs are the *same* after complement we have an overflow
     * (Treat as in addition when we get an overflow)
     */
    bool ov = ((m2 & SIGN72) == (m2c & SIGN72)); ///< the "weird" number.

    if (ov && m2 != 0)
    {
        m2c >>= 1;
        m2c &= MASK72;

        if (e2 == 127)
        {
            SET_I_EOFL;
            if (tstOVFfault ())
                doFault (FAULT_OFL, fst_zero, "dufs exp overflow fault");
        }
        e2 += 1;
    }

    if (m2c == 0)
    {
//      cpu.Ypair[0] = 0400000000000LL;
//      cpu.Ypair[1] = 0;
        ExpMantToYpair(0, -128, cpu.Ypair);
    }
    else
    {
//        cpu.Ypair[0]  = ((word36)e2 & 0377) << 28;
//        cpu.Ypair[0] |= (m2c >> 44) & 01777777777LL;
//        cpu.Ypair[1]  = (m2c >> 8) &  0777777777400LL;
        ExpMantToYpair(m2c, e2, cpu.Ypair);
    }

    dufa ();

}
#endif

/*
 * double-precision Unnormalized Floating Multiply ...
 */
void dufm (bool normalize) {
  // Except for the precision of the mantissa of the operand from main memory,
  //the dufm instruction is identical to the ufm instruction.

  // The ufm instruction is executed as follows:
  //      C(E) + C(Y)0,7 → C(E)
  //      ( C(AQ) × C(Y)8,35 )0,71 → C(AQ)

  // * Zero: If C(AQ) = 0, then ON; otherwise OFF
  // * Neg: If C(AQ)0 = 1, then ON; otherwise OFF
  // * Exp Ovr: If exponent is greater than +127, then ON
  // * Exp Undr: If exponent is less than -128, then ON

  CPTUR (cptUseE);
  L68_ (cpu.ou.cycle |= ou_GOS;)
  word72 m1 = convert_to_word72 (cpu.rA, cpu.rQ);
  int    e1 = SIGNEXT8_int (cpu . rE & MASK8);

#if !defined(NEED_128)
  sim_debug (DBG_TRACEEXT, & cpu_dev, "dufm e1 %d %03o m1 %012"PRIo64" %012"PRIo64"\n",
             e1, e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
#endif
   // 64-bit mantissa (incl sign)
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.Ypair[0], 8)), 44u); // 28-bit mantissa (incl sign)
         m2 = or_128 (m2, lshift_128 (construct_128 (0, cpu.Ypair[1]), 8u));
#else
  word72 m2 = ((word72) getbits36_28 (cpu.Ypair[0], 8)) << 44;
         m2 |= (word72) cpu.Ypair[1] << 8;
#endif

  // 8-bit signed integer (incl sign)
  int    e2 = SIGNEXT8_int (getbits36_8 (cpu.Ypair[0], 0));

#if !defined(NEED_128)
  sim_debug (DBG_TRACEEXT, & cpu_dev, "dufm e2 %d %03o m2 %012"PRIo64" %012"PRIo64"\n",
             e2, e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
#endif

  if (ISZERO_128 (m1) || ISZERO_128 (m2)) {
    SET_I_ZERO;
    CLR_I_NEG;

    cpu.rE = 0200U; /*-128*/
    cpu.rA = 0;
#if defined(TESTING)
    HDBGRegAW ("dufm");
#endif
    cpu.rQ = 0;

    return; // normalized 0
  }

  int e3 = e1 + e2;

#if 0
  if (e3 >  127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dufm exp overflow fault");
  }
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dufm exp underflow fault");
  }
#endif

  // RJ78: This multiplication is executed in the following way:
  // C(E) + C(Y)(0-7) -> C(E)
  // C(AQ) * C(Y-pair)(8-71) results in a 134-bit product plus sign. This sign plus the
  // leading 71 bits are loaded into the AQ.

  // do a 128x64 signed multiplication

  // fast signed multiplication algorithm without 2's complements
  // passes ISOLTS-745 08

#if defined(NEED_128)
  // shift the CY mantissa
  int128 m2s = rshift_s128 (SIGNEXT72_128(m2), 8);

  // do a 128x64 signed multiplication
  int128 m1l = and_s128 (cast_s128 (m1), construct_128 (0, MASK64));
  int128 m1h = rshift_s128 (SIGNEXT72_128(m1), 64);
  int128 m3h = multiply_s128 (m1h, m2s); // hi partial product
  int128 m3l = multiply_s128 (m1l, m2s); // lo partial product

  // realign to 72bits
  m3l = rshift_s128 (m3l, 63);
  m3h = lshift_s128 (m3h, 1); // m3h is hi by 64, align it for addition. The result is 135 bits so this cannot overflow.
  word72 m3a = and_128 (add_128 (cast_128 (m3h), cast_128 (m3l)), MASK72);
#else
  // shift the CY mantissa
  int128 m2s = SIGNEXT72_128(m2) >> 8;

  // do a 128x64 signed multiplication
  int128 m1l = m1 & (((uint128)1<<64)-1);
  int128 m1h = SIGNEXT72_128(m1) >> 64;
  int128 m3h = m1h * m2s; // hi partial product
  int128 m3l = m1l * m2s; // lo partial product

  // realign to 72bits
  m3l >>= 63;
  // m3h <<= 1; // m3h is hi by 64, align it for addition. The result is 135 bits so this cannot overflow.
  // ubsan
  m3h = (int128) (((uint128) m3h) << 1); // m3h is hi by 64, align it for addition. The result is 135 bits so this cannot overflow.
  word72 m3a = ((word72) (m3h+m3l)) & MASK72;
#endif

  // A normalization is performed only in the case of both factor mantissas being 100...0
  // which is the twos complement approximation to the decimal value -1.0.
  if (ISEQ_128 (m1, SIGN72) && ISEQ_128 (m2, SIGN72)) {
    if (e3 == 127) {
      SET_I_EOFL;
      if (tstOVFfault ())
          dlyDoFault (FAULT_OFL, fst_zero, "dufm exp overflow fault");
    }
#if defined(NEED_128)
    m3a = rshift_128 (m3a, 1);
#else
    m3a >>= 1;
#endif
    e3 += 1;
  }

  convert_to_word36 (m3a, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("dufm");
#endif
  cpu.rE = (word8) e3 & MASK8;

  SC_I_NEG (cpu.rA & SIGN36);

  if (cpu.rA == 0 && cpu.rQ == 0) {
    SET_I_ZERO;
    cpu . rE = 0200U; /*-128*/
  } else {
    CLR_I_ZERO;
  }

  if (normalize) {
    fno_ext (& e3, & cpu.rE, & cpu.rA, & cpu.rQ);
  }

  if (e3 >  127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dufm exp overflow fault");
  }
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dufm exp underflow fault");
  }
}

/*
 * floating divide ...
 */
// CANFAULT
static void dfdvX (bool bInvert) {
  // C(EAQ) / C (Y) → C(EA)
  // C(Y) / C(EAQ) → C(EA) (Inverted)

  // 00...0 → C(Q)

  // The dfdv instruction is executed as follows:
  // The dividend mantissa C(AQ) is shifted right and the dividend exponent
  // C(E) increased accordingly until
  //    | C(AQ)0,63 | < | C(Y-pair)8,71 |
  //    C(E) - C(Y-pair)0,7 → C(E)
  //    C(AQ) / C(Y-pair)8,71 → C(AQ)0,63 00...0 → C(Q)64,71

  CPTUR (cptUseE);
  L68_ (cpu.ou.cycle |= ou_GOS;)
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  word72 m1;
  int    e1;

  word72 m2;
  int    e2;

  bool roundovf = 0;

  if (! bInvert) {
    m1 = convert_to_word72 (cpu.rA, cpu.rQ);
    e1 = SIGNEXT8_int (cpu.rE & MASK8);

    // 64-bit mantissa (incl sign)
#if defined(NEED_128)
    m2 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.Ypair[0], 8)), 44u); // 28-bit mantissa (incl sign)
    m2 = or_128 (m2, lshift_128 (construct_128 (0, cpu.Ypair[1]), 8u));
#else
    m2 = ((word72) getbits36_28 (cpu.Ypair[0], 8)) << 44;
    m2 |= (word72) cpu.Ypair[1] << 8;
#endif

    e2 = SIGNEXT8_int (getbits36_8 (cpu.Ypair[0], 0));
  } else { // invert
    m2 = convert_to_word72 (cpu.rA, cpu.rQ);
    e2 = SIGNEXT8_int (cpu.rE & MASK8);

    // round divisor per RJ78
    // If AQ(64-71) is not = 0 and A(0) = 0, a 1 is added to AQ(63). Zero is moved to
    // AQ(64-71), unconditionally. AQ(0-63) is then used as the divisor mantissa.
    // ISOLTS-745 10b
#if defined(NEED_128)
    if ((iszero_128 (and_128 (m2, SIGN72))) && m2.l & 0377)
#else
    if (!(m2 & SIGN72) && m2 & 0377)
#endif
    {
#if defined(NEED_128)
      m2 = add_128 (m2, construct_128 (0, 0400));
#else
      m2 += 0400;
#endif
      // ISOLTS-745 10e asserts that an overflowing addition of 400
      //   to 377777777777 7777777774xx does not shift the quotient (nor divisor)
      // I surmise that the divisor is taken as unsigned 64 bits in this case
      roundovf = 1;
    }
#if defined(NEED_128)
    putbits72 (& m2, 64, 8, construct_128 (0, 0));
#else
    putbits72 (& m2, 64, 8, 0);
#endif

    // 64-bit mantissa (incl sign)
#if defined(NEED_128)
    m1 = lshift_128 (construct_128 (0, (uint64_t) getbits36_28 (cpu.Ypair[0], 8)), 44u); // 28-bit mantissa (incl sign)
    m1 = or_128 (m1, lshift_128 (construct_128 (0, cpu.Ypair[1]), 8u));
#else
    m1 = ((word72) getbits36_28 (cpu.Ypair[0], 8)) << 44;
    m1 |= (word72) cpu.Ypair[1] << 8;
#endif

    e1 = SIGNEXT8_int (getbits36_8 (cpu.Ypair[0], 0));
  } // invert

  if (ISZERO_128 (m1)) {
    SET_I_ZERO;
    CLR_I_NEG;

    cpu.rE = 0200U; /*-128*/
    cpu.rA = 0;
#if defined(TESTING)
    HDBGRegAW ("dfdvX");
#endif
    cpu.rQ = 0;

    return; // normalized 0
  }

    // make everything positive, but save sign info for later....
  int sign = 1;
#if defined(NEED_128)
  if (isnonzero_128 (and_128 (m1, SIGN72))) {
    SET_I_NEG; // in case of divide fault
    if (iseq_128 (m1, SIGN72)) {
      m1 = rshift_128 (m1, shift_amt);
      e1 += 1;
    } else {
      m1 = and_128 (negate_128 (m1), MASK72);
    }
    sign = -sign;
  } else {
    CLR_I_NEG; // in case of divide fault
  }

  if ((isnonzero_128 (and_128 (m2, SIGN72))) && !roundovf) {
    if (iseq_128 (m2, SIGN72)) {
      m2 = rshift_128 (m2, shift_amt);
      e2 += 1;
    } else {
      m2 = and_128 (negate_128 (m2), MASK72);
    }
    sign = -sign;
  }
#else
  if (m1 & SIGN72) {
    SET_I_NEG; // in case of divide fault
    if (m1 == SIGN72) {
      m1 >>= shift_amt;
      e1 += 1;
    } else {
      m1 = (~m1 + 1) & MASK72;
    }
    sign = -sign;
  } else {
    CLR_I_NEG; // in case of divide fault
  }

  if ((m2 & SIGN72) && !roundovf) {
    if (m2 == SIGN72) {
      m2 >>= shift_amt;
      e2 += 1;
    } else {
      m2 = (~m2 + 1) & MASK72;
    }
    sign = -sign;
  }
#endif

  if (ISZERO_128 (m2)) {
    // NB: If C(Y-pair)8,71 == 0 then the alignment loop will never exit! That's why it been moved before the alignment

    SET_I_ZERO;
    // NEG already set

    // FDV: If the divisor mantissa C(Y-pair)8,71 is zero after alignment (HWR: why after?), the division does
    // not take place. Instead, a divide check fault occurs, C(AQ) contains the dividend magnitude, and
    // the negative indicator reflects the dividend sign.
    // FDI: If the divisor mantissa C(AQ) is zero, the division does not take place.
    // Instead, a divide check fault occurs and all the registers remain unchanged.
    if (! bInvert) {
      convert_to_word36 (m1, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
      HDBGRegAW ("dfdvX");
#endif
    }
    doFault (FAULT_DIV, fst_zero, "DFDV: divide check fault");
  } // m2 == 0

  L68_ (cpu.ou.cycle |= ou_GOA;)
#if defined(NEED_128)
  while (isge_128 (m1, m2)) {
    m1 = rshift_128 (m1, shift_amt);
    e1 += 1;
  }
#else
  while (m1 >= m2) {
    m1 >>= shift_amt;
    e1 += 1;
  }
#endif
  int e3 = e1 - e2;
  if (e3 > 127) {
    SET_I_EOFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dfdvX exp overflow fault");
   }
  if (e3 < -128) {
    SET_I_EUFL;
    if (tstOVFfault ())
      dlyDoFault (FAULT_OFL, fst_zero, "dfdvX exp underflow fault");
  }

  L68_ (cpu.ou.cycle |= ou_GD1;)

  // We need 63 bits quotient + sign. Divisor is at most 64 bits.
  // Do a 127 by 64 fractional divide
  // lo 8bits are always zero
#if defined(NEED_128)
  word72 m3 = divide_128 (lshift_128 (m1, 63-8), rshift_128 (m2, 8), NULL);
#else
  word72 m3 = ((uint128)m1 << (63-8)) / ((uint128)m2 >> 8);
#endif
  L68_ (cpu.ou.cycle |= ou_GD2;)

#if defined(NEED_128)
  m3 = lshift_128 (m3, 8);  // convert back to float
  if (sign == -1)
    m3 = and_128 (negate_128 (m3), MASK72);
#else
  m3 <<= 8;  // convert back to float
  if (sign == -1)
    m3 = (~m3 + 1) & MASK72;
#endif

  convert_to_word36 (m3, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("dfdvX");
#endif
  cpu.rE = (word8) e3 & MASK8;

  SC_I_ZERO (cpu.rA == 0 && cpu . rQ == 0);
  SC_I_NEG (cpu.rA & SIGN36);

  if (cpu.rA == 0 && cpu.rQ == 0)    // set to normalized 0
      cpu.rE = 0200U; /*-128*/
} // dfdvX

// CANFAULT
void dfdv (void) {
  dfdvX (false);    // no inversion
}

// CANFAULT
void dfdi (void) {
  dfdvX (true); // invert
}

//#define DVF_HWR
//#define DVF_FRACTIONAL
#define DVF_CAC
// CANFAULT
void dvf (void)
{
//#if defined(DBGCAC)
//sim_printf ("DVF %"PRId64" %06o:%06o\n", cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC);
//#endif
    // C(AQ) / (Y)
    //  fractional quotient → C(A)
    //  fractional remainder → C(Q)

    // A 71-bit fractional dividend (including sign) is divided by a 36-bit
    // fractional divisor yielding a 36-bit fractional quotient (including
    // sign) and a 36-bit fractional remainder (including sign). C(AQ)71 is
    // ignored; bit position 35 of the remainder corresponds to bit position
    // 70 of the dividend. The remainder sign is equal to the dividend sign
    // unless the remainder is zero.
    //
    // If | dividend | >= | divisor | or if the divisor = 0, division does
    // not take place. Instead, a divide check fault occurs, C(AQ) contains
    // the dividend magnitude in absolute, and the negative indicator
    // reflects the dividend sign.

// HWR code
//HWR--#if defined(DVF_HWR)
//HWR--    // m1 Dividend
//HWR--    // m2 divisor
//HWR--
//HWR--    word72 m1 = SIGNEXT36_72((cpu . rA << 36) | (cpu . rQ & 0777777777776LLU));
//HWR--    word72 m2 = SIGNEXT36_72(cpu.CY);
//HWR--
//HWR--//sim_debug (DBG_CAC, & cpu_dev, "[%"PRId64"]\n", cpu.cycleCnt);
//HWR--//sim_debug (DBG_CAC, & cpu_dev, "m1 "); print_int128 (m1); sim_printf ("\n");
//HWR--//sim_debug (DBG_CAC, & cpu_dev, "-----------------\n");
//HWR--//sim_debug (DBG_CAC, & cpu_dev, "m2 "); print_int128 (m2); sim_printf ("\n");
//HWR--
//HWR--    if (m2 == 0)
//HWR--    {
//HWR--        // XXX check flags
//HWR--        SET_I_ZERO;
//HWR--        SC_I_NEG (cpu . rA & SIGN36);
//HWR--
//HWR--        cpu . rA = 0;
//HWR--        cpu . rQ = 0;
//HWR--
//HWR--        return;
//HWR--    }
//HWR--
//HWR--    // make everything positive, but save sign info for later....
//HWR--    int sign = 1;
//HWR--    int dividendSign = 1;
//HWR--    if (m1 & SIGN72)
//HWR--    {
//HWR--        m1 = (~m1 + 1);
//HWR--        sign = -sign;
//HWR--        dividendSign = -1;
//HWR--    }
//HWR--
//HWR--    if (m2 & SIGN72)
//HWR--    {
//HWR--        m2 = (~m2 + 1);
//HWR--        sign = -sign;
//HWR--    }
//HWR--
//HWR--    if (m1 >= m2 || m2 == 0)
//HWR--    {
//HWR--        //cpu . rA = m1;
//HWR--        cpu . rA = (m1 >> 36) & MASK36;
//HWR--        cpu . rQ = m1 & 0777777777776LLU;
//HWR--
//HWR--        SET_I_ZERO;
//HWR--        SC_I_NEG (cpu . rA & SIGN36);
//HWR--
//HWR--        doFault(FAULT_DIV, fst_zero, "DVF: divide check fault");
//HWR--    }
//HWR--
//HWR--    uint128 dividend = (uint128)m1 << 63;
//HWR--    uint128 divisor = (uint128)m2;
//HWR--
//HWR--    //uint128 m3  = ((uint128)m1 << 63) / (uint128)m2;
//HWR--    //uint128 m3r = ((uint128)m1 << 63) % (uint128)m2;
//HWR--    int128 m3  = (int128)(dividend / divisor);
//HWR--    int128 m3r = (int128)(dividend % divisor);
//HWR--
//HWR--    if (sign == -1)
//HWR--        m3 = -m3;   //(~m3 + 1);
//HWR--
//HWR--    if (dividendSign == -1) // The remainder sign is equal to the dividend sign unless the remainder is zero.
//HWR--        m3r = -m3r; //(~m3r + 1);
//HWR--
//HWR--    cpu . rA = (m3 >> 64) & MASK36;
//HWR--    cpu . rQ = m3r & MASK36;   //01777777777LL;
//HWR--#endif

// canonical code
#if defined(DVF_FRACTIONAL)
// See: https://www.ece.ucsb.edu/~parhami/pres_folder/f31-book-arith-pres-pt4.pdf
// Slide 10: Sequential Algorithm

    // dividend format
    // 0  1     70 71
    // s  dividend x
    //  C(AQ)

  L68_ (cpu.ou.cycle |= ou_GD1;)
  int sign = 1;
  bool dividendNegative = (getbits36_1 (cpu.rA, 0) != 0);
  bool divisorNegative = (getbits36_1 (cpu.CY, 0) != 0);

  // Get the 70 bits of the dividend (72 bits less the sign bit and the
  // ignored bit 71.

  // dividend format:   . d(0) ...  d(69)

  uint128 zFrac = (((uint128) (cpu.rA & MASK35)) << 35) | ((cpu.rQ >> 1) & MASK35);
  if (dividendNegative) {
    zFrac = ~zFrac + 1;
    sign = - sign;
  }
  zFrac &= MASK70;
  //char buf [128];
  //buf [0] = 0;
  //print_int128o (zFrac, buf);
  //sim_printf ("zFrac %s \r\n", buf);

  // Get the 35 bits of the divisor (36 bits less the sign bit)

  // divisor format: . d(0) .... d(34) 0 0 0 .... 0

# if 0
  // divisor goes in the high half
  uint128 dFrac = ((uint128) (cpu.CY & MASK35)) << 35;
  if (divisorNegative) {
    dFrac = ~dFrac + 1;
    sign = - sign;
  }
  dFrac &= ((uint128) MASK35) << 35;
# else
  // divisor goes in the low half
  uint128 dFrac = cpu.CY & MASK35;
  if (divisorNegative) {
    dFrac = ~dFrac + 1;
    sign = - sign;
  }
  dFrac &= MASK35;
# endif

  if (dFrac == 0) {
//sim_printf ("DVFa A %012"PRIo64" Q %012"PRIo64" Y %012"PRIo64"\r\n", cpu.rA, cpu.rQ, cpu.CY);
// case 1: 400000000000 000000000000 000000000000 --> 400000000000 000000000000
//         dFrac 000000000000 000000000000

    cpu.rA = (zFrac >> 35) & MASK35;
    cpu.rQ = (zFrac & MASK35) << 1;

    SC_I_ZERO (cpu.CY == 0);
    SC_I_NEG (cpu.rA & SIGN36);
// /* if (current_running_cpu_idx) */
//        sim_printf ("dvf 1 divide check fault A %012llo B %012llo Y %012llo Z %d N %d\r\n",
//                    cpu.rA, cpu.rQ, cpu.CY, TST_I_ZERO, TST_I_NEG);
    doFault(FAULT_DIV, fst_zero, "DVF: divide check fault");
  }

  //buf [0] = 0;
  //print_int128o (dFrac, buf);
  //sim_printf ("dFrac %s \r\n", buf);

  L68_ (cpu.ou.cycle |= ou_GD2;)
  uint128 sn = zFrac;
  word36 quot = 0;
  for (uint i = 0; i < 35; i ++) {
    // 71 bit number
    uint128 s2n = sn << 1;
    if (s2n > dFrac) {
      s2n -= dFrac;
      quot |= (1llu << (34 - i));
    }
    sn = s2n;
    //buf [0] = 0;
    //print_int128o (sn, buf);
    //sim_printf ("sn %s \r\n", buf);
    //buf [0] = 0;
    //print_int128o (quot, buf);
    //sim_printf ("quot %s \r\n", buf);
  }
  word36 remainder = sn;

  if (sign == -1)
    quot = ~quot + 1;

  if (dividendNegative)
    remainder = ~remainder + 1;

  L68_ (cpu.ou.cycle |= ou_GD2;)
    // I am surmising that the "If | dividend | >= | divisor |" is an
    // overflow prediction; implement it by checking that the calculated
    // quotient will fit in 35 bits.

  if (quot & ~((uint128) MASK35)) {
    SC_I_ZERO (cpu.rA == 0);
    SC_I_NEG (cpu.rA & SIGN36);
// /* if (current_running_cpu_idx) */
//       sim_printf ("dvf 2 divide check fault A %012llo B %012llo Y %012llo Z %d N %d\r\n",
//                   cpu.rA, cpu.rQ, cpu.CY, TST_I_ZERO, TST_I_NEG);
    doFault(FAULT_DIV, fst_zero, "DVF: divide check fault");
  }
  cpu.rA = quot & MASK36;
# if defined(TESTING)
  HDBGRegAW ("dvf");
# endif
  cpu.rQ = remainder & MASK36;

#endif

// MM code
#if defined(DVF_CAC)

# if 0
    if (cpu.CY == 0)
      {
        // XXX check flags
        SET_I_ZERO;
        SC_I_NEG (cpu . rA & SIGN36);

        cpu . rA = 0;
        cpu . rQ = 0;

        return;
    }
# endif

// /* if (current_running_cpu_idx) */ sim_printf ("dvf 0 A %012llo B %012llo Y %012llo\r\n", cpu.rA, cpu.rQ, cpu.CY);
    // dividend format
    // 0  1     70 71
    // s  dividend x
    //  C(AQ)

    L68_ (cpu.ou.cycle |= ou_GD1;)
    int sign = 1;
    bool dividendNegative = (getbits36_1 (cpu . rA, 0) != 0);
    bool divisorNegative = (getbits36_1 (cpu.CY, 0) != 0);

    // Get the 70 bits of the dividend (72 bits less the sign bit and the
    // ignored bit 71.

    // dividend format:   . d(0) ...  d(69)

# if defined(NEED_128)
    uint128 zFrac = lshift_128 (construct_128 (0, cpu.rA & MASK35), 35);
    zFrac = or_128 (zFrac, construct_128 (0, (cpu.rQ >> 1) & MASK35));
//sim_debug (DBG_TRACEEXT, & cpu_dev, "zfrac %016"PRIx64" %016"PRIx64"\n", zFrac.h, zFrac.l);
# else
    uint128 zFrac = ((uint128) (cpu . rA & MASK35) << 35) | ((cpu . rQ >> 1) & MASK35);
//sim_debug (DBG_TRACEEXT, & cpu_dev, "zfrac %016"PRIx64" %016"PRIx64"\n", (uint64) (zFrac>>64), (uint64) zFrac);
# endif
    //zFrac <<= 1; -- Makes Multics unbootable.

    if (dividendNegative)
      {
# if defined(NEED_128)
        zFrac = negate_128 (zFrac);
# else
        // zFrac = ~zFrac + 1;
        // ubsan
        zFrac = (uint128) (((int128) (~zFrac)) + 1);
# endif
        sign = - sign;
      }
# if defined(NEED_128)
    zFrac = and_128 (zFrac, MASK70);
//sim_debug (DBG_TRACEEXT, & cpu_dev, "zfrac %016"PRIx64" %016"PRIx64"\n", zFrac.h, zFrac.l);
# else
    zFrac &= MASK70;
//sim_debug (DBG_TRACEEXT, & cpu_dev, "zfrac %016"PRIx64" %016"PRIx64"\n", (uint64) (zFrac>>64), (uint64) zFrac);
# endif

  //char buf [128];
  //buf [0] = 0;
  //print_int128o (zFrac, buf);
  //sim_printf ("zFrac %s \r\n", buf);
    //char buf [128] = "";
    //print_int128 (zFrac, buf);
    //sim_debug (DBG_CAC, & cpu_dev, "zFrac %s\n", buf);

    // Get the 35 bits of the divisor (36 bits less the sign bit)

    // divisor format: . d(0) .... d(34) 0 0 0 .... 0

    // divisor goes in the low half
    uint128 dFrac = convert_to_word72 (0, cpu.CY & MASK35);
# if defined(NEED_128)
//sim_debug (DBG_TRACEEXT, & cpu_dev, "dfrac %016"PRIx64" %016"PRIx64"\n", dFrac.h, dFrac.l);
# else
//sim_debug (DBG_TRACEEXT, & cpu_dev, "dfrac %016"PRIx64" %016"PRIx64"\n", (uint64) (dFrac>>64), (uint64) dFrac);
# endif
    if (divisorNegative)
      {
# if defined(NEED_128)
        dFrac = negate_128 (dFrac);
# else
        // dFrac = ~dFrac + 1;
        // ubsan
        dFrac = (uint128) (((int128) (~dFrac)) + 1);
# endif
        sign = - sign;
      }
# if defined(NEED_128)
    dFrac = and_128 (dFrac, construct_128 (0, MASK35));
//sim_debug (DBG_TRACEEXT, & cpu_dev, "dfrac %016"PRIx64" %016"PRIx64"\n", dFrac.h, dFrac.l);
# else
    dFrac &= MASK35;
//sim_debug (DBG_TRACEEXT, & cpu_dev, "dfrac %016"PRIx64" %016"PRIx64"\n", (uint64) (dFrac>>64), (uint64) dFrac);
# endif

    //char buf2 [128] = "";
    //print_int128 (dFrac, buf2);
    //sim_debug (DBG_CAC, & cpu_dev, "dFrac %s\n", buf2);

    //if (dFrac == 0 || zFrac >= dFrac)
    //if (dFrac == 0 || zFrac >= dFrac << 35)
# if defined(NEED_128)
    if (iszero_128 (dFrac))
# else
    if (dFrac == 0)
# endif
      {
// case 1: 400000000000 000000000000 000000000000 --> 400000000000 000000000000
//         dFrac 000000000000 000000000000

        //cpu . rA = (zFrac >> 35) & MASK35;
        //cpu . rQ = (word36) ((zFrac & MASK35) << 1);
// ISOLTS 730 expects the right to be zero and the sign
// bit to be untouched.
        cpu.rQ = cpu.rQ & (MASK35 << 1);

        //SC_I_ZERO (dFrac == 0);
        //SC_I_NEG (cpu . rA & SIGN36);
        SC_I_ZERO (cpu.CY == 0);
        SC_I_NEG (cpu.rA & SIGN36);
        dlyDoFault(FAULT_DIV, fst_zero, "DVF: divide check fault");
        return;
      }
  //buf [0] = 0;
  //print_int128o (dFrac, buf);
  //sim_printf ("dFrac %s \r\n", buf);

    L68_ (cpu.ou.cycle |= ou_GD2;)
# if defined(NEED_128)
    uint128 remainder;
    uint128 quot = divide_128 (zFrac, dFrac, & remainder);
//sim_debug (DBG_TRACEEXT, & cpu_dev, "remainder %016"PRIx64" %016"PRIx64"\n", remainder.h, remainder.l);
//sim_debug (DBG_TRACEEXT, & cpu_dev, "quot %016"PRIx64" %016"PRIx64"\n", quot.h, quot.l);
# else
    uint128 quot = zFrac / dFrac;
    uint128 remainder = zFrac % dFrac;
//sim_debug (DBG_TRACEEXT, & cpu_dev, "remainder %016"PRIx64" %016"PRIx64"\n", (uint64) (remainder>>64), (uint64) remainder);
//sim_debug (DBG_TRACEEXT, & cpu_dev, "quot %016"PRIx64" %016"PRIx64"\n", (uint64) (quot>>64), (uint64) quot);
# endif

    // I am surmising that the "If | dividend | >= | divisor |" is an
    // overflow prediction; implement it by checking that the calculated
    // quotient will fit in 35 bits.

# if defined(NEED_128)
    if (isnonzero_128 (and_128 (quot, construct_128 (MASK36,  ~MASK35))))
# else
    if (quot & ~MASK35)
# endif
      {
//
// this got:
//            s/b 373737373737 373737373740 200200
//            was 373737373740 373737373740 000200
//                          ~~
# if 1
        bool Aneg = (cpu.rA & SIGN36) != 0; // blood type
        bool AQzero = cpu.rA == 0 && cpu.rQ == 0;
        if (cpu.rA & SIGN36)
          {
            cpu.rA = (~cpu.rA) & MASK36;
            cpu.rQ = (~cpu.rQ) & MASK36;
            cpu.rQ += 1;
            if (cpu.rQ & BIT37) // overflow?
              {
                cpu.rQ &= MASK36;
                cpu.rA = (cpu.rA + 1) & MASK36;
              }
          }
# else
        if (cpu.rA & SIGN36)
          {
            cpu.rA = (cpu.rA + 1) & MASK36;
            cpu.rQ = (cpu.rQ + 1) & MASK36;
          }
# endif
# if defined(TESTING)
        HDBGRegAW ("dvf");
# endif
        //cpu . rA = (zFrac >> 35) & MASK35;
        //cpu . rQ = (word36) ((zFrac & MASK35) << 1);
// ISOLTS 730 expects the right to be zero and the sign
// bit to be untouched.
        cpu.rQ = cpu.rQ & (MASK35 << 1);

        //SC_I_ZERO (dFrac == 0);
        //SC_I_NEG (cpu . rA & SIGN36);
        SC_I_ZERO (AQzero);
        SC_I_NEG (Aneg);

        dlyDoFault(FAULT_DIV, fst_zero, "DVF: divide check fault");
        return;
      }
    //char buf3 [128] = "";
    //print_int128 (remainder, buf3);
    //sim_debug (DBG_CAC, & cpu_dev, "remainder %s\n", buf3);

    if (sign == -1)
# if defined(NEED_128)
      quot = negate_128 (quot);
# else
      quot = ~quot + 1;
# endif

    if (dividendNegative)
# if defined(NEED_128)
      remainder = negate_128 (remainder);
# else
      remainder = ~remainder + 1;
# endif

# if defined(NEED_128)
    cpu.rA = quot.l & MASK36;
    cpu.rQ = remainder.l & MASK36;
# else
    cpu . rA = quot & MASK36;
    cpu . rQ = remainder & MASK36;
# endif
# if defined(TESTING)
    HDBGRegAW ("dvf");
# endif
#endif

//sim_debug (DBG_CAC, & cpu_dev, "Quotient %"PRId64" (%"PRIo64")\n", cpu . rA, cpu . rA);
//sim_debug (DBG_CAC, & cpu_dev, "Remainder %"PRId64"\n", cpu . rQ);
    SC_I_ZERO (cpu . rA == 0 && cpu . rQ == 0);
    SC_I_NEG (cpu . rA & SIGN36);
// /* if (current_running_cpu_idx) */
//      sim_printf ("dvf 3 A %012llo B %012llo Y %012llo Z %d N %d\r\n",
//                  cpu.rA, cpu.rQ, cpu.CY, TST_I_ZERO, TST_I_NEG);
}

/*!
 * double precision floating round ...
 */
void dfrd (void) {
  // The dfrd instruction is identical to the frd instruction except that the
  // rounding constant used is (11...1)65,71 instead of (11...1)29,71.

  // If C(AQ) != 0, the frd instruction performs a true round to a precision
  // of 64 bits and a normalization on C(EAQ).
  // A true round is a rounding operation such that the sum of the result of
  // applying the operation to two numbers of equal magnitude but opposite
  // sign is exactly zero.

  // The frd instruction is executed as follows:
  // C(AQ) + (11...1)65,71 -> C(AQ)
  // * If C(AQ)0 = 0, then a carry is added at AQ71
  // * If overflow occurs, C(AQ) is shifted one place to the right and C(E)
  // is increased by 1.
  // * If overflow does not occur, C(EAQ) is normalized.
  // * If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.

  CPTUR (cptUseE);
  float72 m = convert_to_word72 (cpu.rA, cpu.rQ);
  if (ISZERO_128 (m)) {
    cpu.rE = 0200U; /*-128*/
    SET_I_ZERO;
    CLR_I_NEG;
    return;
  }

  // C(AQ) + (11...1)65,71 -> C(AQ)
  bool ovf;
  word18 flags1 = 0;
  word1 carry = 0;
  // If C(AQ)0 = 0, then a carry is added at AQ71
#if defined(NEED_128)
  if (iszero_128 (and_128 (m, SIGN72)))
#else
  if ((m & SIGN72) == 0)
#endif
  {
    carry = 1;
  }
#if defined(NEED_128)
  m = Add72b (m, construct_128 (0, 0177), carry, I_OFLOW, & flags1, & ovf);
#else
  m = Add72b (m, 0177, carry, I_OFLOW, & flags1, & ovf);
#endif

  // 0 -> C(AQ)64,71
#if defined(NEED_128)
  putbits72 (& m, 64, 8, construct_128 (0, 0));  // 64-71 => 0 per DH02
#else
  putbits72 (& m, 64, 8, 0);  // 64-71 => 0 per DH02
#endif

  // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is
  // increased by 1.
  // If overflow does not occur, C(EAQ) is normalized.
  // All of this is done by fno, we just need to save the overflow flag

  bool savedovf = TST_I_OFLOW;
  SC_I_OFLOW (ovf);
  convert_to_word36 (m, & cpu.rA, & cpu.rQ);

  fno (& cpu.rE, & cpu.rA, & cpu.rQ);
#if defined(TESTING)
  HDBGRegAW ("dfrd");
#endif
  SC_I_OFLOW (savedovf);
}

void dfstr (word36 *Ypair)
{
    // The dfstr instruction performs a double-precision true round and normalization on C(EAQ) as it is stored.
    // The definition of true round is located under the description of the frd instruction.
    // The definition of normalization is located under the description of the fno instruction.
    // Except for the precision of the stored result, the dfstr instruction is identical to the fstr instruction.

    // The dfrd instruction is identical to the frd instruction except that the rounding constant used is
    // (11...1)65,71 instead of (11...1)29,71.

    // If C(AQ) ≠ 0, the frd instruction performs a true round to a precision of 28 bits and a normalization on C(EAQ).
    // A true round is a rounding operation such that the sum of the result of applying the operation to two numbers of
    // equal magnitude but opposite sign is exactly zero.

    // The frd instruction is executed as follows:
    // C(AQ) + (11...1)29,71 → C(AQ)
    // * If C(AQ)0 = 0, then a carry is added at AQ71
    // * If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
    // * If overflow does not occur, C(EAQ) is normalized.
    // * If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.

    // I believe AL39 is incorrect; bits 64-71 should be set to 0, not 65-71. DH02-01 & Bull 9000 is correct.

    CPTUR (cptUseE);
    word36 A = cpu . rA, Q = cpu . rQ;
    word8 E = cpu . rE;
    //A &= DMASK;
    //Q &= DMASK;

    float72 m = convert_to_word72 (A, Q);
#if defined(NEED_128)
    if (iszero_128 (m))
#else
    if (m == 0)
#endif
    {
        E = (word8)-128;
        SET_I_ZERO;
        CLR_I_NEG;

        Ypair[0] = ((word36) E & MASK8) << 28;
        Ypair[1] = 0;

        return;
    }

    // C(AQ) + (11...1)65,71 → C(AQ)
    bool ovf;
    word18 flags1 = 0;
    word1 carry = 0;
    // If C(AQ)0 = 0, then a carry is added at AQ71
#if defined(NEED_128)
    if (iszero_128 (and_128 (m, SIGN72)))
#else
    if ((m & SIGN72) == 0)
#endif
      {
        carry = 1;
      }
#if defined(NEED_128)
    m = Add72b (m, construct_128 (0, 0177), carry, I_OFLOW, & flags1, & ovf);
#else
    m = Add72b (m, 0177, carry, I_OFLOW, & flags1, & ovf);
#endif

    // 0 -> C(AQ)65,71  (per. RJ78)
#if defined(NEED_128)
    putbits72 (& m, 64, 8, construct_128 (0, 0));  // 64-71 => 0 per DH02
#else
    putbits72 (& m, 64, 8, 0);  // 64-71 => 0 per DH02

#endif

    // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is
    // increased by 1.
    // If overflow does not occur, C(EAQ) is normalized.
    // All of this is done by fno, we just need to save the overflow flag

    bool savedovf = TST_I_OFLOW;
    SC_I_OFLOW(ovf);
    convert_to_word36 (m, & A, & Q);

    fno (& E, & A, & Q);
    SC_I_OFLOW(savedovf);

    Ypair[0] = (((word36)E & MASK8) << 28) | ((A & 0777777777400LL) >> 8);
    Ypair[1] = ((A & 0377) << 28) | ((Q & 0777777777400LL) >> 8);
}

/*
 * double precision Floating Compare ...
 */
void dfcmp (void) {
  // C(E) :: C(Y)0,7
  // C(AQ)0,63 :: C(Y-pair)8,71
  // * Zero: If | C(EAQ)| = | C(Y-pair) |, then ON; otherwise OFF
  // * Neg : If | C(EAQ)| < | C(Y-pair) |, then ON; otherwise OFF

  // The dfcmp instruction is identical to the fcmp instruction except for
  // the precision of the mantissas actually compared.

  // Notes: The fcmp instruction is executed as follows:
  // The mantissas are aligned by shifting the mantissa of the operand with
  // the algebraically smaller exponent to the right the number of places
  // equal to the difference in the two exponents.
  // The aligned mantissas are compared and the indicators set accordingly.

  // C(AQ)0,63
  CPTUR (cptUseE);
#if defined(HEX_MODE)
  uint shift_amt = isHex() ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  word72 m1 = convert_to_word72 (cpu.rA, cpu.rQ & 0777777777400LL);
  int   e1 = SIGNEXT8_int (cpu . rE & MASK8);

  // C(Y-pair)8,71
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, getbits36_28 (cpu.Ypair[0], 8)), (36 + 8));
  m2 = or_128 (m2, lshift_128 (construct_128 (0, cpu.Ypair[1]), 8u));
#else
  word72 m2 = (word72) getbits36_28 (cpu.Ypair[0], 8) << (36 + 8);
  m2 |= cpu.Ypair[1] << 8;
#endif
  int   e2 = SIGNEXT8_int (getbits36_8 (cpu.Ypair[0], 0));

  // Which exponent is smaller???

  int shift_count = -1;
  word1 notallzeros = 0;

#if defined(NEED_128)
  if (e1 == e2) {
    shift_count = 0;
  } else if (e1 < e2) {
    shift_count = abs (e2 - e1) * (int) shift_amt;
    bool s = isnonzero_128 (and_128 (m1, SIGN72));   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m1.l & 1;
      m1 = rshift_128 (m1, 1);
      if (s)
        m1 = or_128 (m1, SIGN72);
    }

# if defined(HEX_MODE)
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = construct_128 (0, 0);
# else
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count > 71)
      m1 = construct_128 (0, 0);
# endif
    m1 = and_128 (m1, MASK72);
  } else { // e2 < e1;
    shift_count = abs(e1 - e2) * (int) shift_amt;
    bool s = isnonzero_128 (and_128 (m2, SIGN72));   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m2.l & 1;
      m2 = rshift_128 (m2, 1);
      if (s)
        m2 = or_128 (m2, SIGN72);
    }
# if defined(HEX_MODE)
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = construct_128 (0, 0);
# else
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count > 71)
      m2 = construct_128 (0, 0);
# endif
    m2 = and_128 (m2, MASK72);
  }

  SC_I_ZERO (iseq_128 (m1, m2));
  int128 sm1 = SIGNEXT72_128 (m1);
  int128 sm2 = SIGNEXT72_128 (m2);
  SC_I_NEG (islt_s128 (sm1, sm2));
#else // NEED_128
  if (e1 == e2) {
    shift_count = 0;
  } else if (e1 < e2) {
    shift_count = abs(e2 - e1) * (int) shift_amt;
    bool s = m1 & SIGN72;   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m1 & 1;
      m1 >>= 1;
      if (s)
        m1 |= SIGN72;
    }

# if defined(HEX_MODE)
    if (m1 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = 0;
# else
    if (m1 == MASK72 && notallzeros == 1 && shift_count > 71)
      m1 = 0;
# endif
    m1 &= MASK72;
  } else { // e2 < e1;
    shift_count = abs(e1 - e2) * (int) shift_amt;
    bool s = m2 & SIGN72;   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m2 & 1;
      m2 >>= 1;
      if (s)
        m2 |= SIGN72;
    }
# if defined(HEX_MODE)
    if (m2 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = 0;
# else
    if (m2 == MASK72 && notallzeros == 1 && shift_count > 71)
      m2 = 0;
# endif
    m2 &= MASK72;
  }

  SC_I_ZERO (m1 == m2);
  int128 sm1 = SIGNEXT72_128 (m1);
  int128 sm2 = SIGNEXT72_128 (m2);
  SC_I_NEG (sm1 < sm2);
#endif // NEED_128
}

/*
 * double precision Floating Compare magnitude ...
 */
void dfcmg (void) {
  // C(E) :: C(Y)0,7
  // | C(AQ)0,27 | :: | C(Y)8,35 |
  // * Zero: If | C(EAQ)| = | C(Y) |, then ON; otherwise OFF
  // * Neg : If | C(EAQ)| < | C(Y) |, then ON; otherwise OFF

  // Notes: The fcmp instruction is executed as follows:
  // The mantissas are aligned by shifting the mantissa of the operand with
  // the algebraically smaller exponent to the right the number of places
  // equal to the difference in the two exponents.
  // The aligned mantissas are compared and the indicators set accordingly.

  // The dfcmg instruction is identical to the dfcmp instruction except that
  // the magnitudes of the mantissas are compared instead of the algebraic
  // values.

  CPTUR (cptUseE);
#if defined(HEX_MODE)
  uint shift_amt = isHex () ? 4 : 1;
#else
  uint shift_amt = 1;
#endif
  // C(AQ)0,63
  word72 m1 = convert_to_word72 (cpu.rA & MASK36, cpu.rQ & 0777777777400LL);
  int    e1 = SIGNEXT8_int (cpu.rE & MASK8);

  // C(Y-pair)8,71
#if defined(NEED_128)
  word72 m2 = lshift_128 (construct_128 (0, getbits36_28 (cpu.Ypair[0], 8)), (36 + 8));
  m2 = or_128 (m2, lshift_128 (construct_128 (0, cpu.Ypair[1]), 8u));
#else
  word72 m2 = (word72) getbits36_28 (cpu.Ypair[0], 8) << (36 + 8);
  m2 |= cpu.Ypair[1] << 8;
#endif
  int    e2 = SIGNEXT8_int (getbits36_8 (cpu.Ypair[0], 0));

  // Which exponent is smaller???
  L68_ (cpu.ou.cycle = ou_GOE;)
  int shift_count = -1;
  word1 notallzeros = 0;

  if (e1 == e2) {
    shift_count = 0;
  } else if (e1 < e2) {
    L68_ (cpu.ou.cycle = ou_GOA;)
    shift_count = abs(e2 - e1) * (int) shift_amt;
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m1, SIGN72));   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m1.l & 1;
      m1 = rshift_128 (m1, 1);
      if (s)
        m1 = or_128 (m1, SIGN72);
    }
# if defined(HEX_MODE)
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = construct_128 (0, 0);
# else
    if (iseq_128 (m1, MASK72) && notallzeros == 1 && shift_count > 71)
      m1 = construct_128 (0, 0);
# endif
    m1 = and_128 (m1, MASK72);
#else
    bool s = m1 & SIGN72;   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m1 & 1;
      m1 >>= 1;
      if (s)
        m1 |= SIGN72;
    }
# if defined(HEX_MODE)
    if (m1 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m1 = 0;
# else
    if (m1 == MASK72 && notallzeros == 1 && shift_count > 71)
      m1 = 0;
# endif
    m1 &= MASK72;
#endif
  } else { // e2 < e1;
    shift_count = abs(e1 - e2) * (int) shift_amt;
#if defined(NEED_128)
    bool s = isnonzero_128 (and_128 (m2, SIGN72));   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m2.l & 1;
      m2 = rshift_128 (m2, 1);
      if (s)
        m2 = or_128 (m2, SIGN72);
    }
# if defined(HEX_MODE)
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = construct_128 (0, 0);
# else
    if (iseq_128 (m2, MASK72) && notallzeros == 1 && shift_count > 71)
      m2 = construct_128 (0, 0);
# endif
    m2 = and_128 (m2, MASK72);
#else
    bool s = m2 & SIGN72;   ///< mantissa negative?
    for (int n = 0; n < shift_count; n += 1) {
      notallzeros |= m2 & 1;
      m2 >>= 1;
      if (s)
        m2 |= SIGN72;
    }
# if defined(HEX_MODE)
    if (m2 == MASK72 && notallzeros == 1 && shift_count * (int) shift_amt > 71)
      m2 = 0;
# else
    if (m2 == MASK72 && notallzeros == 1 && shift_count > 71)
      m2 = 0;
# endif
    m2 &= MASK72;
#endif
  }

#if defined(NEED_128)
  SC_I_ZERO (iseq_128 (m1, m2));
  int128 sm1 = SIGNEXT72_128 (m1);
  if (sm1.h < 0)
    sm1 = negate_s128 (sm1);
  int128 sm2 = SIGNEXT72_128 (m2);
  if (sm2.h < 0)
    sm2 = negate_s128 (sm2);

  SC_I_NEG (islt_s128 (sm1, sm2));
#else // ! NEED_128
  SC_I_ZERO (m1 == m2);
  int128 sm1 = SIGNEXT72_128 (m1);
  if (sm1 < 0)
    sm1 = - sm1;
  int128 sm2 = SIGNEXT72_128 (m2);
  if (sm2 < 0)
    sm2 = - sm2;

  SC_I_NEG (sm1 < sm2);
#endif // ! NEED_128
}

#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma global vector
#endif
