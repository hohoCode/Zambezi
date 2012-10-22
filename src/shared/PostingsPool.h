#ifndef POSTINGS_POOL_H_GUARD
#define POSTINGS_POOL_H_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfordelta/opt_p4.h"

#define MAX_INT_VALUE ((unsigned int) 0xFFFFFFFF)
#define UNDEFINED_POINTER -1l
#define LAST_SEGMENT -1

#define DECODE_SEGMENT(P) ((unsigned int) (P >> 32))
#define DECODE_OFFSET(P) ((unsigned int) (P & 0xFFFFFFFF))
#define ENCODE_POINTER(S, O) ((((long) S)<<32) | (long) O)

typedef unsigned int SinglePool[MAX_INT_VALUE];
typedef struct PostingsPool PostingsPool;

struct PostingsPool {
  unsigned int segment;
  unsigned int offset;
  SinglePool* pool;
};

void writePostingsPool(PostingsPool* pool, FILE* fp) {
  fwrite(&pool->segment, sizeof(unsigned int), 1, fp);
  fwrite(&pool->offset, sizeof(unsigned int), 1, fp);

  int i;
  for(i = 0; i <= segment; i++) {
    fwrite(pool->pool[i], sizeof(unsigned int), MAX_INT_VALUE, fp);
  }
}

PostingsPool* readPostingsPool(FILE* fp) {
  PostingsPool* pool = (PostingsPool*) malloc(sizeof(PostingsPool));
  fread(&pool->segment, sizeof(unsigned int), 1, fp);
  fread(&pool->offset, sizeof(unsigned int), 1, fp);

  pool->pool = (SinglePool*) malloc(segment * sizeof(SinglePool));
  int i;
  for(i = 0; i <= segment; i++) {
    fread(pool->pool[i], sizeof(unsigned int), MAX_INT_VALUE, fp);
  }
  return pool;
}

PostingsPool* createPostingsPool(int numberOfPools) {
  PostingsPool* pool = (PostingsPool*) malloc(sizeof(PostingsPool));
  pool->pool = (SinglePool*) malloc(numberOfPools * sizeof(SinglePool));
  pool->segment = 0;
  pool->offset = 0;
  return pool;
}

void destroyPostingsPool(PostingsPool* pool) {
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

  unsigned int block[BLOCK_SIZE];
  int csize = OPT4(data, BLOCK_SIZE, block) / 4;

  if(pool->offset != 0 && csize + 4 > (MAX_INT_VALUE - pool->offset + 1)) {
    pool->segment++;
    pool->offset = 0;
  }

  pool->pool[pool->segment][pool->offset] = csize;
  pool->pool[pool->segment][pool->offset + 1] = len;
  pool->pool[pool->segment][pool->offset + 2] = LAST_SEGMENT;
  pool->pool[pool->segment][pool->offset + 3] = LAST_SEGMENT;
  memcpy(&pool->pool[pool->segment][pool->offset + 4],
         block, csize * sizeof(unsigned int));

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 2] = pool->segment;
    pool->pool[lastSegment][lastOffset + 3] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += (csize + 4);
  return newPointer;
}

long nextPointer(PostingsPool* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  int pOffset = DECODE_OFFSET(pointer);
  return ENCODE_POINTER(pool->pool[pSegment][pOffset + 2], pool->pool[pSegment][pOffset + 3]);
}

int decompressBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE];
  unsigned int* block = &pool->pool[pSegment][pOffset + 4];
  detailed_p4_decode(outBlock, block, aux);

  return pool->pool[pSegment][pOffset + 1];
}

#endif
