/*
 * Copyright (c) 2018 IOTA Stiftung
 * https://github.com/iotaledger/iota_common
 *
 * Refer to the LICENSE file for licensing information
 */

#ifndef __COMMON_TRINARY_PTRIT_H_
#define __COMMON_TRINARY_PTRIT_H_

#include "common/stdint.h"
#include "common/trinary/trits.h"

#if !defined(PTRIT_64) && !defined(PTRIT_SSE2) && !defined(PTRIT_AVX2) && !defined(PTRIT_AVX512) && !defined(PTRIT_NEON)
// Detect PTRIT_PLATFORM
#if defined(__AVX512F__)
#define PTRIT_AVX512
#elif defined(__AVX2__)
#define PTRIT_AVX2
#elif defined(__SSE2__)
#define PTRIT_SSE2
#elif defined(__ARM_NEON__)
#define PTRIT_NEON
#else
#define PTRIT_64
#endif
#endif

#if defined(PTRIT_64)
typedef uint64_t ptrit_s;
#define PTRIT_SIZE 64
#if !defined(PTRIT_CVT_ORN) && !defined(PTRIT_CVT_ANDN)
#define PTRIT_CVT_ANDN
#endif

#elif defined(PTRIT_NEON)
#include <arm_neon.h>
typedef uint64x2_t ptrit_s;
#define PTRIT_SIZE 128
#if defined(PTRIT_CVT_ANDN)
#error ARM NEON curl impl must use PTRIT_CVT_ORN
#endif
#if !defined(PTRIT_CVT_ORN)
// -1 -> (0,0); 0 -> (1,0); +1 -> (1,1); (0,1) -- NaT
#define PTRIT_CVT_ORN
#endif

#else
#if defined(PTRIT_AVX512)
#if !defined(__AVX512F__)
#error __AVX512F__ is not defined
#endif
#include <immintrin.h>
typedef __m512i ptrit_s;
#define PTRIT_SIZE 512

#elif defined(PTRIT_AVX2)
#if !defined(__AVX2__)
#error __AVX2__ is not defined
#endif
#include <immintrin.h>
typedef __m256i ptrit_s;
#define PTRIT_SIZE 256

#elif defined(PTRIT_AVX)
#if !defined(__AVX__)
#error __AVX__ is not defined
#endif
#include <immintrin.h>
typedef __m256d ptrit_s;
#define PTRIT_SIZE 256

#elif defined(PTRIT_SSE2)
#if !defined(__SSE2__)
#if !defined(_MSC_VER)
#error __SSE2__ is not defined
#endif
#endif
#include <immintrin.h>
typedef __m128i ptrit_s;
#define PTRIT_SIZE 128

#elif defined(PTRIT_SSE)
#if !defined(__SSE__)
#error __SSE__ is not defined
#endif
#include <immintrin.h>
typedef __m128d ptrit_s;
#define PTRIT_SIZE 128

#else
#error Invalid PTRIT_PLATFORM.

#endif

#if defined(PTRIT_CVT_ORN)
#error Intel intrinsics curl impl must use PTRIT_CVT_ANDN
#endif
#if !defined(PTRIT_CVT_ANDN)
// -1 -> (1,0); 0 -> (1,1); +1 -> (0,1); (0,0) -- NaT
#define PTRIT_CVT_ANDN
#endif

#endif

typedef struct {
  ptrit_s low;
  ptrit_s high;
} ptrit_t;

#if defined(PTRIT_NEON)
#define ORN(x, y) vornq_u64(y, x)
#define XOR(x, y) veorq_u64(x, y)
#define AND(x, y) vandq_u64(x, y)
#define OR(x, y) vorrq_u64(x, y)
#define NOT(x) ORN(x, XOR(x, x))

#define XORORN(x, y, z) XOR(x, ORN(y, z))
#define ORORN(x, y, z) OR(x, ORN(y, z))

#else
#if defined(PTRIT_AVX512F)
/*
  x y z x^(~y&z)  x^(y&z) (x&y)&z
  0 0 0   0         0         0
  0 0 1   1         0         0
  0 1 0   0         0         0
  0 1 1   0         1         0
  1 0 0   1         1         0
  1 0 1   0         1         0
  1 1 0   1         1         0
  1 1 1   1         0         1
         D2        78         80
*/
#define XORANDN(x, y, z) _mm512_ternarylogic_epi64(x, y, z, 0xD2)
#define XORAND(x, y, z) _mm512_ternarylogic_epi64(x, y, z, 0x78)
#define ANDAND(x, y, z) _mm512_ternarylogic_epi64(x, y, z, 0x80)
#else

#if defined(PTRIT_AVX2)
#define ANDN(x, y) _mm256_andnot_si256(x, y)
#define AND(x, y) _mm256_and_si256(x, y)
#define XOR(x, y) _mm256_xor_si256(x, y)
#define OR(x, y) _mm256_or_si256(x, y)
#define NOT(x) _mm256_andnot_si256(x, _mm256_set_epi64x(-1ll, -1ll, -1ll, -1ll))

#elif defined(PTRIT_AVX)
#define ANDN(x, y) _mm256_andnot_pd(x, y)
#define AND(x, y) _mm256_and_pd(x, y)
#define XOR(x, y) _mm256_xor_pd(x, y)

#elif defined(PTRIT_SSE2)
#if defined(_MSC_VER) && _MSC_VER < 1900 /*&& !defined(_WIN64)*/
static __inline __m128i _mm_set_epi64x(__int64 _I1, __int64 _I0) {
  __m128i i;
  i.m128i_i64[0] = _I0;
  i.m128i_i64[1] = _I1;
  return i;
}
#endif
#define ANDN(x, y) _mm_andnot_si128(x, y)
#define AND(x, y) _mm_and_si128(x, y)
#define XOR(x, y) _mm_xor_si128(x, y)
#define OR(x, y) _mm_or_si128(x, y)
#define NOT(x) _mm_andnot_si128(x, _mm_set_epi64x(-1ll, -1ll))

#elif defined(PTRIT_SSE)
#define ANDN(x, y) _mm_andnot_pd(x, y)
#define AND(x, y) _mm_and_pd(x, y)
#define XOR(x, y) _mm_xor_pd(x, y)

#else
#define AND(x, y) ((x) & (y))
#define XOR(x, y) ((x) ^ (y))
#define OR(x, y) ((x) | (y))
#define NOT(x) (~(x))
#define ANDN(x, y) AND(NOT(x), (y))
#define ORN(x, y) OR(NOT(x), (y))
#endif  // PTRIT_PLATFORM generic

#define XORANDN(x, y, z) XOR(x, ANDN(y, z))
#define ANDAND(x, y, z) AND(x, AND(y, z))
#define ORORN(x, y, z) OR(x, ORN(y, z))
#define XORAND(x, y, z) XOR(x, AND(y, z))
#define XORORN(x, y, z) XOR(x, ORN(y, z))
#endif  // PTRIT_AVX512F
#endif  // PTRIT_NEON

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fill ptrit with a fixed trit value
 *
 * The lowest 2 bits of `t` are used.
 * An invalid value of `t` (i.e. `2`) maps to `NaT`.
 * The representation of `NaT` depends on `PTRIT_CVT`.
 *
 * @param[out] p pointer to the ptrit
 * @param[in] t trit value
 */
void ptrit_fill(ptrit_t *p, trit_t t);

/**
 * @brief Set `idx`-th trit of `*p` to `t`.
 *
 * Precondition: `idx < PTRIT_SIZE`.
 *
 * @param[in,out] p pointer to the ptrit
 * @param[in] idx slice index
 * @param[in] t trit value
 */
void ptrit_set(ptrit_t *p, size_t idx, trit_t t);

/**
 * @brief Return `idx`-th trit of `*p`.
 *
 * Precondition: `idx < PTRIT_SIZE`.
 *
 * @param[in] p pointer to the ptrit
 * @param[in] idx slice index
 * @return trit value
 */
trit_t ptrit_get(ptrit_t const *p, size_t idx);

/**
 * @brief Fill ptrits in `dst` with corresponding trits in `src`
 *
 * @param[in] n number of ptrits in `dst` and trits in `src`
 * @param[out] dst pointer to ptrits
 * @param[in] src pointer to trits
 */
void ptrits_fill(size_t n, ptrit_t *dst, trit_t const *src);

/**
 * @brief Set `idx`-th trits in ptrits in `dst` with corresponding trits in `src`
 *
 * @param[in] n number of ptrits in `dst` and trits in `src`
 * @param[out] dst pointer to ptrits
 * @param[in] idx slice index
 * @param[in] src pointer to trits
 */
void ptrits_set_slice(size_t n, ptrit_t *dst, size_t idx, trit_t const *src);

/**
 * @brief Put `idx`-th trits in ptrits in `src` into corresponding trits in `dst`
 *
 * @param[in] n number of ptrits in `dst` and trits in `src`
 * @param[out] dst pointer to trits
 * @param[in] src pointer to ptrits
 * @param[in] idx slice index
 */
void ptrits_get_slice(size_t n, trit_t *dst, ptrit_t const *src, size_t idx);

/**
 * @brief Find such `idx` that all `idx`-th trits in ptrits in `p` are zero
 *
 * @param[in] n number of ptrits in `p`
 * @param[in] p pointer to ptrits
 * @return idx or `PTRIT_SIZE` if no such index is found
 */
size_t ptrits_find_zero_slice(size_t n, ptrit_t const *p);

/**
 * @brief Find sum of `idx`-th trits in ptrits in `p`
 *
 * @param[in] n number of ptrits in `p`
 * @param[in] p pointer to ptrits
 * @param[in] idx slice index
 * @return sum of trits in `idx`-th slice
 */
long ptrits_sum_slice(size_t n, ptrit_t const *p, size_t idx);

#ifdef __cplusplus
}
#endif

#endif
