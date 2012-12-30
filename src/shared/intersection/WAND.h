#ifndef BMW_H_GUARD
#define BMW_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "heap/Heap.h"
#include "scorer/BM25.h"
#include "PostingsPool.h"

#define MIN(X, Y) (X < Y ? X : Y)
#define TERMINAL_DOCID -1

int* wand(PostingsPool* pool, long* startPointers, int* df, float* UB, int len,
         int* docLen, int totalDocs, float avgDocLen, int hits) {
  Heap* elements = initHeap(hits);
  int origLen = len;
  unsigned int** blockDocid = (unsigned int**) calloc(len, sizeof(unsigned int*));
  unsigned int** blockTf = (unsigned int**) calloc(len, sizeof(unsigned int*));
  unsigned int* counts = (unsigned int*) calloc(len, sizeof(unsigned int));
  int* posting = (int*) calloc(len, sizeof(int));
  int* mapping = (int*) calloc(len, sizeof(int));
  float threshold = 0;

  int i, j;
  for(i = 0; i < len; i++) {
    blockDocid[i] = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    blockTf[i] = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    counts[i] = decompressDocidBlock(pool, blockDocid[i], startPointers[i]);
    decompressTfBlock(pool, blockTf[i], startPointers[i]);
    posting[i] = 0;
    mapping[i] = i;
    if(UB[i] <= threshold) {
      threshold = UB[i] - 1;
    }
  }

  for(i = 0; i < len; i++) {
    for(j = i + 1; j < len; j++) {
      if(blockDocid[mapping[i]][posting[mapping[i]]] >
         blockDocid[mapping[j]][posting[mapping[j]]]) {
        int temp = mapping[i];
        mapping[i] = mapping[j];
        mapping[j] = temp;
      }
    }
  }

  int curDoc = 0;
  int pTerm = 0;
  int pTermIdx = 0;

  while(1) {
    float sum = 0;
    pTerm = -1;
    pTermIdx = -1;
    for(i = 0; i < len; i++) {
      sum += UB[mapping[i]];
      if(sum > threshold) {
        pTerm = mapping[i];
        pTermIdx = i;
        if(i < len - 1) {
          if(blockDocid[mapping[i]][posting[mapping[i]]] ==
             blockDocid[mapping[i + 1]][posting[mapping[i + 1]]]) {
            continue;
          }
        }
        break;
      }
    }

    if(sum == 0 || pTerm == -1) {
      break;
    }

    int pivot = blockDocid[pTerm][posting[pTerm]];

    if(pivot <= curDoc) {
      int atermIdx;
      for(atermIdx = 0; atermIdx < MIN(pTermIdx + 1, len); atermIdx++) {
        int aterm = mapping[atermIdx];

        if(posting[aterm] == counts[aterm] - 1 &&
           nextPointer(pool, startPointers[aterm]) == UNDEFINED_POINTER) {
          int k = 0;
          for(i = 0; i < len; i++) {
            if(i != atermIdx) {
              mapping[k++] = mapping[i];
            }
          }
          len--;
          atermIdx--;
          continue;
        }

        while(blockDocid[aterm][posting[aterm]] <= curDoc) {
          posting[aterm]++;

          if(posting[aterm] >= counts[aterm] - 1) {
            startPointers[aterm] = nextPointer(pool, startPointers[aterm]);
            if(startPointers[aterm] == UNDEFINED_POINTER) {
              break;
            } else {
              counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
              decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
              posting[aterm] = 0;
            }
          }
        }
      }

      for(i = 0; i < len; i++) {
        for(j = i + 1; j < len; j++) {
          if(blockDocid[mapping[i]][posting[mapping[i]]] >
             blockDocid[mapping[j]][posting[mapping[j]]]) {
            int temp = mapping[i];
            mapping[i] = mapping[j];
            mapping[j] = temp;
          }
        }
      }
    } else {
      if(blockDocid[mapping[0]][posting[mapping[0]]] == pivot) {
        curDoc = pivot;
        float score = 0;
        for(i = 0; i <= pTermIdx; i++) {
          score += bm25(blockTf[mapping[i]][posting[mapping[i]]],
                        df[mapping[i]], totalDocs, docLen[curDoc], avgDocLen);
        }

        insertHeap(elements, curDoc, score);
        if(isFullHeap(elements)) {
          threshold = minScoreHeap(elements);
        }
      }

      int atermIdx;
      for(atermIdx = 0; atermIdx < MIN(pTermIdx + 1, len); atermIdx++) {
        int aterm = mapping[atermIdx];

        if(posting[aterm] == counts[aterm] - 1 &&
           nextPointer(pool, startPointers[aterm]) == UNDEFINED_POINTER) {
          int k = 0;
          for(i = 0; i < len; i++) {
            if(i != atermIdx) {
              mapping[k++] = mapping[i];
            }
          }
          len--;
          atermIdx--;
          continue;
        }

        while(blockDocid[aterm][posting[aterm]] <= pivot) {
          posting[aterm]++;

          if(posting[aterm] >= counts[aterm] - 1) {
            startPointers[aterm] = nextPointer(pool, startPointers[aterm]);
            if(startPointers[aterm] == UNDEFINED_POINTER) {
              break;
            } else {
              counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
              decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
              posting[aterm] = 0;
            }
          }
        }
      }

      for(i = 0; i < len; i++) {
        for(j = i + 1; j < len; j++) {
          if(blockDocid[mapping[i]][posting[mapping[i]]] >
             blockDocid[mapping[j]][posting[mapping[j]]]) {
            int temp = mapping[i];
            mapping[i] = mapping[j];
            mapping[j] = temp;
          }
        }
      }
    }
  }

  // Free the allocated memory
  free(posting);
  free(mapping);
  for(i = 0; i < origLen; i++) {
    free(blockDocid[i]);
    free(blockTf[i]);
  }
  free(blockDocid);
  free(blockTf);
  free(counts);

  int* set = (int*) calloc(elements->index + 1, sizeof(int));
  memcpy(set, &elements->docid[1], elements->index * sizeof(int));
  if(!isFullHeap(elements)) {
    set[elements->index] = TERMINAL_DOCID;
  }
  destroyHeap(elements);
  return set;
}

#endif
