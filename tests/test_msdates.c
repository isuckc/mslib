#include "../msdates.h"
#include "test.h"

static void test_make_and_validate(void) {
  msdate_t d = msdate_make(2026, 11, 22);
  TEST_ASSERT(msdate_is_valid(d));
  TEST_ASSERT_EQ(msdate_year(d), 2026);
  TEST_ASSERT_EQ(msdate_month(d), 11);
  TEST_ASSERT_EQ(msdate_day(d), 22);

  /* Leap year checks */
  TEST_ASSERT_EQ(msdate_is_leap_year(2000), 1);
  TEST_ASSERT_EQ(msdate_is_leap_year(1900), 0);
  TEST_ASSERT_EQ(msdate_is_leap_year(2024), 1);
  TEST_ASSERT_EQ(msdate_is_leap_year(2026), 0);

  TEST_ASSERT(msdate_is_valid(msdate_make(2024, 2, 29)));
  TEST_ASSERT(msdate_is_valid(msdate_make(2000, 2, 29)));
  TEST_ASSERT_EQ(msdate_make(1900, 2, 29), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_make(2026, 2, 29), MSDATE_INVALID);

  /* Boundary and invalid dates */
  TEST_ASSERT(msdate_is_valid(msdate_make(1, 1, 1)));
  TEST_ASSERT(msdate_is_valid(msdate_make(9999, 12, 31)));
  TEST_ASSERT_EQ(msdate_make(0, 1, 1), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_make(10000, 1, 1), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_make(2026, 0, 1), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_make(2026, 13, 1), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_make(2026, 4, 31), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_make(2026, 1, 0), MSDATE_INVALID);
  TEST_ASSERT(!msdate_is_valid(MSDATE_INVALID));
}

static void test_calendar_helpers(void) {
  TEST_ASSERT_EQ(msdate_days_in_month(2024, 2), 29);
  TEST_ASSERT_EQ(msdate_days_in_month(2026, 2), 28);
  TEST_ASSERT_EQ(msdate_days_in_month(2026, 4), 30);
  TEST_ASSERT_EQ(msdate_days_in_month(2026, 12), 31);
  TEST_ASSERT_EQ(msdate_days_in_month(2026, 13), 0);

  TEST_ASSERT_EQ(msdate_days_in_year(2024), 366);
  TEST_ASSERT_EQ(msdate_days_in_year(2026), 365);
  TEST_ASSERT_EQ(msdate_days_in_year(0), 0);

  TEST_ASSERT_EQ(msdate_day_of_year(msdate_make(2026, 1, 1)), 1);
  TEST_ASSERT_EQ(msdate_day_of_year(msdate_make(2026, 12, 31)), 365);
  TEST_ASSERT_EQ(msdate_day_of_year(msdate_make(2024, 3, 1)), 61);
  TEST_ASSERT_EQ(msdate_day_of_year(msdate_make(2026, 3, 1)), 60);
  TEST_ASSERT_EQ(msdate_day_of_year(MSDATE_INVALID), 0);

  TEST_ASSERT_EQ(msdate_weekday(msdate_make(1970, 1, 1)), MSDATE_THU);
  TEST_ASSERT_EQ(msdate_weekday(msdate_make(1969, 12, 28)), MSDATE_SUN);
  TEST_ASSERT_EQ(msdate_weekday(msdate_make(2000, 2, 29)), MSDATE_TUE);
  TEST_ASSERT_EQ(msdate_weekday(msdate_make(2026, 11, 22)), MSDATE_SUN);
  TEST_ASSERT_EQ(msdate_weekday(MSDATE_INVALID), -1);
}

static void test_days_and_int_formats(void) {
  msdate_t epoch = msdate_make(1970, 1, 1);
  TEST_ASSERT_EQ(msdate_to_days(epoch), 0);
  TEST_ASSERT_EQ(msdate_from_days(0), epoch);

  msdate_t min_d = msdate_make(1, 1, 1);
  msdate_t max_d = msdate_make(9999, 12, 31);
  TEST_ASSERT_EQ(msdate_to_days(min_d), MSDATE_MIN_DAYS);
  TEST_ASSERT_EQ(msdate_to_days(max_d), MSDATE_MAX_DAYS);
  TEST_ASSERT_EQ(msdate_from_days(MSDATE_MIN_DAYS), min_d);
  TEST_ASSERT_EQ(msdate_from_days(MSDATE_MAX_DAYS), max_d);
  TEST_ASSERT_EQ(msdate_from_days(MSDATE_MIN_DAYS - 1), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_days(MSDATE_MAX_DAYS + 1), MSDATE_INVALID);

  /* Roundtrip every day across 2000-01-01 .. 2005-12-31 */
  int32_t start = msdate_to_days(msdate_make(2000, 1, 1));
  int32_t end = msdate_to_days(msdate_make(2005, 12, 31));
  for (int32_t day = start; day <= end; day++) {
    msdate_t cur = msdate_from_days(day);
    TEST_ASSERT(msdate_is_valid(cur));
    TEST_ASSERT_EQ(msdate_to_days(cur), day);
  }

  /* YYYYMMDD integer format */
  msdate_t d = msdate_make(2026, 11, 22);
  TEST_ASSERT_EQ(msdate_to_int(d), (uint32_t)20261122);
  TEST_ASSERT_EQ(msdate_from_int(20261122), d);
  TEST_ASSERT_EQ(msdate_from_int(10101), msdate_make(1, 1, 1));
  TEST_ASSERT_EQ(msdate_from_int(20260229), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_int(0), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_to_int(MSDATE_INVALID), (uint32_t)0);
}

static void test_string_formats(void) {
  msdate_t d = msdate_make(2026, 3, 5);
  char buf[MSDATE_STR_SIZE];
  char cbuf[MSDATE_COMPACT_STR_SIZE];

  TEST_ASSERT_EQ(msdate_to_str(d, buf, sizeof(buf)), 10);
  TEST_ASSERT_STR_EQ(buf, "2026-03-05");

  TEST_ASSERT_EQ(msdate_to_compact_str(d, cbuf, sizeof(cbuf)), 8);
  TEST_ASSERT_STR_EQ(cbuf, "20260305");

  /* Leading zero padding for small years */
  TEST_ASSERT_EQ(msdate_to_str(msdate_make(42, 1, 9), buf, sizeof(buf)), 10);
  TEST_ASSERT_STR_EQ(buf, "0042-01-09");

  /* Buffer too small / invalid */
  char small[5];
  TEST_ASSERT_EQ(msdate_to_str(d, small, sizeof(small)), 0);
  TEST_ASSERT_STR_EQ(small, "");
  TEST_ASSERT_EQ(msdate_to_compact_str(d, small, sizeof(small)), 0);
  TEST_ASSERT_EQ(msdate_to_str(MSDATE_INVALID, buf, sizeof(buf)), 0);
  TEST_ASSERT_EQ(msdate_to_str(d, NULL, 16), 0);

  /* Parsing */
  TEST_ASSERT_EQ(msdate_from_str("2026-03-05"), d);
  TEST_ASSERT_EQ(msdate_from_str("20260305"), d);
  TEST_ASSERT_EQ(msdate_from_str("0042-01-09"), msdate_make(42, 1, 9));
  TEST_ASSERT_EQ(msdate_from_str(NULL), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_str(""), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_str("2026-03-05x"), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_str("2026/03/05"), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_str("2026-02-29"), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_str("20260229"), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_from_str("2026-0a-05"), MSDATE_INVALID);
}

static void test_arithmetic_and_cmp(void) {
  msdate_t a = msdate_make(2024, 2, 28);
  msdate_t b = msdate_make(2024, 3, 1);

  /* Native packed integer ordering */
  TEST_ASSERT(a < b);
  TEST_ASSERT(b > a);
  TEST_ASSERT_EQ(msdate_cmp(a, b), -1);
  TEST_ASSERT_EQ(msdate_cmp(b, a), 1);
  TEST_ASSERT_EQ(msdate_cmp(a, a), 0);

  /* add_days and diff_days */
  TEST_ASSERT_EQ(msdate_add_days(a, 1), msdate_make(2024, 2, 29));
  TEST_ASSERT_EQ(msdate_add_days(a, 2), b);
  TEST_ASSERT_EQ(msdate_add_days(b, -2), a);
  TEST_ASSERT_EQ(msdate_diff_days(b, a), 2);
  TEST_ASSERT_EQ(msdate_diff_days(a, b), -2);
  TEST_ASSERT_EQ(msdate_add_days(msdate_make(9999, 12, 31), 1), MSDATE_INVALID);
  TEST_ASSERT_EQ(msdate_add_days(msdate_make(1, 1, 1), -1), MSDATE_INVALID);

  /* add_months with end-of-month clamping */
  TEST_ASSERT_EQ(msdate_add_months(msdate_make(2026, 1, 31), 1),
                 msdate_make(2026, 2, 28));
  TEST_ASSERT_EQ(msdate_add_months(msdate_make(2024, 1, 31), 1),
                 msdate_make(2024, 2, 29));
  TEST_ASSERT_EQ(msdate_add_months(msdate_make(2026, 3, 15), -3),
                 msdate_make(2025, 12, 15));
  TEST_ASSERT_EQ(msdate_add_months(msdate_make(1, 1, 15), -1), MSDATE_INVALID);

  /* add_years with leap-day clamping */
  TEST_ASSERT_EQ(msdate_add_years(msdate_make(2024, 2, 29), 1),
                 msdate_make(2025, 2, 28));
  TEST_ASSERT_EQ(msdate_add_years(msdate_make(2024, 2, 29), 4),
                 msdate_make(2028, 2, 29));
  TEST_ASSERT_EQ(msdate_add_years(msdate_make(2024, 2, 29), -1),
                 msdate_make(2023, 2, 28));
  TEST_ASSERT_EQ(msdate_add_years(msdate_make(9999, 1, 1), 1), MSDATE_INVALID);

  /* msdate_today returns a valid date */
  TEST_ASSERT(msdate_is_valid(msdate_today()));
}

int main(void) {
  TEST_SUITE("msdates.h");
  RUN_TEST(test_make_and_validate);
  RUN_TEST(test_calendar_helpers);
  RUN_TEST(test_days_and_int_formats);
  RUN_TEST(test_string_formats);
  RUN_TEST(test_arithmetic_and_cmp);
  TEST_SUMMARY();
}
