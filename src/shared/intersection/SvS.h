#ifndef SVS_H_GUARD
#define SVS_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PostingsPool.h"

#define TERMINAL_DOCID -1

int* intersectPostingsLists_SvS(PostingsPool* pool, long a, long b, int minDf) {
  int* set = (int*) calloc(minDf, sizeof(int));
  unsigned int* dataA = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  unsigned int* dataB = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));

  int cA = decompressDocidBlock(pool, dataA, a);
  int cB = decompressDocidBlock(pool, dataB, b);
  int iSet = 0, iA = 0, iB = 0;

  while(a != UNDEFINED_POINTER && b != UNDEFINED_POINTER) {
    if(dataB[iB] == dataA[iA]) {
      set[iSet++] = dataA[iA];
      iA++;
      iB++;
    }

    if(iA == cA) {
      a = nextPointer(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cA = decompressDocidBlock(pool, dataA, a);
      iA = 0;
    }
    if(iB == cB) {
      b = nextPointer(pool, b);
      if(b == UNDEFINED_POINTER) {
        break;
      }
      memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cB = decompressDocidBlock(pool, dataB, b);
      iB = 0;
    }

    if(dataA[iA] < dataB[iB]) {
      if(dataA[cA - 1] < dataB[iB]) {
        iA = cA - 1;
      }
      while(dataA[iA] < dataB[iB]) {
        iA++;
        if(iA == cA) {
          a = nextPointer(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cA = decompressDocidBlock(pool, dataA, a);
          iA = 0;
        }
        if(dataA[cA - 1] < dataB[iB]) {
          iA = cA - 1;
        }
      }
    } else {
      if(dataB[cB - 1] < dataA[iA]) {
        iB = cB - 1;
      }
      while(dataB[iB] < dataA[iA]) {
        iB++;
        if(iB == cB) {
          b = nextPointer(pool, b);
          if(b == UNDEFINED_POINTER) {
            break;
          }
          memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cB = decompressDocidBlock(pool, dataB, b);
          iB = 0;
        }
        if(dataB[cB - 1] < dataA[iA]) {
          iB = cB - 1;
        }
      }
    }
  }

  if(iSet < minDf) {
    set[iSet] = TERMINAL_DOCID;
  }

  free(dataA);
  free(dataB);

  return set;
}

int intersectSetPostingsList_SvS(PostingsPool* pool, long a, int* currentSet, int len) {
  unsigned int* data = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  int c = decompressDocidBlock(pool, data, a);
  int iSet = 0, iCurrent = 0, i = 0;

  while(a != UNDEFINED_POINTER && iCurrent < len) {
    if(currentSet[iCurrent] == TERMINAL_DOCID) {
      break;
    }
    if(data[i] == currentSet[iCurrent]) {
      currentSet[iSet++] = currentSet[iCurrent];
      iCurrent++;
      i++;
    }

    if(i == c) {
      a = nextPointer(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      c = decompressDocidBlock(pool, data, a);
      i = 0;
    }
    if(iCurrent == len) {
      break;
    }
    if(currentSet[iCurrent] == TERMINAL_DOCID) {
      break;
    }

    if(data[i] < currentSet[iCurrent]) {
      if(data[c - 1] < currentSet[iCurrent]) {
        i = c - 1;
      }
      while(data[i] < currentSet[iCurrent]) {
        i++;
        if(i == c) {
          a = nextPointer(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          c = decompressDocidBlock(pool, data, a);
          i = 0;
        }
        if(data[c - 1] < currentSet[iCurrent]) {
          i = c - 1;
        }
      }
    } else {
      while(currentSet[iCurrent] < data[i]) {
        iCurrent++;
        if(iCurrent == len) {
          break;
        }
        if(currentSet[iCurrent] == TERMINAL_DOCID) {
          break;
        }
      }
    }
  }

  if(iSet < len) {
    currentSet[iSet] = TERMINAL_DOCID;
  }

  free(data);
  return iSet;
}

int* intersectSvS(PostingsPool* pool, long* startPointers, int len, int minDf) {
  if(len < 2) {
    unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    int* set = (int*) calloc(minDf, sizeof(int));
    int iSet = 0;
    long t = startPointers[0];
    while(t != UNDEFINED_POINTER) {
      memset(block, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      int c = decompressDocidBlock(pool, block, t);
      memcpy(&set[iSet], block, c * sizeof(int));
      iSet += c;
      t = nextPointer(pool, t);
    }
    free(block);
    return set;
  }

  int* set = intersectPostingsLists_SvS(pool, startPointers[0], startPointers[1], minDf);
  int i;
  for(i = 2; i < len; i++) {
    intersectSetPostingsList_SvS(pool, startPointers[i], set, minDf);
  }
  return set;
}

#endif
