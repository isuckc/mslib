/*
 * Author: strommach
 * Date: 20th November 2026
 * msstrings.h - v0.0.2: A single-header C99 dynamic string library using
 * stretchy buffers (NUL-terminated `char *` with a hidden length/cap header).
 *
 * Usage:
 *   Just #include "msstrings.h" in any C99 (or C++) translation unit.
 *   Because mutating operations (copy, concat, push) may reallocate, always
 *   assign their return value back to your string variable:
 *
 *     char *s = msstr_new("hello");
 *     s = msstr_concat_cstr(s, " world");
 *     printf("%s (len=%zu)\n", s, msstr_len(s));
 *     msstr_free(s);
 *
 * API:
 *   Inspection : msstr_len, msstr_cap, msstr_empty
 *   Lifecycle  : msstr_with_cap, msstr_from_len, msstr_new, msstr_dup,
 *                msstr_clear, msstr_destroy, msstr_free
 *   Mutation   : msstr_copy, msstr_copy_len, msstr_copy_cstr,
 *                msstr_concat, msstr_concat_len, msstr_concat_cstr, msstr_push
 *   Comparison : msstr_cmp, msstr_eq, msstr_eq_cstr
 *   Transforms : msstr_substr, msstr_trim, msstr_split
 */

#ifndef MSSTRINGS_H
#define MSSTRINGS_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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

typedef struct {
  size_t len;
  size_t cap;
} strviewhdr;

/* Internal helpers */

MSH_INLINE strviewhdr *__msstr_hdr(const char *str) {
  return (strviewhdr *)((char *)str - sizeof(strviewhdr));
}

MSH_INLINE size_t __msstr_cstr_len(const char *str) {
  if (!str)
    return 0;
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
  }
  return len;
}

MSH_INLINE void __msstr_copy_bytes(char *dst, const char *src, size_t n) {
  for (size_t i = 0; i < n; i++) {
    dst[i] = src[i];
  }
  dst[n] = '\0';
}

MSH_INLINE int __msstr_cmp_bytes(const char *str1, const char *str2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (str1[i] != str2[i]) {
      return (int)(uint8_t)str1[i] - (int)(uint8_t)str2[i];
    }
  }
  return 0;
}

MSH_INLINE char *__msstr_grow(char *str, size_t min_len) {
  size_t needed = min_len + 1; /* +1 for null terminator */

  if (!str) {
    size_t cap = needed < 16 ? 16 : needed;
    strviewhdr *h =
        (strviewhdr *)malloc(sizeof(strviewhdr) + sizeof(char) * cap);
    if (!h)
      return NULL;
    h->len = 0;
    h->cap = cap;
    char *out = (char *)(h + 1);
    out[0] = '\0';
    return out;
  }

  strviewhdr *h = __msstr_hdr(str);
  if (h->cap >= needed) {
    return str;
  }

  size_t new_cap = h->cap > 0 ? h->cap * 2 : 16;
  while (new_cap < needed) {
    if (new_cap > SIZE_MAX / 2) {
      new_cap = needed;
      break;
    }
    new_cap *= 2;
  }

  void *tmp = realloc(h, sizeof(strviewhdr) + sizeof(char) * new_cap);
  if (!tmp) {
    return NULL;
  }

  h = (strviewhdr *)tmp;
  h->cap = new_cap;

  return (char *)(h + 1);
}

/* API */

MSH_INLINE size_t msstr_len(const char *msstr) {
  if (!msstr)
    return 0;
  return __msstr_hdr(msstr)->len;
}

MSH_INLINE size_t msstr_cap(const char *msstr) {
  if (!msstr)
    return 0;
  return __msstr_hdr(msstr)->cap;
}

MSH_INLINE int msstr_empty(const char *msstr) {
  return msstr == NULL || __msstr_hdr(msstr)->len == 0;
}

MSH_INLINE void msstr_clear(char *msstr) {
  if (!msstr)
    return;
  __msstr_hdr(msstr)->len = 0;
  msstr[0] = '\0';
}

MSH_INLINE char *msstr_with_cap(size_t cap) {
  size_t alloc_cap = cap + 1; /* +1 for null terminator */
  strviewhdr *h =
      (strviewhdr *)malloc(sizeof(strviewhdr) + sizeof(char) * alloc_cap);
  if (!h)
    return NULL;

  h->len = 0;
  h->cap = alloc_cap;

  char *msstr = (char *)(h + 1);
  msstr[0] = '\0';
  return msstr;
}

MSH_INLINE char *msstr_from_len(const char *str, size_t len) {
  if (!str)
    return msstr_with_cap(len);

  strviewhdr *h =
      (strviewhdr *)malloc(sizeof(strviewhdr) + sizeof(char) * (len + 1));
  if (!h)
    return NULL;

  h->len = len;
  h->cap = len + 1;

  char *msstr = (char *)(h + 1);
  __msstr_copy_bytes(msstr, str, len);
  return msstr;
}

MSH_INLINE char *msstr_new(const char *str) {
  return msstr_from_len(str, __msstr_cstr_len(str));
}

MSH_INLINE char *msstr_dup(const char *mssrc) {
  if (!mssrc)
    return msstr_with_cap(0);
  return msstr_from_len(mssrc, __msstr_hdr(mssrc)->len);
}

MSH_INLINE char *msstr_copy_len(char *msdst, const char *src, size_t len) {
  if (!src) {
    if (!msdst)
      return msstr_with_cap(0);
    msstr_clear(msdst);
    return msdst;
  }

  char *grown = __msstr_grow(msdst, len);
  if (!grown)
    return NULL;

  __msstr_copy_bytes(grown, src, len);
  __msstr_hdr(grown)->len = len;
  return grown;
}

MSH_INLINE char *msstr_copy(char *msdst, const char *mssrc) {
  if (!mssrc)
    return msstr_copy_len(msdst, NULL, 0);
  return msstr_copy_len(msdst, mssrc, __msstr_hdr(mssrc)->len);
}

MSH_INLINE char *msstr_copy_cstr(char *msdst, const char *cstr) {
  return msstr_copy_len(msdst, cstr, __msstr_cstr_len(cstr));
}

MSH_INLINE char *msstr_concat_len(char *into, const char *src, size_t len) {
  if (!src || len == 0) {
    return into ? into : msstr_with_cap(0);
  }

  size_t into_len = into ? __msstr_hdr(into)->len : 0;
  char *grown = __msstr_grow(into, into_len + len);
  if (!grown)
    return NULL;

  for (size_t i = 0; i < len; i++) {
    grown[into_len + i] = src[i];
  }

  strviewhdr *gh = __msstr_hdr(grown);
  gh->len = into_len + len;
  grown[gh->len] = '\0';
  return grown;
}

MSH_INLINE char *msstr_concat(char *into, const char *that) {
  if (!that)
    return into ? into : msstr_with_cap(0);
  return msstr_concat_len(into, that, __msstr_hdr(that)->len);
}

MSH_INLINE char *msstr_concat_cstr(char *into, const char *cstr) {
  return msstr_concat_len(into, cstr, __msstr_cstr_len(cstr));
}

MSH_INLINE char *msstr_push(char *into, char c) {
  return msstr_concat_len(into, &c, 1);
}

MSH_INLINE int msstr_cmp(const char *str1, const char *str2) {
  if (str1 == str2)
    return 0;
  if (!str1)
    return -1;
  if (!str2)
    return 1;

  size_t l1 = __msstr_hdr(str1)->len;
  size_t l2 = __msstr_hdr(str2)->len;
  size_t min_len = l1 < l2 ? l1 : l2;

  int cmp = __msstr_cmp_bytes(str1, str2, min_len);
  if (cmp != 0)
    return cmp;

  if (l1 < l2)
    return -1;
  if (l1 > l2)
    return 1;
  return 0;
}

MSH_INLINE int msstr_eq(const char *str1, const char *str2) {
  if (str1 == str2)
    return 1;
  if (!str1 || !str2)
    return 0;

  size_t l1 = __msstr_hdr(str1)->len;
  size_t l2 = __msstr_hdr(str2)->len;
  if (l1 != l2)
    return 0;

  return __msstr_cmp_bytes(str1, str2, l1) == 0;
}

MSH_INLINE int msstr_eq_cstr(const char *msstr, const char *cstr) {
  if (!msstr && !cstr)
    return 1;
  if (!msstr || !cstr)
    return 0;

  size_t l1 = __msstr_hdr(msstr)->len;
  size_t l2 = __msstr_cstr_len(cstr);
  if (l1 != l2)
    return 0;

  return __msstr_cmp_bytes(msstr, cstr, l1) == 0;
}

MSH_INLINE char *msstr_substr(const char *msstr, size_t start, size_t len) {
  if (!msstr)
    return msstr_with_cap(0);

  size_t total = __msstr_hdr(msstr)->len;
  if (start >= total)
    return msstr_with_cap(0);

  size_t avail = total - start;
  if (len > avail)
    len = avail;

  return msstr_from_len(msstr + start, len);
}

MSH_INLINE char *msstr_trim(char *msstr) {
  if (!msstr)
    return NULL;

  strviewhdr *h = __msstr_hdr(msstr);
  if (h->len == 0)
    return msstr;

  size_t start = 0;
  while (start < h->len && (msstr[start] == ' ' || msstr[start] == '\t' ||
                            msstr[start] == '\n' || msstr[start] == '\r' ||
                            msstr[start] == '\v' || msstr[start] == '\f')) {
    start++;
  }

  size_t end = h->len;
  while (end > start && (msstr[end - 1] == ' ' || msstr[end - 1] == '\t' ||
                         msstr[end - 1] == '\n' || msstr[end - 1] == '\r' ||
                         msstr[end - 1] == '\v' || msstr[end - 1] == '\f')) {
    end--;
  }

  size_t new_len = end - start;
  if (start > 0 && new_len > 0) {
    for (size_t i = 0; i < new_len; i++) {
      msstr[i] = msstr[start + i];
    }
  }

  h->len = new_len;
  msstr[new_len] = '\0';
  return msstr;
}

MSH_INLINE size_t msstr_split(const char *msstr, char **into,
                              const size_t n_splits, const char *sep) {
  if (!msstr || !into || n_splits == 0 || !sep || sep[0] == '\0')
    return 0;

  strviewhdr *h = __msstr_hdr(msstr);
  if (h->len == 0)
    return 0;

  size_t seplen = __msstr_cstr_len(sep);
  size_t n = 0;
  size_t last_split_pos = 0;

  if (h->len >= seplen) {
    for (size_t i = 0; i <= h->len - seplen; i++) {
      if (n >= n_splits)
        return n;
      if (__msstr_cmp_bytes(msstr + i, sep, seplen) == 0) {
        char *part = msstr_from_len(msstr + last_split_pos, i - last_split_pos);
        if (!part)
          return n;
        into[n++] = part;
        last_split_pos = i + seplen;
        i += seplen - 1;
      }
    }
  }

  if (n < n_splits) {
    char *part =
        msstr_from_len(msstr + last_split_pos, h->len - last_split_pos);
    if (part) {
      into[n++] = part;
    }
  }

  return n;
}

MSH_INLINE void msstr_destroy(char *msstr) {
  if (!msstr)
    return;
  free(__msstr_hdr(msstr));
}

#define msstr_free(s)                                                          \
  do {                                                                         \
    msstr_destroy(s);                                                          \
    (s) = NULL;                                                                \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* MSSTRINGS_H */
