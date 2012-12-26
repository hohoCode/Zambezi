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

long compressAndAddNonPositional(PostingsPool* pool, unsigned int* data,
                                 unsigned int len, long tailPointer) {
  int lastSegment = -1;
  unsigned int lastOffset = 0;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);

  int reqspace = csize + 5;
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

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 1] = pool->segment;
    pool->pool[lastSegment][lastOffset + 2] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += reqspace;

  free(block);
  return newPointer;
}

long compressAndAddTfOnly(PostingsPool* pool, unsigned int* data,
    unsigned short* tf, unsigned int len, long tailPointer) {
  int lastSegment = -1;
  unsigned int lastOffset = 0;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* tfblock = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);
  unsigned int tfcsize = OPT4Short(tf, len, tfblock, 0);

  int reqspace = csize + tfcsize + 6;
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

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 1] = pool->segment;
    pool->pool[lastSegment][lastOffset + 2] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += reqspace;

  free(block);
  free(tfblock);

  return newPointer;
}

long compressAndAddPositional(PostingsPool* pool, unsigned int* data,
    unsigned short* tf, unsigned short* positions,
    unsigned int len, unsigned int plen, long tailPointer) {
  int lastSegment = -1;
  unsigned int lastOffset = 0;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  int pblocksize = 3 * ((plen / BLOCK_SIZE) + 1) * BLOCK_SIZE;
  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* tfblock = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* pblock = (unsigned int*) calloc(pblocksize, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);
  unsigned int tfcsize = OPT4Short(tf, len, tfblock, 0);

  // compressing positions
  unsigned int pcsize = 0;
  int nb = plen / BLOCK_SIZE;
  int res = plen % BLOCK_SIZE;
  int i = 0;

  for(i = 0; i < nb; i++) {
    int tempPcsize = OPT4Short(&positions[i * BLOCK_SIZE], BLOCK_SIZE, &pblock[pcsize+1], 0);
    pblock[pcsize] = tempPcsize;
    pcsize += tempPcsize + 1;
  }

  if(res > 0) {
    unsigned short* a = (unsigned short*) calloc(BLOCK_SIZE, sizeof(unsigned short));
    memcpy(a, &positions[nb * BLOCK_SIZE], res * sizeof(unsigned short));
    int tempPcsize = OPT4Short(a, res, &pblock[pcsize+1], 0);
    pblock[pcsize] = tempPcsize;
    pcsize += tempPcsize + 1;
    i++;
    free(a);
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

/**
 * Given the current pointer, this function returns
 * the next pointer. If the current pointer points to
 * the last block (i.e., there is no "next" block),
 * then this function returns UNDEFINED_POINTER.
 */
long nextPointer(PostingsPool* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  if(pool->pool[pSegment][pOffset + 1] == UNKNOWN_SEGMENT) {
    return UNDEFINED_POINTER;
  }

  return ENCODE_POINTER(pool->pool[pSegment][pOffset + 1],
                        pool->pool[pSegment][pOffset + 2]);
}

/**
 * Decompresses the docid block from the segment pointed to by "pointer,"
 * into the "outBlock" buffer. Block size is 128.
 *
 * Note that outBlock must be at least 128 integers long.
 */
int decompressDocidBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int* block = &pool->pool[pSegment][pOffset + 5];
  detailed_p4_decode(outBlock, block, aux, 1);

  return pool->pool[pSegment][pOffset + 3];
}

int decompressTfBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int csize = pool->pool[pSegment][pOffset + 4];
  unsigned int* block = &pool->pool[pSegment][pOffset + csize + 6];
  detailed_p4_decode(outBlock, block, aux, 0);

  return pool->pool[pSegment][pOffset + 3];
}

/**
 * Retrieved the number of positions stored in the block
 * pointed to by "pointer".
 */
int numberOfPositionBlocks(PostingsPool* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int csize = pool->pool[pSegment][pOffset + 4];
  unsigned int tfsize = pool->pool[pSegment][pOffset + 5 + csize];
  return pool->pool[pSegment][pOffset + csize + tfsize + 7];
}

/**
 * Decompressed the position block into the "outBlock."
 * Note that outBlock's length must be:
 *
 *     numberOfPositionBlocks() * BLOCK_SIZE,
 *
 * where BLOCK_SIZE is 128.
 */
int decompressPositionBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int csize = pool->pool[pSegment][pOffset + 4];
  unsigned int tfsize = pool->pool[pSegment][pOffset + 5 + csize];
  unsigned int nb = pool->pool[pSegment][pOffset + csize + tfsize + 7];

  int i;
  unsigned int index = pOffset + csize + tfsize + 8;
  for(i = 0; i < nb; i++) {
    unsigned int sb = pool->pool[pSegment][index];
    unsigned int* block = &pool->pool[pSegment][index + 1];
    detailed_p4_decode(&outBlock[i * BLOCK_SIZE], block, aux, 0);
    memset(aux, 0, BLOCK_SIZE * 4 * sizeof(unsigned int));
    index += sb + 1;
  }
  return pool->pool[pSegment][pOffset + csize + tfsize + 6];
}

/**
 * Reads postings for a term from an index stored on hard-disk,
 * and stores it into "pool."
 *
 * @param pointer StartPointer.
 */
long readPostingsForTerm(PostingsPool* pool, long pointer, FILE* fp) {
  int sSegment = -1, ppSegment = -1;
  unsigned int sOffset = 0, ppOffset = 0;
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  while(pSegment != UNKNOWN_SEGMENT) {
    long pos = ((pSegment * (unsigned long) MAX_INT_VALUE) + pOffset) * 4 + 8;

    fseek(fp, pos, SEEK_SET);
    int reqspace = 0;
    fread(&reqspace, sizeof(int), 1, fp);

    if(reqspace > (MAX_INT_VALUE - pool->offset)) {
      pool->segment++;
      pool->offset = 0;
    }

    pool->pool[pool->segment][pool->offset] = reqspace;
    fread(&pool->pool[pool->segment][pool->offset + 1], sizeof(unsigned int),
          reqspace - 1, fp);

    pSegment = pool->pool[pool->segment][pool->offset + 1];
    pOffset = (unsigned int) pool->pool[pool->segment][pool->offset + 2];

    if(ppSegment != -1) {
      pool->pool[ppSegment][ppOffset + 1] = pool->segment;
      pool->pool[ppSegment][ppOffset + 2] = pool->offset;
    }

    if(sSegment == -1) {
      sSegment = pool->segment;
      sOffset = pool->offset;
    }

    ppSegment = pool->segment;
    ppOffset = pool->offset;

    pool->offset += reqspace;
  }
  return ENCODE_POINTER(sSegment, sOffset);
}

#endif
