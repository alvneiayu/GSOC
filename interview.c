/*
 * Memory view
 *
 * A memory view presents an address space (the "view") that maps onto
 * underlying memory buffers (the "memory").  This layer of indirection makes
 * it easy to access isolated pieces of memory as if they were adjacent.  For
 * example:
 *
 * Memory address   Name
 * 0x08000-0x0c000  BUFFER1
 * 0x10000-0x11000  BUFFER2
 *
 * BUFFER2 can be accessed as if it was located immediately after BUFFER1 using
 * a memory view:
 *
 * View offset      Name
 * 0x0000-0xc000    BUFFER1
 * 0xc000-0xd000    BUFFER2
 *
 * This technique is used in text, audio, and video editors to perform
 * insertions and deletions without copying data.  In this exercise you will
 * implement a simplified memory view.
 */

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* TODO You may include C Standard Library headers and define additional
 * structs.  You cannot change the function prototypes, the API is fixed.
 */

/** A piece of memory */
typedef struct {
	void *data;
	size_t len;
} Buffer;

typedef struct {
	Buffer *buffers;
	int nbuffers;
	int nbytes;
} MemView;

/**
 * Initialize @memview given an array of memory buffers.
 *
 * Returns: true on success, false on error
 */
bool memview_init(MemView *memview,
                  const Buffer *buffers,
                  size_t nbuffers)
{
	int i;

	memview->buffers = calloc(1, nbuffers * sizeof(Buffer));
	if (memview->buffers == NULL)
		return false;

	memcpy(memview->buffers, buffers, nbuffers * sizeof(Buffer));
	memview->nbuffers = nbuffers;

	for (i = 0; i < nbuffers; i++)
		memview->nbytes += memview->buffers[i].len;

	return true;
}

/**
 * Free any allocated resources.
 */
void memview_cleanup(MemView *memview)
{
	free((void *)memview->buffers);
}

/**
 * Drop @nbytes from the start of the memory view.
 */
void memview_discard_front(MemView *memview, size_t nbytes)
{
	int i, deloffset = nbytes, pos;

	for (i = 0; i < memview->nbuffers; i++) {
		if (deloffset > memview->buffers[i].len) {
			deloffset -= memview->buffers[i].len;
			continue;
		}

		memview->buffers[i].data += deloffset;
		memview->buffers[i].len -= deloffset;
		memview->nbuffers -= i;
		memview->nbytes -= nbytes;
		memview->buffers[0] = memview->buffers[i];
		break;
	}
}

/**
 * Copy @len bytes starting at @offset to @data.
 *
 * @offset: the starting location in the view
 * @data: the copy destination
 * @len: the number of bytes to copy
 *
 * Returns: true on success, false on error
 */
bool memview_read(MemView *memview, uint64_t offset, void *data, size_t len)
{
	int i, pos, lencopy = len, coffset = offset;
	Buffer b;


	if (len < 0 || offset >= memview->nbytes)
		return false;

	/* Searching the buffer */
	for (i = 0; i < memview->nbuffers; i++) {
		if (coffset > memview->buffers[i].len) {
			coffset -= memview->buffers[i].len;
			continue;
		}

		pos = i;
		b = memview->buffers[i];
		break;
	}

	/* Let copy the info */
	while (lencopy > 0) {
		if (b.len >= lencopy)
			memcpy(data + (len - lencopy), b.data + coffset,
			       lencopy);
		else
			memcpy(data + (len - lencopy), b.data + coffset, b.len);

		lencopy -= b.len - coffset;
		coffset = 0;

		if (pos < memview->nbuffers)
			pos++;
		else
			break;

		b = memview->buffers[pos];
	}

	return true;
}

/* This serves both as an API usage example and a test case. */
int main(int argc, char **argv) {
	Buffer buffers[] = {
		{"hello", 5},
		{"world", 5},
		{"!", 1},
	};

	MemView memview;
	assert(memview_init(&memview, buffers,
	       sizeof(buffers) / sizeof(buffers[0])));

	/* Read across one buffer boundary */
	char buf[7];
	assert(memview_read(&memview, 3, buf, 4));
	assert(buf[0] == 'l' && buf[1] == 'o' && buf[2] == 'w' &&
	       buf[3] == 'o');


	/* Discard within first buffer */
	memview_discard_front(&memview, 2);
	assert(memview_read(&memview, 0, buf, 4));

	assert(buf[0] == 'l' && buf[1] == 'l' && buf[2] == 'o' &&
	       buf[3] == 'w');

	/* Read across two buffer boundaries */
	assert(memview_read(&memview, 2, buf, 7));
	assert(buf[0] == 'o' && buf[1] == 'w' && buf[2] == 'o' &&
	       buf[3] == 'r' && buf[4] == 'l' && buf[5] == 'd' &&
	       buf[6] == '!');

	/* Read beyond end */
	assert(!memview_read(&memview, 9, buf, 1));

	/* Discard first buffer and into second */
	memview_discard_front(&memview, 4);
	assert(memview_read(&memview, 0, buf, 3));
	assert(buf[0] == 'o' && buf[1] == 'r' && buf[2] == 'l');

	memview_cleanup(&memview);
	return 0;
}
