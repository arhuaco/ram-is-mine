#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  int i;
  const int limit = 7;
  void *ptr[limit];
  for (i= 0; i < limit; ++i) {
    ptr[i] = realloc(0, 100); // Allocate 100 bytes.
  }
  for (i= 0; i < limit; ++i) {
    ptr[i] = realloc(ptr[i], i * 1500);
  }
  for (i= 0; i < limit; ++i) {
    assert(realloc(ptr[i], 0) == NULL);
  }
  return 0;
}
