int strlen(const char *str) {
  int len;
  
  for (len=0; *str; str++) {
    len++;
  }

  return len;
}
