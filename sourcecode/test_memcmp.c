#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include <assert.h>

enum test_constants {
  constant_overallocate = 256,
  constant_megabyte = 1 * 1024 * 1024
};

extern int memcmp_neon(const void *ptr1, const void *ptr2, size_t num);

typedef int (*memcmp_func)(const void*, const void*, size_t);

void fill_buffer(void *p, unsigned size) {
  unsigned i;
  uint32_t *buf;

  assert(size * constant_megabyte % 4 == 0);

  buf = p;

  for (i = 0; i < size * constant_megabyte / 4; ++i) {
    *buf = random();
    ++buf;
  }
}

int main(int argc, char* argv[]) {
  const memcmp_func funcs[2] = {memcmp, memcmp_neon};
  const unsigned iterations = 100;

  /* minimum and maximum amount of megabytes to allocate */
  const unsigned alloc_param[2] = {8, 128};

  unsigned i;
  unsigned mismatches = 0;

  srand(time(NULL));

  /* Mismatch loop. */
  for (i = 0; i < iterations; ++i) {
    void *ptr[2];
    int res[2];

    const unsigned size = alloc_param[0] + (random() % (alloc_param[1] - alloc_param[0]));
 
    const unsigned shift[2] = {
      random() % (constant_overallocate / 2),
      random() % (constant_overallocate / 2)
    };

    fprintf(stderr, "info: allocating %u megabytes\n", size);

    ptr[0] = malloc(size * constant_megabyte + constant_overallocate);
    ptr[1] = malloc(size * constant_megabyte + constant_overallocate);

    fill_buffer(ptr[0] + shift[0], size);
    fill_buffer(ptr[1] + shift[1], size);

    res[0] = funcs[0](ptr[0] + shift[0], ptr[1] + shift[1], size * constant_megabyte);
    res[1] = funcs[1](ptr[0] + shift[0], ptr[1] + shift[1], size * constant_megabyte);

    if (res[0] != res[1]) {
      fprintf(stderr, "info: memcmp result: %d / %d\n", res[0], res[1]);
      fprintf(stderr, "error: mismatch found!\n", size);
      ++mismatches;
    }

    free(ptr[0]);
    free(ptr[1]);
  }

  /* Match loop. */
  for (i = 0; i < iterations; ++i) {
    void *ptr[2];
    int res[2];

    const unsigned size = alloc_param[0] + (random() % (alloc_param[1] - alloc_param[0]));
 
    const unsigned shift[2] = {
      random() % (constant_overallocate / 2),
      random() % (constant_overallocate / 2)
    };

    fprintf(stderr, "info: allocating %u megabytes\n", size);

    ptr[0] = malloc(size * constant_megabyte + constant_overallocate);
    ptr[1] = malloc(size * constant_megabyte + constant_overallocate);

    fill_buffer(ptr[0] + shift[0], size);
    memcpy(ptr[1] + shift[1], ptr[0] + shift[0], size * constant_megabyte);

    res[0] = funcs[0](ptr[0] + shift[0], ptr[1] + shift[1], size * constant_megabyte);
    res[1] = funcs[1](ptr[0] + shift[0], ptr[1] + shift[1], size * constant_megabyte);

    if (res[0] != res[1]) {
      fprintf(stderr, "info: memcmp result: %d / %d\n", res[0], res[1]);
      fprintf(stderr, "error: mismatch found!\n", size);
      ++mismatches;
    }

    free(ptr[0]);
    free(ptr[1]);
  }

  fprintf(stderr, "info: number of mismatches = %u\n", mismatches);

  return 0;
}
