#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  int i;
  const int limit = 7;
  void *ptr[limit];
  for (i= 0; i < limit; ++i) {
    ptr[i] = realloc(0, 100); // Allocate 100 bytes.
  }
  for (i= 0; i < limit; ++i) {
    size_t new_size = i * 1500;
    void *new_ptr = realloc(ptr[i], new_size);
    if (new_size == 0) {
      /* special case. This is a free. */
      assert(new_ptr == 0);
      ptr[i] = 0;
    } else if (new_ptr) {
      ptr[i] = new_ptr;
    } else {
      fprintf(stderr, "realloc_test: realoc(%p, %zu) returns NULL. Original pointer untouched.\n", ptr[i], new_size);
    }
  }
  for (i= 0; i < limit; ++i) {
    void *before;
    before = ptr[i];
    if (before) {
      ptr[i] = realloc(ptr[i], 0);
      fprintf(stderr, "realloc_test: realoc(%p, 0) returns %p.\n", before, ptr[i]);
    }
  }
  return 0;
}
