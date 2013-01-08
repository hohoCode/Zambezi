#ifndef BLOOM_FILTER_H_GUARD
#define BLOOM_FILTER_H_GUARD

#include <math.h>
#include <stdlib.h>

#define BLOOM_FILTER_UNIT_SIZE (sizeof(unsigned int) * 8)
#define BLOOM_FILTER_UNIT_SIZE_1 (unsigned int) (BLOOM_FILTER_UNIT_SIZE - 1)
#define BLOOM_FILTER_UNIT_EXP (unsigned int) log2(BLOOM_FILTER_UNIT_SIZE)
#define BLOOM_FILTER_ONE (unsigned int) 1
#define DEFAULT_HASH_SEED (unsigned int) 0x7ed55d16

unsigned int hash(unsigned int a, unsigned int seed) {
  a = (a+seed) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+0xd3a2646c) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  return (a^0xb55a4f09) ^ (a>>16);
}

void computeBloomFilterLength(unsigned int df) {
  int r = df >> BLOOM_FILTER_UNIT_EXP;
  int m = df & BLOOM_FILTER_UNIT_SIZE_1;
  int length = r;
  if(m != 0) {
    length = r + 1;
  }
  return length;
}

void insertIntoBloomFilter(unsigned int* filter, unsigned int filterSize,
                           int nbHash, unsigned int value) {
  unsigned int seed = DEFAULT_HASH_SEED;
  unsigned int h;
  int i = 0;
  for(i = 0; i < nbHash; i++) {
    seed = hash(value, seed);
    h = seed % filterSize;
    filter[h>>BLOOM_FILTER_UNIT_EXP] |=
      BLOOM_FILTER_ONE<<(BLOOM_FILTER_UNIT_SIZE_1
                         - (h & BLOOM_FILTER_UNIT_SIZE_1));
  }
}

int containsBloomFilter(unsigned int* filter, unsigned int filterSize,
                        int nbHash, unsigned int value) {
  unsigned int seed = DEFAULT_HASH_SEED;
  unsigned int h;
  int i = 0;
  for(i = 0; i < nbHash; i++) {
    seed = hash(value, seed);
    h = seed % filterSize;
    if(!(filter[h>>BLOOM_FILTER_UNIT_EXP]>>(BLOOM_FILTER_UNIT_SIZE_1 -
                                            (h&BLOOM_FILTER_UNIT_SIZE_1)) & 1)) {
      return 0;
    }
  }
  return 1;
}

#endif
