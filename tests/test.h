#ifndef MS_TEST_H
#define MS_TEST_H

#include <stdio.h>
#include <string.h>

#ifndef TEST_NO_COLOR
#define TEST_CLR_RESET "\033[0m"
#define TEST_CLR_RED "\033[31m"
#define TEST_CLR_GREEN "\033[32m"
#define TEST_CLR_CYAN "\033[1;36m"
#define TEST_CLR_DIM "\033[2m"
#else
#define TEST_CLR_RESET ""
#define TEST_CLR_RED ""
#define TEST_CLR_GREEN ""
#define TEST_CLR_CYAN ""
#define TEST_CLR_DIM ""
#endif

static int _test_passed = 0;
static int _test_failed = 0;
static int _test_cur_failed = 0;

#define TEST_SUITE(name)                                                       \
  printf(TEST_CLR_CYAN "\n[Suite] %s" TEST_CLR_RESET "\n", (name))

#define TEST_ASSERT(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("    " TEST_CLR_RED "ASSERT FAILED" TEST_CLR_RESET                \
             " (%s:%d): %s\n",                                                 \
             __FILE__, __LINE__, #cond);                                       \
      _test_cur_failed = 1;                                                    \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQ(a, b)                                                   \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      printf("    " TEST_CLR_RED "ASSERT_EQ FAILED" TEST_CLR_RESET             \
             " (%s:%d): %s != %s\n",                                           \
             __FILE__, __LINE__, #a, #b);                                      \
      _test_cur_failed = 1;                                                    \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR_EQ(a, b)                                               \
  do {                                                                         \
    const char *_sa = (a);                                                     \
    const char *_sb = (b);                                                     \
    if (!_sa || !_sb || strcmp(_sa, _sb) != 0) {                               \
      printf("    " TEST_CLR_RED "ASSERT_STR_EQ FAILED" TEST_CLR_RESET         \
             " (%s:%d): \"%s\" != \"%s\"\n",                                   \
             __FILE__, __LINE__, _sa ? _sa : "(null)", _sb ? _sb : "(null)");  \
      _test_cur_failed = 1;                                                    \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define RUN_TEST(fn)                                                           \
  do {                                                                         \
    _test_cur_failed = 0;                                                      \
    fn();                                                                      \
    if (_test_cur_failed) {                                                    \
      _test_failed++;                                                          \
      printf("  " TEST_CLR_RED "[FAIL]" TEST_CLR_RESET " %s\n", #fn);          \
    } else {                                                                   \
      _test_passed++;                                                          \
      printf("  " TEST_CLR_GREEN "[PASS]" TEST_CLR_RESET " %s\n", #fn);        \
    }                                                                          \
  } while (0)

#define TEST_SUMMARY()                                                         \
  do {                                                                         \
    printf(TEST_CLR_DIM "----------------------------------------"             \
                        "\n" TEST_CLR_RESET);                                  \
    if (_test_failed == 0) {                                                   \
      printf(TEST_CLR_GREEN "All %d test(s) passed." TEST_CLR_RESET "\n",      \
             _test_passed);                                                    \
    } else {                                                                   \
      printf(TEST_CLR_GREEN "%d passed" TEST_CLR_RESET ", " TEST_CLR_RED       \
                            "%d failed" TEST_CLR_RESET ".\n",                  \
             _test_passed, _test_failed);                                      \
    }                                                                          \
  } while (0)

#define TEST_EXIT_CODE() (_test_failed > 0 ? 1 : 0)

#endif /* MS_TEST_H */
