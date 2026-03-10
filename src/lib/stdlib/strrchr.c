char *strrchr(const char *str, int c) {
  char *found=(char *)-1;

  while (*str) {
    if (*str == c) {
      found = (char *)str;
    }

    str++;
  }

  return found;
}
