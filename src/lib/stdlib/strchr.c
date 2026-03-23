char *strrchr(const char *str, int c) {
  char *found=(char *)-1;

  while (*str) {
    if (*str == c) {
      return (char *)str;
    }

    str++;
  }

  return found;
}
