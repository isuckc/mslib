#ifndef MSHASHING_H
#define MSHASHING_H

/*
 * Author: strommach
 * mshashing.h - Minimal, fast, non-cryptographic hashing library in C99.
 *
 * Designed for hash tables, bloom filters, checksums, and general data
 * structures where speed and distribution matter (NOT cryptographic security).
 *
 * Just #include "mshashing.h" in any C99+ or C++11+ source file.
 * Passing data == NULL is treated safely as an empty slice (len == 0).
 *
 * Algorithms provided:
 *   - FNV-1a   (32-bit & 64-bit) : tiny, classic, great for short keys
 *   - DJB2     (64-bit)          : minimal, fast for short byte/ASCII keys
 *   - Murmur3  (32-bit)          : strong 32-bit avalanche & seeded hashing
 *   - xxHash64 (64-bit)          : very fast bulk throughput, classic 64-bit
 * hash
 *   - Wyhash   (64-bit)          : ultra-fast 64-bit hash for short & long keys
 *
 * Utilities provided:
 *   - ms_hash_bytes / ms_hash_cstr : convenient default 64-bit hashes
 *   - ms_hash_u32 / ms_hash_u64    : fast integer mixers (bijections)
 *   - ms_hash_ptr                  : pointer hashing
 *   - ms_hash_combine32 / _64      : combine multiple hashes (structs/tuples)
 *   - ms_hash_with                 : runtime algorithm dispatch via enum
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

#ifndef MSH_INLINE
#if defined(__cplusplus)
#define MSH_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSH_INLINE static inline __attribute__((unused))
#else
#define MSH_INLINE static inline
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

MSH_INLINE uint32_t msh__read32_le(const uint8_t *p) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&             \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  uint32_t v;
  memcpy(&v, p, sizeof(v));
  return v;
#elif defined(_WIN32)
  uint32_t v;
  memcpy(&v, p, sizeof(v));
  return v;
#else
  return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
#endif
}

MSH_INLINE uint64_t msh__read64_le(const uint8_t *p) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&             \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  uint64_t v;
  memcpy(&v, p, sizeof(v));
  return v;
#elif defined(_WIN32)
  uint64_t v;
  memcpy(&v, p, sizeof(v));
  return v;
#else
  return ((uint64_t)p[0]) | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) |
         ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) |
         ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) |
         ((uint64_t)p[7] << 56);
#endif
}

MSH_INLINE uint32_t msh__rotl32(uint32_t x, int r) {
  return (x << r) | (x >> (32 - r));
}

MSH_INLINE uint64_t msh__rotl64(uint64_t x, int r) {
  return (x << r) | (x >> (64 - r));
}

/* -------------------------------------------------------------------------- */
/* Integer mixers & hash combiners                                            */
/* -------------------------------------------------------------------------- */

/* MurmurHash3 32-bit finalizer (bijection on uint32_t). */
MSH_INLINE uint32_t ms_hash_u32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x85ebca6bu;
  x ^= x >> 13;
  x *= 0xc2b2ae35u;
  x ^= x >> 16;
  return x;
}

/* SplitMix64 finalizer (bijection on uint64_t). */
MSH_INLINE uint64_t ms_hash_u64(uint64_t x) {
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

/* Pointer hash (casts to uintptr_t and mixes to 64 bits). */
MSH_INLINE uint64_t ms_hash_ptr(const void *ptr) {
  return ms_hash_u64((uint64_t)(uintptr_t)ptr);
}

/* Combine two 32-bit hashes into one. */
MSH_INLINE uint32_t ms_hash_combine32(uint32_t seed, uint32_t hash) {
  seed ^= hash + 0x9e3779b9u + (seed << 6) + (seed >> 2);
  return seed;
}

/* Combine two 64-bit hashes into one. */
MSH_INLINE uint64_t ms_hash_combine64(uint64_t seed, uint64_t hash) {
  seed ^= hash + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  return seed;
}

/* -------------------------------------------------------------------------- */
/* FNV-1a (32-bit and 64-bit)                                                 */
/* -------------------------------------------------------------------------- */

MSH_INLINE uint32_t ms_hash_fnv1a32_seed(const void *data, size_t len,
                                         uint32_t seed) {
  const uint8_t *p = (const uint8_t *)data;
  uint32_t h = 0x811c9dc5u ^ seed;
  if (!p || len == 0) {
    return h;
  }
  for (size_t i = 0; i < len; ++i) {
    h ^= (uint32_t)p[i];
    h *= 0x01000193u;
  }
  return h;
}

MSH_INLINE uint32_t ms_hash_fnv1a32(const void *data, size_t len) {
  return ms_hash_fnv1a32_seed(data, len, 0);
}

MSH_INLINE uint64_t ms_hash_fnv1a64_seed(const void *data, size_t len,
                                         uint64_t seed) {
  const uint8_t *p = (const uint8_t *)data;
  uint64_t h = 0xcbf29ce484222325ULL ^ seed;
  if (!p || len == 0) {
    return h;
  }
  for (size_t i = 0; i < len; ++i) {
    h ^= (uint64_t)p[i];
    h *= 0x00000100000001b3ULL;
  }
  return h;
}

MSH_INLINE uint64_t ms_hash_fnv1a64(const void *data, size_t len) {
  return ms_hash_fnv1a64_seed(data, len, 0);
}

/* -------------------------------------------------------------------------- */
/* DJB2 (64-bit)                                                              */
/* -------------------------------------------------------------------------- */

MSH_INLINE uint64_t ms_hash_djb2_seed(const void *data, size_t len,
                                      uint64_t seed) {
  const uint8_t *p = (const uint8_t *)data;
  uint64_t h = 5381ULL ^ seed;
  if (!p || len == 0) {
    return h;
  }
  for (size_t i = 0; i < len; ++i) {
    h = ((h << 5) + h) + (uint64_t)p[i];
  }
  return h;
}

MSH_INLINE uint64_t ms_hash_djb2(const void *data, size_t len) {
  return ms_hash_djb2_seed(data, len, 0);
}

/* -------------------------------------------------------------------------- */
/* MurmurHash3 (x86 32-bit)                                                   */
/* -------------------------------------------------------------------------- */

MSH_INLINE uint32_t ms_hash_murmur3_32(const void *data, size_t len,
                                       uint32_t seed) {
  const uint8_t *bytes = (const uint8_t *)data;
  const uint32_t c1 = 0xcc9e2d51u;
  const uint32_t c2 = 0x1b873593u;
  uint32_t h1 = seed;

  if (bytes && len > 0) {
    const size_t nblocks = len / 4;
    const uint8_t *blocks = bytes;

    for (size_t i = 0; i < nblocks; ++i) {
      uint32_t k1 = msh__read32_le(blocks + i * 4);
      k1 *= c1;
      k1 = msh__rotl32(k1, 15);
      k1 *= c2;

      h1 ^= k1;
      h1 = msh__rotl32(h1, 13);
      h1 = h1 * 5u + 0xe6546b64u;
    }

    const uint8_t *tail = bytes + nblocks * 4;
    uint32_t k1 = 0;
    switch (len & 3u) {
    case 3:
      k1 ^= (uint32_t)tail[2] << 16; /* fall through */
    case 2:
      k1 ^= (uint32_t)tail[1] << 8; /* fall through */
    case 1:
      k1 ^= (uint32_t)tail[0];
      k1 *= c1;
      k1 = msh__rotl32(k1, 15);
      k1 *= c2;
      h1 ^= k1;
      break;
    default:
      break;
    }
  }

  h1 ^= (uint32_t)len;
  return ms_hash_u32(h1);
}

/* -------------------------------------------------------------------------- */
/* xxHash64 (XXH64)                                                           */
/* -------------------------------------------------------------------------- */

MSH_INLINE uint64_t msh__xxh64_round(uint64_t acc, uint64_t input) {
  acc += input * 0xC2B2AE3D27D4EB4FULL;
  acc = msh__rotl64(acc, 31);
  acc *= 0x9E3779B185EBCA87ULL;
  return acc;
}

MSH_INLINE uint64_t msh__xxh64_merge_round(uint64_t acc, uint64_t val) {
  val = msh__xxh64_round(0, val);
  acc ^= val;
  acc = acc * 0x9E3779B185EBCA87ULL + 0x85EBCA77C2B2AE63ULL;
  return acc;
}

MSH_INLINE uint64_t ms_hash_xxh64(const void *data, size_t len, uint64_t seed) {
  const uint64_t p1 = 0x9E3779B185EBCA87ULL;
  const uint64_t p2 = 0xC2B2AE3D27D4EB4FULL;
  const uint64_t p3 = 0x165667B19E3779F9ULL;
  const uint64_t p4 = 0x85EBCA77C2B2AE63ULL;
  const uint64_t p5 = 0x27D4EB2F165667C5ULL;

  const uint8_t *p = (const uint8_t *)data;
  uint64_t h64;

  if (!p) {
    len = 0;
  }

  if (len >= 32) {
    const uint8_t *const end = p + len;
    const uint8_t *const limit = end - 32;
    uint64_t v1 = seed + p1 + p2;
    uint64_t v2 = seed + p2;
    uint64_t v3 = seed;
    uint64_t v4 = seed - p1;

    do {
      v1 = msh__xxh64_round(v1, msh__read64_le(p));
      p += 8;
      v2 = msh__xxh64_round(v2, msh__read64_le(p));
      p += 8;
      v3 = msh__xxh64_round(v3, msh__read64_le(p));
      p += 8;
      v4 = msh__xxh64_round(v4, msh__read64_le(p));
      p += 8;
    } while (p <= limit);

    h64 = msh__rotl64(v1, 1) + msh__rotl64(v2, 7) + msh__rotl64(v3, 12) +
          msh__rotl64(v4, 18);
    h64 = msh__xxh64_merge_round(h64, v1);
    h64 = msh__xxh64_merge_round(h64, v2);
    h64 = msh__xxh64_merge_round(h64, v3);
    h64 = msh__xxh64_merge_round(h64, v4);
  } else {
    h64 = seed + p5;
  }

  h64 += (uint64_t)len;

  if (p) {
    const uint8_t *const end = ((const uint8_t *)data) + len;
    while (p + 8 <= end) {
      uint64_t k1 = msh__xxh64_round(0, msh__read64_le(p));
      h64 ^= k1;
      h64 = msh__rotl64(h64, 27) * p1 + p4;
      p += 8;
    }
    if (p + 4 <= end) {
      h64 ^= (uint64_t)msh__read32_le(p) * p1;
      h64 = msh__rotl64(h64, 23) * p2 + p3;
      p += 4;
    }
    while (p < end) {
      h64 ^= (uint64_t)(*p) * p5;
      h64 = msh__rotl64(h64, 11) * p1;
      p++;
    }
  }

  h64 ^= h64 >> 33;
  h64 *= p2;
  h64 ^= h64 >> 29;
  h64 *= p3;
  h64 ^= h64 >> 32;
  return h64;
}

/* -------------------------------------------------------------------------- */
/* Wyhash (final4, 64-bit)                                                    */
/* -------------------------------------------------------------------------- */

MSH_INLINE void msh__wymum(uint64_t *a, uint64_t *b) {
#if defined(__SIZEOF_INT128__)
  __extension__ __uint128_t r =
      (__extension__(__uint128_t)(*a)) * (__extension__(__uint128_t)(*b));
  *a = (uint64_t)r;
  *b = (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
  *a = _umul128(*a, *b, b);
#else
  uint64_t ha = *a >> 32, hb = *b >> 32;
  uint64_t la = (uint32_t)*a, lb = (uint32_t)*b;
  uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb;
  uint64_t t = rl + (rm0 << 32);
  uint64_t c = t < rl ? 1ULL : 0ULL;
  uint64_t lo = t + (rm1 << 32);
  c += lo < t ? 1ULL : 0ULL;
  uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
  *a = lo;
  *b = hi;
#endif
}

MSH_INLINE uint64_t msh__wymix(uint64_t a, uint64_t b) {
  msh__wymum(&a, &b);
  return a ^ b;
}

MSH_INLINE uint64_t msh__wyr3(const uint8_t *p, size_t k) {
  return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

MSH_INLINE uint64_t ms_hash_wyhash(const void *data, size_t len,
                                   uint64_t seed) {
  static const uint64_t secret[4] = {
      0xa0761d6478bd642fULL, 0xe7037ed1a0b428dbULL, 0x8ebc6af09c88c6e3ULL,
      0x589965cc75374cc3ULL};

  const uint8_t *p = (const uint8_t *)data;
  if (!p) {
    len = 0;
  }

  seed ^= msh__wymix(seed ^ secret[0], secret[1]);
  uint64_t a, b;

  if (len <= 16) {
    if (len >= 4) {
      a = ((uint64_t)msh__read32_le(p) << 32) |
          msh__read32_le(p + ((len >> 3) << 2));
      b = ((uint64_t)msh__read32_le(p + len - 4) << 32) |
          msh__read32_le(p + len - 4 - ((len >> 3) << 2));
    } else if (len > 0) {
      a = msh__wyr3(p, len);
      b = 0;
    } else {
      a = 0;
      b = 0;
    }
  } else {
    size_t i = len;
    if (i > 48) {
      uint64_t see1 = seed;
      uint64_t see2 = seed;
      do {
        seed = msh__wymix(msh__read64_le(p) ^ secret[1],
                          msh__read64_le(p + 8) ^ seed);
        see1 = msh__wymix(msh__read64_le(p + 16) ^ secret[2],
                          msh__read64_le(p + 24) ^ see1);
        see2 = msh__wymix(msh__read64_le(p + 32) ^ secret[3],
                          msh__read64_le(p + 40) ^ see2);
        p += 48;
        i -= 48;
      } while (i > 48);
      seed ^= see1 ^ see2;
    }
    while (i > 16) {
      seed = msh__wymix(msh__read64_le(p) ^ secret[1],
                        msh__read64_le(p + 8) ^ seed);
      i -= 16;
      p += 16;
    }
    a = msh__read64_le(p + i - 16);
    b = msh__read64_le(p + i - 8);
  }

  a ^= secret[1];
  b ^= seed;
  msh__wymum(&a, &b);
  return msh__wymix(a ^ secret[0] ^ (uint64_t)len, b ^ secret[1]);
}

/* -------------------------------------------------------------------------- */
/* Convenience defaults & runtime dispatch                                    */
/* -------------------------------------------------------------------------- */

/* Default fast 64-bit hash for arbitrary byte slices (uses Wyhash, seed 0). */
MSH_INLINE uint64_t ms_hash_bytes(const void *data, size_t len) {
  return ms_hash_wyhash(data, len, 0);
}

/* Default fast 64-bit hash for null-terminated C strings. */
MSH_INLINE uint64_t ms_hash_cstr(const char *str) {
  if (!str) {
    return ms_hash_wyhash(NULL, 0, 0);
  }
  return ms_hash_wyhash(str, strlen(str), 0);
}

typedef enum {
  MS_HASH_WYHASH = 0,
  MS_HASH_XXH64,
  MS_HASH_MURMUR3_32,
  MS_HASH_FNV1A_64,
  MS_HASH_FNV1A_32,
  MS_HASH_DJB2
} ms_hash_alg_t;

/* Hash a buffer using an algorithm chosen at runtime. */
MSH_INLINE uint64_t ms_hash_with(ms_hash_alg_t alg, const void *data,
                                 size_t len, uint64_t seed) {
  switch (alg) {
  case MS_HASH_WYHASH:
    return ms_hash_wyhash(data, len, seed);
  case MS_HASH_XXH64:
    return ms_hash_xxh64(data, len, seed);
  case MS_HASH_MURMUR3_32:
    return (uint64_t)ms_hash_murmur3_32(data, len, (uint32_t)seed);
  case MS_HASH_FNV1A_64:
    return ms_hash_fnv1a64_seed(data, len, seed);
  case MS_HASH_FNV1A_32:
    return (uint64_t)ms_hash_fnv1a32_seed(data, len, (uint32_t)seed);
  case MS_HASH_DJB2:
    return ms_hash_djb2_seed(data, len, seed);
  default:
    return ms_hash_wyhash(data, len, seed);
  }
}

#ifdef __cplusplus
}
#endif

#endif /* MSHASHING_H */
