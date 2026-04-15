#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "cli.h"

void ltrim(char *str) {
  char *sp = str;

  if (isspace(*sp)) {
    /* Leading space(s) */
    while (isspace(*sp)) {
      sp++;
    }

    while (*sp) {
      *str++ = *sp++;
    }
    *str = EOS;
  }
}

void rtrim(char *str) {
  int len = strlen(str)-1;
  int i = len;

  while ((i >= 0) && isspace(str[i])) {
    i--;
  }

  if (i < len) {
    str[i+1] = EOS;
  }
}

void trim(char *str) {
    ltrim(str);
    rtrim(str);
}

int is_hex_char(char c) {
  c = tolower(c);

  if ((c >= '0') && (c <= '9')) {
    return 1;
  }

  if ((c >= 'a') && (c <= 'f')) {
    return 1;
  }

  return 0;
}

int hexval(char c) {
    c = tolower(c);

    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }

    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }

    return -1;
}