#ifndef POSTINGS_POOL_H_GUARD
#define POSTINGS_POOL_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pfordelta/opt_p4.h"

#define MAX_INT_VALUE ((unsigned int) 0xFFFFFFFF)
#define UNDEFINED_POINTER -1l
#define LAST_SEGMENT -1

#define DECODE_SEGMENT(P) ((int) (P >> 32))
#define DECODE_OFFSET(P) ((int) (P & 0xFFFFFFFF))
#define ENCODE_POINTER(S, O) ((((long) S)<<32) | (long) O)

typedef struct PostingsPool PostingsPool;

struct PostingsPool {
  unsigned int numberOfPools;
  unsigned int segment;
  unsigned int offset;
  int** pool;
};

void writePostingsPool(PostingsPool* pool, FILE* fp) {
  fwrite(&pool->segment, sizeof(unsigned int), 1, fp);
  fwrite(&pool->offset, sizeof(unsigned int), 1, fp);

  int i;
  for(i = 0; i < pool->segment; i++) {
    fwrite(pool->pool[i], sizeof(int), MAX_INT_VALUE, fp);
  }
  fwrite(pool->pool[pool->segment], sizeof(int), pool->offset, fp);
}

PostingsPool* readPostingsPool(FILE* fp) {
  PostingsPool* pool = (PostingsPool*) malloc(sizeof(PostingsPool));
  fread(&pool->segment, sizeof(unsigned int), 1, fp);
  fread(&pool->offset, sizeof(unsigned int), 1, fp);

  pool->pool = (int**) malloc((pool->segment + 1) * sizeof(int*));
  int i;
  for(i = 0; i < pool->segment; i++) {
    pool->pool[i] = (int*) calloc(MAX_INT_VALUE, sizeof(int));
    fread(pool->pool[i], sizeof(int), MAX_INT_VALUE, fp);
  }
  pool->pool[pool->segment] = (int*) calloc(MAX_INT_VALUE, sizeof(int));
  fread(pool->pool[pool->segment], sizeof(int), pool->offset, fp);
  return pool;
}

PostingsPool* createPostingsPool(int numberOfPools) {
  PostingsPool* pool = (PostingsPool*) malloc(sizeof(PostingsPool));
  pool->pool = (int**) malloc(numberOfPools * sizeof(int*));
  int i;
  for(i = 0; i < numberOfPools; i++) {
    pool->pool[i] = (int*) calloc(MAX_INT_VALUE, sizeof(int));
  }
  pool->segment = 0;
  pool->offset = 0;
  pool->numberOfPools = numberOfPools;
  return pool;
}

void destroyPostingsPool(PostingsPool* pool) {
  int i;
  for(i = 0; i < pool->numberOfPools; i++) {
    free(pool->pool[i]);
  }
  free(pool->pool);
  free(pool);
}

long compressAndAdd(PostingsPool* pool, unsigned int* data,
                    unsigned int len, long tailPointer) {
  int lastSegment = -1;
  int lastOffset = -1;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block);

  if((csize + 4) > (MAX_INT_VALUE - pool->offset)) {
    pool->segment++;
    pool->offset = 0;
  }

  pool->pool[pool->segment][pool->offset] = csize;
  pool->pool[pool->segment][pool->offset + 1] = len;
  pool->pool[pool->segment][pool->offset + 2] = LAST_SEGMENT;
  pool->pool[pool->segment][pool->offset + 3] = LAST_SEGMENT;
  memcpy(&pool->pool[pool->segment][pool->offset + 4],
         block, csize * sizeof(int));

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 2] = pool->segment;
    pool->pool[lastSegment][lastOffset + 3] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += (csize + 4);
  free(block);
  return newPointer;
}

long nextPointer(PostingsPool* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  int pOffset = DECODE_OFFSET(pointer);
  if(pSegment == LAST_SEGMENT) {
    return UNDEFINED_POINTER;
  }

  return ENCODE_POINTER(pool->pool[pSegment][pOffset + 2], pool->pool[pSegment][pOffset + 3]);
}

int decompressBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*2];
  unsigned int* block = &pool->pool[pSegment][pOffset + 4];
  detailed_p4_decode(outBlock, block, aux);

  return pool->pool[pSegment][pOffset + 1];
}

#endif
