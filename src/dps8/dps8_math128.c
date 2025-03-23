/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 928c0e86-f62e-11ec-ab5c-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Jean-Michel Merliot
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License..
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

#if !defined(DPS8_MATH128)
# define DPS8_MATH128
# include "dps8.h"
# include "dps8_math128.h"

# if defined(NEED_128)

bool iszero_128 (uint128 w)
  {
    if (w.h || w.l)
      return false;
    return true;
  }

bool isnonzero_128 (uint128 w)
  {
    if (w.h || w.l)
      return true;
    return false;
  }

bool iseq_128 (uint128 a, uint128 b)
  {
    return a.h == b.h && a.l == b.l;
  }

bool isgt_128 (uint128 a, uint128 b)
  {
    if (a.h > b.h) return true;
    if (a.h < b.h) return false;
    if (a.l > b.l) return true;
    return false;
  }

bool islt_128 (uint128 a, uint128 b)
  {
    if (a.h < b.h) return true;
    if (a.h > b.h) return false;
    if (a.l < b.l) return true;
    return false;
  }

bool isge_128 (uint128 a, uint128 b)
  {
    if (a.h >  b.h) return true;
    if (a.h <  b.h) return false;
    if (a.l >= b.l) return true;
    return false;
  }

bool islt_s128 (int128 a, int128 b)
  {
    if (a.h < b.h) return true;
    if (a.h > b.h) return false;
    if (a.l < b.l) return true;
    return false;
  }

bool isgt_s128 (int128 a, int128 b)
  {
    if (a.h > b.h) return true;
    if (a.h < b.h) return false;
    if (a.l > b.l) return true;
    return false;
  }

uint128 and_128 (uint128 a, uint128 b)
  {
    return (uint128) {a.h & b.h, a.l & b.l};
  }

int128 and_s128 (int128 a, uint128 b)
  {
    return (int128) {a.h & (int64_t)b.h, a.l & b.l};
  }

uint128 or_128 (uint128 a, uint128 b)
  {
    return (uint128) {a.h | b.h, a.l | b.l};
  }

uint128 xor_128 (uint128 a, uint128 b)
  {
    return (uint128) {a.h ^ b.h, a.l ^ b.l};
  }

uint128 complement_128 (uint128 a)
  {
    return (uint128) {~ a.h, ~ a.l};
  }

uint128 add_128 (uint128 a, uint128 b)
  {
// To do carry detection from low to high, bust the low into 1 bit/63
// bit fields; add the 63 bit fields checking for carry in the "sign"
// bit; add the 1 bit fields plus that carry

    uint64_t al63 = a.l & MASK63;  // low 63 bits of a
    uint64_t bl63 = b.l & MASK63;  // low 63 bits of b
    uint64_t l63 = al63 + bl63;    // low 63 bits of a + b, with carry into bit 64
    uint64_t c63 = l63 & SIGN64;   // the carry out of low 63 a + b
    l63 &= MASK63;                 // lose the carry bit

    unsigned int al64 = (a.l >> 63) & 1; // bit 64 of a
    unsigned int bl64 = (b.l >> 63) & 1; // bit 64 of b
    unsigned int cl64 = (c63 >> 63) & 1; // the carry out of bit 63
    uint64_t l64 = al64 + bl64 + cl64;   // bit 64 a + b + carry in
    unsigned int c64 = l64 > 1 ? 1 : 0;  // carry out of bit 64
    l64 = (l64 & 1) << 63;               // put bit 64 in the right place
    l64 |= l63;                          // put low 63 bits in
    uint64_t h64 = a.h + b.h + c64;      // compute the high
    return construct_128 (h64, l64);
  }

uint128 subtract_128 (uint128 a, uint128 b)
  {
//(void)printf ("sub a %016llx %016llx\r\n", a.h, a.l);
//(void)printf ("sub b %016llx %016llx\r\n", b.h, b.l);
    bool borrow = !! (b.l > a.l);
    uint128 res = construct_128 (a.h - b.h, a.l - b.l);
    if (borrow)
      res.h --;
//(void)printf ("sub res %016llx %016llx\r\n", res.h, res.l);
    return res;
  }

uint128 negate_128 (uint128 a)
  {
    return add_128 (complement_128 (a), construct_128 (0, 1));
  }

int128 negate_s128 (int128 a)
  {
    uint128 t = add_128 (complement_128 (cast_128 (a)), construct_128 (0, 1));
    return cast_s128 (t);
  }

uint128 lshift_128 (uint128 a, unsigned int n)
#  if ( defined(__clang__) || defined(__clang_version__) ) && defined(__llvm__)
#   if defined(NEED_128)
__attribute__ ((optnone))
#   endif /* NEED_128 */
#  endif /* if ( defined(__clang__) || defined(__clang_version__) ) && defined(__llvm__) */
  {
    if (n < 64)
      {
        uint64_t nmask = (uint64_t) ((~(MASK64 << n)));
//(void)printf ("nmask %016llx\r\n", nmask);
        // capture the high bits of the low half
        uint64_t keep = (a.l >> (64 - n)) & nmask;
//(void)printf ("keep %016llx\r\n", keep);
        // shift the low half
        uint64_t l = a.l << n;
//(void)printf ("l %016llx\r\n", l);
        // shift the high half
        uint64_t h = a.h << n;
//(void)printf ("h %016llx\r\n", h);
        // put the bits from the low into the high
        h |= keep;
//(void)printf ("h|keep %016llx\r\n", h);
        return construct_128 (h, l);
      }
    uint64_t h = a.l << (n - 64);
    return construct_128 (h, 0);
  }

int128 lshift_s128 (int128 a, unsigned int n)
  {
    uint128 t = lshift_128 (cast_128 (a), n);
    return cast_s128 (t);
  }

uint128 rshift_128 (uint128 a, unsigned int n)
  {
#  if 0
    uint64_t sign = a.h & SIGN64;
    if (n < 64)
      {
        uint64_t nmask = (uint64_t) ((~(-1 << n)));
        // capture the low bits of the high half
        uint64_t keep = (a.h & nmask) << (64 - n);
        // shift the low half
        uint64_t l = a.l >> n;
        // shifting zeros in from on high
        l &= (uint64_t) (-(1 << (64 - n)));
        // put in the bits from the high half
        l |= keep;
        // shift the high half
        uint64_t h = a.h >> n;
        if (sign)
          // extending the signbit
          l |= nmask << (64 - n);
        else
          // shifting zeros from on high
          l &= (uint64_t) (-(1 << (64 - n)));
        return construct_128 (h, l);
      }
    if (n == 64)
      {
        if (sign)
          return construct_128 (MASK64, a.h);
        return construct_128 (0, a.h);
      }
    uint64_t nmask = (uint64_t) ((1LLU << (n - 65)) - 1);
(void)printf ("nmask %016llx\r\n", nmask);
    uint64_t l = a.h >> (n - 64);
(void)printf ("l %016llx\r\n", l);
    uint64_t h = sign ? MASK64 : 0;
(void)printf ("h %016llx\r\n", h);
    if (sign)
      {
        // extending the signbit
        l |= nmask << (64 - n);
      }
    else
      {
        // shifting zeros from on high
        l &= (uint64_t) (~(-1 << (64 - n)));
      }
(void)printf ("l2 %016llx\r\n", l);
#  endif

    uint64_t h = a.h;
    uint64_t l = a.l;
    uint64_t sign = a.h & SIGN64;
    while (n)
      {
        uint64_t b = (h & 1) ? SIGN64 : 0;
        h >>= 1;
        h |= sign;
        l >>= 1;
        l &= MASK63;
        l |= b;
        n --;
      }
    return construct_128 (h, l);
  }

int128 rshift_s128 (int128 a, unsigned int n)
  {
    uint128 t = rshift_128 (cast_128 (a), n);
    return cast_s128 (t);
  }

// See: https://web.archive.org/web/20190219173849/http://www.hackersdelight.org/

static void mulmn (uint32_t w[], uint32_t u[],
                   uint32_t v[], int m, int n)
  {
//for (int i = m - 1; i >= 0; i --)
//  (void)fprintf (stderr, "%08x", u [i]);
//(void)fprintf (stderr, "  ");
//for (int i = n - 1; i >= 0; i --)
//  (void)fprintf (stderr, "%08x", v [i]);
//(void)fprintf (stderr, "\r\n");
    uint64_t k, t;
    int i, j;

    for (i = 0; i < m; i++)
       w[i] = 0;

    for (j = 0; j < n; j++)
      {
        k = 0;
        for (i = 0; i < m; i++)
          {
            t = (uint64_t) u[i] * (uint64_t) v[j] + (uint64_t) w[i + j] + k;
//(void)printf ("%d %d %016llx\r\n",i, j, t);
            w[i + j] = (uint32_t) t;        // (i.e., t & 0xFFFF).
            k = t >> 32;
          }
        w[j + m] = (uint32_t) k;
      }
  }

static void mulmns (uint32_t w[], uint32_t u[],
                    uint32_t v[], int m, int n)
  {
    mulmn (w, u, v, m, n);

    // Now w[] has the unsigned product. Correct by
    // subtracting v*2**16m if u < 0, and
    // subtracting u*2**16n if v < 0.

    if ((int32_t)u[m - 1] < 0)
      {
        int b = 0;  // Initialize borrow.
        for (int j = 0; j < n; j++)
          {
            int t = (int) w[j + m] - (int) v[j] - b;
            w[j + m] = (uint32_t) t;
            b = (t & 0x40000000) ? 1 : 0;
          }
      }
    if ((int32_t)v[n - 1] < 0)
      {
        int b = 0;
        for (int i = 0; i < m; i++)
          {
            int t = (int) w[i + n] - (int) u[i] - b;
            w[i + n] = (uint32_t) t;
            b = (t & 0x40000000) ? 1 : 0;
          }
      }
    return;
  }

static int
nlz (unsigned x)
{
  int n;

  if (x == 0)
    return (32);

  n = 0;

  if (x <= 0x0000FFFF)
    {
      n = n  + 16;
      x = x << 16;
    }

  if (x <= 0x00FFFFFF)
    {
      n = n  + 8;
      x = x << 8;
    }

  if (x <= 0x0FFFFFFF)
    {
      n = n  + 4;
      x = x << 4;
    }

  if (x <= 0x3FFFFFFF)
    {
      n = n  + 2;
      x = x << 2;
    }

  if (x <= 0x7FFFFFFF)
    {
      n = n + 1;
    }

  return n;
}

unsigned int kd_div_errors = 0;

int divmnu (uint16_t q[], uint16_t r[],
            const uint16_t u[], const uint16_t v[],
            int m, int n)
  {
    const uint32_t b = 65536;   // Number base (16 bits).
    //uint16_t *un, *vn;        // Normalized form of u, v.
    unsigned qhat;              // Estimated quotient digit.
    unsigned rhat;              // A remainder.
    unsigned p;                 // Product of two digits.
    int s, i, j, t, k;

    if (m < n || n <= 0 || v[n-1] == 0)
      return 1;                 // Return if invalid param.

    // Take care of the case of a single-digit span
    if (n == 1)
      {
        k = 0;
        for (j = m - 1; j >= 0; j--)
          {
            q[j] = (uint16_t) (((unsigned int) k*b + u[j])/v[0]);    // divisor here.
            k = (int) (((unsigned int) k*b + u[j]) - q[j]*v[0]);
          }
        if (r != NULL) r[0] = (uint16_t) k;
        return 0;
      }

    // Normalize by shifting v left just enough so that
    // its high-order bit is on, and shift u left the
    // same amount. We may have to append a high-order
    // digit on the dividend; we do that unconditionally.

    s = nlz (v[n-1]) - 16;      // 0 <= s <= 16.
    //vn = (uint16_t *) alloca (2*n);
    uint16_t vn [n];
    for (i = n - 1; i > 0; i--)
      vn[i] = (uint16_t) (v[i] << s) | (uint16_t) (v[i-1] >> (16-s));
    vn[0] = (uint16_t) (v[0] << s);

    //un = (uint16_t *)alloca(2*(m + 1));
    uint16_t un [m+1];
    un[m] = u[m-1] >> (16-s);
    for (i = m - 1; i > 0; i--)
      un[i] = (uint16_t) (u[i] << s) | (uint16_t) (u[i-1] >> (16-s));
    un[0] = (uint16_t) (u[0] << s);
    for (j = m - n; j >= 0; j--)
      {       // Main loop.
        // Compute estimate qhat of q[j].
        qhat = (un[j+n]*b + un[j+n-1])/vn[n-1];
        rhat = (un[j+n]*b + un[j+n-1])%vn[n-1];
cmp_again:
        if (qhat >= b || (unsigned)qhat*(unsigned long long)vn[n-2] > b*rhat + un[j+n-2])
          {
            qhat = qhat - 1;
            rhat = rhat + vn[n-1];
            if (rhat < b) goto cmp_again;
          }

        // Multiply and subtract.
        k = 0;
        for (i = 0; i < n; i++)
          {
            p = (unsigned)qhat*(unsigned long long)vn[i];
            t = (int32_t) un[i+j] - k - (int32_t) (p & 0xFFFF);
            un[i+j] = (uint16_t) t;
            k = (int) (p >> 16) - (t >> 16);
          }
        t = un[j+n] - k;
        un[j+n] = (uint16_t) t;

        q[j] = (uint16_t) qhat;  // Store quotient digit.
        if (t < 0)
          {                      // If we subtracted too
            q[j] = q[j] - 1;     // much, add back.
            k = 0;
            for (i = 0; i < n; i++)
              {
                t = un[i+j] + vn[i] + k;
                un[i+j] = (uint16_t) t;
                k = t >> 16;
               }
             un[j+n] = (uint16_t) (un[j+n] + k);
          }
      } // End j.
    // If the caller wants the remainder, denormalize
    // it and pass it back.
    if (r != NULL)
      {
        for (i = 0; i < n; i++)
          r[i] = (uint16_t) (un[i] >> s) | (uint16_t) (un[i+1] << (16-s));
      }
    return 0;
  }

uint128 multiply_128 (uint128 a, uint128 b)
  {
//(void)printf ("%016llx%016llx %016llx%016llx\r\n", a.h,a.l,b.h,b.l);
    const int l = 4;
    uint32_t w[l+l], u[l], v[l];

    u[3] = (uint32_t) (a.h >> 32);
    u[2] = (uint32_t)  a.h;
    u[1] = (uint32_t) (a.l >> 32);
    u[0] = (uint32_t)  a.l;
    v[3] = (uint32_t) (b.h >> 32);
    v[2] = (uint32_t)  b.h;
    v[1] = (uint32_t) (b.l >> 32);
    v[0] = (uint32_t)  b.l;
    mulmn (w, u, v, l, l);
    return construct_128 (
       (((uint64_t) w[3]) << 32) | w[2],
       (((uint64_t) w[1]) << 32) | w[0]);
  }

int128 multiply_s128 (int128 a, int128 b)
  {
//(void)printf ("%016llx%016llx %016llx%016llx\r\n", a.h,a.l,b.h,b.l);
    const int l = 4;
    uint32_t w[l+l], u[l], v[l];

    u[3] = (uint32_t) (a.h >> 32);
    u[2] = (uint32_t)  a.h;
    u[1] = (uint32_t) (a.l >> 32);
    u[0] = (uint32_t)  a.l;
    v[3] = (uint32_t) (b.h >> 32);
    v[2] = (uint32_t)  b.h;
    v[1] = (uint32_t) (b.l >> 32);
    v[0] = (uint32_t)  b.l;
    mulmns (w, u, v, l, l);
    return construct_s128 (
       (((int64_t)  w[3]) << 32) | w[2],
       (((uint64_t) w[1]) << 32) | w[0]);
  }

// Note: divisor is < 2^16
uint128 divide_128 (uint128 a, uint128 b, uint128 * remp)
  {
    const int m = 8;
    const int n = 8;
    uint16_t q[m], u[m], v[n];
    u[0] = (uint16_t)  a.l;
    u[1] = (uint16_t) (a.l >> 16);
    u[2] = (uint16_t) (a.l >> 32);
    u[3] = (uint16_t) (a.l >> 48);
    u[4] = (uint16_t)  a.h;
    u[5] = (uint16_t) (a.h >> 16);
    u[6] = (uint16_t) (a.h >> 32);
    u[7] = (uint16_t) (a.h >> 48);

    v[0] = (uint16_t)  b.l;
    v[1] = (uint16_t) (b.l >> 16);
    v[2] = (uint16_t) (b.l >> 32);
    v[3] = (uint16_t) (b.l >> 48);
    v[4] = (uint16_t)  b.h;
    v[5] = (uint16_t) (b.h >> 16);
    v[6] = (uint16_t) (b.h >> 32);
    v[7] = (uint16_t) (b.h >> 48);

    q[0] = q[1] = q[2] = q[3] = q[4] = q[5] = q[6] = q[7] = 0;

    int normlen;
    for (normlen = 8; normlen > 0; normlen --)
      if (v [normlen - 1])
        break;
    uint16_t r [8] = { 8 * 0 };
    divmnu (q, remp ? r : NULL, u, v, m, normlen);
    if (remp)
      {
        * remp =  construct_128 (
       (((uint64_t) r [7]) << 48) |
       (((uint64_t) r [6]) << 32) |
       (((uint64_t) r [5]) << 16) |
       (((uint64_t) r [4]) <<  0),
       (((uint64_t) r [3]) << 48) |
       (((uint64_t) r [2]) << 32) |
       (((uint64_t) r [1]) << 16) |
       (((uint64_t) r [0]) <<  0));
      }
    return construct_128 (
       (((uint64_t) q [7]) << 48) |
       (((uint64_t) q [6]) << 32) |
       (((uint64_t) q [5]) << 16) |
       (((uint64_t) q [4]) <<  0),
       (((uint64_t) q [3]) << 48) |
       (((uint64_t) q [2]) << 32) |
       (((uint64_t) q [1]) << 16) |
       (((uint64_t) q [0]) <<  0));
  }

// Note: divisor is < 2^16
uint128 divide_128_16 (uint128 a, uint16_t b, uint16_t * remp)
  {
    const int m = 8;
    const int n = 1;
    uint16_t q[m], u[m], v[n];
    u[0] = (uint16_t)  a.l;
    u[1] = (uint16_t) (a.l >> 16);
    u[2] = (uint16_t) (a.l >> 32);
    u[3] = (uint16_t) (a.l >> 48);
    u[4] = (uint16_t)  a.h;
    u[5] = (uint16_t) (a.h >> 16);
    u[6] = (uint16_t) (a.h >> 32);
    u[7] = (uint16_t) (a.h >> 48);

    v[0] = (uint16_t) b;

    q[0] = q[1] = q[2] = q[3] = q[4] = q[5] = q[6] = q[7] = 0;

    divmnu (q, remp, u, v, m, n);
    return construct_128 (
       (((uint64_t) q [7]) << 48) |
       (((uint64_t) q [6]) << 32) |
       (((uint64_t) q [5]) << 16) |
       (((uint64_t) q [4]) <<  0),
       (((uint64_t) q [3]) << 48) |
       (((uint64_t) q [2]) << 32) |
       (((uint64_t) q [1]) << 16) |
       (((uint64_t) q [0]) <<  0));
  }

//divide_128_32 (
//00000000000000010000000000000000, 00010000) returned
//00000000000000000001000000000000, 00000000
//divide_128_32 (ffffffffffffffffffffffffffffffff, 00010000) returned f771ffffffffffffffffffffffffffff, 0000ffff

// Note: divisor is < 2^32 and >= 2^16; i.e. high 16 bits *must* be non-zero
uint128 divide_128_32 (uint128 a, uint32_t b, uint32_t * remp)
  {
    const int m = 8;
    const int n = 2;
    uint16_t q[m], u[m], v[n], r[2];
    u[0] = (uint16_t)  a.l;
    u[1] = (uint16_t) (a.l >> 16);
    u[2] = (uint16_t) (a.l >> 32);
    u[3] = (uint16_t) (a.l >> 48);
    u[4] = (uint16_t)  a.h;
    u[5] = (uint16_t) (a.h >> 16);
    u[6] = (uint16_t) (a.h >> 32);
    u[7] = (uint16_t) (a.h >> 48);

    v[0] = (uint16_t) b;
    v[1] = (uint16_t) (b >> 16);

    q[0] = q[1] = q[2] = q[3] = q[4] = q[5] = q[6] = q[7] = 0;

    divmnu (q, remp ? r : NULL, u, v, m, n);
    if (remp)
      * remp = r [0] | (uint32_t) r[1] << 16;

    return construct_128 (
       (((uint64_t) q [7]) << 48) |
       (((uint64_t) q [6]) << 32) |
       (((uint64_t) q [5]) << 16) |
       (((uint64_t) q [4]) <<  0),
       (((uint64_t) q [3]) << 48) |
       (((uint64_t) q [2]) << 32) |
       (((uint64_t) q [1]) << 16) |
       (((uint64_t) q [0]) <<  0));
  }

unsigned ti_test_iter = 0;

static void tisz (uint64_t h, uint64_t l, bool expect)
  {
    bool r = iszero_128 (construct_128 (h, l));
    ti_test_iter++;
    if (r != expect) {
      (void)fprintf (stderr, "FATAL (%u): iszero_128 (%llu, %llu) returned %lu\r\n",
                     ti_test_iter,
                     (unsigned long long)h, (unsigned long long)l, (unsigned long)r);
      kd_div_errors++;
    }
  }

static void tand (uint64_t ah, uint64_t al, uint64_t bh, uint64_t bl,
                  uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 b = construct_128 (bh, bl);
    uint128 r = and_128 (a, b);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): and_128 (%016llx%016llx, %016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al,  (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tor (uint64_t ah, uint64_t al, uint64_t bh, uint64_t bl,
                 uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 b = construct_128 (bh, bl);
    uint128 r = or_128 (a, b);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): or_128 (%016llx%016llx, %016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al, (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tcomp (uint64_t ah, uint64_t al,
                   uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 r = complement_128 (a);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): complement_128 (%016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al, (unsigned long long)r.h,
                     (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tadd (uint64_t ah, uint64_t al, uint64_t bh, uint64_t bl,
                  uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 b = construct_128 (bh, bl);
    uint128 r = add_128 (a, b);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL %u): add_128 (%016llx%016llx, %016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al,  (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tsub (uint64_t ah, uint64_t al, uint64_t bh, uint64_t bl,
                  uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 b = construct_128 (bh, bl);
    uint128 r = subtract_128 (a, b);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): subtract_128 (%016llx%016llx, %016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al,  (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tneg (uint64_t ah, uint64_t al,
                  uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 r = negate_128 (a);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): negate_128 (%016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al, (unsigned long long)r.h,
                     (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tgt (uint64_t ah, uint64_t al, uint64_t bh, uint64_t bl,
                 bool expect)
  {
    uint128 a = construct_128 (ah, al);
    uint128 b = construct_128 (bh, bl);
    ti_test_iter++;
    bool r = isgt_128 (a, b);
    if (r != expect) {
      (void)fprintf (stderr, "FATAL (%u): gt_128 (%016llx%016llx, %016llx%016llx) returned %llu\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al, (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r);
      kd_div_errors++;
    }
  }

static void tls (uint64_t ah, uint64_t al, unsigned int n,
                 uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 r = lshift_128 (a, n);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): lshift_128 (%016llx%016llx, %llu) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah,  (unsigned long long)al, (unsigned long long)n,
                     (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void trs (uint64_t ah, uint64_t al, unsigned int n,
                 uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 r = rshift_128 (a, n);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): rshift_128 (%016llx%016llx, %llu) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah,  (unsigned long long)al, (unsigned long long)n,
                     (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tmul (uint64_t ah, uint64_t al, uint64_t bh, uint64_t bl,
                  uint64_t rh, uint64_t rl)
  {
    uint128 a = construct_128 (ah, al);
    uint128 b = construct_128 (bh, bl);
    uint128 r = multiply_128 (a, b);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): multiply_128 (%016llx%016llx, %016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al,  (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tsmul (int64_t ah, uint64_t al, int64_t bh, uint64_t bl,
                   int64_t rh, uint64_t rl)
  {
    int128 a = construct_s128 (ah, al);
    int128 b = construct_s128 (bh, bl);
    int128 r = multiply_s128 (a, b);
    ti_test_iter++;
    if (r.h != rh || r.l != rl) {
      (void)fprintf (stderr, "FATAL (%u): multiply_s128 (%016llx%016llx, %016llx%016llx) returned %016llx%016llx\r\n",
                     ti_test_iter,
                     (unsigned long long)ah, (unsigned long long)al,  (unsigned long long)bh,
                     (unsigned long long)bl, (unsigned long long)r.h, (unsigned long long)r.l);
      kd_div_errors++;
    }
  }

static void tdiv16 (uint64_t ah, uint64_t al, uint16_t b,
                    uint64_t resh, uint64_t resl,
                    uint16_t remainder)
  {
    uint128 a = construct_128 (ah, al);
    uint16_t rem;
    uint128 res = divide_128_16 (a, b, & rem);
    ti_test_iter++;
    if (res.h != resh || res.l != resl || rem != remainder) {
      (void)fprintf (stderr, "FATAL (%u): divide_128_16 (%016llx%016llx, %04x) returned %016llx%016llx, %04x\r\n",
                     ti_test_iter,
                     (unsigned long long)ah,    (unsigned long long)al, b,
                     (unsigned long long)res.h, (unsigned long long)res.l, rem);
      kd_div_errors++;
    }
  }

static void tdiv32 (uint64_t ah, uint64_t al, uint32_t b,
                    uint64_t resh, uint64_t resl,
                    uint32_t remainder)
  {
    uint128 a = construct_128 (ah, al);
    uint32_t rem;
    uint128 res = divide_128_32 (a, b, & rem);
    ti_test_iter++;
    if (res.h != resh || res.l != resl || rem != remainder) {
      (void)fprintf (stderr, "FATAL (%u): divide_128_32 (%016llx%016llx, %08x) returned %016llx%016llx, %08x\r\n",
                     ti_test_iter,
                     (unsigned long long)ah,    (unsigned long long)al, b,
                     (unsigned long long)res.h, (unsigned long long)res.l, rem);
      kd_div_errors++;
    }
  }

int test_math128 (void)
  {
    tisz   (0,      0,      true);
    tisz   (1,      0,      false);
    tisz   (0,      1,      false);
    tisz   (1,      1,      false);
    tisz   (SIGN64, 0,      false);
    tisz   (0,      SIGN64, false);
    tisz   (SIGN64, SIGN64, false);
    tneg   (0,      0,      0,      0);
    tneg   (MASK64, MASK64, 0,      1);
    tcomp  (MASK64, MASK64, 0,      0);
    tneg   (0,      1,      MASK64, MASK64);
    tcomp  (0,      0,      MASK64, MASK64);
    tcomp  (0,      1,      MASK64, MASK64-1);
    tgt    (0,      0,      0,      0,          false);
    tgt    (0,      0,      0,      1,          false);
    tgt    (0,      1,      0,      0,          true);
    tgt    (MASK64, MASK64, MASK64, MASK64,     false);
    tgt    (0,      0,      MASK64, MASK64,     false);
    tgt    (MASK64, MASK64, 0,      0,          true);
#  if !defined(__PGI) && !defined(__PGLLVM__) && !defined(__NVCOMPILER) && !defined(__NVCOMPILER_LLVM__)
    tls    (0,      0,      0,      0,          0);
    tls    (MASK64, MASK64, 0,      MASK64,     MASK64);
#  endif /* if !defined(__PGI) && !defined(__PGLLVM__) && !defined(__NVCOMPILER) && !defined(__NVCOMPILER_LLVM__) */
    tls    (0,      1,      127,    SIGN64,     0);
    tls    (0,      MASK64, 64,     MASK64,     0);
    tls    (0,      MASK64, 1,      1,          MASK64-1);
    tls    (0,      1,      64,     1,          0);
    tls    (0,      1,      63,     0,          SIGN64);
    tls    (1,      0,      63,     SIGN64,     0);
    trs    (0,      0,      0,      0,          0);
    trs    (SIGN64, 0,      127,    MASK64,     MASK64);
    trs    (MASK64, 0,      64,     MASK64,     MASK64);
    trs    (MASK64, 0,      1,      MASK64,     SIGN64);
    trs    (1,      0,      64,     0,          1);
    trs    (1,      0,      1,      0,          SIGN64);
    trs    (MASK64, MASK64, 0,      MASK64,     MASK64);
    trs    (SIGN64, 0,      63,     MASK64,     0);
    trs    (SIGN64, 0,      64,     MASK64,     SIGN64);
    tand   (0,      0,      0,      0,          0,       0);
    tand   (MASK64, MASK64, 0,      0,          0,       0);
    tand   (0,      0,      MASK64, MASK64,     0,       0);
    tand   (MASK64, MASK64, MASK64, MASK64,     MASK64,  MASK64);
    tor    (0,      0,      0,      0,          0,       0);
    tor    (MASK64, MASK64, 0,      0,          MASK64,  MASK64);
    tor    (0,      0,      MASK64, MASK64,     MASK64,  MASK64);
    tor    (MASK64, MASK64, MASK64, MASK64,     MASK64,  MASK64);
    tadd   (0,      0,      0,      0,          0,       0);
    tadd   (0,      0,      0,      1,          0,       1);
    tadd   (0,      1,      0,      0,          0,       1);
    tadd   (0,      1,      0,      1,          0,       2);
    tadd   (0,      1,      0,      MASK64,     1,       0);
    tadd   (0,      1,      MASK64, MASK64,     0,       0);
    tsub   (0,      0,      0,      0,          0,       0);
    tsub   (0,      1,      0,      1,          0,       0);
    tsub   (MASK64, MASK64, MASK64, MASK64,     0,       0);
    tsub   (MASK64, MASK64, 0,      0,          MASK64,  MASK64);
    tsub   (0,      0,      0,      1,          MASK64,  MASK64);
    tmul   (0,      0,      0,      0,          0,       0);
    tmul   (MASK64, MASK64, 0,      0,          0,       0);
    tmul   (0,      0,      MASK64, MASK64,     0,       0);
    tmul   (0,      1,      0,      1,          0,       1);
    tmul   (0,      1,      0,      10,         0,       10);
    tmul   (0,      10,     0,      10,         0,       100);
    tmul   (0,      100,    0,      10,         0,       1000);
    tmul   (0,      MASK64, 0,      2,          1,       MASK64-1);
    tmul   (MASK64, MASK64, MASK64, MASK64,     0,       1);
    tdiv16 (MASK64,   MASK64, 16,   MASK64>>4,  MASK64,  15);
    tsmul  (0,      1,      MASK64, MASK64,     MASK64,  MASK64);
    tsmul  (MASK64, MASK64, MASK64, MASK64,     0,       1);
    tdiv32 (1,      0,      1<<16,  0,          1ll<<48, 0);
    tdiv16 (0,      1,      1,      0,          1,       0);
    tdiv16 (0,      10,     2,      0,          5,       0);
    tdiv16 (0,      3,      2,      0,          1,       1);
    tdiv32 (MASK64, MASK64, 1<<16,  MASK64>>16, MASK64,  0xffff);
    if (kd_div_errors)
      abort();
    return 0;
  }
# else
#  if defined(NEED_128)
#   include "dps8_math128.h"

void __udivmodti3(UTItype div, UTItype dvd,UTItype *result,UTItype *remain);
UTItype __udivti3(UTItype div, UTItype dvd);
UTItype __umodti3(UTItype div, UTItype dvd);

UTItype __udivti3(UTItype div, UTItype dvd)
{
        UTItype result,remain;

        __udivmodti3(div,dvd,&result,&remain);

        return result;
}

void __udivmodti3(UTItype div, UTItype dvd,UTItype *result,UTItype *remain)
{
        UTItype z1 = dvd;
        UTItype z2 = (UTItype)1;

        *result = (UTItype)0;
        *remain = div;

        if ( z1 == (UTItype)0)
#   if !defined (CPPCHECK)
          1/0;
#   else
          abort();
#   endif /* if !defined(CPPCHECK) */

        while ( z1 < *remain )
          {
            z1 <<= 1 ;
            z2 <<= 1;
          }

        do
          {
            if ( *remain >= z1 )
              {
                *remain -= z1;
                *result += z2;
              }
            z1 >>= 1;
            z2 >>= 1;
          } while ( z2 );
}

TItype __divti3(TItype div, TItype dvd)
{
        int sign=1;

        if (div < (TItype)0)
          {
            sign = -1;
            div = -div;
          }

        if (dvd < (TItype)0)
          {
            sign = -sign;
            dvd = -dvd;
          }

        if (sign > 0)
          return (TItype)__udivti3(div,dvd);
        else
          return -((TItype)__udivti3(div,dvd));
}

TItype __modti3(TItype div, TItype dvd)
{
        int sign=1;

        if (div < (TItype)0)
          {
            sign = -1;
            div = -div;
          }

        if (dvd < (TItype)0)
          {
            sign = -sign;
            dvd = -dvd;
          }

        if (sign > 0)
          return (TItype)__umodti3(div,dvd);
        else
          return ((TItype)0-(TItype)__umodti3(div,dvd));
}

UTItype __umodti3(UTItype div, UTItype dvd)
{
        UTItype result,remain;

        __udivmodti3(div,dvd,&result,&remain);

        return remain;
}

TItype __multi3 (TItype u, TItype v)
{
        TItype result = (TItype)0;
        int sign = 1;

        if (u<0)
          {
            sign = -1;
            u = -u;
          }

        while (u != (TItype)0)
          {
            if ( u&(TItype)1 )
              result += v;
            u>>=1;
            v<<=1;
          }

        if ( sign < 0 )
          return -result;
        else
          return result;
}
#  endif
# endif
#endif
