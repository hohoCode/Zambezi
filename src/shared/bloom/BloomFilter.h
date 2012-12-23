#ifndef BLOOM_FILTER_H_GUARD
#define BLOOM_FILTER_H_GUARD

#include <math.h>
#include <stdlib.h>

#define BLOOM_FILTER_UNIT_SIZE (sizeof(unsigned int) * 8)
#define BLOOM_FILTER_UNIT_SIZE_1 (unsigned int) (BLOOM_FILTER_UNIT_SIZE - 1)
#define BLOOM_FILTER_UNIT_EXP (unsigned int) log2(BLOOM_FILTER_UNIT_SIZE)
#define BLOOM_FILTER_ONE (unsigned int) 1
#define DEFAULT_HASH_SEED (unsigned int) 0x7ed55d16

typedef struct BloomFilter BloomFilter;

struct BloomFilter {
  unsigned int *bits;
  unsigned int size;
  unsigned int (*hash)(unsigned int value, unsigned int seed);
  unsigned int nbHash;
};

BloomFilter* createFilters(int size) {
  BloomFilter* filters = (BloomFilter*) malloc(size * sizeof(BloomFilter));
  return filters;
}

void destroy(BloomFilter* filter) {
  free(filter->bits);
}

void initialize(BloomFilter* filter, unsigned int size,
                            unsigned int (*hash)(unsigned int, unsigned int), int nbHash) {
  int r = size >> BLOOM_FILTER_UNIT_EXP;
  int m = size & BLOOM_FILTER_UNIT_SIZE_1;
  int length = r;
  if(m != 0) {
    length = r + 1;
  }

  filter->bits = (unsigned int*) malloc(length * sizeof(unsigned int));
  filter->size = size;
  filter->hash = hash;
  filter->nbHash = nbHash;

  int i = 0;
  for (i = 0; i < length; i++) {
    filter->bits[i] = 0;
  }
}

void add(BloomFilter* filter, unsigned int value) {
  unsigned int seed = DEFAULT_HASH_SEED;
  unsigned int h;
  int i = 0;
  for(i = 0; i < filter->nbHash; i++) {
    seed = filter->hash(value, seed);
    h = seed % filter->size;
    filter->bits[h>>BLOOM_FILTER_UNIT_EXP] |= BLOOM_FILTER_ONE<<(BLOOM_FILTER_UNIT_SIZE_1
                                                                 - (h & BLOOM_FILTER_UNIT_SIZE_1));
  }
}

int contains(BloomFilter* filter, unsigned int value) {
  unsigned int seed = DEFAULT_HASH_SEED;
  unsigned int h;
  int i = 0;
  for(i = 0; i < filter->nbHash; i++) {
    seed = filter->hash(value, seed);
    h = seed % filter->size;
    if(!(filter->bits[h>>BLOOM_FILTER_UNIT_EXP]>>(BLOOM_FILTER_UNIT_SIZE_1 - (h&BLOOM_FILTER_UNIT_SIZE_1)) & 1)) {
      return 0;
    }
  }
  return 1;
}

#endif
