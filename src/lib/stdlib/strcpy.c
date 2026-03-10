char *strcpy(char *dst, const char *src) {
  char *cpy = dst;
  while (*src) {
    *cpy++ = *src++;
  }

  return dst;
}
