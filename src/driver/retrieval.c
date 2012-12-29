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
#include "intersection/WAND.h"

#ifndef RETRIEVAL_ALGO_ENUM_GUARD
#define RETRIEVAL_ALGO_ENUM_GUARD
typedef enum Algorithm Algorithm;
enum Algorithm {
  SVS = 0,
  WAND = 1
};
#endif

int main (int argc, char** args) {
  // Index path
  char* inputPath = getValueCL(argc, args, "-index");
  // Query path
  char* queryPath = getValueCL(argc, args, "-query");
  // Output path (optional)
  char* outputPath = getValueCL(argc, args, "-output");
  // Hits
  int hits = 1000;
  if(isPresentCL(argc, args, "-hits")) {
    hits = atoi(getValueCL(argc, args, "-hits"));
  }
  // Algorithm
  char* intersectionAlgorithm = getValueCL(argc, args, "-algorithm");
  Algorithm algorithm = SVS;

  // Algorithm is limited to the following list (case sensitive):
  // - SvS (conjunctive)
  // - WAND (disjunctive)
  if(!strcmp(intersectionAlgorithm, "SvS")) {
    algorithm = SVS;
  } else if(!strcmp(intersectionAlgorithm, "WAND")) {
    algorithm = WAND;
  } else {
    printf("Invalid algorithm (Options: SvS | WAND)\n");
    return;
  }

  // Read the inverted index
  InvertedIndex* index = readInvertedIndex(inputPath);

  // Read queries. Query file must be in the following format:
  // - First line: <number of queries: integer>
  // - <query id: integer> <query length: integer> <query text: string>
  // Note that, if a query term does not have a corresponding postings list,
  // then we drop the query term from the query. Empty queries are not evaluated.
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
        if(getStartPointer(index->pointers, termid) != UNDEFINED_POINTER) {
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

  // Evaluate queries by iterating over the queries that are not empty
  id = -1;
  while((id = nextIndexFixedIntCounter(queryLength, id)) != -1) {
    // Measure elapsed time
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

    // Sort query terms w.r.t. df
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
      qdf[i] = getDf(index->pointers, queries[qindex][sortedDfIndex[i]]);
    }

    // Compute intersection set (or in disjunctive mode, top-k)
    int* set;
    if(algorithm == SVS) {
      set = intersectSvS(index->pool, qStartPointers, qlen, minimumDf);
    } else if(algorithm == WAND) {
      float* UB = (float*) malloc(qlen * sizeof(float));
      for(i = 0; i < qlen; i++) {
        int tf = getMaxTf(index->pointers, queries[qindex][sortedDfIndex[i]]);
        int dl = getMaxTfDocLen(index->pointers, queries[qindex][sortedDfIndex[i]]);
        UB[i] = bm25(tf, qdf[i],
                     index->pointers->totalDocs, dl,
                     index->pointers->totalDocLen /
                     ((float) index->pointers->totalDocs));
      }
      set = wand(index->pool, qStartPointers, qdf, UB, qlen,
                 index->pointers->docLen->counter,
                 index->pointers->totalDocs,
                 index->pointers->totalDocLen / (float) index->pointers->totalDocs,
                 hits);
      free(UB);
    }

    // If output is specified, write the retrieved set to output
    if(outputPath) {
      for(i = 0; i < minimumDf && set[i] != TERMINAL_DOCID; i++) {
        fprintf(fp, "q: %d no: %u\n", id, set[i]);
      }
    }

    // Free the allocated memory
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
