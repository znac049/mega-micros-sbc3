void *memset(void *mem, int fill, unsigned int count) {
  for (int i=0; i<count; i++) {
    ((char *)mem)[i] = fill;
  }

  return  mem;
}
