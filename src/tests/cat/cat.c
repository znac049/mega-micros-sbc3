#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>


void cat_file(char *name) {
    int fd = open(name, 0);
    size_t count;
    char buff[1024];

    if (fd == NOT_OK) {
        printf("open() failed.\n");
        exit(1);
    }

    printf("\n\n****** %s ******\n", name);
    while ((count = read(fd, buff, sizeof(buff))) > 0) {
        printf("PRINTING %d bytes:\n", count);
        for (size_t i=0; i<count; i++) {
            putchar(buff[i]);
        }
    }

    printf("\n*** EOF ***.\n");
    close(fd);
}

int main(void) {
    cat_file("motd");
    cat_file("passwd");

    exit(0);
    return OK;
}