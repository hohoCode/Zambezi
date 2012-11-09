#ifndef POSTINGS_POOL_H_GUARD
#define POSTINGS_POOL_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pfordelta/opt_p4.h"

#define MAX_INT_VALUE ((unsigned int) 0xFFFFFFFF)
#define UNDEFINED_POINTER -1l
#define UNKNOWN_SEGMENT -1

#define DECODE_SEGMENT(P) ((int) (P >> 32))
#define DECODE_OFFSET(P) ((unsigned int) (P & 0xFFFFFFFF))
#define ENCODE_POINTER(S, O) ((((unsigned long) S)<<32) | (unsigned int) O)

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
                    unsigned int* tf, unsigned int* positions,
                    unsigned int len, unsigned int plen, long tailPointer) {
  int lastSegment = -1;
  unsigned int lastOffset = 0;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  int pblocksize = 3 * ((plen / BLOCK_SIZE) + 1 ) * BLOCK_SIZE;
  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* tfblock = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* pblock = (unsigned int*) calloc(pblocksize, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);
  unsigned int tfcsize = OPT4(tf, len, tfblock, 0);

  // compressing positions
  unsigned int pcsize = 0;
  int nb = plen / BLOCK_SIZE;
  int res = plen % BLOCK_SIZE;
  int i;

  for(i = 0; i < nb; i++) {
    int tempPcsize = OPT4(&positions[i * BLOCK_SIZE], BLOCK_SIZE, &pblock[pcsize+1], 0);
    pblock[pcsize] = tempPcsize;
    pcsize += tempPcsize + 1;
  }

  if(res > 0) {
    int tempPcsize = OPT4(&positions[nb * BLOCK_SIZE], res, &pblock[pcsize+1], 0);
    pblock[pcsize] = tempPcsize;
    pcsize += tempPcsize + 1;
    i++;
  }
  // end compressing positions

  int reqspace = csize + tfcsize + pcsize + 8;
  if(reqspace > (MAX_INT_VALUE - pool->offset)) {
    pool->segment++;
    pool->offset = 0;
  }

  pool->pool[pool->segment][pool->offset] = reqspace;
  pool->pool[pool->segment][pool->offset + 1] = UNKNOWN_SEGMENT;
  pool->pool[pool->segment][pool->offset + 2] = 0;
  pool->pool[pool->segment][pool->offset + 3] = len;
  pool->pool[pool->segment][pool->offset + 4] = csize;

  memcpy(&pool->pool[pool->segment][pool->offset + 5],
         block, csize * sizeof(int));

  pool->pool[pool->segment][pool->offset + 5 + csize] = tfcsize;
  memcpy(&pool->pool[pool->segment][pool->offset + 6 + csize],
         tfblock, tfcsize * sizeof(int));

  pool->pool[pool->segment][pool->offset + 6 + csize + tfcsize] = plen;
  pool->pool[pool->segment][pool->offset + 7 + csize + tfcsize] = i;
  memcpy(&pool->pool[pool->segment][pool->offset + 8 + csize + tfcsize],
         pblock, pcsize * sizeof(int));

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 1] = pool->segment;
    pool->pool[lastSegment][lastOffset + 2] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += reqspace;

  free(block);
  free(tfblock);
  free(pblock);

  return newPointer;
}

long nextPointer(PostingsPool* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  if(pool->pool[pSegment][pOffset + 1] == UNKNOWN_SEGMENT) {
    return UNDEFINED_POINTER;
  }

  return ENCODE_POINTER(pool->pool[pSegment][pOffset + 1],
                        pool->pool[pSegment][pOffset + 2]);
}

int decompressDocidBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int* block = &pool->pool[pSegment][pOffset + 5];
  detailed_p4_decode(outBlock, block, aux, 1);

  return pool->pool[pSegment][pOffset + 3];
}

#endif
