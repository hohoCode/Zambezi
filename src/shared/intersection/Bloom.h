#ifndef BLOOM_H_GUARD
#define BLOOM_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PostingsPool.h"
#include "bloom/BloomIndex.h"

#define TERMINAL_DOCID -1

int* intersectBloom(PostingsPool* pool, BloomIndex* bloom,
                    long* startPointers, int* queries, int len, int minDf) {
  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  int* set = (int*) calloc(minDf, sizeof(int));
  int iSet = 0, i, j, found;
  long t = startPointers[0];

  while(t != UNDEFINED_POINTER) {
    memset(block, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
    int c = decompressDocidBlock(pool, block, t);

    for(i = 0; i < c; i++) {
      found = 1;
      for(j = 1; j < len; j++) {
        if(!containsElementInIndex(bloom, queries[j], block[i])) {
          found = 0;
          break;
        }
      }
      if(found) {
        set[iSet++] = block[i];
      }
    }
    t = nextPointer(pool, t);
  }
  if(iSet < minDf) {
    set[iSet] = TERMINAL_DOCID;
  }
  free(block);
  return set;
}

#endif
