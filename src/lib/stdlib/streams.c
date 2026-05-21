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
#include <machine.h>

FILE _stream_table[STREAM_TABLE_SIZE];

static FILE *find_free_slot(void) {
	for (int i=0; i<STREAM_TABLE_SIZE; i++) {
		if (_stream_table[i].is_open == 0)
			return &_stream_table[i];
	}

	return NULL;
}

FILE *fopen(const char *pathname, const char *mode) {
	int flags = 0;
	int fd;
	FILE *stream;
	int len = strlen(mode);

	if ((len > 2) || ((len == 2) && (mode[1] != '+'))) {
		errno = EINVAL;
		return NULL;
	}

	switch (mode[0]) {
		case 'r':
			if (mode[1] == '+')
				flags = O_RDWR;
			else
				flags = O_RDONLY; 
			break;

		case 'w':
			if (mode[1] == '+') 
				flags = O_RDWR | O_CREAT;
			else
				flags = O_WRONLY;
			break;

		case 'a':
			if (mode[1] == '+')
				flags = O_RDWR | O_CREAT | O_APPEND;
			else
				flags = O_WRONLY | O_CREAT | O_APPEND;
			break;

		default:
			errno = EINVAL;
			return NULL;
	}

	stream = find_free_slot();
	if (stream == NULL) {
		errno = ENOMEM;
		return stream;
	}

	// now let open(2) does all the heavy lifting
	fd = open(pathname, flags);
	if (fd == -1) {
		return NULL;
	}

	stream->is_open = 1;
	stream->fd = fd;

	return stream;
}