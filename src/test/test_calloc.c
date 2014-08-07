#include <stdlib.h>

int main(int argc, char *argv[]) {
  int i;
  const int limit = 5;
  void *ptr[limit];
  for (i= 0; i < limit; ++i) {
    ptr[i] = calloc(3000, i);
  }
  for (i= 0; i < limit; ++i) {
    free(ptr[i]);
  }
  return 0;
}
