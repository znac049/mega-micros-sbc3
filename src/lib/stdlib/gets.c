#include "stddef.h"
#include "proto.h"

char *gets(char *s) {
    char c = getchar();
    char *str = s;

    while (c) {
        if ((c == '\r') || (c == '\n')) {
            *str = '\n';
            return s; 
        }

        *str++ = c;
    }

    return NULL;   
}