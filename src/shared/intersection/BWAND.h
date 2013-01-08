#ifndef BWAND_H_GUARD
#define BWAND_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "heap/Heap.h"
#include "PostingsPool.h"

#define TERMINAL_DOCID -1

// Note: change start pointers to tail pointers
int* bwand(PostingsPool* pool, long* startPointers,
           float* UB, int len, int hits) {
  Heap* elements = initHeap(hits);
  unsigned int* blockDocid = (unsigned int*) calloc(2 * BLOCK_SIZE, sizeof(unsigned int));
  unsigned int count;
  int posting;
  float threshold = 0;
  int i, j;

  count = decompressDocidBlock(pool, blockDocid, startPointers[0]);
  posting = 0;
  if(UB[0] <= threshold) {
    threshold = UB[0] - 1;
  }

  while(1) {
    int pivot = blockDocid[posting++];

    float score = UB[0];
    for(i = 1; i < len; i++) {
      if(containsDocid(pool, pivot, &startPointers[i])) {
        score += UB[i];
      }
    }

    if(score > threshold) {
      insertHeap(elements, pivot, score);
      if(isFullHeap(elements)) {
        threshold = minScoreHeap(elements);
      }
    }

    if(posting == count) {
      startPointers[0] = nextPointer(pool, startPointers[0]);
      if(startPointers[0] == UNDEFINED_POINTER) {
        break;
      }
      memset(blockDocid, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      count = decompressDocidBlock(pool, blockDocid, startPointers[0]);
      posting = 0;
    }
  }

  // Free the allocated memory
  free(blockDocid);

  int* set = (int*) calloc(elements->index + 1, sizeof(int));
  memcpy(set, &elements->docid[1], elements->index * sizeof(int));
  if(!isFullHeap(elements)) {
    set[elements->index] = TERMINAL_DOCID;
  }
  destroyHeap(elements);
  return set;
}

#endif
