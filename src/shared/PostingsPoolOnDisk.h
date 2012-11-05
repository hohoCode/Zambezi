#ifndef POSTINGS_POOL_ONDISK_H_GUARD
#define POSTINGS_POOL_ONDISK_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pfordelta/opt_p4.h"
#include "PostingsPool.h"

#define COMPUTE_OFFSET(S, O) (((unsigned long) ((((unsigned long) S) * MAX_INT_VALUE) + O + 2)) * 4)

typedef struct PostingsPoolOnDisk PostingsPoolOnDisk;

struct PostingsPoolOnDisk {
  FILE* fp;
  unsigned int segment;
  unsigned int offset;
};

PostingsPoolOnDisk* readPostingsPoolOnDisk(FILE* fp) {
  PostingsPoolOnDisk* pool = (PostingsPoolOnDisk*) malloc(sizeof(PostingsPoolOnDisk));
  fread(&pool->segment, sizeof(unsigned int), 1, fp);
  fread(&pool->offset, sizeof(unsigned int), 1, fp);
  pool->fp = fp;
  return pool;
}

long nextPointerOnDisk(PostingsPoolOnDisk* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  fseek(pool->fp, COMPUTE_OFFSET(pSegment, pOffset + 2), SEEK_SET);
  int nSegment;
  fread(&nSegment, sizeof(int), 1, pool->fp);
  if(nSegment == UNKNOWN_SEGMENT) {
    return UNDEFINED_POINTER;
  }

  unsigned int nOffset;
  fread(&nOffset, sizeof(unsigned int), 1, pool->fp);
  return ENCODE_POINTER(nSegment, nOffset);
}

int decompressBlockOnDisk(PostingsPoolOnDisk* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];

  int r = fseek(pool->fp, COMPUTE_OFFSET(pSegment, pOffset), SEEK_SET);
  unsigned int csize = 0, len = 0, i;
  fread(&csize, sizeof(unsigned int), 1, pool->fp);

  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*4, sizeof(unsigned int));
  fread(&len, sizeof(int), 1, pool->fp);
  fseek(pool->fp, COMPUTE_OFFSET(pSegment, pOffset + 4), SEEK_SET);
  fread(block, sizeof(int), csize, pool->fp);

  detailed_p4_decode(outBlock, block, aux);
  free(block);

  return len;
}

#endif
