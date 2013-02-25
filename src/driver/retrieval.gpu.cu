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

#define THREADS_PER_BLOCK 512 
#define THREADS_PER_BLOCK_GLOBALPAIRS 64
#define LINEARBLOCK 100

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
  //unsigned int** queries = (unsigned int**) malloc(totalQueries * sizeof(unsigned int*));
  unsigned int* linearQ = (unsigned int*) malloc(100 * totalQueries * sizeof(unsigned int));
  int* linearQ_count = (unsigned int*) malloc(totalQueries * sizeof(unsigned int));

  int totalLen = 0;
  for(i = 0; i < totalQueries; i++) {
    fscanf(fp, "%d %d", &id, &qlen);
    //queries[i] = (unsigned int*) malloc(qlen * sizeof(unsigned int));
    pos = 0;
    fqlen = qlen;
    for(j = 0; j < qlen; j++) {
      fscanf(fp, "%s", query);
      termid = getTermId(index->dictionary, query);
      if(termid >= 0) {
        if(getStartPointer(index->pointers, termid) != UNDEFINED_POINTER) {
			linearQ[totalLen] = termid;
			totalLen++;
          //queries[i][pos++] = termid;
        } else {
          fqlen--;
        }
      } else {
        fqlen--;
      }
    }
    setFixedIntCounter(idToIndexMap, id, i);
    setFixedIntCounter(queryLength, id, fqlen);
	linearQ_count[i] = totalLen;
  }
  fclose(fp);

  if(outputPath) {
    fp = fopen(outputPath, "w");
  }

  // Evaluate queries by iterating over the queries that are not empty
  id = -1;

/////////////////////// CUDA Entry
  SvS_GPU_Entry(
  	queryLength, 
  	idToIndexMap, 
  	outputPath, 
  	index, 
  	fp, 
  	totalQueries,
  	linearQ,
  	linearQ_count,
  	totalLen);
//////////////////////

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

__device__ int decompressDocidBlock_GPU(int* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int* block = &pool[pOffset + 5];
  detailed_p4_decode(outBlock, block, aux, 1);

  return pool[pOffset + 3];
}

__device__ long nextPointer_GPU(int* pool, long pointer) {
  if(pointer == UNDEFINED_POINTER) {
    return UNDEFINED_POINTER;
  }
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  if(pool[pOffset + 1] == UNKNOWN_SEGMENT) {
    return UNDEFINED_POINTER;
  }

  return ENCODE_POINTER(pool[pOffset + 1],
                        pool[pOffset + 2]);
}

__device__ int* intersectPostingsLists_SvS_GPU(int* pool, long a, long b, int minDf) {
  int* set = (int*) calloc(minDf, sizeof(int));
  unsigned int* dataA = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  unsigned int* dataB = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));

  int cA = decompressDocidBlock_GPU(pool, dataA, a);
  int cB = decompressDocidBlock_GPU(pool, dataB, b);
  int iSet = 0, iA = 0, iB = 0;

  while(a != UNDEFINED_POINTER && b != UNDEFINED_POINTER) {
    if(dataB[iB] == dataA[iA]) {
      set[iSet++] = dataA[iA];
      iA++;
      iB++;
    }

    if(iA == cA) {
      a = nextPointer_GPU(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cA = decompressDocidBlock_GPU(pool, dataA, a);
      iA = 0;
    }
    if(iB == cB) {
      b = nextPointer_GPU(pool, b);
      if(b == UNDEFINED_POINTER) {
        break;
      }
      memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cB = decompressDocidBlock_GPU(pool, dataB, b);
      iB = 0;
    }

    if(dataA[iA] < dataB[iB]) {
      if(dataA[cA - 1] < dataB[iB]) {
        iA = cA - 1;
      }
      while(dataA[iA] < dataB[iB]) {
        iA++;
        if(iA == cA) {
          a = nextPointer_GPU(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cA = decompressDocidBlock_GPU(pool, dataA, a);
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
          b = nextPointer_GPU(pool, b);
          if(b == UNDEFINED_POINTER) {
            break;
          }
          memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cB = decompressDocidBlock_GPU(pool, dataB, b);
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

__device__ int intersectSetPostingsList_SvS_GPU(int* pool, long a, int* currentSet, int len) {
  unsigned int* data = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  int c = decompressDocidBlock_GPU(pool, data, a);
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
      a = nextPointer_GPU(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      c = decompressDocidBlock_GPU(pool, data, a);
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
          a = nextPointer_GPU(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          c = decompressDocidBlock_GPU(pool, data, a);
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

__device__ int* intersectSvS_GPU(int* pool, long* startPointers, int len, int minDf) {
  if(len < 2) {
    unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
    int* set = (int*) calloc(minDf, sizeof(int));
    int iSet = 0;
    long t = startPointers[0];
    while(t != UNDEFINED_POINTER) {
      memset(block, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      int c = decompressDocidBlock_GPU(pool, block, t);
      memcpy(&set[iSet], block, c * sizeof(int));
      iSet += c;
      t = nextPointer_GPU(pool, t);
    }
    free(block);
    return set;
  }

  int* set = intersectPostingsLists_SvS_GPU(pool, startPointers[0], startPointers[1], minDf);
  int i;
  for(i = 2; i < len; i++) {
    intersectSetPostingsList_SvS_GPU(pool, startPointers[i], set, minDf);
  }
  return set;
}

__global__ void SvS_GPU(
	int* queryLength_counter,
	unsigned int queryLength_vocabSize,
	DefaultValue queryLength_defaultValue,	
	int* idToIndexMap_counter,
	unsigned int idToIndexMap_vocabSize,
	DefaultValue idToIndexMap_defaultValue,		
	int* index_df_counter,
	unsigned int index_df_vocabSize,
	DefaultValue index_df_defaultValue,		
	long* index_pointer_counter,
	unsigned int index_pointer_vocabSize,
	DefaultValue index_pointer_defaultValue,	
	int* index_pool_firstseg, //index->pool->pool[0]
	unsigned int index_pool_offset,
	unsigned int index_pool_segment,	
	unsigned int* linearQ,
	int* linearQ_count,
	int totalQuery){

	int id = threadIdx.x + THREADS_PER_BLOCK * blockIdx.x;
	if(id >= queryLength_vocabSize) {
	  return;
	}
	
	if(queryLength_counter[id] == queryLength_defaultValue) {
	  return;
	}

	  //printf("id = %d\n", id);
	  // Measure elapsed time
	  int i, j;
	  int qlen = queryLength_counter[id];
	  int qindex = idToIndexMap_counter[id];
	  if (qindex > totalQuery){
	  	printf("Exceed the range!\n");
		return;
	  }
	  
	  unsigned int* qdf = (unsigned int*) calloc(qlen, sizeof(unsigned int));
	  int* sortedDfIndex = (int*) calloc(qlen, sizeof(int));
	  long* qStartPointers = (long*) calloc(qlen, sizeof(long));
	  int end = linearQ_count[qindex];
	  int start = 0;
	  if (qindex > 0){
		start = linearQ_count[qindex-1];
	  }
	  
	  if (linearQ[start]>= index_df_vocabSize ){
		printf("DF range exceeded\n");
		return;
	  }
	  qdf[0] = index_df_counter[linearQ[start]];//getDf(index->pointers, queries[qindex][0]);
	  unsigned int minimumDf = qdf[0];
	  for(i = 1; i < qlen; i++) {
	  	if(start+i > end){
			printf("out of range 1 \n");
			return;
	  	}
		  if (linearQ[start+i]>= index_df_vocabSize ){
			printf("DF range exceeded - Inside Loop - Not possible!\n");
			return;
		  }
		qdf[i] = index_df_counter[linearQ[start+i]];//getDf(index->pointers, queries[qindex][i]);
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
	  	if(start+sortedDfIndex[i] > end){
			printf("out of range 2\n");
			return;
	  	}
		if (linearQ[start+sortedDfIndex[i]]>= index_pointer_vocabSize){
			printf("Pointer range exceeded - Inside Second Loop - Not possible!\n");
			return;
		}
		qStartPointers[i] = index_pointer_counter[linearQ[start+sortedDfIndex[i]]]; //getStartPointer(index->pointers, queries[qindex][sortedDfIndex[i]]);
		if (linearQ[start+sortedDfIndex[i]]>= index_df_vocabSize ){
			printf("DF range exceeded - Inside Second Loop - Not possible!\n");
			return;
		}
		qdf[i] = index_df_counter[linearQ[start+sortedDfIndex[i]]];
		//qdf[i] = getDf(index->pointers, queries[qindex][sortedDfIndex[i]]);
	  }
	
	  // Compute intersection set (or in disjunctive mode, top-k)
	  int* set;	  
	  int hits = minimumDf;
	  set = intersectSvS_GPU(index_pool_firstseg, qStartPointers, qlen, minimumDf);
	  	
	  // If output is specified, write the retrieved set to output
	  /*if(outputPath) {
		printf("Output\n");
		for(i = 0; i < hits && set[i] != TERMINAL_DOCID; i++) {
		  fprintf(fp, "q: %d no: %u\n", id, set[i]);
		}
	  } else {*/
		for(i = 0; i < hits && set[i] != TERMINAL_DOCID; i++) {
			printf("q: %d no: %u\n", id, set[i]);
		}
	  //}
	
	  // Free the allocated memory
	  free(set);
	  free(qdf);
	  free(sortedDfIndex);
	  free(qStartPointers);
}

void SvS_GPU_Entry(
	FixedIntCounter* queryLength, 
	FixedIntCounter* idToIndexMap, 
	char* outputPath, 
	InvertedIndex* index, 
	FILE * fp,
	int totalQuery,
	unsigned int* linearQ,
	int* linearQ_count,
	int tt){
	
	int i, j;
	int id = -1;
	int fqlen, pos, termid;	
	int hits = 1000;
	Algorithm algorithm = SVS;

	//printf("INside\n");
	if(queryLength==NULL || idToIndexMap == NULL || outputPath == NULL || queries == NULL || index == NULL || fp == NULL){
		printf("NULLL\n");
	}	

	fprintf(stderr, "Start SvS Data Transfer\n");

	struct timeval transferstart, transferend, gpustart, gpuend;
	gettimeofday(&transferstart, NULL);
	int* queryLength_counter;
	int* idToIndexMap_counter;
	int* index_df_counter;
	long* index_pointer_counter;
	int* index_pool_firstseg;
	unsigned int* linearQ_cuda;
	int* linearQ_count_cuda;

	cudaMalloc((void**)&(queryLength_counter), 32768*sizeof(int));
	cudaMalloc((void**)&(idToIndexMap_counter), 32768*sizeof(int));
	cudaMalloc((void**)&(index_df_counter), DEFAULT_VOCAB_SIZE*sizeof(int));
	cudaMalloc((void**)&(index_pointer_counter), DEFAULT_VOCAB_SIZE*sizeof(long));
	cudaMalloc((void**)&(index_pool_firstseg), index->pool->offset*sizeof(int));	
	cudaMalloc((void**)&(linearQ_cuda), tt*sizeof(unsigned int));
	cudaMalloc((void**)&(linearQ_count_cuda), totalQuery*sizeof(int));

	cudaMemcpy(queryLength_counter, queryLength->counter, 32768*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(idToIndexMap_counter, idToIndexMap->counter, 32768*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(index_df_counter, index->pointers->df->counter, DEFAULT_VOCAB_SIZE*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(index_pointer_counter, index->pointers->startPointers->counter, DEFAULT_VOCAB_SIZE*sizeof(long), cudaMemcpyHostToDevice);
	cudaMemcpy(index_pool_firstseg, index->pool->pool[0], index->pool->offset*sizeof(int), cudaMemcpyHostToDevice);

	cudaMemcpy(linearQ_cuda, linearQ, tt*sizeof(unsigned int), cudaMemcpyHostToDevice);
	cudaMemcpy(linearQ_count_cuda, linearQ_count, totalQuery*sizeof(int), cudaMemcpyHostToDevice);

	gettimeofday(&transferend, NULL);

	gettimeofday(&gpustart, NULL);
	dim3  block(THREADS_PER_BLOCK, 1);
	dim3  grid((totalQuery + THREADS_PER_BLOCK - 1)/THREADS_PER_BLOCK, 1);

	SvS_GPU<<<grid, block>>>(	
		queryLength_counter,
		queryLength->vocabSize,//queryLength_vocabSize,
		queryLength->defaultValue,//queryLength_defaultValue,	
		idToIndexMap_counter,
		idToIndexMap->vocabSize,
		idToIndexMap->defaultValue,		
		index_df_counter,
		index->pointers->df->vocabSize,//_df_vocabSize,
		index->pointers->df->defaultValue,		
		index_pointer_counter,
		index->pointers->startPointers->vocabSize,
		index->pointers->startPointers->defaultValue,
		index_pool_firstseg, //index->pool->pool[0]
		index->pool->offset,
		index->pool->segment,
		linearQ_cuda,
		linearQ_count_cuda,
		totalQuery);

	gettimeofday(&gpuend, NULL);

	printf("Transfer Timing: %10.0f\n",
		   ((float) ((transferend.tv_sec * 1000000 + transferend.tv_usec) -
					 (transferstart.tv_sec * 1000000 + transferstart.tv_usec))));
	printf("GPU Timing: %10.0f\n",
		   ((float) ((gpuend.tv_sec * 1000000 + gpuend.tv_usec) -
					 (gpustart.tv_sec * 1000000 + gpustart.tv_usec))));

}



