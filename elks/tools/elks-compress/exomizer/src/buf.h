#ifndef ALREADY_INCLUDED_BUF
#define ALREADY_INCLUDED_BUF
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2016 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */
#include <stdio.h>
#ifndef __GNUC__
#define  __attribute__(x)  /*NOTHING*/
#endif

#define STATIC_BUF_INIT {0, 0, 0}

struct buf {
    void *data;
    int size;
    int capacity;
};

void buf_init(struct buf *b);
void buf_free(struct buf *b);

void buf_new(struct buf **bp);
void buf_delete(struct buf **bp);

/* Gets the number of bytes currently in the buffer. Won't change the
 * buffer content or size. Won't invalidate previously fetched
 * internal buffer pointers. */
int buf_size(const struct buf *b);

/* Gets the current capacity of the buf. Won't change the buffer
 * content or size. Won't invalidate previously fetched internal
 * buffer pointers. */
int buf_capacity(const struct buf *b);

/* Gets a pointer to the internal buffer. Don't dereferece it beyond
 * the size of the buffer. The returned pointer might be invalidated
 * by any future modifying operation. Might return NULL for a just
 * initiated buffer. */
void *buf_data(const struct buf *b);

/* Ensures that at least the requested capacity is available.  Won't
 * change the buffer content or size but might invalidate previously
 * fetched internal buffer pointers. returns a pointer to the internal
 * buffer like a buf_data call. */
void *buf_reserve(struct buf *b, int capacity);

/* Clears the given buffer. This will set its size to zero. Won't
 * invalidate previously fetched internal buffer pointers. */
void buf_clear(struct buf *b);

/* Removes b_n number of bytes at offset b_off from the given
 * buffer. A value of -1 for b_n is allowed and interpreted as "to the
 * end of the buffer". Won't invalidate previously fetched internal
 * buffer pointers. Returns a pointer to the internal buffer like a
 * buf_data call. */
void buf_remove(struct buf *b, int b_off, int b_n);

/* Inserts b_n number of bytes, copied from mem, at offset b_off in
 * the given buffer. Negative values for b_off are allowed and
 * interpreted as size + b_off + 1. A value of NULL for m is allowed
 * and will leave the target buffer area uninitialized. Might
 * invalidate previously fetched internal buffer pointers. Returns a
 * pointer to the inserted area in the internal buffer. */
void *buf_insert(struct buf *b, int b_off, const void *m, int m_n);

/* Appends to the end of the buffer. Returns a pointer to the internal
 * buffer like a buf_data call. A value of NULL for m is allowed and
 * will leave the target buffer area uninitialized. Might invalidate
 * previously fetched internal buffer pointers. Returns a pointer to
 * the appended area in the internal buffer.*/
void *buf_append(struct buf *b, const void *m, int m_n);

/* Appends a char to the end of the buffer. Might invalidate
 * previously fetched internal buffer pointers. */
void buf_append_char(struct buf *b, char c);

/* Appends a zero terminated string to the end of the buffer excluding
 * the zero terminator. Might invalidate previously fetched internal
 * buffer pointers. */
void buf_append_str(struct buf *b, const char *str);

/* Performs inserts, deletes, appends, prepends and replacements.
 * Negative values for b_off are allowed and interpreted as size +
 * b_off + 1. A value of -1 for b_n is allowed and interpreted as size
 * - b_off. A value of NULL for m is allowed and will leave the target
 * buffer area uninitialized. Might invalidate previously fetched
 * internal buffer pointers. Returns a pointer to the replaced area in
 * the internal buffer. */
void *buf_replace(struct buf *b, int b_off, int b_n, const void *m, int m_n);

/* Performs inserts, deletes, appends, prepends and replacements from
 * the given file f. A negative value for b_off is allowed and
 * interpreted as size + b_off + 1. A value of -1 for b_n is allowed
 * and interpreted as size - b_off. A negative value for f_off is
 * allowed and interpreted as the file size + b_off + 1. A value of -1
 * for f_n is allowed and interpreted as the file size - f_off. Might
 * invalidate previously fetched internal buffer pointers.  Returns a
 * pointer to the replaces area in the internal buffer like a buf_data
 * call. */
void *buf_freplace(struct buf *b, int b_off, int b_n,
                   FILE *f, int f_off, int f_n);

/**
 * Creates a read only view into another buf starting at the given
 * offset and being of the given size. The view buf will not take
 * ownership of the data and is not to be freed.
 */
const struct buf *buf_view(struct buf *v,
                           const struct buf *b, int b_off, int b_n);

/* Prints formatted to the end of the buffer using the vsnsprintf
 * function. Will place a zero byte string terminator in the
 * position directly following the end of the buffer to make it
 * printf-able. */
void buf_printf(struct buf *b, const char *format, ...)
    __attribute__((format(printf,2,3)));

#ifdef __cplusplus
}
#endif
#endif
