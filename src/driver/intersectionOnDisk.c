#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "pfordelta/opt_p4.h"
#include "dictionary/Dictionary.h"
#include "dictionary/Vocab.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "PostingsPoolOnDisk.h"

#define TERMINAL_DOCID -1
#define DF_CUTOFF 9
#define INDEX_FILE "index"
#define POINTER_FILE "pointers"
#define DICTIONARY_FILE "dictionary"

int* intersectPostingsLists(PostingsPoolOnDisk* pool, long a, long b, int minDf) {
  int* set = (int*) calloc(minDf, sizeof(int));
  unsigned int* dataA = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  unsigned int* dataB = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));

  int cA = decompressBlockOnDisk(pool, dataA, a);
  int cB = decompressBlockOnDisk(pool, dataB, b);
  int iSet = 0, iA = 0, iB = 0;

  while(a != UNDEFINED_POINTER && b != UNDEFINED_POINTER) {
    if(dataB[iB] == dataA[iA]) {
      set[iSet++] = dataA[iA];
      iA++;
      iB++;
    }

    if(iA == cA) {
      a = nextPointerOnDisk(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cA = decompressBlockOnDisk(pool, dataA, a);
      iA = 0;
    }
    if(iB == cB) {
      b = nextPointerOnDisk(pool, b);
      if(b == UNDEFINED_POINTER) {
        break;
      }
      memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cB = decompressBlockOnDisk(pool, dataB, b);
      iB = 0;
    }

    if(dataA[iA] < dataB[iB]) {
      if(dataA[cA - 1] < dataB[iB]) {
        iA = cA - 1;
      }
      while(dataA[iA] < dataB[iB]) {
        iA++;
        if(iA == cA) {
          a = nextPointerOnDisk(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cA = decompressBlockOnDisk(pool, dataA, a);
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
          b = nextPointerOnDisk(pool, b);
          if(b == UNDEFINED_POINTER) {
            break;
          }
          memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cB = decompressBlockOnDisk(pool, dataB, b);
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

int intersectSetPostingsList(PostingsPoolOnDisk* pool, long a, int* currentSet, int len) {
  unsigned int* data = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  int c = decompressBlockOnDisk(pool, data, a);
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
      a = nextPointerOnDisk(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      c = decompressBlockOnDisk(pool, data, a);
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
          a = nextPointerOnDisk(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          c = decompressBlockOnDisk(pool, data, a);
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

int* intersect(PostingsPoolOnDisk* pool, long* startPointers, int len, int minDf) {
  if(len < 2) {
    unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    int* set = (int*) calloc(minDf, sizeof(int));
    int iSet = 0;
    long t = startPointers[0];
    while(t != UNDEFINED_POINTER) {
      memset(block, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      int c = decompressBlockOnDisk(pool, block, t);
      memcpy(&set[iSet], block, c * sizeof(int));
      iSet += c;
      t = nextPointerOnDisk(pool, t);
    }
    free(block);
    return set;
  }

  int* set = intersectPostingsLists(pool, startPointers[0], startPointers[1], minDf);
  int i;
  for(i = 2; i < len; i++) {
    intersectSetPostingsList(pool, startPointers[i], set, minDf);
  }
  return set;
}

int main (int argc, char** args) {
  char* inputPath = args[1];
  char* queryPath = args[2];
  char* outputPath = NULL;
  if(argc == 4) {
    outputPath = args[3];
  }

  char dicPath[1024];
  strcpy(dicPath, inputPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* fp = fopen(dicPath, "rb");
  Dictionary* dic = readDictionary(fp);
  fclose(fp);

  char indexPath[1024];
  strcpy(indexPath, inputPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  FILE* fpIndex = fopen(indexPath, "rb");
  PostingsPoolOnDisk* pool = readPostingsPoolOnDisk(fpIndex);

  FixedIntCounter* df = createFixedIntCounter(DEFAULT_VOCAB_SIZE, 0);
  FixedLongCounter* startPointers =
    createFixedLongCounter(DEFAULT_VOCAB_SIZE, UNDEFINED_POINTER);
  char pointerPath[1024];
  strcpy(pointerPath, inputPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  fp = fopen(pointerPath, "rb");
  unsigned int size = 0;
  fread(&size, sizeof(unsigned int), 1, fp);
  int i, term, value;
  long pointer;
  for(i = 0; i < size; i++) {
    fread(&term, sizeof(int), 1, fp);
    fread(&value, sizeof(int), 1, fp);
    fread(&pointer, sizeof(long), 1, fp);
    setFixedIntCounter(df, term, value);
    setFixedLongCounter(startPointers, term, pointer);
  }
  fclose(fp);

  //Read queries
  FixedIntCounter* queryLength = createFixedIntCounter(32768, 0);
  FixedIntCounter* idToIndexMap = createFixedIntCounter(32768, 0);
  fp = fopen(queryPath, "r");
  int totalQueries = 0, id, qlen, fqlen, j, pos, termid;
  char query[1024];
  fscanf(fp, "%d", &totalQueries);
  unsigned int** queries = (unsigned int**) malloc(totalQueries * sizeof(unsigned int*));
  for(i = 0; i < totalQueries; i++) {
    fscanf(fp, "%d %d", &id, &qlen);
    queries[i] = (unsigned int*) malloc(qlen * sizeof(unsigned int));
    pos = 0;
    fqlen = qlen;
    for(j = 0; j < qlen; j++) {
      fscanf(fp, "%s", query);
      termid = getDictionary(dic, query, strlen(query));
      if(termid >= 0) {
        if(getFixedIntCounter(df, termid) > DF_CUTOFF) {
          queries[i][pos++] = termid;
        } else {
          fqlen--;
        }
      } else {
        fqlen--;
      }
    }
    setFixedIntCounter(idToIndexMap, id, i);
    setFixedIntCounter(queryLength, id, fqlen);
  }
  fclose(fp);

  if(outputPath) {
    fp = fopen(outputPath, "w");
  }

  //query evaluation
  id = -1;
  while((id = nextIndexFixedIntCounter(queryLength, id)) != -1) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    qlen = queryLength->counter[id];
    int qindex = idToIndexMap->counter[id];

    unsigned int* qdf = (unsigned int*) calloc(qlen, sizeof(unsigned int));
    int* sortedDfIndex = (int*) calloc(qlen, sizeof(int));
    long* qStartPointers = (long*) calloc(qlen, sizeof(long));

    qdf[0] = getFixedIntCounter(df, queries[qindex][0]);
    unsigned int minimumDf = qdf[0];
    for(i = 1; i < qlen; i++) {
      qdf[i] = getFixedIntCounter(df, queries[qindex][i]);
      if(qdf[i] < minimumDf) {
        minimumDf = qdf[i];
      }
    }

    for(i = 0; i < qlen; i++) {
      unsigned int minDf = 0xFFFFFFFF;
      for(j = 0; j < qlen; j++) {
        if(qdf[j] < minDf) {
          minDf = qdf[j];
          sortedDfIndex[i] = j;
        }
      }
      qdf[sortedDfIndex[i]] = 0xFFFFFFFF;
    }

    for(i = 0; i < qlen; i++) {
      qStartPointers[i] = getFixedLongCounter(startPointers,
                                              queries[qindex][sortedDfIndex[i]]);
    }

    int* set = intersect(pool, qStartPointers, qlen, minimumDf);

    if(outputPath) {
      for(i = 0; i < minimumDf && set[i] != TERMINAL_DOCID; i++) {
        fprintf(fp, "q: %d no: %u\n", id, set[i]);
      }
    }

    free(set);
    free(qdf);
    free(sortedDfIndex);
    free(qStartPointers);

    gettimeofday(&end, NULL);
    printf("%10.0f length: %d\n",
           ((float) ((end.tv_sec * 1000000 + end.tv_usec) -
                     (start.tv_sec * 1000000 + start.tv_usec))), qlen);
    fflush(stdout);
  }
  //end query evaluation

  if(outputPath) {
    fclose(fp);
  }
  for(i = 0; i < totalQueries; i++) {
    if(queries[i]) {
      free(queries[i]);
    }
  }

  fclose(fpIndex);
  free(queries);
  destroyFixedIntCounter(queryLength);
  destroyFixedIntCounter(idToIndexMap);
  destroyDictionary(dic);
  free(pool);
  destroyFixedIntCounter(df);
  destroyFixedLongCounter(startPointers);
  return 0;
}
