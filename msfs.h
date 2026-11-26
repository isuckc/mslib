/*
 * Author: neurd
 * Date: 26th November 2026
 * msfs.h - v0.0.1: A lightweight, portable single-header C99 library for
 * file handling and I/O operations.
 *
 * Usage:
 *   Just #include "msfs.h" in any C99 (or C++) translation unit.
 *   No separate compilation or implementation macro is required.
 *
 * Overview:
 *   1. Whole-file helpers:
 *        msfs_read_text   - read entire file as NUL-terminated heap string
 *        msfs_read_bytes  - read entire file as heap-allocated byte buffer
 *        msfs_read_into   - read file into a caller-provided buffer
 *        msfs_write_text  - write/overwrite file with a C string
 *        msfs_write_bytes - write/overwrite file with raw bytes
 *        msfs_append_text - append a C string to a file
 *        msfs_append_bytes- append raw bytes to a file
 *
 *   2. File queries & management:
 *        msfs_exists      - check if a file exists and is readable
 *        msfs_size        - get file size in bytes
 *        msfs_copy        - copy a file in fixed-size chunks
 *        msfs_move        - rename/move a file
 *        msfs_remove      - delete a file
 *        msfs_touch       - create an empty file if it does not exist
 *
 *   3. Streamed file handle API (msfs_file_t):
 *        msfs_open        - open a file with msfs_mode_t
 *        msfs_close       - close an open file handle
 *        msfs_is_open     - check whether handle is open
 *        msfs_read        - read bytes from handle
 *        msfs_write       - write bytes to handle
 *        msfs_read_line   - read a single line (strips trailing \r\n or \n)
 *        msfs_write_line  - write a string followed by \n
 *        msfs_seek        - seek to offset
 *        msfs_tell        - get current byte offset
 *        msfs_file_size   - get size of open file (preserves offset)
 *        msfs_flush       - flush buffered output
 *        msfs_eof         - check end-of-file indicator
 *        msfs_has_error   - check stream error indicator
 *
 *   4. Portable path helpers (string-only, handles '/' and '\\'):
 *        msfs_basename    - filename component of a path
 *        msfs_extension   - file extension including '.' (or "")
 *        msfs_dirname     - directory component of a path
 *        msfs_path_join   - join two path segments with '/'
 */

#ifndef MSFS_H
#define MSFS_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MSF_INLINE
#if defined(__cplusplus)
#define MSF_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSF_INLINE static inline __attribute__((unused))
#else
#define MSF_INLINE static inline
#endif
#endif

#ifndef MSFS_COPY_BUF_SIZE
#define MSFS_COPY_BUF_SIZE 4096
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Status / error codes */
typedef enum {
  MSFS_OK = 0,
  MSFS_EOF = 1,
  MSFS_ERR_INVALID = -1,
  MSFS_ERR_OPEN = -2,
  MSFS_ERR_READ = -3,
  MSFS_ERR_WRITE = -4,
  MSFS_ERR_SEEK = -5,
  MSFS_ERR_ALLOC = -6,
  MSFS_ERR_IO = -7
} msfs_err_t;

/* File open modes (always opened in binary mode for portable byte counts) */
typedef enum {
  MSFS_MODE_READ = 0,   /* "rb"  - read existing file */
  MSFS_MODE_WRITE,      /* "wb"  - create/truncate for writing */
  MSFS_MODE_APPEND,     /* "ab"  - create/append for writing */
  MSFS_MODE_READ_WRITE, /* "r+b" - read and write existing file */
  MSFS_MODE_WRITE_READ, /* "w+b" - create/truncate for read and write */
  MSFS_MODE_APPEND_READ /* "a+b" - create/append for read and write */
} msfs_mode_t;

/* Seek origin */
typedef enum {
  MSFS_SEEK_SET = SEEK_SET,
  MSFS_SEEK_CUR = SEEK_CUR,
  MSFS_SEEK_END = SEEK_END
} msfs_seek_t;

/* Lightweight file handle */
typedef struct {
  FILE *fp;
  msfs_mode_t mode;
} msfs_file_t;

/* Error description */

MSF_INLINE const char *msfs_strerror(msfs_err_t err) {
  switch (err) {
  case MSFS_OK:
    return "ok";
  case MSFS_EOF:
    return "end of file";
  case MSFS_ERR_INVALID:
    return "invalid argument";
  case MSFS_ERR_OPEN:
    return "failed to open file";
  case MSFS_ERR_READ:
    return "read error";
  case MSFS_ERR_WRITE:
    return "write error";
  case MSFS_ERR_SEEK:
    return "seek error";
  case MSFS_ERR_ALLOC:
    return "memory allocation failed";
  case MSFS_ERR_IO:
    return "i/o error";
  default:
    return "unknown error";
  }
}

/* Internal helpers */

MSF_INLINE int __msfs_is_sep(char c) { return c == '/' || c == '\\'; }

MSF_INLINE const char *__msfs_mode_str(msfs_mode_t mode) {
  switch (mode) {
  case MSFS_MODE_READ:
    return "rb";
  case MSFS_MODE_WRITE:
    return "wb";
  case MSFS_MODE_APPEND:
    return "ab";
  case MSFS_MODE_READ_WRITE:
    return "r+b";
  case MSFS_MODE_WRITE_READ:
    return "w+b";
  case MSFS_MODE_APPEND_READ:
    return "a+b";
  default:
    return NULL;
  }
}

MSF_INLINE uint8_t *__msfs_read_all(const char *path, size_t *out_size) {
  if (out_size) {
    *out_size = 0;
  }
  if (!path || path[0] == '\0') {
    return NULL;
  }

  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return NULL;
  }

  size_t cap = 4096;
  if (fseek(fp, 0, SEEK_END) == 0) {
    long end_pos = ftell(fp);
    if (end_pos >= 0 && fseek(fp, 0, SEEK_SET) == 0) {
      cap = (size_t)end_pos;
    } else {
      rewind(fp);
    }
  }

  if (cap == SIZE_MAX) {
    fclose(fp);
    return NULL;
  }

  uint8_t *buf = (uint8_t *)malloc(cap + 1);
  if (!buf) {
    fclose(fp);
    return NULL;
  }

  size_t total = 0;
  for (;;) {
    if (total < cap) {
      size_t n = fread(buf + total, 1, cap - total, fp);
      total += n;
      if (n == 0) {
        if (ferror(fp)) {
          free(buf);
          fclose(fp);
          return NULL;
        }
        break;
      }
    } else {
      int c = fgetc(fp);
      if (c == EOF) {
        if (ferror(fp)) {
          free(buf);
          fclose(fp);
          return NULL;
        }
        break;
      }

      size_t new_cap =
          cap == 0 ? 4096 : (cap > (SIZE_MAX - 1) / 2 ? SIZE_MAX - 1 : cap * 2);
      if (new_cap <= cap) {
        free(buf);
        fclose(fp);
        return NULL;
      }

      uint8_t *tmp = (uint8_t *)realloc(buf, new_cap + 1);
      if (!tmp) {
        free(buf);
        fclose(fp);
        return NULL;
      }
      buf = tmp;
      cap = new_cap;
      buf[total++] = (uint8_t)c;
    }
  }

  fclose(fp);
  buf[total] = '\0';
  if (out_size) {
    *out_size = total;
  }
  return buf;
}

MSF_INLINE msfs_err_t __msfs_write_all(const char *path, const char *mode,
                                       const void *data, size_t size) {
  if (!path || path[0] == '\0' || (!data && size > 0)) {
    return MSFS_ERR_INVALID;
  }

  FILE *fp = fopen(path, mode);
  if (!fp) {
    return MSFS_ERR_OPEN;
  }

  if (size > 0 && fwrite(data, 1, size, fp) != size) {
    fclose(fp);
    return MSFS_ERR_WRITE;
  }

  if (fclose(fp) != 0) {
    return MSFS_ERR_IO;
  }

  return MSFS_OK;
}

/* ==========================================================================
 * 1. Whole-file helpers
 * ========================================================================== */

/*
 * Reads an entire file into a heap-allocated byte buffer (NUL-terminated for
 * convenience). Caller must free() the returned pointer. Returns NULL on error.
 * For empty files, returns a valid 1-byte buffer with *out_size = 0.
 */
MSF_INLINE uint8_t *msfs_read_bytes(const char *path, size_t *out_size) {
  return __msfs_read_all(path, out_size);
}

/*
 * Reads an entire file as a NUL-terminated heap-allocated C string.
 * Caller must free() the returned pointer. out_len may be NULL.
 */
MSF_INLINE char *msfs_read_text(const char *path, size_t *out_len) {
  return (char *)__msfs_read_all(path, out_len);
}

/*
 * Reads up to buf_size bytes from path into a caller-provided buffer.
 * Sets *out_read (if non-NULL) to the number of bytes actually read.
 */
MSF_INLINE msfs_err_t msfs_read_into(const char *path, void *buf,
                                     size_t buf_size, size_t *out_read) {
  if (out_read) {
    *out_read = 0;
  }
  if (!path || path[0] == '\0' || (!buf && buf_size > 0)) {
    return MSFS_ERR_INVALID;
  }

  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return MSFS_ERR_OPEN;
  }

  size_t n = buf_size > 0 ? fread(buf, 1, buf_size, fp) : 0;
  int had_err = ferror(fp);
  fclose(fp);

  if (had_err) {
    return MSFS_ERR_READ;
  }

  if (out_read) {
    *out_read = n;
  }
  return MSFS_OK;
}

/* Writes raw bytes to path, creating or overwriting the file. */
MSF_INLINE msfs_err_t msfs_write_bytes(const char *path, const void *data,
                                       size_t size) {
  return __msfs_write_all(path, "wb", data, size);
}

/* Writes a NUL-terminated string to path, creating or overwriting the file. */
MSF_INLINE msfs_err_t msfs_write_text(const char *path, const char *text) {
  if (!text) {
    return MSFS_ERR_INVALID;
  }
  return __msfs_write_all(path, "wb", text, strlen(text));
}

/* Appends raw bytes to path, creating the file if it does not exist. */
MSF_INLINE msfs_err_t msfs_append_bytes(const char *path, const void *data,
                                        size_t size) {
  return __msfs_write_all(path, "ab", data, size);
}

/* Appends a NUL-terminated string to path, creating it if needed. */
MSF_INLINE msfs_err_t msfs_append_text(const char *path, const char *text) {
  if (!text) {
    return MSFS_ERR_INVALID;
  }
  return __msfs_write_all(path, "ab", text, strlen(text));
}

/* ==========================================================================
 * 2. File queries & management
 * ========================================================================== */

/* Returns 1 if path exists and can be opened for reading, 0 otherwise. */
MSF_INLINE int msfs_exists(const char *path) {
  if (!path || path[0] == '\0') {
    return 0;
  }
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return 0;
  }
  fclose(fp);
  return 1;
}

/* Returns the size of the file at path in bytes, or -1 on error. */
MSF_INLINE int64_t msfs_size(const char *path) {
  if (!path || path[0] == '\0') {
    return -1;
  }
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return -1;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return -1;
  }
  long sz = ftell(fp);
  fclose(fp);
  return sz < 0 ? -1 : (int64_t)sz;
}

/* Copies src_path to dst_path (overwriting dst_path if it exists). */
MSF_INLINE msfs_err_t msfs_copy(const char *src_path, const char *dst_path) {
  if (!src_path || src_path[0] == '\0' || !dst_path || dst_path[0] == '\0') {
    return MSFS_ERR_INVALID;
  }

  FILE *src = fopen(src_path, "rb");
  if (!src) {
    return MSFS_ERR_OPEN;
  }

  FILE *dst = fopen(dst_path, "wb");
  if (!dst) {
    fclose(src);
    return MSFS_ERR_OPEN;
  }

  uint8_t buf[MSFS_COPY_BUF_SIZE];
  msfs_err_t err = MSFS_OK;

  for (;;) {
    size_t n = fread(buf, 1, sizeof(buf), src);
    if (n > 0) {
      if (fwrite(buf, 1, n, dst) != n) {
        err = MSFS_ERR_WRITE;
        break;
      }
    }
    if (n < sizeof(buf)) {
      if (ferror(src)) {
        err = MSFS_ERR_READ;
      }
      break;
    }
  }

  int src_close = fclose(src);
  int dst_close = fclose(dst);
  if (err == MSFS_OK && (src_close != 0 || dst_close != 0)) {
    err = MSFS_ERR_IO;
  }
  return err;
}

/* Renames or moves a file from src_path to dst_path. */
MSF_INLINE msfs_err_t msfs_move(const char *src_path, const char *dst_path) {
  if (!src_path || src_path[0] == '\0' || !dst_path || dst_path[0] == '\0') {
    return MSFS_ERR_INVALID;
  }
  return rename(src_path, dst_path) == 0 ? MSFS_OK : MSFS_ERR_IO;
}

/* Deletes the file at path. */
MSF_INLINE msfs_err_t msfs_remove(const char *path) {
  if (!path || path[0] == '\0') {
    return MSFS_ERR_INVALID;
  }
  return remove(path) == 0 ? MSFS_OK : MSFS_ERR_IO;
}

/* Creates an empty file at path if it does not exist; leaves content intact. */
MSF_INLINE msfs_err_t msfs_touch(const char *path) {
  if (!path || path[0] == '\0') {
    return MSFS_ERR_INVALID;
  }
  FILE *fp = fopen(path, "ab");
  if (!fp) {
    return MSFS_ERR_OPEN;
  }
  return fclose(fp) == 0 ? MSFS_OK : MSFS_ERR_IO;
}

/* ==========================================================================
 * 3. Streamed file handle API
 * ========================================================================== */

/* Opens a file and initializes the msfs_file_t handle. */
MSF_INLINE msfs_err_t msfs_open(msfs_file_t *f, const char *path,
                                msfs_mode_t mode) {
  if (!f || !path || path[0] == '\0') {
    return MSFS_ERR_INVALID;
  }
  f->fp = NULL;
  f->mode = mode;

  const char *mstr = __msfs_mode_str(mode);
  if (!mstr) {
    return MSFS_ERR_INVALID;
  }

  f->fp = fopen(path, mstr);
  if (!f->fp) {
    return MSFS_ERR_OPEN;
  }
  return MSFS_OK;
}

/* Closes an open file handle. Safe to call on an already-closed handle. */
MSF_INLINE msfs_err_t msfs_close(msfs_file_t *f) {
  if (!f) {
    return MSFS_ERR_INVALID;
  }
  if (!f->fp) {
    return MSFS_OK;
  }
  int rc = fclose(f->fp);
  f->fp = NULL;
  return rc == 0 ? MSFS_OK : MSFS_ERR_IO;
}

/* Returns 1 if the file handle is currently open, 0 otherwise. */
MSF_INLINE int msfs_is_open(const msfs_file_t *f) {
  return f != NULL && f->fp != NULL;
}

/* Reads up to size bytes into buf. Returns the number of bytes read. */
MSF_INLINE size_t msfs_read(msfs_file_t *f, void *buf, size_t size) {
  if (!f || !f->fp || !buf || size == 0) {
    return 0;
  }
  return fread(buf, 1, size, f->fp);
}

/* Writes up to size bytes from buf. Returns the number of bytes written. */
MSF_INLINE size_t msfs_write(msfs_file_t *f, const void *buf, size_t size) {
  if (!f || !f->fp || !buf || size == 0) {
    return 0;
  }
  return fwrite(buf, 1, size, f->fp);
}

/*
 * Reads a single line into buf (up to buf_size - 1 chars) and NUL-terminates.
 * Strips a trailing "\n" or "\r\n" if present.
 * Returns MSFS_OK on success, MSFS_EOF at end-of-file, or an error code.
 */
MSF_INLINE msfs_err_t msfs_read_line(msfs_file_t *f, char *buf, size_t buf_size,
                                     size_t *out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (!f || !f->fp || !buf || buf_size == 0) {
    return MSFS_ERR_INVALID;
  }
  if (buf_size == 1) {
    buf[0] = '\0';
    return MSFS_ERR_INVALID;
  }

  int limit = buf_size > (size_t)INT_MAX ? INT_MAX : (int)buf_size;
  if (!fgets(buf, limit, f->fp)) {
    buf[0] = '\0';
    return ferror(f->fp) ? MSFS_ERR_READ : MSFS_EOF;
  }

  size_t len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n') {
    buf[--len] = '\0';
    if (len > 0 && buf[len - 1] == '\r') {
      buf[--len] = '\0';
    }
  }

  if (out_len) {
    *out_len = len;
  }
  return MSFS_OK;
}

/* Writes a NUL-terminated string followed by '\n'. */
MSF_INLINE msfs_err_t msfs_write_line(msfs_file_t *f, const char *line) {
  if (!f || !f->fp || !line) {
    return MSFS_ERR_INVALID;
  }
  size_t len = strlen(line);
  if (len > 0 && fwrite(line, 1, len, f->fp) != len) {
    return MSFS_ERR_WRITE;
  }
  if (fputc('\n', f->fp) == EOF) {
    return MSFS_ERR_WRITE;
  }
  return MSFS_OK;
}

/* Moves the file position indicator. */
MSF_INLINE msfs_err_t msfs_seek(msfs_file_t *f, int64_t offset,
                                msfs_seek_t origin) {
  if (!f || !f->fp) {
    return MSFS_ERR_INVALID;
  }
  if (offset < (int64_t)LONG_MIN || offset > (int64_t)LONG_MAX) {
    return MSFS_ERR_SEEK;
  }
  return fseek(f->fp, (long)offset, (int)origin) == 0 ? MSFS_OK : MSFS_ERR_SEEK;
}

/* Returns the current file position in bytes, or -1 on error. */
MSF_INLINE int64_t msfs_tell(const msfs_file_t *f) {
  if (!f || !f->fp) {
    return -1;
  }
  long pos = ftell(f->fp);
  return pos < 0 ? -1 : (int64_t)pos;
}

/* Returns the total size of an open file in bytes (preserves offset), or -1. */
MSF_INLINE int64_t msfs_file_size(msfs_file_t *f) {
  if (!f || !f->fp) {
    return -1;
  }
  long cur = ftell(f->fp);
  if (cur < 0) {
    return -1;
  }
  if (fseek(f->fp, 0, SEEK_END) != 0) {
    return -1;
  }
  long end = ftell(f->fp);
  if (fseek(f->fp, cur, SEEK_SET) != 0) {
    return -1;
  }
  return end < 0 ? -1 : (int64_t)end;
}

/* Flushes any buffered output to the underlying file. */
MSF_INLINE msfs_err_t msfs_flush(msfs_file_t *f) {
  if (!f || !f->fp) {
    return MSFS_ERR_INVALID;
  }
  return fflush(f->fp) == 0 ? MSFS_OK : MSFS_ERR_IO;
}

/* Returns 1 if the end-of-file indicator is set, 0 otherwise. */
MSF_INLINE int msfs_eof(const msfs_file_t *f) {
  if (!f || !f->fp) {
    return 0;
  }
  return feof(f->fp) != 0;
}

/* Returns 1 if the stream error indicator is set, 0 otherwise. */
MSF_INLINE int msfs_has_error(const msfs_file_t *f) {
  if (!f || !f->fp) {
    return 0;
  }
  return ferror(f->fp) != 0;
}

/* ==========================================================================
 * 4. Portable path helpers
 * ========================================================================== */

/*
 * Returns a pointer to the filename component of path (after the last '/' or
 * '\\'). Returns "" if path is NULL.
 */
MSF_INLINE const char *msfs_basename(const char *path) {
  if (!path) {
    return "";
  }
  const char *last = path;
  for (const char *p = path; *p != '\0'; p++) {
    if (__msfs_is_sep(*p)) {
      last = p + 1;
    }
  }
  return last;
}

/*
 * Returns a pointer to the file extension including the leading '.'
 * (e.g., ".txt"), or "" if there is no extension. Leading dots in filenames
 * such as ".gitignore" are not treated as extensions.
 */
MSF_INLINE const char *msfs_extension(const char *path) {
  const char *base = msfs_basename(path);
  const char *dot = NULL;
  for (const char *p = base; *p != '\0'; p++) {
    if (*p == '.') {
      dot = p;
    }
  }
  if (!dot || dot == base) {
    return "";
  }
  return dot;
}

/*
 * Writes the directory component of path into buf (NUL-terminated).
 * Examples: "a/b/c.txt" -> "a/b", "/file.txt" -> "/", "file.txt" -> ".".
 * Returns the length written, or 0 on invalid arguments / insufficient space.
 */
MSF_INLINE size_t msfs_dirname(const char *path, char *buf, size_t buf_size) {
  if (!buf || buf_size == 0) {
    return 0;
  }
  buf[0] = '\0';
  if (!path || path[0] == '\0') {
    return 0;
  }

  size_t len = strlen(path);
  while (len > 1 && __msfs_is_sep(path[len - 1])) {
    len--;
  }

  size_t sep_pos = len;
  while (sep_pos > 0 && !__msfs_is_sep(path[sep_pos - 1])) {
    sep_pos--;
  }

  if (sep_pos == 0) {
    if (buf_size < 2) {
      return 0;
    }
    buf[0] = '.';
    buf[1] = '\0';
    return 1;
  }

  size_t dir_len = sep_pos;
  while (dir_len > 1 && __msfs_is_sep(path[dir_len - 1])) {
    dir_len--;
  }

  if (dir_len + 1 > buf_size) {
    return 0;
  }

  memcpy(buf, path, dir_len);
  buf[dir_len] = '\0';
  return dir_len;
}

/*
 * Joins path segments a and b into buf using '/'.
 * Avoids duplicate separators when a ends with a separator or b starts with
 * one. Returns the length written, or 0 on error / insufficient buffer space.
 */
MSF_INLINE size_t msfs_path_join(char *buf, size_t buf_size, const char *a,
                                 const char *b) {
  if (!buf || buf_size == 0) {
    return 0;
  }
  buf[0] = '\0';
  if (!a || !b) {
    return 0;
  }

  size_t a_len = strlen(a);
  while (a_len > 1 && __msfs_is_sep(a[a_len - 1])) {
    a_len--;
  }

  while (*b != '\0' && __msfs_is_sep(*b)) {
    b++;
  }
  size_t b_len = strlen(b);

  if (a_len == 0) {
    if (b_len + 1 > buf_size) {
      return 0;
    }
    memcpy(buf, b, b_len);
    buf[b_len] = '\0';
    return b_len;
  }

  if (b_len == 0) {
    if (a_len + 1 > buf_size) {
      return 0;
    }
    memcpy(buf, a, a_len);
    buf[a_len] = '\0';
    return a_len;
  }

  int need_sep = !__msfs_is_sep(a[a_len - 1]);
  size_t total = a_len + (size_t)need_sep + b_len;
  if (total + 1 > buf_size) {
    return 0;
  }

  memcpy(buf, a, a_len);
  size_t pos = a_len;
  if (need_sep) {
    buf[pos++] = '/';
  }
  memcpy(buf + pos, b, b_len);
  buf[total] = '\0';
  return total;
}

#ifdef __cplusplus
}
#endif

#endif /* MSFS_H */
