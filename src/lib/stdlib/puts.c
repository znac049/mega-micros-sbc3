#include "tutor.h"

int puts(const char *str) {
  while (*str) {
    _t14_outch(*str++);
  }
  
  _t14_outch('\n');

  return 0;
}
