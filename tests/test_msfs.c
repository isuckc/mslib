#include "../msfs.h"
#include "test.h"

#include <stdlib.h>

static void test_whole_file_text_and_bytes(void) {
  const char *path = "/tmp/msfs_test_whole.txt";
  const char *bin_path = "/tmp/msfs_test_whole.bin";

  msfs_remove(path);
  msfs_remove(bin_path);

  TEST_ASSERT_EQ(msfs_exists(path), 0);
  TEST_ASSERT_EQ(msfs_size(path), (int64_t)-1);

  /* Write & read text */
  TEST_ASSERT_EQ(msfs_write_text(path, "hello"), MSFS_OK);
  TEST_ASSERT_EQ(msfs_exists(path), 1);
  TEST_ASSERT_EQ(msfs_size(path), (int64_t)5);

  TEST_ASSERT_EQ(msfs_append_text(path, ", world!"), MSFS_OK);
  TEST_ASSERT_EQ(msfs_size(path), (int64_t)13);

  size_t len = 0;
  char *text = msfs_read_text(path, &len);
  TEST_ASSERT(text != NULL);
  TEST_ASSERT_EQ(len, (size_t)13);
  TEST_ASSERT_STR_EQ(text, "hello, world!");
  free(text);

  /* read_into */
  char small[6];
  size_t nread = 0;
  TEST_ASSERT_EQ(msfs_read_into(path, small, 5, &nread), MSFS_OK);
  small[nread] = '\0';
  TEST_ASSERT_EQ(nread, (size_t)5);
  TEST_ASSERT_STR_EQ(small, "hello");

  /* Binary bytes with embedded NULs */
  const uint8_t payload[] = {0x00, 0x01, 0x7F, 0x80, 0xFF, 0x00, 0x42};
  TEST_ASSERT_EQ(msfs_write_bytes(bin_path, payload, sizeof(payload)), MSFS_OK);
  TEST_ASSERT_EQ(msfs_append_bytes(bin_path, payload, 2), MSFS_OK);

  size_t bsize = 0;
  uint8_t *bytes = msfs_read_bytes(bin_path, &bsize);
  TEST_ASSERT(bytes != NULL);
  TEST_ASSERT_EQ(bsize, sizeof(payload) + 2);
  TEST_ASSERT_EQ(memcmp(bytes, payload, sizeof(payload)), 0);
  TEST_ASSERT_EQ(bytes[sizeof(payload)], 0x00);
  TEST_ASSERT_EQ(bytes[sizeof(payload) + 1], 0x01);
  free(bytes);

  /* Empty file */
  TEST_ASSERT_EQ(msfs_write_bytes(path, NULL, 0), MSFS_OK);
  TEST_ASSERT_EQ(msfs_size(path), (int64_t)0);
  char *empty = msfs_read_text(path, &len);
  TEST_ASSERT(empty != NULL);
  TEST_ASSERT_EQ(len, (size_t)0);
  TEST_ASSERT_STR_EQ(empty, "");
  free(empty);

  TEST_ASSERT_EQ(msfs_remove(path), MSFS_OK);
  TEST_ASSERT_EQ(msfs_remove(bin_path), MSFS_OK);
}

static void test_file_management(void) {
  const char *a = "/tmp/msfs_test_a.dat";
  const char *b = "/tmp/msfs_test_b.dat";
  const char *c = "/tmp/msfs_test_c.dat";

  msfs_remove(a);
  msfs_remove(b);
  msfs_remove(c);

  /* Touch creates empty file and preserves existing content */
  TEST_ASSERT_EQ(msfs_touch(a), MSFS_OK);
  TEST_ASSERT_EQ(msfs_exists(a), 1);
  TEST_ASSERT_EQ(msfs_size(a), (int64_t)0);

  /* Multi-chunk file (> MSFS_COPY_BUF_SIZE) */
  size_t big_size = MSFS_COPY_BUF_SIZE * 2 + 137;
  uint8_t *big = (uint8_t *)malloc(big_size);
  TEST_ASSERT(big != NULL);
  for (size_t i = 0; i < big_size; i++) {
    big[i] = (uint8_t)(i % 251);
  }
  TEST_ASSERT_EQ(msfs_write_bytes(a, big, big_size), MSFS_OK);
  TEST_ASSERT_EQ(msfs_touch(a), MSFS_OK);
  TEST_ASSERT_EQ(msfs_size(a), (int64_t)big_size);

  /* Copy */
  TEST_ASSERT_EQ(msfs_copy(a, b), MSFS_OK);
  TEST_ASSERT_EQ(msfs_size(b), (int64_t)big_size);

  size_t copy_size = 0;
  uint8_t *copied = msfs_read_bytes(b, &copy_size);
  TEST_ASSERT(copied != NULL);
  TEST_ASSERT_EQ(copy_size, big_size);
  TEST_ASSERT_EQ(memcmp(big, copied, big_size), 0);
  free(copied);
  free(big);

  /* Move */
  TEST_ASSERT_EQ(msfs_move(b, c), MSFS_OK);
  TEST_ASSERT_EQ(msfs_exists(b), 0);
  TEST_ASSERT_EQ(msfs_exists(c), 1);
  TEST_ASSERT_EQ(msfs_size(c), (int64_t)big_size);

  /* Remove */
  TEST_ASSERT_EQ(msfs_remove(a), MSFS_OK);
  TEST_ASSERT_EQ(msfs_remove(c), MSFS_OK);
  TEST_ASSERT_EQ(msfs_exists(a), 0);
  TEST_ASSERT_EQ(msfs_exists(c), 0);
}

static void test_streamed_handle_io(void) {
  const char *path = "/tmp/msfs_test_stream.txt";
  msfs_remove(path);

  msfs_file_t f = {0};
  TEST_ASSERT_EQ(msfs_is_open(&f), 0);
  TEST_ASSERT_EQ(msfs_close(&f), MSFS_OK);

  /* Write lines and raw bytes */
  TEST_ASSERT_EQ(msfs_open(&f, path, MSFS_MODE_WRITE), MSFS_OK);
  TEST_ASSERT_EQ(msfs_is_open(&f), 1);
  TEST_ASSERT_EQ(msfs_write_line(&f, "first line"), MSFS_OK);
  TEST_ASSERT_EQ(msfs_write(&f, "second line\r\n", 13), (size_t)13);
  TEST_ASSERT_EQ(msfs_write(&f, "last line", 9), (size_t)9);
  TEST_ASSERT_EQ(msfs_flush(&f), MSFS_OK);
  TEST_ASSERT_EQ(msfs_close(&f), MSFS_OK);
  TEST_ASSERT_EQ(msfs_is_open(&f), 0);

  /* Read lines (handles \n, \r\n, and final line without newline) */
  TEST_ASSERT_EQ(msfs_open(&f, path, MSFS_MODE_READ), MSFS_OK);
  TEST_ASSERT_EQ(msfs_file_size(&f), (int64_t)33);
  TEST_ASSERT_EQ(msfs_tell(&f), (int64_t)0);

  char line[64];
  size_t len = 0;

  TEST_ASSERT_EQ(msfs_read_line(&f, line, sizeof(line), &len), MSFS_OK);
  TEST_ASSERT_STR_EQ(line, "first line");
  TEST_ASSERT_EQ(len, (size_t)10);

  TEST_ASSERT_EQ(msfs_read_line(&f, line, sizeof(line), &len), MSFS_OK);
  TEST_ASSERT_STR_EQ(line, "second line");
  TEST_ASSERT_EQ(len, (size_t)11);

  TEST_ASSERT_EQ(msfs_read_line(&f, line, sizeof(line), &len), MSFS_OK);
  TEST_ASSERT_STR_EQ(line, "last line");
  TEST_ASSERT_EQ(len, (size_t)9);

  TEST_ASSERT_EQ(msfs_read_line(&f, line, sizeof(line), &len), MSFS_EOF);
  TEST_ASSERT_EQ(msfs_eof(&f), 1);
  TEST_ASSERT_EQ(msfs_has_error(&f), 0);

  /* Seek & tell */
  TEST_ASSERT_EQ(msfs_seek(&f, 6, MSFS_SEEK_SET), MSFS_OK);
  TEST_ASSERT_EQ(msfs_tell(&f), (int64_t)6);
  TEST_ASSERT_EQ(msfs_file_size(&f), (int64_t)33);
  TEST_ASSERT_EQ(msfs_tell(&f), (int64_t)6); /* position preserved */

  char chunk[5];
  TEST_ASSERT_EQ(msfs_read(&f, chunk, 4), (size_t)4);
  chunk[4] = '\0';
  TEST_ASSERT_STR_EQ(chunk, "line");

  TEST_ASSERT_EQ(msfs_close(&f), MSFS_OK);
  TEST_ASSERT_EQ(msfs_remove(path), MSFS_OK);
}

static void test_path_helpers(void) {
  /* basename */
  TEST_ASSERT_STR_EQ(msfs_basename("a/b/c.txt"), "c.txt");
  TEST_ASSERT_STR_EQ(msfs_basename("a\\b\\c.txt"), "c.txt");
  TEST_ASSERT_STR_EQ(msfs_basename("plain.c"), "plain.c");
  TEST_ASSERT_STR_EQ(msfs_basename("/trailing/"), "");
  TEST_ASSERT_STR_EQ(msfs_basename(NULL), "");

  /* extension */
  TEST_ASSERT_STR_EQ(msfs_extension("a/b/file.tar.gz"), ".gz");
  TEST_ASSERT_STR_EQ(msfs_extension("file.txt"), ".txt");
  TEST_ASSERT_STR_EQ(msfs_extension(".gitignore"), "");
  TEST_ASSERT_STR_EQ(msfs_extension("dir.with.dot/noext"), "");
  TEST_ASSERT_STR_EQ(msfs_extension("noext"), "");
  TEST_ASSERT_STR_EQ(msfs_extension(NULL), "");

  /* dirname */
  char buf[64];
  TEST_ASSERT_EQ(msfs_dirname("a/b/c.txt", buf, sizeof(buf)), (size_t)3);
  TEST_ASSERT_STR_EQ(buf, "a/b");
  TEST_ASSERT_EQ(msfs_dirname("a/b/c/", buf, sizeof(buf)), (size_t)3);
  TEST_ASSERT_STR_EQ(buf, "a/b");
  TEST_ASSERT_EQ(msfs_dirname("/file.txt", buf, sizeof(buf)), (size_t)1);
  TEST_ASSERT_STR_EQ(buf, "/");
  TEST_ASSERT_EQ(msfs_dirname("file.txt", buf, sizeof(buf)), (size_t)1);
  TEST_ASSERT_STR_EQ(buf, ".");
  TEST_ASSERT_EQ(msfs_dirname("a/b/c.txt", buf, 2), (size_t)0);

  /* path_join */
  TEST_ASSERT_EQ(msfs_path_join(buf, sizeof(buf), "a/b", "c.txt"), (size_t)9);
  TEST_ASSERT_STR_EQ(buf, "a/b/c.txt");
  TEST_ASSERT_EQ(msfs_path_join(buf, sizeof(buf), "a/b/", "/c.txt"), (size_t)9);
  TEST_ASSERT_STR_EQ(buf, "a/b/c.txt");
  TEST_ASSERT_EQ(msfs_path_join(buf, sizeof(buf), "/", "etc"), (size_t)4);
  TEST_ASSERT_STR_EQ(buf, "/etc");
  TEST_ASSERT_EQ(msfs_path_join(buf, sizeof(buf), "", "rel.txt"), (size_t)7);
  TEST_ASSERT_STR_EQ(buf, "rel.txt");
  TEST_ASSERT_EQ(msfs_path_join(buf, sizeof(buf), "dir", ""), (size_t)3);
  TEST_ASSERT_STR_EQ(buf, "dir");
  TEST_ASSERT_EQ(msfs_path_join(buf, 4, "abc", "def"), (size_t)0);
}

static void test_error_handling(void) {
  TEST_ASSERT(msfs_read_text(NULL, NULL) == NULL);
  TEST_ASSERT(msfs_read_text("", NULL) == NULL);
  TEST_ASSERT(msfs_read_text("/tmp/msfs_does_not_exist_xyz", NULL) == NULL);

  TEST_ASSERT_EQ(msfs_write_text(NULL, "x"), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_write_text("/tmp/x", NULL), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_write_bytes("/tmp/x", NULL, 4), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_read_into(NULL, NULL, 0, NULL), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_copy(NULL, "/tmp/y"), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_move("", "/tmp/y"), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_remove(NULL), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_touch(""), MSFS_ERR_INVALID);

  msfs_file_t f = {0};
  TEST_ASSERT_EQ(msfs_open(NULL, "/tmp/x", MSFS_MODE_READ), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_open(&f, "/tmp/msfs_does_not_exist_xyz", MSFS_MODE_READ),
                 MSFS_ERR_OPEN);
  TEST_ASSERT_EQ(msfs_tell(&f), (int64_t)-1);
  TEST_ASSERT_EQ(msfs_file_size(&f), (int64_t)-1);
  TEST_ASSERT_EQ(msfs_seek(&f, 0, MSFS_SEEK_SET), MSFS_ERR_INVALID);
  TEST_ASSERT_EQ(msfs_flush(&f), MSFS_ERR_INVALID);

  TEST_ASSERT_STR_EQ(msfs_strerror(MSFS_OK), "ok");
  TEST_ASSERT_STR_EQ(msfs_strerror(MSFS_EOF), "end of file");
  TEST_ASSERT_STR_EQ(msfs_strerror(MSFS_ERR_OPEN), "failed to open file");
}

int main(void) {
  TEST_SUITE("msfs.h");
  RUN_TEST(test_whole_file_text_and_bytes);
  RUN_TEST(test_file_management);
  RUN_TEST(test_streamed_handle_io);
  RUN_TEST(test_path_helpers);
  RUN_TEST(test_error_handling);
  TEST_SUMMARY();
  return TEST_EXIT_CODE();
}
