#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "pfordelta/opt_p4.h"
#include "dictionary/Dictionary.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "util/ParseCommandLine.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "Config.h"
#include "InvertedIndex.h"
#include "intersection/SvS.h"

int main (int argc, char** args) {
  char* inputPath = getValueCL(argc, args, "-index");
  char* queryPath = getValueCL(argc, args, "-query");
  char* outputPath = getValueCL(argc, args, "-output");
  char* intersectionAlgorithm = getValueCL(argc, args, "-algorithm");

  int* (*intersect)(PostingsPool* pool, long* startPointers, int len, int minDf);
  if(!strcmp(intersectionAlgorithm, "SvS")) {
    intersect = &intersectSvS;
  } else {
    printf("Invalid algorithm (Options: SvS)\n");
    return;
  }

  InvertedIndex* index = readInvertedIndex(inputPath);

  //Read queries
  FixedIntCounter* queryLength = createFixedIntCounter(32768, 0);
  FixedIntCounter* idToIndexMap = createFixedIntCounter(32768, 0);
  FILE* fp = fopen(queryPath, "r");
  int totalQueries = 0, id, qlen, fqlen, j, pos, termid, i;
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
      termid = getTermId(index->dictionary, query);
      if(termid >= 0) {
        if(getDf(index->pointers, termid) > DF_CUTOFF) {
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

    qdf[0] = getDf(index->pointers, queries[qindex][0]);
    unsigned int minimumDf = qdf[0];
    for(i = 1; i < qlen; i++) {
      qdf[i] = getDf(index->pointers, queries[qindex][i]);
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
      qStartPointers[i] = getStartPointer(index->pointers,
                                          queries[qindex][sortedDfIndex[i]]);
    }

    int* set = intersect(index->pool, qStartPointers, qlen, minimumDf);

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
  free(queries);
  destroyFixedIntCounter(queryLength);
  destroyFixedIntCounter(idToIndexMap);
  destroyInvertedIndex(index);
  return 0;
}
