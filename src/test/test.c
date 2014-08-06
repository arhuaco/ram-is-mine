#include <stdlib.h>

int main(int argc, char *argv[]) {
  int i;
  for (i= 0; i < 10; ++i) {
    void* x = malloc(10000 + i);
    free(x);
  }
  return 0;
}
