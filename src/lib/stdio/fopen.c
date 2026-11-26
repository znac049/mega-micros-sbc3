/*
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <machine.h>

/*
 * The streams. The first three are stdin, stdout and stderr, on the
 * monitor's fds 0, 1 and 2, which are always open.
 */
FILE _stream_table[STREAM_TABLE_SIZE] = {
    { .fd = 0, .is_open = YES },
    { .fd = 1, .is_open = YES },
    { .fd = 2, .is_open = YES },
};

FILE *stdin = &_stream_table[0];
FILE *stdout = &_stream_table[1];
FILE *stderr = &_stream_table[2];

static FILE *find_free_slot(void) {
	for (int i=0; i<STREAM_TABLE_SIZE; i++) {
		if (_stream_table[i].is_open == 0)
			return &_stream_table[i];
	}

	return NULL;
}

/*
 * Modes: "r", "w" or "a", optionally followed by "+", with a "b" allowed
 * anywhere after the first letter ("rb", "r+b", "rb+"). Binary and text
 * are the same here.
 */
FILE *fopen(const char *pathname, const char *mode) {
	int flags = 0;
	int plus = NO;
	int fd;
	FILE *stream;

	for (const char *m = mode + 1; *m != EOS; m++) {
		if (*m == '+') {
			plus = YES;
		}
		else if (*m != 'b') {
			errno = EINVAL;
			return NULL;
		}
	}

	switch (mode[0]) {
		case 'r':
			flags = plus?O_RDWR:O_RDONLY;
			break;

		case 'w':
			flags = (plus?O_RDWR:O_WRONLY) | O_CREAT | O_TRUNC;
			break;

		case 'a':
			flags = (plus?O_RDWR:O_WRONLY) | O_CREAT | O_APPEND;
			break;

		default:
			errno = EINVAL;
			return NULL;
	}

	stream = find_free_slot();
	if (stream == NULL) {
		errno = EMFILE;
		return stream;
	}

	// now let open(2) does all the heavy lifting
	fd = open(pathname, flags, 0644);
	if (fd == NOT_OK) {
		return NULL;
	}

	stream->is_open = YES;
	stream->fd = fd;
	stream->eof = NO;
	stream->error = NO;
	stream->has_ungot = NO;

	return stream;
}

/*
 * A stream for a file descriptor that's already open. The mode isn't
 * checked against how it was opened: streams just read and write it.
 */
FILE *fdopen(int fd, const char *mode) {
	FILE *stream;

	if ((mode == NULL) || ((mode[0] != 'r') && (mode[0] != 'w') && (mode[0] != 'a'))) {
		errno = EINVAL;
		return NULL;
	}

	if ((fd < 0) || (lseek(fd, 0, SEEK_CUR) == NOT_OK && !isatty(fd))) {
		errno = EBADF;
		return NULL;
	}

	stream = find_free_slot();
	if (stream == NULL) {
		errno = EMFILE;
		return NULL;
	}

	stream->is_open = YES;
	stream->fd = fd;
	stream->eof = NO;
	stream->error = NO;
	stream->has_ungot = NO;

	if (mode[0] == 'a') {
		lseek(fd, 0, SEEK_END);
	}

	return stream;
}
