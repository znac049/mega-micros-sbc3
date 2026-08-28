#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    int fd = open("motd", 0);
    size_t count;
    char buff[1024];

    if (fd == NOT_OK) {
        printf("open() failed.\n");
        exit(1);
    }

    printf("****** MOTD ******\n");
    count = read(fd, buff, sizeof(buff));
    while (count != (size_t)NOT_OK) {
        for (size_t i=0; i<count; i++) {
            putchar(buff[i]);
        }
    }

    printf("\nAll done.\n");
    close(fd);

    return OK;
}