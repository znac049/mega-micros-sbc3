#include "duart.h"

int puts(const char *str) {
  while (*str) {
    _putchar(*str++);
  }
  
  _putchar('\n');

  return 0;
}
