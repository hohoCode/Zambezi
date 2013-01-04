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

int* bmw(PostingsPool* pool, long* startPointers, int* df, float* UB, int len,
         int* docLen, int totalDocs, float avgDocLen, int hits) {
  Heap* elements = initHeap(hits);
  int origLen = len;
  unsigned int** blockDocid = (unsigned int**) calloc(len, sizeof(unsigned int*));
  unsigned int** blockTf = (unsigned int**) calloc(len, sizeof(unsigned int*));
  unsigned int* counts = (unsigned int*) calloc(len, sizeof(unsigned int));
  int* posting = (int*) calloc(len, sizeof(int));
  int* mapping = (int*) calloc(len, sizeof(int));
  int* blockChanged = (int*) calloc(len, sizeof(int));
  float* blockMaxScores = (float*) malloc(len * sizeof(float));
  float threshold = 0;

  int i, j;
  for(i = 0; i < len; i++) {
    blockDocid[i] = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    blockTf[i] = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    counts[i] = decompressDocidBlock(pool, blockDocid[i], startPointers[i]);
    decompressTfBlock(pool, blockTf[i], startPointers[i]);
    posting[i] = 0;
    mapping[i] = i;
    blockChanged[i] = 0;
    if(UB[i] <= threshold) {
      threshold = UB[i] - 1;
    }
    blockMaxScores[i] = bm25(getBlockMaxTf(pool, startPointers[i]),
                             df[i], totalDocs,
                             getBlockMaxTfDocLen(pool, startPointers[i]),
                             avgDocLen);
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

  int pTerm = 0;
  int pTermIdx = 0;

  while(1) {
    // Pivoting using global upper-bounds
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

    // NextShallow() and CheckBlockMax()
    float blockMaxScore = 0.0;
    int candidate = 0;
    for(i = 0; i <= pTermIdx; i++) {
      int iterm = mapping[i];
      int maxid;
      while((maxid = getBlockMaxDocid(pool, startPointers[iterm])) < pivot) {
        long newPointer = nextPointer(pool, startPointers[iterm]);
        if(newPointer == UNDEFINED_POINTER) {
          break;
        } else {
          startPointers[iterm] = newPointer;
          blockChanged[iterm] = 1;
        }
      }

      if(maxid >= pivot) {
        if(candidate == 0 || maxid + 1 < candidate) {
          candidate = maxid + 1;
        }
        if(blockChanged[iterm]) {
          blockMaxScores[iterm] = bm25(getBlockMaxTf(pool, startPointers[iterm]),
                                       df[iterm], totalDocs,
                                       getBlockMaxTfDocLen(pool, startPointers[iterm]),
                                       avgDocLen);
        }
        blockMaxScore += blockMaxScores[iterm];
      }
    }

    if(blockMaxScore <= threshold) {
      if(pTermIdx + 1 < len) {
        int maxid = blockDocid[mapping[pTermIdx + 1]][posting[mapping[pTermIdx + 1]]];
        if(maxid < candidate) {
          candidate = maxid;
        }
      }
      candidate = pivot + 1 > candidate ? pivot + 1 : candidate;

      int aterm = mapping[0];
      int atermIdx;
      for(atermIdx = 0; atermIdx < MIN(pTermIdx + 1, len); atermIdx++) {
        if(df[mapping[atermIdx]] <= df[aterm] &&
           blockDocid[mapping[atermIdx]][posting[mapping[atermIdx]]] < candidate) {
          int atermTemp = mapping[atermIdx];

          if(posting[atermTemp] >= counts[atermTemp] - 1 &&
             nextPointer(pool, startPointers[atermTemp]) == UNDEFINED_POINTER) {
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
          aterm = atermTemp;
        }
      }

      if(blockChanged[aterm]) {
        counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
        decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
        posting[aterm] = 0;
        blockChanged[aterm] = 0;
      }

      while(blockDocid[aterm][posting[aterm]] < candidate) {
        posting[aterm]++;
        if(posting[aterm] > counts[aterm] - 1) {
          long newPointer = nextPointer(pool, startPointers[aterm]);
          if(newPointer == UNDEFINED_POINTER) {
            break;
          } else {
            startPointers[aterm] = newPointer;
            counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
            decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
            posting[aterm] = 0;
          }
        }
        if(blockDocid[aterm][posting[aterm]] == 88598) {
          float a = bm25(blockTf[aterm][posting[aterm]],
                         df[aterm], totalDocs,
                         docLen[88598],
                         avgDocLen);
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
      continue;
    }

    if(blockDocid[mapping[0]][posting[mapping[0]]] == pivot) {
      float score = 0;
      for(i = 0; i <= pTermIdx; i++) {
        score += bm25(blockTf[mapping[i]][posting[mapping[i]]],
                      df[mapping[i]], totalDocs, docLen[pivot], avgDocLen);
      }

      insertHeap(elements, pivot, score);
      if(isFullHeap(elements)) {
        threshold = minScoreHeap(elements);
      }

      int atermIdx;
      for(atermIdx = 0; atermIdx < MIN(pTermIdx + 1, len); atermIdx++) {
        int aterm = mapping[atermIdx];

        if(posting[aterm] >= counts[aterm] - 1 &&
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

        if(blockChanged[aterm]) {
          counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
          decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
          posting[aterm] = 0;
          blockChanged[aterm] = 0;
        }

        while(blockDocid[aterm][posting[aterm]] <= pivot) {
          posting[aterm]++;
          if(posting[aterm] > counts[aterm] - 1) {
            long newPointer = nextPointer(pool, startPointers[aterm]);
            if(newPointer == UNDEFINED_POINTER) {
              break;
            } else {
              startPointers[aterm] = newPointer;
              counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
              decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
              posting[aterm] = 0;
            }
          }
        }
      }
    } else {
      int aterm = mapping[0];
      int atermIdx;
      for(atermIdx = 0; atermIdx < MIN(pTermIdx + 1, len); atermIdx++) {
        if(df[mapping[atermIdx]] <= df[aterm] &&
           blockDocid[mapping[atermIdx]][posting[mapping[atermIdx]]] < pivot) {
          int atermTemp = mapping[atermIdx];

          if(posting[atermTemp] >= counts[atermTemp] - 1 &&
             nextPointer(pool, startPointers[atermTemp]) == UNDEFINED_POINTER) {
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
          aterm = atermTemp;
        }
      }

      if(blockChanged[aterm]) {
        counts[aterm] = decompressDocidBlock(pool, blockDocid[aterm], startPointers[aterm]);
        decompressTfBlock(pool, blockTf[aterm], startPointers[aterm]);
        posting[aterm] = 0;
        blockChanged[aterm] = 0;
      }

      while(blockDocid[aterm][posting[aterm]] < pivot) {
        posting[aterm]++;
        if(posting[aterm] > counts[aterm] - 1) {
          long newPointer = nextPointer(pool, startPointers[aterm]);
          if(newPointer == UNDEFINED_POINTER) {
            break;
          } else {
            startPointers[aterm] = newPointer;
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

  // Free the allocated memory
  free(posting);
  free(mapping);
  free(blockChanged);
  for(i = 0; i < origLen; i++) {
    free(blockDocid[i]);
    free(blockTf[i]);
  }
  free(blockDocid);
  free(blockTf);
  free(blockMaxScores);
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
