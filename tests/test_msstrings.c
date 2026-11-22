#include "msstrings.h"
#include "test.h"

static void test_creation_and_len(void) {
  char *s = msstr_new("hello");
  TEST_ASSERT(s != NULL);
  TEST_ASSERT_EQ(msstr_len(s), (size_t)5);
  TEST_ASSERT(msstr_cap(s) >= 6);
  TEST_ASSERT(!msstr_empty(s));
  TEST_ASSERT_STR_EQ(s, "hello");

  char *empty = msstr_new(NULL);
  TEST_ASSERT(empty != NULL);
  TEST_ASSERT_EQ(msstr_len(empty), (size_t)0);
  TEST_ASSERT(msstr_empty(empty));
  TEST_ASSERT_STR_EQ(empty, "");

  char *sub = msstr_from_len("hello world", 5);
  TEST_ASSERT(sub != NULL);
  TEST_ASSERT(msstr_eq(s, sub));

  msstr_free(s);
  TEST_ASSERT(s == NULL);
  msstr_destroy(empty);
  msstr_destroy(sub);
  msstr_destroy(NULL);
}

static void test_copy_and_dup(void) {
  char *src = msstr_new("a much longer string than before");
  char *dst = msstr_new("hi");

  dst = msstr_copy(dst, src);
  TEST_ASSERT(dst != NULL);
  TEST_ASSERT(msstr_eq(dst, src));
  TEST_ASSERT_EQ(msstr_len(dst), msstr_len(src));

  char *s2 = msstr_copy(NULL, src);
  TEST_ASSERT(s2 != NULL);
  TEST_ASSERT(msstr_eq(s2, src));
  TEST_ASSERT(s2 != src);

  char *s3 = msstr_dup(src);
  TEST_ASSERT(s3 != NULL);
  TEST_ASSERT(msstr_eq(s3, src));

  s3 = msstr_copy_cstr(s3, "short");
  TEST_ASSERT(msstr_eq_cstr(s3, "short"));
  TEST_ASSERT_EQ(msstr_len(s3), (size_t)5);

  msstr_free(src);
  msstr_free(dst);
  msstr_free(s2);
  msstr_free(s3);
}

static void test_concat_and_push(void) {
  char *s = msstr_new("hello");
  char *w = msstr_new(" world");

  s = msstr_concat(s, w);
  TEST_ASSERT(s != NULL);
  TEST_ASSERT(msstr_eq_cstr(s, "hello world"));
  TEST_ASSERT_EQ(msstr_len(s), (size_t)11);

  s = msstr_concat_cstr(s, "! How are you doing today? Let's grow!");
  TEST_ASSERT(s != NULL);
  TEST_ASSERT_STR_EQ(s, "hello world! How are you doing today? Let's grow!");
  TEST_ASSERT_EQ(msstr_len(s), strlen(s));

  s = msstr_push(s, '?');
  TEST_ASSERT_STR_EQ(s, "hello world! How are you doing today? Let's grow!?");

  char *from_null = msstr_concat_cstr(NULL, "start");
  TEST_ASSERT(msstr_eq_cstr(from_null, "start"));

  msstr_free(s);
  msstr_free(w);
  msstr_free(from_null);
}

static void test_cmp_and_eq(void) {
  char *a = msstr_new("abc");
  char *b = msstr_new("abd");
  char *a2 = msstr_new("abc");
  char *ab = msstr_new("ab");

  TEST_ASSERT_EQ(msstr_eq(a, a2), 1);
  TEST_ASSERT_EQ(msstr_eq(a, b), 0);
  TEST_ASSERT_EQ(msstr_eq(a, ab), 0);
  TEST_ASSERT_EQ(msstr_eq_cstr(a, "abc"), 1);
  TEST_ASSERT_EQ(msstr_eq_cstr(a, "abd"), 0);

  TEST_ASSERT_EQ(msstr_cmp(a, a2), 0);
  TEST_ASSERT(msstr_cmp(a, b) < 0);
  TEST_ASSERT(msstr_cmp(b, a) > 0);
  TEST_ASSERT(msstr_cmp(ab, a) < 0);
  TEST_ASSERT(msstr_cmp(a, ab) > 0);

  msstr_free(a);
  msstr_free(b);
  msstr_free(a2);
  msstr_free(ab);
}

static void test_split(void) {
  char *s = msstr_new("one,two,three");
  char *parts[8] = {0};
  size_t n = msstr_split(s, parts, 8, ",");
  TEST_ASSERT_EQ(n, (size_t)3);
  TEST_ASSERT(msstr_eq_cstr(parts[0], "one"));
  TEST_ASSERT(msstr_eq_cstr(parts[1], "two"));
  TEST_ASSERT(msstr_eq_cstr(parts[2], "three"));
  for (size_t i = 0; i < n; i++)
    msstr_destroy(parts[i]);
  msstr_free(s);

  char *s2 = msstr_new("a::b::");
  char *parts2[8] = {0};
  n = msstr_split(s2, parts2, 8, "::");
  TEST_ASSERT_EQ(n, (size_t)3);
  TEST_ASSERT(msstr_eq_cstr(parts2[0], "a"));
  TEST_ASSERT(msstr_eq_cstr(parts2[1], "b"));
  TEST_ASSERT(msstr_eq_cstr(parts2[2], ""));
  for (size_t i = 0; i < n; i++)
    msstr_destroy(parts2[i]);
  msstr_free(s2);

  char *s3 = msstr_new("a,b,c,d");
  char *parts3[2] = {0};
  n = msstr_split(s3, parts3, 2, ",");
  TEST_ASSERT_EQ(n, (size_t)2);
  TEST_ASSERT(msstr_eq_cstr(parts3[0], "a"));
  TEST_ASSERT(msstr_eq_cstr(parts3[1], "b"));
  for (size_t i = 0; i < n; i++)
    msstr_destroy(parts3[i]);
  msstr_free(s3);
}

static void test_substr_trim_clear(void) {
  char *s = msstr_new("  \t hello world \n ");
  s = msstr_trim(s);
  TEST_ASSERT(msstr_eq_cstr(s, "hello world"));
  TEST_ASSERT_EQ(msstr_len(s), (size_t)11);

  char *sub = msstr_substr(s, 6, 100);
  TEST_ASSERT(msstr_eq_cstr(sub, "world"));

  msstr_clear(s);
  TEST_ASSERT(msstr_empty(s));
  TEST_ASSERT(msstr_cap(s) >= 12);

  msstr_free(s);
  msstr_free(sub);
}

int main(void) {
  TEST_SUITE("msstrings.h");
  RUN_TEST(test_creation_and_len);
  RUN_TEST(test_copy_and_dup);
  RUN_TEST(test_concat_and_push);
  RUN_TEST(test_cmp_and_eq);
  RUN_TEST(test_split);
  RUN_TEST(test_substr_trim_clear);
  TEST_SUMMARY();
  return TEST_EXIT_CODE();
}
