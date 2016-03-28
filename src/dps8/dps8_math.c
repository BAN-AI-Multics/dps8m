/**
 * \file dps8_math.c
 * \project dps8
 * \date 12/6/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
 * \brief stuff related to math routines for the dps8 simulator
 *        * floating-point routines
 *        * fixed-point routines (eventually)
*/

#include <stdio.h>
#include <math.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_cpu.h"
#include "dps8_ins.h"
#include "dps8_math.h"
#include "dps8_utils.h"

#ifdef __CYGWIN__
long double ldexpl(long double x, int n) {
       return __builtin_ldexpl(x, n);
}

long double exp2l (long double e) {
       return __builtin_exp2l(e);
}
#endif

//! floating-point stuff ....
//! quad to octal
//char *Qtoo(__uint128_t n128);

/*!
 * convert floating point quantity in C(EAQ) to a IEEE long double ...
 */
long double EAQToIEEElongdouble(void)
{
    // mantissa
    word72 M = ((word72)(cpu . rA & DMASK) << 36) | ((word72) cpu . rQ & DMASK);

    if (M == 0)
        return 0;
    
    bool S = M & SIGN72; // sign of mantissa
    if (S)
        M = (-M) & MASK72;  //((1LL << 63) - 1); // 63 bits (not 28!)
    
    long double m = 0;  // mantissa value;
    int e = SIGNEXT8_int (cpu . rE & MASK8); // make signed

    long double v = 0.5;
    for(int n = 70 ; n >= 0 ; n -= 1)
    {
        if (M & ((word72)1 << n))
        {
            m += v;
        }
        v /= 2.0;
    }
    
    if (m == 0 && e == -128)    // special case - normalized 0
        return 0;
    if (m == 0)
        return (S ? -1 : 1) * exp2l(e);
    
    return (S ? -1 : 1) * ldexpl(m, e);
}

#ifndef QUIET_UNUSED
/*!
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
        result = bitfieldInsert72(result, 1, 63, 1);
        exp -= 1;
        mant -= 0.5;
    }
    
    long double bitval = 0.5;    ///< start at 1/2 and go down .....
    for(int n = 62 ; n >= 0 && mant > 0; n -= 1)
    {
        if (mant >= bitval)
        {
            result = bitfieldInsert72(result, 1, n, 1);
            mant -= bitval;
            //sim_printf ("Inserting a bit @ %d %012llo %012llo\n", n , (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
        }
        bitval /= 2.0;
    }
    //sim_printf ("n=%d mant=%f\n", n, mant);
    
    //sim_printf ("result=%012llo %012llo\n", (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
    
    // if f is < 0 then take 2-comp of result ...
    if (sign)
    {
        result = -result & (((word72)1 << 64) - 1);
        //sim_printf ("-result=%012llo %012llo\n", (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
    }
    //! insert exponent ...
    int e = (int)exp;
    result = bitfieldInsert72(result, e & 0377, 64, 8);    ///< & 0777777777777LL;
    
    // XXX TODO test for exp under/overflow ...
    
    return result;
}
#endif


#ifndef QUIET_UNUSED
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

#ifndef QUIET_UNUSED
/*!
 * Store EAQ with normalized dps8 representation of IEEE double f0 ...
 */
void IEEElongdoubleToEAQ(long double f0)
{
    if (f0 == 0)
    {
        cpu . rA = 0;
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
        result = bitfieldInsert72(result, 1, 63, 1);
        exp -= 1;
        mant -= 0.5;
    }

    long double bitval = 0.5;    ///< start at 1/2 and go down .....
    for(int n = 70 ; n >= 0 && mant > 0; n -= 1)
    {
        if (mant >= bitval)
        {
            result = bitfieldInsert72(result, 1, n, 1);
            mant -= bitval;
            //sim_printf ("Inserting a bit @ %d %012llo %012llo\n", n , (word36)((result >> 36) & DMASK), (word36)(result & DMASK));
        }
        bitval /= 2.0;
    }
    
    // if f is < 0 then take 2-comp of result ...
    if (sign)
        result = -result & (((word72)1 << 72) - 1);
    
    cpu . rE = exp & MASK8;
    cpu . rA = (result >> 36) & MASK36;
    cpu . rQ = result & MASK36;
}
#endif

#ifndef QUIET_UNUSED
/*!
 * return IEEE double version dps8 single-precision number ...
 */
double float36ToIEEEdouble(uint64_t f36)
{
    unsigned char E;    ///< exponent
    uint64_t M;         ///< mantissa
    E = (f36 >> 28) & 0xff;
    M = f36 & 01777777777LL;
    if (M == 0)
        return 0;
    
    bool S = M & 01000000000LL; ///< sign of mantissa
    if (S)
        M = (-M) & 0777777777; // 27 bits (not 28!)
    
    double m = 0;       ///< mantissa value;
    int e = (char)E;  ///< make signed
    
    double v = 0.5;
    for(int n = 26 ; n >= 0 ; n -= 1)
    {
        if (M & (1 << n))
        {
            m += v;
        }   //else
        v /= 2.0;
    }
    
    if (m == 0 && e == -128)    // special case - normalized 0
        return 0;
    if (m == 0)
        return (S ? -1 : 1) * exp2(e);
    
    return (S ? -1 : 1) * ldexp(m, e);
}
#endif

#ifndef QUIET_UNUSED
/*!
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
            result = bitfieldInsert36(result, 1, n, 1);
            mant -= bitval;
            //sim_printf ("Inserting a bit @ %d result=%012llo\n", n, result);
        }
        bitval /= 2.0;
    }
    //sim_printf ("result=%012llo\n", result);
    
    // if f is < 0 then take 2-comp of result ...
    if (sign)
    {
        result = -result & 001777777777LL;
        //sim_printf ("-result=%012llo\n", result);
    }
    // insert exponent ...
    int e = (int)exp;
    result = bitfieldInsert36(result, e, 28, 8) & 0777777777777LL;
    
    // XXX TODO test for exp under/overflow ...
    
    return result;
}
#endif

/*
 * single-precision arithmetic routines ...
 */

/*!
 * unnormalized floating single-precision add
 */
#if 0
void ufa (void)
{
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

#if 1
//#define NO_RND
//#define RND_INF
//#define RND_FRND
#define RND_FRND2
if (currentRunningCPUnum)
sim_printf ("UFA E %03o A %012llo Q %012llo Y %012llo\n", cpu.rE, cpu.rA, cpu.rQ, cpu.CY);
    word72 m1 = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    word72 m2 = (word72) bitfieldExtract36 (cpu.CY, 0, 28) << 44; // 28-bit mantissa (incl sign)
if (currentRunningCPUnum)
sim_printf ("UFA m1 %012llo %012llo\n", (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
if (currentRunningCPUnum)
sim_printf ("UFA m2 %012llo %012llo\n", (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);

    int e1 = SIGNEXT8_int (cpu . rE & MASK8); 
    int e2 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));
    
if (currentRunningCPUnum)
sim_printf ("UFA e1 %d\n", e1);
if (currentRunningCPUnum)
sim_printf ("UFA e2 %d\n", e2);

    int e3 = -1;

    // which exponent is smaller?
    
    int shift_count = -1;
#ifdef RND_INF
    word1 r = 0;
#endif
#ifdef RND_FRND2
    word1 last = 0;
#endif

    if (e1 == e2)
    {
        shift_count = 0;
        e3 = e1;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
if (currentRunningCPUnum)
sim_printf ("UFA e1 < e2; shift m1 %d right\n", shift_count);
        bool s = m1 & SIGN72;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
#ifdef RND_INF
            r |= m1 & 1;
#endif
#ifdef RND_FRND2
            last = m1 & 1;
#endif
            m1 >>= 1;
            if (s)
                m1 |= SIGN72;
        }
        
        m1 &= MASK72;
        e3 = e2;
#ifdef RND_FRND2
        if (s)
          last = 1 - last;
#endif
        //if (s)
          //r = 1 - r;
if (currentRunningCPUnum)
sim_printf ("UFA m1 now %012llo %012llo\n", (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
if (currentRunningCPUnum)
sim_printf ("UFA e1 > e2; shift m2 %d right\n", shift_count);
        bool s = m2 & SIGN72;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
#ifdef RND_INF
            r |= m2 & 1;
#endif
#ifdef RND_FRND2
            last = m2 & 1;
#endif
            m2 >>= 1;
            if (s)
                m2 |= SIGN72;
        }
        m2 &= MASK72;
        e3 = e1;
#ifdef RND_FRND2
        if (s)
          last = 1 - last;
#endif
        //if (s)
          //r = 1 - r;
if (currentRunningCPUnum)
sim_printf ("UFA m2 now %012llo %012llo\n", (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
    }
    //sim_printf ("shift_count = %d\n", shift_count);
    
if (currentRunningCPUnum)
sim_printf ("UFA e3 %d\n", e3);

    //m3 = m1 + m2;
    bool ovf;
#ifdef RND_INF
    word18 carry0 = 0;
    word72 m3 = Add72b (m1, m2, 0, I_CARRY, & carry0, & ovf);
#endif
#ifdef NO_RND
    word72 m3 = Add72b (m1, m2, 0, I_CARRY, & cpu.cu.IR, & ovf);
#endif
#ifdef RND_FRND
    word72 m3 = Add72b (m1, m2, last, I_CARRY, & cpu.cu.IR, & ovf);
#endif
#ifdef RND_FRND2
    word1 carry = (m1 & SIGN72) ? 0 : 1;
    word72 m3 = Add72b (m1, m2, carry, I_CARRY, & cpu.cu.IR, & ovf);
#endif

    if (ovf)
    {
if (currentRunningCPUnum)
sim_printf ("UFA correcting ovf %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
        word72 signbit = m3 & SIGN72;
        m3 >>= 1;
        //m3 &= MASK72; // MAY need to preserve sign not just set it
        m3 = (m3 & MASK71) | signbit;
        m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
        e3 += 1;
if (currentRunningCPUnum)
sim_printf ("UFA m3 now %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
if (currentRunningCPUnum)
sim_printf ("UFA e3 now %d\n", e3);
    }

#ifdef RND_INF
    m3 = Add72b (m3, 0, r, I_CARRY, & cpu.cu.IR, & ovf);
    cpu.cu.IR |= carry0;
    if (ovf)
    {
if (currentRunningCPUnum)
sim_printf ("UFA correcting ovf2 %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
        word72 signbit = m3 & SIGN72;
        m3 >>= 1;
        //m3 &= MASK72; // MAY need to preserve sign not just set it
        m3 = (m3 & MASK71) | signbit;
        m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
        e3 += 1;
if (currentRunningCPUnum)
sim_printf ("UFA m3 now %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
if (currentRunningCPUnum)
sim_printf ("UFA e3 now %d\n", e3);
    }
#endif

    cpu . rA = (m3 >> 36) & MASK36;
    cpu . rQ = m3 & MASK36;
    cpu . rE = e3 & 0377;

    SC_I_NEG (cpu.rA & SIGN36); // Do this here instead of in Add72b because
                                // of ovf handling above
    if (cpu.rA == 0 && cpu.rQ == 0)
    {
      SET_I_ZERO;
      cpu . rE = 0200U; /*-128*/
    }
    else
    {
      CLR_I_ZERO;
    }

    // EOFL: If exponent is greater than +127, then ON
    if (e3 > 127)
    {
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "ufa exp overflow fault");
    }
    
    // EUFL: If exponent is less than -128, then ON
    if(e3 < -128)
    {
        SET_I_EUFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "ufa exp underflow fault");
    }

#else

    float72 m1 = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    float72 op2 = cpu.CY;
            
    int e1 = SIGNEXT8_int (cpu . rE & MASK8); 
    
    //int8   e2 = (int8)(bitfieldExtract36(op2, 28, 8) & 0377U);      ///< 8-bit signed integer (incl sign)
    int e2 = SIGNEXT8_int (getbits36 (op2, 0, 8));
    word72 m2 = (word72)bitfieldExtract36(op2, 0, 28) << 44; ///< 28-bit mantissa (incl sign)
    
    int e3 = -1;
    word72 m3 = 0;
    
//    if (m1 == 0) // op1 is 0
//    {
//        m3 = m2;
//        e3 = e2;
//        goto here;
//    }
//    if (m2 == 0) // op2 is 0
//    {
//        m3 = m1;
//        e3 = e1;
//        goto here;
//    }

    //which exponent is smaller???
    
 
    int shift_count = -1;
    
    if (e1 == e2)
    {
        shift_count = 0;
        e3 = e1;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
        bool s = m1 & SIGN72;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m1 >>= 1;
            if (s)
                m1 |= SIGN72;
        }
        
        m1 &= MASK72;
        e3 = e2;
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
        bool s = m2 & SIGN72;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m2 >>= 1;
            if (s)
                m2 |= SIGN72;
        }
        m2 &= MASK72;
        e3 = e1;
    }
    //sim_printf ("shift_count = %d\n", shift_count);
    
    m3 = m1 + m2;

    /*
     oVerflow rules .....
     
     oVerflow occurs for addition if the operands have the
     same sign and the result has a different sign. MSB(a) = MSB(b) and MSB(r) <> MSB(a)
     */
    bool ov = ((m1 & SIGN72) == (m2 & SIGN72)) && ((m1 & SIGN72) != (m3 & SIGN72)) ;
    
    //ov = m3 & 077000000000000LL;
    if (ov)
    {
        m3 >>= 1;
        m3 &= MASK72; /// MAY need to preserve sign not just set it
        e3 += 1;
    }
    
//here:;
    // EOFL: If exponent is greater than +127, then ON
    if (e3 > 127)
    {
        cpu . rE = e3 & 0377;
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "ufa exp overflow fault");
    }
    
    // EUFL: If exponent is less than -128, then ON
    if(e3 < -128)
    {
        cpu . rE = e3 & 0377;
        SET_I_EUFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "ufa exp underflow fault");
    }

    cpu . rA = (m3 >> 36) & MASK36;
    cpu . rQ = m3 & MASK36;
    cpu . rE = e3 & 0377;

    // Carry: If a carry out of AQ0 is generated, then ON; otherwise OFF
    SC_I_CARRY (m3 > MASK72);
    
    // Zero: If C(AQ) = 0, then ON; otherwise OFF
    SC_I_ZERO (cpu.rA == 0 && cpu.rQ == 0);
    
    // Neg: If C(AQ)0 = 1, then ON; otherwise OFF
    SC_I_NEG (cpu.rA & SIGN36);

    if (cpu.rA == 0 && cpu.rQ == 0)
    {
      cpu . rE = 0200U; /*-128*/
    }
#endif
if(currentRunningCPUnum)
sim_printf ("UFA returning %03o %012llo %012llo\n", cpu.rE, cpu.rA, cpu.rQ);
}
#else
void ufa (void)
{
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

static int testno = 1;
if (currentRunningCPUnum)
sim_printf ("UFA testno %d\n", testno ++);
if (currentRunningCPUnum)
sim_printf ("UFA E %03o A %012llo Q %012llo Y %012llo\n", cpu.rE, cpu.rA, cpu.rQ, cpu.CY);

    word72 m1 = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    // 28-bit mantissa (incl sign)
    word72 m2 = (word72) bitfieldExtract36 (cpu.CY, 0, 28) << 44;

    int e1 = SIGNEXT8_int (cpu . rE & MASK8); 
    int e2 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));
    
if (currentRunningCPUnum)
sim_printf ("UFA e1 %d m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
if (currentRunningCPUnum)
sim_printf ("UFA e2 %d m2 %012llo %012llo\n", e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);

    int e3 = -1;

    // which exponent is smaller?
    
    int shift_count = -1;
    word1 allones = 1;
    word1 notallzeros = 0;
    word1 last = 0;
    if (e1 == e2)
    {
        shift_count = 0;
        e3 = e1;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
if (currentRunningCPUnum)
sim_printf ("UFA e1 < e2; shift m1 %d right\n", shift_count);
        bool sign = m1 & SIGN72;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            last = m1 & 1;
            allones &= m1 & 1;
            notallzeros |= m1 & 1;
            m1 >>= 1;
            if (sign)
                m1 |= SIGN72;
        }
if (currentRunningCPUnum)
sim_printf ("UFA m1 shifted %012llo %012llo\n", (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
        
//if (sign && (notallzeros == 1))
  //m1 ++;
//if ((! sign) && (allones == 1))
  //m1 ++;
if (m1 == MASK72 && notallzeros == 1 && shift_count > 71)
  m1 = 0;
        m1 &= MASK72;
        e3 = e2;
if (currentRunningCPUnum)
sim_printf ("UFA m1 now %012llo %012llo\n", (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
if (currentRunningCPUnum)
sim_printf ("UFA e1 > e2; shift m2 %d right\n", shift_count);
        bool sign = m2 & SIGN72;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            last = m2 & 1;
            allones &= m2 & 1;
            notallzeros |= m2 & 1;
            m2 >>= 1;
            if (sign)
                m2 |= SIGN72;
        }
if (currentRunningCPUnum)
sim_printf ("UFA m2 shifted %012llo %012llo\n", (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
        //if (allones)
        //if (sign == (allones != 1))
          //m2 ++;
//if (sign && (notallzeros == 1))
  //m2 ++;
//if ((! sign) && (allones == 1))
  //m2 ++;
if (m2 == MASK72 && notallzeros == 1 && shift_count > 71)
  m2 = 0;
        m2 &= MASK72;
        e3 = e1;
if (currentRunningCPUnum)
sim_printf ("UFA m2 now %012llo %012llo\n", (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
    }
    //sim_printf ("shift_count = %d\n", shift_count);
    
if (currentRunningCPUnum)
sim_printf ("UFA last %d allones %d notallzeros %d\n", last, allones, notallzeros);
if (currentRunningCPUnum)
sim_printf ("UFA e3 %d\n", e3);

    //m3 = m1 + m2;
    bool ovf;
    word72 m3 = Add72b (m1, m2, 0, I_CARRY, & cpu.cu.IR, & ovf);

    if (ovf)
    {
if (currentRunningCPUnum)
sim_printf ("UFA correcting ovf %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
        word72 signbit = m3 & SIGN72;
        m3 >>= 1;
        m3 = (m3 & MASK71) | signbit;
        m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
        e3 += 1;
if (currentRunningCPUnum)
sim_printf ("UFA now e3 %03o m3 now %012llo %012llo\n", e3, (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
    }

    cpu . rA = (m3 >> 36) & MASK36;
    cpu . rQ = m3 & MASK36;
    cpu . rE = e3 & 0377;

    SC_I_NEG (cpu.rA & SIGN36); // Do this here instead of in Add72b because
                                // of ovf handling above
    if (cpu.rA == 0 && cpu.rQ == 0)
    {
      SET_I_ZERO;
      cpu . rE = 0200U; /*-128*/
    }
    else
    {
      CLR_I_ZERO;
    }

    // EOFL: If exponent is greater than +127, then ON
    if (e3 > 127)
    {
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "ufa exp overflow fault");
    }
    
    // EUFL: If exponent is less than -128, then ON
    if(e3 < -128)
    {
        SET_I_EUFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "ufa exp underflow fault");
    }

if(currentRunningCPUnum)
sim_printf ("UFA returning E %03o A %012llo Q %012llo\n", cpu.rE, cpu.rA, cpu.rQ);
}
#endif

/*!
 * unnormalized floating single-precision subtract
 */
void ufs (void)
{
    //! The ufs instruction is identical to the ufa instruction with the exception that the twos complement of the mantissa of the operand from main memory (op2) is used.
    
    //! They're probably a few gotcha's here but we'll see.
    //! Yup ... when mantissa 1 000 000 .... 000 we can't do 2'c comp.
    
    word36 m2 = bitfieldExtract36(cpu.CY, 0, 28) & FLOAT36MASK; ///< 28-bit mantissa (incl sign)
    // -1  001000000000 = 2^0 * -1 = -1
    // S          S    
    // 000 000 00 1 000 000 000 000 000 000 000 000 000
    // eee eee ee m mmm mmm mmm mmm mmm mmm mmm mmm mmm
    //      ~     0 111 111 111 111 111 111 111 111 111
    // (add 1)
    //            1 000 000 000 000 000 000 000 000 000  <== Oops! The "weird" number problem.
    //
    // +1 002400000000
    // 000 000 01 0 100 000 000 000 000 000 000 000 000 // what we wan't
    //
    
    word36 m2c = ~m2 + 1;     ///< & 01777777777LL;     ///< take 2-comp of mantissa
    m2c &= FLOAT36MASK;
   
    /*
     * When signs are the *same* after complement we have an overflow
     * (Treat as in addition when we get an overflow)
     */
    bool ov = ((m2 & 01000000000LL) == (m2c & 01000000000LL)); // the "weird" number.

    if (ov && m2 != 0)
    {
        //sim_printf ("OV\n");
        //int8   e = (int8)(bitfieldExtract36(cpu.CY, 28, 8) & 0377U);      ///< 8-bit signed integer (incl sign)
        int e = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));

        m2c >>= 1;
        m2c &= FLOAT36MASK;
        
        e += 1;
        if (e > 127)
        {
            SET_I_EOFL;
            if (tstOVFfault ())
                doFault (FAULT_OFL, 0, "ufs exp overflow fault");
        }
        
        //cpu.CY = bitfieldInsert36(cpu.CY, (word36)e, 28, 8) & DMASK;
        putbits36 (& cpu.CY, 0, 8, e & 0377);
        
    }

    if (m2c == 0)
        cpu.CY = 0400000000000LL;
    else
        //cpu.CY = bitfieldInsert36(cpu.CY, m2c & FLOAT36MASK, 0, 28) & MASK36;
        putbits36 (& cpu.CY, 8, 28, m2c & FLOAT36MASK);
   
    ufa();

}

/*
 * floating normalize ...
 */

void fno (void)
{
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
    
    cpu . rA &= DMASK;
    cpu . rQ &= DMASK;
    float72 m = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    if (TST_I_OFLOW)
    {
        CLR_I_OFLOW;
        word72 s = m & SIGN72; // save the sign bit
        m >>= 1; // renormalize the mantissa
        m |= SIGN72; // set the sign bit
        m ^= s; // if the was 0, leave it 1; if it was 1, make it 0

        // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
        if (m == 0)
        {
            cpu . rE = 0200U; /*-128*/
            SET_I_ZERO;
        }
        else
        {
            if (cpu . rE == 127)
            {
                SET_I_EOFL;
                if (tstOVFfault ())
                    doFault (FAULT_OFL, 0, "fno exp overflow fault");
            }
            cpu . rE ++;
            cpu . rE &= MASK8;
        }

        cpu . rA = (m >> 36) & MASK36;
        cpu . rQ = m & MASK36;
        SC_I_NEG (cpu . rA & SIGN72);

        return;
    }
    
    // only normalize C(EAQ) if C(AQ) ≠ 0 and the overflow indicator is OFF
    if (m == 0) // C(AQ) == 0.
    {
        //cpu . rA = (m >> 36) & MASK36;
        //cpu . rQ = m & MASK36;
        cpu . rE = 0200U; /*-128*/
        SET_I_ZERO;
        CLR_I_NEG;
        return;
    }

    int e = SIGNEXT8_int (cpu . rE & MASK8);
    bool s = (m & SIGN72) != (word72)0;    ///< save sign bit

    while (s  == !! getbits72 (m, 1, 1)) // until C(AQ)0 != C(AQ)1?
    //while (s  == !! bitfieldExtract72(m, 70, 1)) // until C(AQ)0 != C(AQ)1?
    {
        m <<= 1;
        e -= 1;
        if (m == 0) // XXX: necessary??
            break;
    }

    m &= MASK71;
        
    if (s)
      m |= SIGN72;
      
    if (e < -127)
    {
        SET_I_EUFL;
        if (tstOVFfault ())
            dlyDoFault (FAULT_OFL, 0, "fno exp underflow fault");
    }

    cpu . rE = e & MASK8;
    cpu . rA = (m >> 36) & MASK36;
    cpu . rQ = m & MASK36;

    // EAQ is normalized, so if A is 0, so is Q, and the check can be elided
    if (cpu . rA == 0)    // set to normalized 0
        cpu . rE = 0200U; /*-128*/
    
    // Zero: If C(AQ) = floating point 0, then ON; otherwise OFF
    SC_I_ZERO (cpu . rA == 0 && cpu . rQ == 0);
    
    // Neg: If C(AQ)0 = 1, then ON; otherwise OFF
    SC_I_NEG (cpu . rA & SIGN36);
}

#if 0
// XXX eventually replace fno() with fnoEAQ()
void fnoEAQ(word8 *E, word36 *A, word36 *Q)
{
    //! The fno instruction normalizes the number in C(EAQ) if C(AQ) ≠ 0 and the overflow indicator is OFF.
    //!    A normalized floating number is defined as one whose mantissa lies in the interval [0.5,1.0) such that
    //!    0.5<= |C(AQ)| <1.0 which, in turn, requires that C(AQ)0 ≠ C(AQ)1.
    //!    If the overflow indicator is ON, then C(AQ) is shifted one place to the right, C(AQ)0 is inverted to reconstitute the actual sign, and the overflow indicator is set OFF. This action makes the fno instruction useful in correcting overflows that occur with fixed point numbers.
    //!  Normalization is performed by shifting C(AQ)1,71 one place to the left and reducing C(E) by 1, repeatedly, until the conditions for C(AQ)0 and C(AQ)1 are met. Bits shifted out of AQ1 are lost.
    //!  If C(AQ) = 0, then C(E) is set to -128 and the zero indicator is set ON.
    
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
void fneg (void)
{
    // This instruction changes the number in C(EAQ) to its normalized negative
    // (if C(AQ) ≠ 0). The operation is executed by first forming the twos
    // complement of C(AQ), and then normalizing C(EAQ).
    //
    // Even if originally C(EAQ) were normalized, an exponent overflow can
    // still occur, namely when C(E) = +127 and C(AQ) = 100...0 which is the
    // twos complement approximation for the decimal value -1.0.
    
#if 0
    float72 m = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    
    if (m == 0) // (if C(AQ) ≠ 0)
    {
        SET_I_ZERO;      // it's zero
        CLR_I_NEG;       // it ain't negative
        return; //XXX: ????
    }
    
    int8 e = (int8)cpu . rE;
    
    word72 mc = 0;
    if (m == FLOAT72SIGN)
    {
        mc = m >> 1;
        e += 1;
    } else
        mc = ~m + 1;     // take 2-comp of mantissa
    
    
    //mc &= FLOAT72MASK;
    mc &= ((word72)1 << 72) - 1;
    /*
     * When signs are the *same* we have an overflow
     * (Treat as in addition when we get an overflow)
     */
    //bool ov = ((m & FLOAT72SIGN) == (mc & FLOAT72SIGN)); // the "weird" number!
    bool ov = ((m & ((word72)1 << 71)) == (mc & ((word72)1 << 71))); // the "weird" number!
//    if (ov)
//    bool ov = ((m & 01000000000LL) == (mc & 01000000000LL)); // the "weird" number.
    if (ov && m != 0)
    {
        mc >>= 1;
        //mc &= FLOAT72MASK;
        mc &= ((word72)1 << 72) - 1;
        
        if ((e + 1) > 127)
            SET_I_EOFL;
        else    // XXX: this is my interpretation
            e += 1;
    }
    
    cpu . rE = e & 0377;
    cpu . rA = (mc >> 36) & MASK36;
    cpu . rQ = mc & MASK36;
#else
    // Form the mantissa from AQ
    word72 m = ((word72)(cpu . rA & MASK36) << 36) | (word72)(cpu . rQ & MASK36);

    // If the mantissa is 4000...00 (least negative value, it is negable in two's
    // complement arithmetic. Divide it by 2, losing a bit of precision, and increment
    // the exponent.
    if (m == SIGN72)
      {
        // Negation of 400..0 / 2 is 200..0; we can get there shifting; we know that
        // a zero will be shifted into the sign bit becuase fo the masking in 'm='.
        m >>= 1;
        // Increment the exp, checking for overflow.
        if (cpu . rE == 127)
        {
            SET_I_EOFL;
            if (tstOVFfault ())
                doFault (FAULT_OFL, 0, "fneg exp overflow fault");
        }
        cpu . rE ++;
        cpu . rE &= MASK8;
      }
    else
      {
        // Do the negation
        m = -m;
      }
    cpu . rA = (m >> 36) & MASK36;
    cpu . rQ = m & MASK36;
#endif
    fno ();  // normalize
}

/*!
 * Unnormalized Floating Multiply ...
 */
void ufm (void)
{
    // The ufm instruction is executed as follows:
    //      C(E) + C(Y)0,7 → C(E)
    //      ( C(AQ) × C(Y)8,35 )0,71 → C(AQ)
    // A normalization is performed only in the case of both factor mantissas
    // being 100...0 which is the twos  complement approximation to the decimal
    // value -1.0.
    // The definition of normalization is located under the description of the
    // fno instruction.
    
    // Zero: If C(AQ) = 0, then ON; otherwise OFF
    // Neg: If C(AQ)0 = 1, then ON; otherwise OFF
    // Exp Ovr: If exponent is greater than +127, then ON
    // Exp Undr: If exponent is less than -128, then ON
    
    uint64 m1 = (cpu . rA << 28) | ((cpu . rQ & 0777777777400LL) >> 8) ; 
    int    e1 = SIGNEXT8_int (cpu . rE & MASK8);

    uint64 m2 = bitfieldExtract36(cpu.CY, 0, 28) << (8 + 28); ///< 28-bit mantissa (incl sign)
    //int8   e2 = (int8)(bitfieldExtract36(cpu.CY, 28, 8) & 0377U);      ///< 8-bit signed integer (incl sign)
    int    e2 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));
    
    if (m1 == 0 || m2 == 0)
    {
        SET_I_ZERO;
        CLR_I_NEG;
        
        cpu . rE = 0200U; /*-128*/
        cpu . rA = 0;
        cpu . rQ = 0;
        
        return; // normalized 0
    }

    int sign = 1;

    if (m1 & FLOAT72SIGN)   //0x8000000000000000LL)
    {
        if (m1 == FLOAT72SIGN)
        {
            m1 >>= 1;
            e1 += 1;
        }
        else
            m1 = (~m1 + 1);
        sign = -sign;
    }

    if (m2 & FLOAT72SIGN)   //0x8000000000000000LL)
    {
        if (m1 == FLOAT72SIGN)
        {
            m2 >>= 1;
            e2 += 1;
        }
        else
            m2 = (~m2 + 1);
        sign = -sign;
    }
    
    int e3 = e1 + e2;
    
    if (e3 >  127)
    {
      SET_I_EOFL;
      if (tstOVFfault ())
          doFault (FAULT_OFL, 0, "ufm exp overflow fault");
    }
    if (e3 < -128)
    {
      SET_I_EUFL;
      if (tstOVFfault ())
          doFault (FAULT_OFL, 0, "ufm exp underflow fault");
    }

    word72 m3 = ((word72)m1) * ((word72)m2);
    word72 m3a = m3 >> 63;
    
    if (sign == -1)
        m3a = (~m3a + 1) & 0xffffffffffffffffLL;
    
    cpu . rE = e3 & MASK8;
    cpu . rA = (m3a >> 28) & MASK36;
    cpu . rQ = m3a & MASK36;
    
    // A normalization is performed only in the case of both factor mantissas being 100...0 which is the twos complement approximation to the decimal value -1.0.
    //if ((cpu . rE == -128 && cpu . rA == 0 && cpu . rQ == 0) && (m2 == 0 && e2 == -128)) // XXX FixMe
    if ((m1 == ((uint64)1 << 63)) && (m2 == ((uint64)1 << 63)))
        fno ();
    
    SC_I_ZERO (cpu . rA == 0 && cpu . rQ == 0);
    SC_I_NEG (cpu . rA & SIGN36);
}   

/*!
 * floating divide ...
 */
// CANFAULT
static void fdvX(bool bInvert)
{
    //! C(EAQ) / C (Y) → C(EA)
    //! C(Y) / C(EAQ) → C(EA) (Inverted)
    
    //! 00...0 → C(Q)
    
    //! The fdv instruction is executed as follows:
    //! The dividend mantissa C(AQ) is shifted right and the dividend exponent
    //! C(E) increased accordingly until | C(AQ)0,27 | < | C(Y)8,35 |
    //! C(E) - C(Y)0,7 → C(E)
    //! C(AQ) / C(Y)8,35 → C(A)
    //! 00...0 → C(Q)
    //! If the divisor mantissa C(Y)8,35 is zero after alignment, the division does
    //! not take place. Instead, a divide check fault occurs, C(AQ) contains the dividend magnitude, and the negative indicator reflects the dividend sign.
  
    word36 m1;
    int    e1;
    
    word36 m2;
    int    e2;
    
    if (!bInvert)
    {
        m1 = cpu . rA;    // & 0777777777400LL;
        e1 = SIGNEXT8_int (cpu . rE & MASK8);
    
        m2 = bitfieldExtract36(cpu.CY, 0, 28) << 8 ;     // 28-bit mantissa (incl sign)
        //e2 = (int8)(bitfieldExtract36(cpu.CY, 28, 8) & 0377U);    // 8-bit signed integer (incl sign)
        e2 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));
    } else { // invert
        m2 = cpu . rA;    //& 0777777777400LL ;
        e2 = SIGNEXT8_int (cpu . rE & MASK8);
    
        m1 = bitfieldExtract36(cpu.CY, 0, 28) << 8 ;     // 28-bit mantissa (incl sign)
        //e1 = (int8) (bitfieldExtract36(cpu.CY, 28, 8) & 0377U);    // 8-bit signed integer (incl sign)
        e1 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));
    }

    // make everything positive, but save sign info for later....
    int sign = 1;
    if (m1 & SIGN36)
    {
        if (m1 == SIGN36)
        {
            m1 >>= 1;
            e1 += 1;
        } else
            m1 = (~m1 + 1) & 0777777777777;
        sign = -sign;
    }
    
    if (m2 & SIGN36)
    {
        if (m2 == SIGN36)
        {
            m2 >>= 1;
            e2 += 1;
        } else
        m2 = (~m2 + 1) & 0777777777777;
        sign = -sign;
    }
    
    if (m2 == 0)
    {
        // If the divisor mantissa C(Y)8,35 is zero after alignment (HWR: why after?), the division does
        // not take place. Instead, a divide check fault occurs, C(AQ) contains the dividend magnitude, and the negative indicator reflects the dividend sign.
        
        // NB: If C(Y)8,35 ==0 then the alignment loop will never exit! That's why it been moved before the alignment
        
        SET_I_ZERO;
        SC_I_NEG (cpu . rA & SIGN36);
        
        cpu . rA = m1;
        
        doFault(FAULT_DIV, 0, "DFDV: divide check fault");
    }

    while (m1 >= m2)
    {
        m1 >>= 1;
        e1 += 1;
    }

    if (e1 > 127)
    {
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "fdvX exp overflow fault");
    }
    
        
    int e3 = e1 - e2;
        
    word72 m3 = (((word72)m1) << 35) / ((word72)m2);
    word36 m3b = m3 & (0777777777777LL);
    
    if (sign == -1)
        m3b = (~m3b + 1) & 0777777777777LL;
    
    cpu . rE = e3 & MASK8;
    cpu . rA = m3b & MASK36;
    cpu . rQ = 0;
    
    SC_I_ZERO (cpu . rA == 0);
    SC_I_NEG (cpu . rA & SIGN36);
    
    if (cpu . rA == 0)    // set to normalized 0
        cpu . rE = 0200U; /*-128*/
}

void fdv (void)
{
    fdvX(false);    // no inversion
}
void fdi (void)
{
    fdvX(true);
}

/*!
 * single precision floating round ...
 */
void frd (void)
{
    // If C(AQ) ≠ 0, the frd instruction performs a true round to a precision
    // of 28 bits and a normalization on C(EAQ).
    // A true round is a rounding operation such that the sum of the result of
    // applying the operation to two numbers of equal magnitude but opposite
    // sign is exactly zero.
    
    // The frd instruction is executed as follows:
    // C(AQ) + (11...1)29,71 → C(AQ)
    // If C(AQ)0 = 0, then a carry is added at AQ71
    // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
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
    
    // 0 → C(AQ)28,71
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
    
if (currentRunningCPUnum)
sim_printf ("FRD E %03o A %012llo Q %012llo CY %012llo\n", cpu.rE, cpu.rA, cpu.rQ, cpu.CY);

    word72 m = ((word72) cpu.rA << 36) | (word72) cpu.rQ;
    if (m == 0)
    {
        cpu.rE = 0200U; /*-128*/
        SET_I_ZERO;
        CLR_I_NEG;
        return;
    }
    

#if 1 // according to RJ78
    // C(AQ) + (11...1)29,71 → C(AQ)
    bool ovf;
    word18 flags1 = 0;
    word18 flags2 = 0;
    word1 carry = 0;
    // If C(AQ)0 = 0, then a carry is added at AQ71
    if ((m & SIGN72) == 0)
    {
      carry = 1;
    }
    m = Add72b (m, 0177777777777777LL, carry, I_OFLOW, & flags1, & ovf);
if (currentRunningCPUnum)
sim_printf ("FRD add ones E %03o m %012llo %012llo flags %06o\n", cpu.rE, (word36) (m >> 36) & MASK36, (word36) m & MASK36, flags1);
#endif

#if 0 // according to AL39
    // C(AQ) + (11...1)29,71 → C(AQ)
    bool ovf;
    word18 flags1 = 0;
    word18 flags2 = 0;
    m = Add72b (m, 0177777777777777LL, 0, I_OFLOW, & flags1, & ovf);
if (currentRunningCPUnum)
sim_printf ("FRD add ones E %03o m %012llo %012llo flags %06o\n", cpu.rE, (word36) (m >> 36) & MASK36, (word36) m & MASK36, flags1);

    // If C(AQ)0 = 0, then a carry is added at AQ71
    if ((m & SIGN72) == 0)
    {
        m = Add72b (m, 1, 0, I_OFLOW, & flags2, & ovf);
if (currentRunningCPUnum)
sim_printf ("FRD add carry E %03o m %012llo %012llo flags %06o\n", cpu.rE, (word36) (m >> 36) & MASK36, (word36) m & MASK36, flags2);
    }
#endif

    // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is
    // increased by 1.

    if ((flags1 | flags2) & I_OFLOW)
    {
        m >>= 1;
        if (cpu.rE == 127)
        {
            SET_I_EOFL;
            if (tstOVFfault ())
                dlyDoFault (FAULT_OFL, 0, "frd exp overflow fault");
        }
        cpu.rE ++;
if (currentRunningCPUnum)
sim_printf ("FRD overflow E %03o m %012llo %012llo\n", cpu.rE, (word36) (m >> 36) & MASK36, (word36) m & MASK36);
    }

    cpu.rA = (m >> 36) & MASK36;
    cpu.rQ = m & MASK36;

if (currentRunningCPUnum)
sim_printf ("FRD E %03o A %012llo Q %012llo\n", cpu.rE, cpu.rA, cpu.rQ);
    // If overflow does not occur, C(EAQ) is normalized.
    if (! ((flags1 | flags2) & I_OFLOW))
    {
        if (cpu.rA != 0 || cpu.rQ != 0)
          fno ();
if (currentRunningCPUnum)
sim_printf ("FRD normalized E %03o A %012llo Q %012llo\n", cpu.rE, cpu.rA, cpu.rQ);
    }

    // 0 → C(AQ)28,71  (per. RJ78)
    cpu.rA &= 0777777777400;
    cpu.rQ = 0;

    if (cpu.rA == 0 && cpu.rQ == 0)
    {
      SET_I_ZERO;
      cpu . rE = 0200U; /*-128*/
    }
    else
    {
      CLR_I_ZERO;
    }
    SC_I_NEG (cpu.rA & SIGN36);
if (currentRunningCPUnum)
sim_printf ("FRD final E %03o A %012llo Q %012llo\n", cpu.rE, cpu.rA, cpu.rQ);
}

void fstr(word36 *Y)
{
    //The fstr instruction performs a true round and normalization on C(EAQ) as
    //it is stored.
    //The definition of true round is located under the description of the frd
    //instruction.
    //The definition of normalization is located under the description of the
    //fno instruction.
    //Attempted repetition with the rpl instruction causes an illegal procedure
    //fault.
    
    word36 A = cpu . rA, Q = cpu . rQ;
    int E = SIGNEXT8_int (cpu . rE & MASK8);
    A &= DMASK;
    Q &= DMASK;
    E &= MASK8;
   
    float72 m = ((word72)A << 36) | (word72)Q;
    if (m == 0)
    {
        E = -128;
        SET_I_ZERO;
        CLR_I_NEG;
        *Y = 0;
        putbits36 (Y, 0, 8, E & MASK8);
        return;
    }
    
    // C(AQ) + (11...1)29,71 → C(AQ)
    bool s1 = (m & SIGN72) != (word72)0;
    
    m += (word72)0177777777777777LL; // add 1's into lower 43-bits
    
    // If C(AQ)0 = 0, then a carry is added at AQ71
    if (!s1)
        m += 1;
    
    // 0 → C(AQ)29,71 (AL39)
    // 0 → C(AQ)28,71 (DH02-01 / DPS9000)
    //m &= (word72)0777777777400LL << 36; // 28-71 => 0 per DH02-01/Bull DPS9000
    m = bitfieldInsert72(m, 0, 0, 44);    // 28-71 => 0 per DH02
    
    bool s2 = (m & SIGN72) != (word72)0;
    
    bool ov = s1 != s2;   // sign change denotes overflow
    if (ov)
    {
        // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
        m >>= 1;
        if (s1) // (was s2) restore sign if necessary
            m |= SIGN72;
        
        A = (m >> 36) & MASK36;
        Q = m & MASK36;
        
        if (E == 127)
        {
            SET_I_EOFL;
            if (tstOVFfault ())
                doFault (FAULT_OFL, 0, "fstr exp overflow fault");
        }
        E +=  1;
    }
    else
    {
        // If overflow does not occur, C(EAQ) is normalized.
        A = (m >> 36) & MASK36;
        Q = m & MASK36;
        
        fno();
    }
    
    // If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.
    if (A == 0 && Q == 0)
    {
        E = -128;
        SET_I_ZERO;
    }
    
    SC_I_NEG (A & SIGN36);
    
    *Y = bitfieldInsert36(A >> 8, E, 28, 8) & MASK36;
}

/*!
 * single precision Floating Compare ...
 */
void fcmp(void)
{
    //! C(E) :: C(Y)0,7
    //! C(AQ)0,27 :: C(Y)8,35
    
    //! Zero: If C(EAQ) = C(Y), then ON; otherwise OFF
    //! Neg: If C(EAQ) < C(Y), then ON; otherwise OFF
    
    //! Notes: The fcmp instruction is executed as follows:
    //! The mantissas are aligned by shifting the mantissa of the operand with the algebraically smaller exponent to the right the number of places equal to the difference in the two exponents.
    //! The aligned mantissas are compared and the indicators set accordingly.
    
    word36 m1 = cpu . rA & 0777777777400LL;
    int    e1 = SIGNEXT8_int (cpu . rE & MASK8);
    
    word36 m2 = bitfieldExtract36(cpu.CY, 0, 28) << 8;      ///< 28-bit mantissa (incl sign)
    //int8   e2 = (int8) (bitfieldExtract36(cpu.CY, 28, 8) & 0377U);    ///< 8-bit signed integer (incl sign)
    int    e2 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));
    
    //int e3 = -1;
       
    //which exponent is smaller???
    
    int shift_count = -1;
    
    if (e1 == e2)
    {
        shift_count = 0;
        //e3 = e1;
    }
    else
    if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
        bool s = m1 & SIGN36;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m1 >>= 1;
            if (s)
                m1 |= SIGN36;
        }
        
        m1 &= MASK36;
        //e3 = e2;
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
        bool s = m2 & SIGN36;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m2 >>= 1;
            if (s)
                m2 |= SIGN36;
        }
        m2 &= MASK36;
        //e3 = e1;
    }
    
    // need to do algebraic comparisons of mantissae
    SC_I_ZERO ((t_int64)SIGNEXT36_64(m1) == (t_int64)SIGNEXT36_64(m2));
    SC_I_NEG ((t_int64)SIGNEXT36_64(m1) <  (t_int64)SIGNEXT36_64(m2));
}

/*!
 * single precision Floating Compare magnitude ...
 */
void fcmg ()
{
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

if (currentRunningCPUnum)
sim_printf ("FCMG E %03o A %012llo Q %012llo CY %012llo\n", cpu.rE, cpu.rA, cpu.rQ, cpu.CY);
    // C(AQ)0,27
    word36 m1 = cpu . rA & 0777777777400LL;
    int   e1 = SIGNEXT8_int (cpu . rE & MASK8);

if (currentRunningCPUnum)
sim_printf ("FCMG e1 %d m1 %012llo\n", e1, m1);

   // C(Y)0,7
    word36 m2 = bitfieldExtract36(cpu.CY, 0, 28) << 8;      // 28-bit mantissa (incl sign)
    int   e2 = SIGNEXT8_int (getbits36 (cpu.CY, 0, 8));

if (currentRunningCPUnum)
sim_printf ("FCMG e2 %d m2 %012llo\n", e2, m2);

    int e3 = -1;

    //which exponent is smaller???
    
    int shift_count = -1;
    
    if (e1 == e2)
    {
        shift_count = 0;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
        bool s = m1 & SIGN36;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m1 >>= 1;
            if (s)
                m1 |= SIGN36;
        }
        
        m1 &= MASK36;
        e3 = e2;
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
        bool s = m2 & SIGN36;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m2 >>= 1;
            if (s)
                m2 |= SIGN36;
        }
        m2 &= MASK36;
        e3 = e1;
    }
    
if (currentRunningCPUnum)
sim_printf ("FCMG m1 %012llo\n", m1);
if (currentRunningCPUnum)
sim_printf ("FCMG m2 %012llo\n", m2);
    SC_I_ZERO (m1 == m2);
    t_int64 sm1 = labs (SIGNEXT36_64 (m1));
    t_int64 sm2 = labs (SIGNEXT36_64 (m2));
if (currentRunningCPUnum)
sim_printf ("FCMG sm1 %lld\n", sm1);
if (currentRunningCPUnum)
sim_printf ("FCMG sm2 %lld\n", sm2);
if (currentRunningCPUnum)
sim_printf ("FCMG sm1 < sm2 %d\n", sm1 < sm2);
    SC_I_NEG (sm1 < sm2);
}

/*
 * double-precision arithmetic routines ....
 */

#if 0
//! extract mantissa + exponent from a YPair ....
static void YPairToExpMant(word36 Ypair[], word72 *mant, int *exp)
{
    *mant = (word72)bitfieldExtract36(Ypair[0], 0, 28) << 44;   // 28-bit mantissa (incl sign)
    *mant |= (Ypair[1] & DMASK) << 8;
    *exp = SIGNEXT8_int (bitfieldExtract36(Ypair[0], 28, 8) & 0377U);           // 8-bit signed integer (incl sign)
}

//! combine mantissa + exponent intoa YPair ....
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
void dufa (bool subtract)
{
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

    word72 m1 = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    int e1 = SIGNEXT8_int (cpu . rE & MASK8); 

    word72 m2 = (word72) bitfieldExtract36 (cpu.Ypair[0], 0, 28) << 44; // 28-bit mantissa (incl sign)
           m2 |= (word73) cpu.Ypair[1] << 8;
    
    int e2 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));
if (currentRunningCPUnum)
sim_printf ("DUFA e1 %03o m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
if (currentRunningCPUnum)
sim_printf ("DUFA e2 %03o m2 %012llo %012llo\n", e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);

    if (subtract)
    {
      word72 m2s = m2 & SIGN72; // remember the sign bit
      m2 = ~m2 + 1; // two's complement
      m2 &= MASK72;
      // If the signs are the same after complement, then overflow happened
      if (m2s == (m2 & SIGN72))
      {
          m2 >>= 1;
          m2 &= MASK72;
          if (e2 == 127)
          {
            SET_I_EOFL;
            if (tstOVFfault ())
                dlyDoFault (FAULT_OFL, 0, "dufs exp overflow fault");
          }
          e2 ++;
      }
      if (m2 == 0)
      {
          e2 = -128;
      }
    } // subtract

    int e3 = -1;

    // which exponent is smaller?
    
    int shift_count = -1;

    if (e1 == e2)
    {
        shift_count = 0;
        e3 = e1;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
if (currentRunningCPUnum)
sim_printf ("DUFA e1 < e2; shift m1 %d right\n", shift_count);
        bool s = m1 & SIGN72;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m1 >>= 1;
            if (s)
                m1 |= SIGN72;
        }
        
        m1 &= MASK72;
        e3 = e2;
if (currentRunningCPUnum)
sim_printf ("DUFA m1 now %012llo %012llo\n", (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
if (currentRunningCPUnum)
sim_printf ("DUFA e1 > e2; shift m2 %d right\n", shift_count);
        bool s = m2 & SIGN72;   // mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m2 >>= 1;
            if (s)
                m2 |= SIGN72;
        }
        m2 &= MASK72;
        e3 = e1;
if (currentRunningCPUnum)
sim_printf ("DUFA m2 now %012llo %012llo\n", (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
    }
    //sim_printf ("shift_count = %d\n", shift_count);
    
if (currentRunningCPUnum)
sim_printf ("DUFA e3 %d\n", e3);

    bool ovf;
    word72 m3 = Add72b (m1, m2, 0, I_CARRY, & cpu.cu.IR, & ovf);
if (currentRunningCPUnum)
sim_printf ("DUFA m3 %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);

    if (ovf)
    {
if (currentRunningCPUnum)
sim_printf ("DUFA correcting ovf %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
        word72 signbit = m3 & SIGN72;
        m3 >>= 1;
        m3 = (m3 & MASK71) | signbit;
        m3 ^= SIGN72; // C(AQ)0 is inverted to restore the sign
        e3 += 1;
if (currentRunningCPUnum)
sim_printf ("DUFA m3 now %012llo %012llo\n", (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
if (currentRunningCPUnum)
sim_printf ("DUFA e3 now %d\n", e3);
    }

    cpu . rA = (m3 >> 36) & MASK36;
    cpu . rQ = m3 & MASK36;
    cpu . rE = e3 & 0377;

    SC_I_NEG (cpu.rA & SIGN36); // Do this here instead of in Add72b because
                                // of ovf handling above
    if (cpu.rA == 0 && cpu.rQ == 0)
    {
      SET_I_ZERO;
      cpu . rE = 0200U; /*-128*/
    }
    else
    {
      CLR_I_ZERO;
    }

    // EOFL: If exponent is greater than +127, then ON
    if (e3 > 127)
    {
        SET_I_EOFL;
        if (tstOVFfault ())
            dlyDoFault (FAULT_OFL, 0, "dufa exp overflow fault");
    }
    
    // EUFL: If exponent is less than -128, then ON
    if(e3 < -128)
    {
        SET_I_EUFL;
        if (tstOVFfault ())
            dlyDoFault (FAULT_OFL, 0, "dufa exp underflow fault");
    }
}

#if 0
/*!
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
    
    /*!
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
                doFault (FAULT_OFL, 0, "dufs exp overflow fault");
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

/*!
 * double-precision Unnormalized Floating Multiply ...
 */
void dufm (void)
{
    //! Except for the precision of the mantissa of the operand from main memory,
    //!    the dufm instruction is identical to the ufm instruction.
    
    //! The ufm instruction is executed as follows:
    //!      C(E) + C(Y)0,7 → C(E)
    //!      ( C(AQ) × C(Y)8,35 )0,71 → C(AQ)
    //! A normalization is performed only in the case of both factor mantissas being 100...0 which is the twos  complement approximation to the decimal value -1.0.
    //! The definition of normalization is located under the description of the fno instruction.
    
    //! * Zero: If C(AQ) = 0, then ON; otherwise OFF
    //! * Neg: If C(AQ)0 = 1, then ON; otherwise OFF
    //! * Exp Ovr: If exponent is greater than +127, then ON
    //! * Exp Undr: If exponent is less than -128, then ON
    
    uint64 m1 = (cpu . rA << 28) | ((cpu . rQ & 0777777777400LL) >> 8) ; ///< only keep the 1st 64-bits :(
    int    e1 = SIGNEXT8_int (cpu . rE & MASK8);
    
    sim_debug (DBG_TRACE, & cpu_dev, "dufm e1 %d %03o m1 %012llo\n", e1, e1, m1);
    // CAC uint64 m2  = bitfieldExtract36(cpu.Ypair[0], 0, 28) << 44;    ///< 64-bit mantissa (incl sign)
           // CAC m2 |= (uint64) cpu.Ypair[1] << 8;
    uint64 m2  = bitfieldExtract36(cpu.Ypair[0], 0, 28) << 36;    ///< 64-bit mantissa (incl sign)
           m2 |= cpu.Ypair[1];
    
    //int8   e2 = (int8)(bitfieldExtract36(cpu.Ypair[0], 28, 8) & 0377U);    ///< 8-bit signed integer (incl sign)
    int    e2 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));
    sim_debug (DBG_TRACE, & cpu_dev, "dufm e2 %d %03o m2 %012llo\n", e2, e2, m2);

    
    //float72 m2 = 0;
    //int8 e2 = -128;
    
    //YPairToExpMant(cpu.Ypair, &m2, &e2);
    
    if (m1 == 0 || m2 == 0)
    {
        SET_I_ZERO;
        CLR_I_NEG;
        
        cpu . rE = 0200U; /*-128*/
        cpu . rA = 0;
        cpu . rQ = 0;
        
        return; // normalized 0
    }
    
    int sign = 1;

    
    if (m1 & FLOAT72SIGN)
    {
        if (m1 == FLOAT72SIGN)
        {
            sim_debug (DBG_TRACE, & cpu_dev, "dufm case 1\n");
            m1 >>= 1;
            e1 += 1;
        } else {
            sim_debug (DBG_TRACE, & cpu_dev, "dufm case 2\n");
            m1 = (~m1 + 1) & MASK72;
        }
        sign = -sign;
    }
    if (m2 & FLOAT72SIGN)
    {
        if (m2 == FLOAT72SIGN)
        {
            sim_debug (DBG_TRACE, & cpu_dev, "dufm case 3\n");
            m2 >>= 1;
            e2 += 1;
        } else {
            sim_debug (DBG_TRACE, & cpu_dev, "dufm case 4\n");
            m2 = (~m2 + 1) & MASK72;
        }
        sign = -sign;
    }
    
    int e3 = e1 + e2;
    
    uint128 m3 = ((uint128)m1) * ((uint128)m2);
    sim_debug (DBG_TRACE, & cpu_dev, "dufm e3 %d %03o m3 %012llo%012llo\n", e3, e3, (word36) (m3 >> 36) & MASK36, (word36) m3 & MASK36);
    uint128 m3a = m3 >> 63;
    sim_debug (DBG_TRACE, & cpu_dev, "dufm e3 %d %03o m3a %012llo\n", e3, e3, (word36) m3a & MASK36);
    
    if (sign == -1)
    {
        m3a = (~m3a + 1) & (((word72)1 << 71) - 1);    //0xffffffffffffffff;
        sim_debug (DBG_TRACE, & cpu_dev, "dufm sign -1 e3 %d %03o m3a %012llo\n", e3, e3, (word36) m3a & MASK36);
    }
    
    if (e3 > 127)
    {
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "dufm exp overflow fault");
    }

    // EUFL: If exponent is less than -128, then ON
    if(e3 < -128)
    {
        SET_I_EUFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "dufm exp underflow fault");
    }

    cpu . rE = e3 & MASK8;
    cpu . rA = (m3a >> 28) & MASK36;
    cpu . rQ = m3a & MASK36;
    //cpu . rQ = (m3a & 01777777777LL) << 8;
    
    // A normalization is performed only in the case of both factor mantissas being 100...0 which is the twos complement approximation to the decimal value -1.0.
    //if ((cpu . rE == -128 && cpu . rA == 0 && cpu . rQ == 0) && (m2 == 0 && e2 == -128)) // XXX FixMe
    if ((m1 == ((uint64)1 << 63)) && (m2 == ((uint64)1 << 63)))
        fno ();
    
    SC_I_ZERO (cpu . rA == 0 && cpu . rQ == 0);
    SC_I_NEG (cpu . rA & SIGN36);
    
}

/*!
 * floating divide ...
 */
// CANFAULT 
static void dfdvX (bool bInvert)
{
    //! C(EAQ) / C (Y) → C(EA)
    //! C(Y) / C(EAQ) → C(EA) (Inverted)
    
    //! 00...0 → C(Q)
    
    //! The dfdv instruction is executed as follows:
    //! The dividend mantissa C(AQ) is shifted right and the dividend exponent
    //! C(E) increased accordingly until
    //!    | C(AQ)0,63 | < | C(Y-pair)8,71 |
    //!    C(E) - C(Y-pair)0,7 → C(E)
    //!    C(AQ) / C(Y-pair)8,71 → C(AQ)0,63 00...0 → C(Q)64,71
    //! If the divisor mantissa C(Y-pair)8,71 is zero after alignment, the division does not take place. Instead, a divide check fault occurs, C(AQ) contains the dividend magnitude, and the negative indicator reflects the dividend sign.
    
    uint64 m1;
    int    e1;
    
    uint64 m2;
    int    e2;
    
    if (!bInvert)
    {
        m1 = (cpu . rA << 28) | ((cpu . rQ & 0777777777400LL) >> 8) ;  // only keep the 1st 64-bits :(
        e1 = SIGNEXT8_int (cpu . rE & MASK8);
        
        m2  = bitfieldExtract36(cpu.Ypair[0], 0, 28) << 36;    // 64-bit mantissa (incl sign)
        m2 |= cpu.Ypair[1];
        // CAC m2  = (uint64) bitfieldExtract36(cpu.Ypair[0], 0, 28) << 44;    // 64-bit mantissa (incl sign)
        // CAC m2 |= cpu.Ypair[1] << 8;
        
        //e2 = (int8)(bitfieldExtract36(cpu.Ypair[0], 28, 8) & 0377U);    // 8-bit signed integer (incl sign)
        e2 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));
    } else { // invert
        m2 = (cpu . rA << 28) | ((cpu . rQ & 0777777777400LL) >> 8) ; // only keep the 1st 64-bits :(
        e2 = SIGNEXT8_int (cpu . rE & MASK8);
        
        m1  = bitfieldExtract36(cpu.Ypair[0], 0, 28) << 36;    // 64-bit mantissa (incl sign)
        m1 |= cpu.Ypair[1];
        
        //e1 = (int8) (bitfieldExtract36(cpu.Ypair[0], 28, 8) & 0377U);    // 8-bit signed integer (incl sign)
        e1 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));
    }
    
    if (m1 == 0)
    {
        // XXX check flags
        SET_I_ZERO;
        SC_I_NEG (cpu . rA & SIGN36);
        
        cpu . rE = 0200U; /*-128*/
        cpu . rA = 0;
        cpu . rQ = 0;
        
        return;
    }
    

    
    //! make everything positive, but save sign info for later....
    int sign = 1;
    if (m1 & SIGN64)    //((uint64)1 << 63))
    {
        if (m1 == SIGN64)
        {
            m1 >>= 1;
            e1 += 1;
        } else
        m1 = (~m1 + 1);     //& (((uint64)1 << 64) - 1);
        sign = -sign;
    }
    
    if (m2 & SIGN64)    //((uint64)1 << 63))
    {
        if (m2 == SIGN64)
        {
            m2 >>= 1;
            e2 += 1;
        } else
        m2 = (~m2 + 1);     //& (((uint64)1 << 64) - 1);
        sign = -sign;
    }
    
    if (m2 == 0)
    {
        //If the divisor mantissa C(Y-pair)8,71 is zero after alignment, the division does not take place. Instead, a divide check fault occurs, C(AQ) contains the dividend magnitude, and the negative indicator reflects the dividend sign.
        
        // NB: If C(Y-pair)8,71 == 0 then the alignment loop will never exit! That's why it been moved before the alignment
        
        SET_I_ZERO;
        SC_I_NEG (cpu . rA & SIGN36);
        
        cpu . rA = m1;
        
        doFault(FAULT_DIV, 0, "DFDV: divide check fault");
    }
    
    while (m1 >= m2)
    {
        m1 >>= 1;
        e1 += 1;
    }
    if (e1 > 127)
    {
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "dfdvX exp overflow fault");
    }

    int e3 = e1 - e2;
    if (e3 > 127)
      {
        SET_I_EOFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "dfdvX exp overflow fault");
       }
    else if (e3 < -127)
      {
         SET_I_EUFL;
        if (tstOVFfault ())
            doFault (FAULT_OFL, 0, "dfdvX exp underflow fault");
       }

    //uint128 M1 = (uint128)m1 << 63;
    //uint128 M2 = (uint128)m2; ///< << 36;

    //uint128 m3 = M1 / M2;
    //uint128 m3 = (uint128)m1 << 35 / (uint128)m2;
    uint128 m3 = ((uint128)m1 << 63) / (uint128)m2;
    uint64 m3b = m3 & ((uint64)-1);  ///< only keep last 64-bits :-(

    if (sign == -1)
        m3b = (~m3b + 1); // & (((uint64)1 << 63) - 1);

    cpu . rE = e3 & MASK8;
    cpu . rA = (m3b >> 28) & MASK36;
    cpu . rQ = (m3b & 01777777777LL) << 8;//MASK36;
    
    SC_I_ZERO (cpu . rA == 0 && cpu . rQ == 0);
    SC_I_NEG (cpu . rA & SIGN36); 

    if (cpu . rA == 0 && cpu . rQ == 0)    // set to normalized 0
        cpu . rE = 0200U; /*-128*/
}

// CANFAULT 
void dfdv (void)
{
    dfdvX (false);    // no inversion
}

// CANFAULT 
void dfdi (void)
{
    dfdvX (true);
}


//#define DVF_HWR 
//#define DVF_FRACTIONAL
#define DVF_CAC
// CANFAULT 
void dvf (void)
{
//#ifdef DBGCAC
//sim_printf ("DVF %lld %06o:%06o\n", sim_timell (), PPR.PSR, PPR.IC);
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
//HWR--#ifdef DVF_HWR
//HWR--    // m1 divedend
//HWR--    // m2 divisor
//HWR--
//HWR--    word72 m1 = SIGNEXT36_72((cpu . rA << 36) | (cpu . rQ & 0777777777776LLU));
//HWR--    word72 m2 = SIGNEXT36_72(cpu.CY);
//HWR--
//HWR--//sim_debug (DBG_CAC, & cpu_dev, "[%lld]\n", sim_timell ());
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
//HWR--        doFault(FAULT_DIV, 0, "DVF: divide check fault");
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

// canonial code
#ifdef DVF_FRACTIONAL

if (currentRunningCPUnum)
sim_printf ("DVF A %012llo Q %012llo CY %012llo\n", cpu.rA, cpu.rQ, cpu.CY);

// http://www.ece.ucsb.edu/~parhami/pres_folder/f31-book-arith-pres-pt4.pdf
// slide 10: sequential algorithim

    // dividend format
    // 0  1     70 71
    // s  dividend x
    //  C(AQ)

    int sign = 1;
    bool dividendNegative = (getbits36 (cpu . rA, 0, 1) != 0);
    bool divisorNegative = (getbits36 (cpu.CY, 0, 1) != 0);

    // Get the 70 bits of the dividend (72 bits less the sign bit and the
    // ignored bit 71.

    // dividend format:   . d(0) ...  d(69)

    uint128 zFrac = ((uint128) (cpu . rA & MASK35) << 35) | ((cpu . rQ >> 1) & MASK35);
    if (dividendNegative)
      {
        zFrac = ~zFrac + 1;
        sign = - sign;
      }
    zFrac &= MASK70;
//sim_printf ("zFrac "); print_int128 (zFrac); sim_printf ("\n");

    // Get the 35 bits of the divisor (36 bits less the sign bit)

    // divisor format: . d(0) .... d(34) 0 0 0 .... 0 
    
#if 1
    // divisor goes in the high half
    uint128 dFrac = (cpu.CY & MASK35) << 35;
    if (divisorNegative)
      {
        dFrac = ~dFrac + 1;
        sign = - sign;
      }
    dFrac &= MASK35 << 35;
#else
    // divisor goes in the low half
    uint128 dFrac = cpu.CY & MASK35;
    if (divisorNegative)
      {
        dFrac = ~dFrac + 1;
        sign = - sign;
      }
    dFrac &= MASK35;
#endif
//sim_printf ("dFrac "); print_int128 (dFrac); sim_printf ("\n");
if (currentRunningCPUnum)
sim_printf ("zFrac %012llo %02llo\n", (word36) (zFrac >> 36) & MASK36, (word36) zFrac & MASK36);
if (currentRunningCPUnum)
sim_printf ("dFrac %012llo %02llo\n", (word36) (dFrac >> 36) & MASK36, (word36) dFrac & MASK36);

    if (dFrac == 0)
      {
sim_printf ("DVFa A %012llo Q %012llo Y %012llo\n", cpu.rA, cpu.rQ, cpu.CY);
// case 1: 400000000000 000000000000 000000000000 --> 400000000000 000000000000
//         dFrac 000000000000 000000000000

        //cpu . rA = (zFrac >> 35) & MASK35;
        //cpu . rQ = (zFrac & MASK35) << 1;

        //SC_I_ZERO (dFrac == 0);
        //SC_I_NEG (cpu . rA & SIGN36);
        SC_I_ZERO (cpu.CY == 0);
        SC_I_NEG (cpu.rA & SIGN36);
        doFault(FAULT_DIV, 0, "DVF: divide check fault");
      }

    uint128 sn = zFrac;
    word36 quot = 0;
    for (uint i = 0; i < 35; i ++)
      {
        // 71 bit number
        uint128 s2n = sn << 1;
        if (s2n > dFrac)
          {
            s2n -= dFrac;
            quot |= (1llu << (34 - i));
          }
        sn = s2n;
      }
    word36 remainder = sn;

    if (sign == -1)
      quot = ~quot + 1;

    if (dividendNegative)
      remainder = ~remainder + 1;

if (currentRunningCPUnum)
sim_printf ("quot %012llo %012llo\n", (word36) (quot >> 36) & MASK36, (word36) quot & MASK36);
if (currentRunningCPUnum)
sim_printf ("rem  %012llo %012llo\n", (word36) (remainder >> 36) & MASK36, (word36) remainder & MASK36);

    // I am surmising that the "If | dividend | >= | divisor |" is an
    // overflow prediction; implement it by checking that the calculated
    // quotient will fit in 35 bits.

    if (quot & ~MASK35)
      {
if (currentRunningCPUnum)
sim_printf ("DVFb A %012llo Q %012llo Y %012llo\n", cpu.rA, cpu.rQ, cpu.CY);
        //cpu . rA = (zFrac >> 35) & MASK35;
        //cpu . rQ = (zFrac & MASK35) << 1;

        //SC_I_ZERO (dFrac == 0);
        //SC_I_NEG (cpu . rA & SIGN36);
        SC_I_ZERO (cpu.rA == 0);
        SC_I_NEG (cpu.rA & SIGN36);
        
        doFault(FAULT_DIV, 0, "DVF: divide check fault");
      }
    cpu . rA = quot & MASK36;
    cpu . rQ = remainder & MASK36;
 
#endif

// MM code
#ifdef DVF_CAC

//sim_debug (DBG_CAC, & cpu_dev, "dvf [%lld]\n", sim_timell ());
//sim_debug (DBG_CAC, & cpu_dev, "rA %llu\n", cpu . rA);
//sim_debug (DBG_CAC, & cpu_dev, "rQ %llu\n", cpu . rQ);
//sim_debug (DBG_CAC, & cpu_dev, "CY %llu\n", cpu.CY);
//if (currentRunningCPUnum)
//sim_printf ("DVF A %012llo Q %012llo CY %012llo\n", cpu.rA, cpu.rQ, cpu.CY);
#if 0
    if (cpu.CY == 0)
      {
        // XXX check flags
        SET_I_ZERO;
        SC_I_NEG (cpu . rA & SIGN36);
        
        cpu . rA = 0;
        cpu . rQ = 0;
        
        return;
    }
#endif


    // dividend format
    // 0  1     70 71
    // s  dividend x
    //  C(AQ)

    int sign = 1;
    bool dividendNegative = (getbits36 (cpu . rA, 0, 1) != 0);
    bool divisorNegative = (getbits36 (cpu.CY, 0, 1) != 0);

    // Get the 70 bits of the dividend (72 bits less the sign bit and the
    // ignored bit 71.

    // dividend format:   . d(0) ...  d(69)

    uint128 zFrac = ((uint128) (cpu . rA & MASK35) << 35) | ((cpu . rQ >> 1) & MASK35);
    //zFrac <<= 1; -- Makes Multics unbootable.

    if (dividendNegative)
      {
        zFrac = ~zFrac + 1;
        sign = - sign;
      }
    zFrac &= MASK70;

    //char buf [128] = "";
    //print_int128 (zFrac, buf);
    //sim_debug (DBG_CAC, & cpu_dev, "zFrac %s\n", buf);

    // Get the 35 bits of the divisor (36 bits less the sign bit)

    // divisor format: . d(0) .... d(34) 0 0 0 .... 0 
    
    // divisor goes in the low half
    uint128 dFrac = cpu.CY & MASK35;
    if (divisorNegative)
      {
        dFrac = ~dFrac + 1;
        sign = - sign;
      }
    dFrac &= MASK35;

//if (currentRunningCPUnum)
//sim_printf ("zFrac %012llo %02llo\n", (word36) (zFrac >> 36) & MASK36, (word36) zFrac & MASK36);
//if (currentRunningCPUnum)
//sim_printf ("dFrac %012llo %02llo\n", (word36) (dFrac >> 36) & MASK36, (word36) dFrac & MASK36);
    //char buf2 [128] = "";
    //print_int128 (dFrac, buf2);
    //sim_debug (DBG_CAC, & cpu_dev, "dFrac %s\n", buf2);

    //if (dFrac == 0 || zFrac >= dFrac)
    //if (dFrac == 0 || zFrac >= dFrac << 35)
    if (dFrac == 0)
      {
//if (currentRunningCPUnum)
//sim_printf ("DVFa A %012llo Q %012llo Y %012llo\n", cpu.rA, cpu.rQ, cpu.CY);
// case 1: 400000000000 000000000000 000000000000 --> 400000000000 000000000000
//         dFrac 000000000000 000000000000

        //cpu . rA = (zFrac >> 35) & MASK35;
        //cpu . rQ = (zFrac & MASK35) << 1;

        //SC_I_ZERO (dFrac == 0);
        //SC_I_NEG (cpu . rA & SIGN36);
        SC_I_ZERO (cpu.CY == 0);
        SC_I_NEG (cpu.rA & SIGN36);
        doFault(FAULT_DIV, 0, "DVF: divide check fault");
      }

    uint128 quot = zFrac / dFrac;
    uint128 remainder = zFrac % dFrac;
//if (currentRunningCPUnum)
//sim_printf ("quot %012llo %012llo\n", (word36) (quot >> 36) & MASK36, (word36) quot & MASK36);
//if (currentRunningCPUnum)
//sim_printf ("rem  %012llo %012llo\n", (word36) (remainder >> 36) & MASK36, (word36) remainder & MASK36);



    // I am surmising that the "If | dividend | >= | divisor |" is an
    // overflow prediction; implement it by checking that the calculated
    // quotient will fit in 35 bits.

    if (quot & ~MASK35)
      {
//if (currentRunningCPUnum)
//sim_printf ("DVFb A %012llo Q %012llo Y %012llo\n", cpu.rA, cpu.rQ, cpu.CY);
//if (currentRunningCPUnum)
//sim_printf ("quot %012llo %012llo\n", (word36) (quot >> 36) & MASK36, (word36) quot & MASK36);
//
// this got:
//            s/b 373737373737 373737373740 200200
//            was 373737373740 373737373740 000200
//                          ~~ 
#if 1
        bool Aneg = (cpu.rA & SIGN36) != 0; // blood type
        bool AQzero = cpu.rA == 0 && cpu.rQ == 0;
        if (cpu.rA & SIGN36)
          {
//if (currentRunningCPUnum)
//sim_printf ("negating AQ\n");
            cpu.rA = (~cpu.rA) & MASK36;
            cpu.rQ = (~cpu.rQ) & MASK36;
            cpu.rQ += 1;
//if (currentRunningCPUnum)
//sim_printf ("Q after incr: %012llo\n", cpu.rQ);
            if (cpu.rQ & BIT37) // overflow?
              {
//if (currentRunningCPUnum)
//sim_printf ("incr. A\n");
                cpu.rQ &= MASK36;
                cpu.rA = (cpu.rA + 1) & MASK36;
              }
          }
#else
        if (cpu.rA & SIGN36)
          {
            cpu.rA = (cpu.rA + 1) & MASK36;
            cpu.rQ = (cpu.rQ + 1) & MASK36;
          }
#endif
        //cpu . rA = (zFrac >> 35) & MASK35;
        //cpu . rQ = (zFrac & MASK35) << 1;

        //SC_I_ZERO (dFrac == 0);
        //SC_I_NEG (cpu . rA & SIGN36);
        SC_I_ZERO (AQzero);
        SC_I_NEG (Aneg);
        
        doFault(FAULT_DIV, 0, "DVF: divide check fault");
      }
    //char buf3 [128] = "";
    //print_int128 (remainder, buf3);
    //sim_debug (DBG_CAC, & cpu_dev, "remainder %s\n", buf3);

    if (sign == -1)
      quot = ~quot + 1;

    if (dividendNegative)
      remainder = ~remainder + 1;

//if (currentRunningCPUnum)
//sim_printf ("quot %012llo %012llo\n", (word36) (quot >> 36) & MASK36, (word36) quot & MASK36);
//if (currentRunningCPUnum)
//sim_printf ("rem  %012llo %012llo\n", (word36) (remainder >> 36) & MASK36, (word36) remainder & MASK36);
    cpu . rA = quot & MASK36;
    cpu . rQ = remainder & MASK36;
 
#endif

//sim_debug (DBG_CAC, & cpu_dev, "Quotient %lld (%llo)\n", cpu . rA, cpu . rA);
//sim_debug (DBG_CAC, & cpu_dev, "Remainder %lld\n", cpu . rQ);
    SC_I_ZERO (cpu . rA == 0 && cpu . rQ == 0);
    SC_I_NEG (cpu . rA & SIGN36);
}


/*!
 * double precision floating round ...
 */
void dfrd (void)
{
    //! The dfrd instruction is identical to the frd instruction except that the rounding constant used is (11...1)65,71 instead of (11...1)29,71.
    
    //! If C(AQ) ≠ 0, the frd instruction performs a true round to a precision of 64 bits and a normalization on C(EAQ).
    //! A true round is a rounding operation such that the sum of the result of applying the operation to two numbers of equal magnitude but opposite sign is exactly zero.
    
    //! The frd instruction is executed as follows:
    //! C(AQ) + (11...1)65,71 → C(AQ)
    //! * If C(AQ)0 = 0, then a carry is added at AQ71
    //! * If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
    //! * If overflow does not occur, C(EAQ) is normalized.
    //! * If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.
        
    float72 m = ((word72)cpu . rA << 36) | (word72)cpu . rQ;
    if (m == 0)
    {
        cpu . rE = 0200U; /*-128*/
        SET_I_ZERO;
        CLR_I_NEG;
        
        return;
    }

    bool s1 = (bool)(m & SIGN72);
    
    // C(AQ) + (11...1)65,71 → C(AQ)
    m += (word72)0177LL; // add 1's into lower 64-bits

    // If C(AQ)0 = 0, then a carry is added at AQ71
    if (!s1)
        m += 1;
    
    // 0 → C(AQ)64,71 
    //m &= (word72)0777777777777LL << 36 | 0777777777400LL; // 64-71 => 0 per DH02-01/Bull DPS9000
    m = bitfieldInsert72(m, 0, 0, 8);
    
    bool s2 = (bool)(m & SIGN72);
    
    bool ov = s1 != s2;   ///< sign change denotes overflow
    if (ov)
    {
        // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
        m >>= 1;
        if (s1) // restore sign if necessary (was s2)
            m |= SIGN72;
        
        if (cpu . rE == 127)
        {
            SET_I_EOFL;
            if (tstOVFfault ())
                doFault (FAULT_OFL, 0, "dfrd exp overflow fault");
        }
        cpu . rE +=  1;
        cpu . rE &= MASK8;
        cpu . rA = (m >> 36) & MASK36;
        cpu . rQ = m & MASK36;
    }
    else
    {
        // If overflow does not occur, C(EAQ) is normalized.
        cpu . rA = (m >> 36) & MASK36;
        cpu . rQ = m & MASK36;
        
        fno ();
    }
    
    // If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.
    if (cpu . rA == 0 && cpu . rQ == 0)
    {
        cpu . rE = 0200U; /*-128*/
        SET_I_ZERO;
    }
    
    SC_I_NEG (cpu . rA & SIGN36);
}

void dfstr (word36 *Ypair)
{
    //! The dfstr instruction performs a double-precision true round and normalization on C(EAQ) as it is stored.
    //! The definition of true round is located under the description of the frd instruction.
    //! The definition of normalization is located under the description of the fno instruction.
    //! Except for the precision of the stored result, the dfstr instruction is identical to the fstr instruction.
        
    //! The dfrd instruction is identical to the frd instruction except that the rounding constant used is (11...1)65,71 instead of (11...1)29,71.
    
    //! If C(AQ) ≠ 0, the frd instruction performs a true round to a precision of 28 bits and a normalization on C(EAQ).
    //! A true round is a rounding operation such that the sum of the result of applying the operation to two numbers of equal magnitude but opposite sign is exactly zero.
    
    //! The frd instruction is executed as follows:
    //! C(AQ) + (11...1)29,71 → C(AQ)
    //! * If C(AQ)0 = 0, then a carry is added at AQ71
    //! * If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
    //! * If overflow does not occur, C(EAQ) is normalized.
    //! * If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.
    
    //! I believe AL39 is incorrect; bits 64-71 should be set to 0, not 65-71. DH02-01 & Bull 9000 is correct.
    
    word36 A = cpu . rA, Q = cpu . rQ;
    int E = SIGNEXT8_int (cpu . rE & MASK8);
    A &= DMASK;
    Q &= DMASK;

    float72 m = ((word72)A << 36) | (word72)cpu . rQ;
    if (m == 0)
    {
        E = -128;
        SET_I_ZERO;
        CLR_I_NEG;
        
        Ypair[0] = ((word36)(E & MASK8) << 28) | ((A & 0777777777400LLU) >> 8);
        Ypair[1] = ((A & MASK8) << 28) | ((Q & 0777777777400LLU) >> 8);

        return;
    }
    
    
    // C(AQ) + (11...1)65,71 → C(AQ)
    bool s1 = (m & SIGN72) != (word72)0;
    
    m += (word72)0177LLU; // add 1's into lower 43-bits
    
    // If C(AQ)0 = 0, then a carry is added at AQ71
    if (s1 == 0)
        m += 1;
    
    // 0 → C(AQ)64,71
    //m &= (word72)0777777777777LL << 36 | 0777777777400LL; // 64-71 => 0 per DH02-01/Bull DPS9000
    m = bitfieldInsert72(m, 0, 0, 8);
    
    bool s2 = (m & SIGN72) != (word72)0;
    
    bool ov = s1 != s2;   ///< sign change denotes overflow
    if (ov)
    {
        // If overflow occurs, C(AQ) is shifted one place to the right and C(E) is increased by 1.
        m >>= 1;
        if (s1) // restore sign if necessary (was s2)
            m |= SIGN72;
        
        if (E == 127)
            SET_I_EOFL;
            if (tstOVFfault ())
                doFault (FAULT_OFL, 0, "dfrd exp overflow fault");
        E +=  1;
        
        A = (m >> 36) & MASK36;
        Q = m & MASK36;
    }
    else
    {
        // If overflow does not occur, C(EAQ) is normalized.
        A = (m >> 36) & MASK36;
        Q = m & MASK36;
        
        fno();
    }
    
    // If C(AQ) = 0, C(E) is set to -128 and the zero indicator is set ON.
    if (A == 0 && Q == 0)
    {
        E = -128;
        SET_I_ZERO;
    }
    
    SC_I_NEG (A & SIGN36);
    
    Ypair[0] = ((word36)(E & MASK8) << 28) | ((A & 0777777777400LL) >> 8);
    Ypair[1] = ((A & 0377) << 28) | ((Q & 0777777777400LL) >> 8);
}

/*!
 * double precision Floating Compare ...
 */
void dfcmp (void)
{
#if 0
    //! C(E) :: C(Y-pair)0,7
    //! C(AQ)0,63 :: C(Y-pair)8,71
    
    //! Zero: If C(EAQ) = C(Y), then ON; otherwise OFF
    //! Neg: If C(EAQ) < C(Y), then ON; otherwise OFF
    
    //! The dfcmp instruction is identical to the fcmp instruction except for the precision of the mantissas actually compared.
    
    //! Notes: The fcmp instruction is executed as follows:
    //! The mantissas are aligned by shifting the mantissa of the operand with the algebraically smaller exponent to the right the number of places equal to the difference in the two exponents.
    //! The aligned mantissas are compared and the indicators set accordingly.
    
    int64 m1 = (int64) ((cpu . rA << 28) | ((cpu . rQ & 0777777777400LL) >> 8));  ///< only keep the 1st 64-bits :(
    int   e1 = SIGNEXT8_int (cpu . rE & MASK8);
    
    int64 m2  = (int64) (bitfieldExtract36(cpu.Ypair[0], 0, 28) << 36);    ///< 64-bit mantissa (incl sign)
          m2 |= cpu.Ypair[1];
    // CAC int64 m2  = (int64) (bitfieldExtract36(cpu.Ypair[0], 0, 28) << 44);    ///< 64-bit mantissa (incl sign)
          // CAC m2 |= cpu.Ypair[1] << 8;
    
    //int8 e2 = (int8) (bitfieldExtract36(cpu.Ypair[0], 28, 8) & 0377U);    ///< 8-bit signed integer (incl sign)
    int   e2 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));

    //which exponent is smaller???
    
    int shift_count = -1;
    
    if (e1 == e2)
    {
        shift_count = 0;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
        m1 >>= shift_count;
    }
    else
    {
        shift_count = abs(e1 - e2);
        m2 >>= shift_count;
    }
    
    // need to do algebraic comparisons of mantissae
    SC_I_ZERO (m1 == m2);
    SC_I_NEG  (m1 <  m2);
#else
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
    
//if (currentRunningCPUnum)
//sim_printf ("DFCMP E %03o A %012llo Q %012llo CY %012llo %12llo\n", cpu.rE, cpu.rA, cpu.rQ, cpu.Ypair[0], cpu.Ypair[1]);
    // C(AQ)0,63
    word72 m1 = ((uint128) (cpu . rA & MASK36) << 36) | ((cpu . rQ) & 0777777777400LL);
    int   e1 = SIGNEXT8_int (cpu . rE & MASK8);

//if (currentRunningCPUnum)
//sim_printf ("DFCMP e1 %d m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);

    // C(Y-pair)8,71
    word72 m2 = (uint128) getbits36 (cpu.Ypair[0], 8, 28) << (36 + 8);  
    m2 |= cpu.Ypair[1] << 8;
    int   e2 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));
    
//if (currentRunningCPUnum)
//sim_printf ("DFCMP e2 %d m2 %012llo %012llo\n", e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);

    int e3 = -1;

    //which exponent is smaller???
    
    int shift_count = -1;
    
    if (e1 == e2)
    {
        shift_count = 0;
        e3 = e1;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
        bool s = m1 & SIGN72;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m1 >>= 1;
            if (s)
                m1 |= SIGN72;
        }
        
        m1 &= MASK72;
        e3 = e2;
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
        bool s = m2 & SIGN72;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m2 >>= 1;
            if (s)
                m2 |= SIGN72;
        }
        m2 &= MASK72;
        e3 = e1;
    }
    
//if (currentRunningCPUnum)
//sim_printf ("DFCMP shifted e1 %d m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
//if (currentRunningCPUnum)
//sim_printf ("DFCMP shifted e2 %d m2 %012llo %012llo\n", e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
    SC_I_ZERO (m1 == m2);
    int128 sm1 = SIGNEXT72_128 (m1);
    int128 sm2 = SIGNEXT72_128 (m2);
    SC_I_NEG (sm1 < sm2);
#endif
}

/*!
 * double precision Floating Compare magnitude ...
 */
void dfcmg (void)
{
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
    
//if (currentRunningCPUnum)
//sim_printf ("DFCMG E %03o A %012llo Q %012llo CY %012llo %12llo\n", cpu.rE, cpu.rA, cpu.rQ, cpu.Ypair[0], cpu.Ypair[1]);
    // C(AQ)0,63
    word72 m1 = ((uint128) (cpu . rA & MASK36) << 36) | ((cpu . rQ) & 0777777777400LL);
    int   e1 = SIGNEXT8_int (cpu . rE & MASK8);

//if (currentRunningCPUnum)
//sim_printf ("DFCMG e1 %d m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);

    // C(Y-pair)8,71
    word72 m2 = (uint128) getbits36 (cpu.Ypair[0], 8, 28) << (36 + 8);  
    m2 |= cpu.Ypair[1] << 8;
    int   e2 = SIGNEXT8_int (getbits36 (cpu.Ypair[0], 0, 8));
    
if (currentRunningCPUnum)
sim_printf ("DFCMG e2 %d m2 %012llo %012llo\n", e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);

    int e3 = -1;

    //which exponent is smaller???
    
    int shift_count = -1;
    
    if (e1 == e2)
    {
        shift_count = 0;
        e3 = e1;
    }
    else if (e1 < e2)
    {
        shift_count = abs(e2 - e1);
        bool s = m1 & SIGN72;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m1 >>= 1;
            if (s)
                m1 |= SIGN72;
if (currentRunningCPUnum)
sim_printf ("DFCMG >>1 e1 %d m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
        }
        
        m1 &= MASK72;
        e3 = e2;
    }
    else
    {
        // e2 < e1;
        shift_count = abs(e1 - e2);
        bool s = m2 & SIGN72;   ///< mantissa negative?
        for(int n = 0 ; n < shift_count ; n += 1)
        {
            m2 >>= 1;
            if (s)
                m2 |= SIGN72;
        }
        m2 &= MASK72;
        e3 = e1;
    }
    
if (currentRunningCPUnum)
sim_printf ("DFCMG shifted e1 %d m1 %012llo %012llo\n", e1, (word36) (m1 >> 36) & MASK36, (word36) m1 & MASK36);
if (currentRunningCPUnum)
sim_printf ("DFCMG shifted e2 %d m2 %012llo %012llo\n", e2, (word36) (m2 >> 36) & MASK36, (word36) m2 & MASK36);
    SC_I_ZERO (m1 == m2);
    //int128 sm1 = llabs (SIGNEXT72_128 (m1));
    //int128 sm2 = llabs (SIGNEXT72_128 (m2));
    int128 sm1 = SIGNEXT72_128 (m1);
    if (sm1 < 0)
      sm1 = - sm1;
    int128 sm2 = SIGNEXT72_128 (m2);
    if (sm2 < 0)
      sm2 = - sm2;

    SC_I_NEG (sm1 < sm2);
}
