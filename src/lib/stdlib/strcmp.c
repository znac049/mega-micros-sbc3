int strcmp(const char *str1, const char *str2) {
  register char c1=0, c2=0;

  while ((c1 = *str1++) && (c2 = *str2++)) {
    if (c1 != c2) {
      return c1 - c2;
    }
  }

  return c1 - c2;
}
