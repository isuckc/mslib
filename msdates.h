/*
 * Author: strommach
 * Date: 22nd November 2026
 * msdates.h - v0.0.1: A single-header C99 library for managing Gregorian dates
 * in compact formats.
 *
 * Usage:
 *   Just #include "msdates.h" in any C99 (or C++) translation unit.
 *   Valid dates span 0001-01-01 through 9999-12-31; invalid dates are
 *   represented by MSDATE_INVALID (0).
 *
 * API:
 *   Calendar helpers : msdate_is_leap_year, msdate_days_in_month,
 *                      msdate_days_in_year
 *   Construction     : msdate_make, msdate_today, msdate_is_valid,
 *                      msdate_year, msdate_month, msdate_day
 *   Conversions      : msdate_to_days, msdate_from_days,
 *                      msdate_to_int, msdate_from_int,
 *                      msdate_to_str, msdate_to_compact_str, msdate_from_str
 *   Queries & math   : msdate_weekday, msdate_day_of_year,
 *                      msdate_add_days, msdate_add_months, msdate_add_years,
 *                      msdate_diff_days, msdate_cmp
 */

#ifndef MSDATES_H
#define MSDATES_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifndef MSD_INLINE
#if defined(__cplusplus)
#define MSD_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSD_INLINE static inline __attribute__((unused))
#else
#define MSD_INLINE static inline
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Packed 32-bit date representation:
 *   bits  0..4  : day   (1..31)
 *   bits  5..8  : month (1..12)
 *   bits  9..31 : year  (1..9999)
 *
 * Chronological order matches integer order (<, <=, >, >=, ==, !=).
 * 0 represents an invalid/empty date (MSDATE_INVALID).
 */
typedef uint32_t msdate_t;

#define MSDATE_INVALID ((msdate_t)0)

#define MSDATE_MIN_YEAR 1
#define MSDATE_MAX_YEAR 9999

/* Epoch day bounds (days since 1970-01-01) for 0001-01-01 .. 9999-12-31 */
#define MSDATE_MIN_DAYS ((int32_t)-719162)
#define MSDATE_MAX_DAYS ((int32_t)2932896)

/* Minimum buffer sizes (including NUL terminator) for string formatting */
#define MSDATE_STR_SIZE 11        /* "YYYY-MM-DD\0" */
#define MSDATE_COMPACT_STR_SIZE 9 /* "YYYYMMDD\0" */

enum {
  MSDATE_SUN = 0,
  MSDATE_MON = 1,
  MSDATE_TUE = 2,
  MSDATE_WED = 3,
  MSDATE_THU = 4,
  MSDATE_FRI = 5,
  MSDATE_SAT = 6
};

/* Internal helpers */

MSD_INLINE int __msdate_parse_digits(const char *s, size_t n, int *out) {
  int val = 0;
  for (size_t i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c < '0' || c > '9') {
      return 0;
    }
    val = val * 10 + (int)(c - '0');
  }
  *out = val;
  return 1;
}

MSD_INLINE void __msdate_write_digits(char *dst, int val, size_t n) {
  for (size_t i = n; i > 0; i--) {
    dst[i - 1] = (char)('0' + (val % 10));
    val /= 10;
  }
}

/* Calendar helpers */

MSD_INLINE int msdate_is_leap_year(int year) {
  if (year < MSDATE_MIN_YEAR || year > MSDATE_MAX_YEAR) {
    return 0;
  }
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

MSD_INLINE int msdate_days_in_month(int year, int month) {
  static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (year < MSDATE_MIN_YEAR || year > MSDATE_MAX_YEAR || month < 1 ||
      month > 12) {
    return 0;
  }
  if (month == 2 && msdate_is_leap_year(year)) {
    return 29;
  }
  return days[month - 1];
}

MSD_INLINE int msdate_days_in_year(int year) {
  if (year < MSDATE_MIN_YEAR || year > MSDATE_MAX_YEAR) {
    return 0;
  }
  return msdate_is_leap_year(year) ? 366 : 365;
}

/* Construction, validation, and field access */

MSD_INLINE int msdate_year(msdate_t d) { return (int)(d >> 9); }

MSD_INLINE int msdate_month(msdate_t d) { return (int)((d >> 5) & 0x0F); }

MSD_INLINE int msdate_day(msdate_t d) { return (int)(d & 0x1F); }

MSD_INLINE int msdate_is_valid(msdate_t d) {
  if (d == MSDATE_INVALID) {
    return 0;
  }
  int y = msdate_year(d);
  int m = msdate_month(d);
  int day = msdate_day(d);
  int dim = msdate_days_in_month(y, m);
  return dim > 0 && day >= 1 && day <= dim;
}

MSD_INLINE msdate_t msdate_make(int year, int month, int day) {
  int dim = msdate_days_in_month(year, month);
  if (dim == 0 || day < 1 || day > dim) {
    return MSDATE_INVALID;
  }
  return ((uint32_t)year << 9) | ((uint32_t)month << 5) | (uint32_t)day;
}

MSD_INLINE msdate_t msdate_today(void) {
  time_t now = time(NULL);
  if (now == (time_t)-1) {
    return MSDATE_INVALID;
  }
  struct tm *lt = localtime(&now);
  if (!lt) {
    return MSDATE_INVALID;
  }
  return msdate_make(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
}

/* Compact format conversions */

/* Days since 1970-01-01 (Howard Hinnant's civil-to-days algorithm) */
MSD_INLINE int32_t msdate_to_days(msdate_t d) {
  if (!msdate_is_valid(d)) {
    return 0;
  }
  int32_t y = msdate_year(d);
  int32_t m = msdate_month(d);
  int32_t day = msdate_day(d);
  y -= (m <= 2) ? 1 : 0;
  int32_t era = y / 400;
  int32_t yoe = y - era * 400;
  int32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

MSD_INLINE msdate_t msdate_from_days(int32_t days) {
  if (days < MSDATE_MIN_DAYS || days > MSDATE_MAX_DAYS) {
    return MSDATE_INVALID;
  }
  int32_t z = days + 719468;
  int32_t era = z / 146097;
  int32_t doe = z - era * 146097;
  int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int32_t y = yoe + era * 400;
  int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  int32_t mp = (5 * doy + 2) / 153;
  int32_t d = doy - (153 * mp + 2) / 5 + 1;
  int32_t m = mp + (mp < 10 ? 3 : -9);
  y += (m <= 2) ? 1 : 0;
  return msdate_make((int)y, (int)m, (int)d);
}

/* Decimal YYYYMMDD integer (e.g., 20261122) */
MSD_INLINE uint32_t msdate_to_int(msdate_t d) {
  if (!msdate_is_valid(d)) {
    return 0;
  }
  return (uint32_t)msdate_year(d) * 10000U + (uint32_t)msdate_month(d) * 100U +
         (uint32_t)msdate_day(d);
}

MSD_INLINE msdate_t msdate_from_int(uint32_t yyyymmdd) {
  if (yyyymmdd < 10101U || yyyymmdd > 99991231U) {
    return MSDATE_INVALID;
  }
  int y = (int)(yyyymmdd / 10000U);
  int m = (int)((yyyymmdd / 100U) % 100U);
  int d = (int)(yyyymmdd % 100U);
  return msdate_make(y, m, d);
}

/* Format as "YYYY-MM-DD" (returns 10 on success, 0 on failure) */
MSD_INLINE int msdate_to_str(msdate_t d, char *buf, size_t buf_size) {
  if (!buf || buf_size < MSDATE_STR_SIZE || !msdate_is_valid(d)) {
    if (buf && buf_size > 0) {
      buf[0] = '\0';
    }
    return 0;
  }
  __msdate_write_digits(buf, msdate_year(d), 4);
  buf[4] = '-';
  __msdate_write_digits(buf + 5, msdate_month(d), 2);
  buf[7] = '-';
  __msdate_write_digits(buf + 8, msdate_day(d), 2);
  buf[10] = '\0';
  return 10;
}

/* Format as "YYYYMMDD" (returns 8 on success, 0 on failure) */
MSD_INLINE int msdate_to_compact_str(msdate_t d, char *buf, size_t buf_size) {
  if (!buf || buf_size < MSDATE_COMPACT_STR_SIZE || !msdate_is_valid(d)) {
    if (buf && buf_size > 0) {
      buf[0] = '\0';
    }
    return 0;
  }
  __msdate_write_digits(buf, msdate_year(d), 4);
  __msdate_write_digits(buf + 4, msdate_month(d), 2);
  __msdate_write_digits(buf + 6, msdate_day(d), 2);
  buf[8] = '\0';
  return 8;
}

/* Parse "YYYY-MM-DD" or "YYYYMMDD" */
MSD_INLINE msdate_t msdate_from_str(const char *str) {
  if (!str) {
    return MSDATE_INVALID;
  }
  int y = 0, m = 0, d = 0;
  size_t len = 0;
  while (str[len] != '\0' && len <= 10) {
    len++;
  }
  if (len == 10 && str[4] == '-' && str[7] == '-') {
    if (!__msdate_parse_digits(str, 4, &y) ||
        !__msdate_parse_digits(str + 5, 2, &m) ||
        !__msdate_parse_digits(str + 8, 2, &d)) {
      return MSDATE_INVALID;
    }
    return msdate_make(y, m, d);
  }
  if (len == 8) {
    if (!__msdate_parse_digits(str, 4, &y) ||
        !__msdate_parse_digits(str + 4, 2, &m) ||
        !__msdate_parse_digits(str + 6, 2, &d)) {
      return MSDATE_INVALID;
    }
    return msdate_make(y, m, d);
  }
  return MSDATE_INVALID;
}

/* Date queries & arithmetic */

MSD_INLINE int msdate_weekday(msdate_t d) {
  if (!msdate_is_valid(d)) {
    return -1;
  }
  int32_t w = (msdate_to_days(d) + 4) % 7;
  if (w < 0) {
    w += 7;
  }
  return (int)w;
}

MSD_INLINE int msdate_day_of_year(msdate_t d) {
  if (!msdate_is_valid(d)) {
    return 0;
  }
  static const int cum_days[12] = {0,   31,  59,  90,  120, 151,
                                   181, 212, 243, 273, 304, 334};
  int y = msdate_year(d);
  int m = msdate_month(d);
  int day = msdate_day(d);
  int doy = cum_days[m - 1] + day;
  if (m > 2 && msdate_is_leap_year(y)) {
    doy += 1;
  }
  return doy;
}

MSD_INLINE msdate_t msdate_add_days(msdate_t d, int32_t days) {
  if (!msdate_is_valid(d)) {
    return MSDATE_INVALID;
  }
  int64_t target = (int64_t)msdate_to_days(d) + (int64_t)days;
  if (target < MSDATE_MIN_DAYS || target > MSDATE_MAX_DAYS) {
    return MSDATE_INVALID;
  }
  return msdate_from_days((int32_t)target);
}

MSD_INLINE msdate_t msdate_add_months(msdate_t d, int32_t months) {
  if (!msdate_is_valid(d)) {
    return MSDATE_INVALID;
  }
  int64_t total = (int64_t)msdate_year(d) * 12 +
                  (int64_t)(msdate_month(d) - 1) + (int64_t)months;
  if (total < (int64_t)MSDATE_MIN_YEAR * 12 ||
      total > (int64_t)MSDATE_MAX_YEAR * 12 + 11) {
    return MSDATE_INVALID;
  }
  int new_year = (int)(total / 12);
  int new_month = (int)(total % 12) + 1;
  int day = msdate_day(d);
  int max_day = msdate_days_in_month(new_year, new_month);
  if (day > max_day) {
    day = max_day;
  }
  return msdate_make(new_year, new_month, day);
}

MSD_INLINE msdate_t msdate_add_years(msdate_t d, int32_t years) {
  if (!msdate_is_valid(d)) {
    return MSDATE_INVALID;
  }
  int64_t new_year = (int64_t)msdate_year(d) + (int64_t)years;
  if (new_year < MSDATE_MIN_YEAR || new_year > MSDATE_MAX_YEAR) {
    return MSDATE_INVALID;
  }
  int m = msdate_month(d);
  int day = msdate_day(d);
  int max_day = msdate_days_in_month((int)new_year, m);
  if (day > max_day) {
    day = max_day;
  }
  return msdate_make((int)new_year, m, day);
}

MSD_INLINE int32_t msdate_diff_days(msdate_t a, msdate_t b) {
  if (!msdate_is_valid(a) || !msdate_is_valid(b)) {
    return 0;
  }
  return msdate_to_days(a) - msdate_to_days(b);
}

MSD_INLINE int msdate_cmp(msdate_t a, msdate_t b) {
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* MSDATES_H */
