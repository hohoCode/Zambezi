#ifndef POSTINGS_POOL_H_GUARD
#define POSTINGS_POOL_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pfordelta/opt_p4.h"
#include "bloom/BloomFilter.h"

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

  // if Bloom filters enabled
  int bloomEnabled;
  unsigned int nbHash;
  unsigned int bitsPerElement;
};

void writePostingsPool(PostingsPool* pool, FILE* fp) {
  fwrite(&pool->segment, sizeof(unsigned int), 1, fp);
  fwrite(&pool->offset, sizeof(unsigned int), 1, fp);
  fwrite(&pool->bloomEnabled, sizeof(int), 1, fp);
  fwrite(&pool->nbHash, sizeof(unsigned int), 1, fp);
  fwrite(&pool->bitsPerElement, sizeof(unsigned int), 1, fp);

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
  fread(&pool->bloomEnabled, sizeof(int), 1, fp);
  fread(&pool->nbHash, sizeof(unsigned int), 1, fp);
  fread(&pool->bitsPerElement, sizeof(unsigned int), 1, fp);

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

PostingsPool* createPostingsPool(int numberOfPools, int bloomEnabled,
                                 int nbHash, int bitsPerElement) {
  PostingsPool* pool = (PostingsPool*) malloc(sizeof(PostingsPool));
  pool->pool = (int**) malloc(numberOfPools * sizeof(int*));
  int i;
  for(i = 0; i < numberOfPools; i++) {
    pool->pool[i] = (int*) calloc(MAX_INT_VALUE, sizeof(int));
  }
  pool->segment = 0;
  pool->offset = 0;
  pool->numberOfPools = numberOfPools;
  pool->bloomEnabled = bloomEnabled;
  pool->nbHash = nbHash;
  pool->bitsPerElement = bitsPerElement;
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

  // Construct a Bloom filter if required
  unsigned int* filter = 0;
  unsigned int filterSize = 0;
  if(pool->bloomEnabled) {
    filterSize = computeBloomFilterLength(len, pool->bitsPerElement);
    filter = (unsigned int*) calloc(filterSize, sizeof(unsigned int));
    int i;
    for(i = 0; i < len; i++) {
      insertIntoBloomFilter(filter, filterSize, pool->nbHash, data[i]);
    }
  }

  unsigned int maxDocId = data[len - 1];
  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);

  int reqspace = csize + filterSize + 8;
  if(reqspace > (MAX_INT_VALUE - pool->offset)) {
    pool->segment++;
    pool->offset = 0;
  }

  pool->pool[pool->segment][pool->offset] = reqspace;
  pool->pool[pool->segment][pool->offset + 1] = UNKNOWN_SEGMENT;
  pool->pool[pool->segment][pool->offset + 2] = 0;
  pool->pool[pool->segment][pool->offset + 3] = maxDocId;
  // where Bloom filters are stored, if stored at all
  pool->pool[pool->segment][pool->offset + 4] = csize + 7;
  pool->pool[pool->segment][pool->offset + 5] = len;
  pool->pool[pool->segment][pool->offset + 6] = csize;

  memcpy(&pool->pool[pool->segment][pool->offset + 7],
         block, csize * sizeof(int));

  if(filter) {
    pool->pool[pool->segment][pool->offset + csize + 7] = filterSize;
    memcpy(&pool->pool[pool->segment][pool->offset + csize + 8],
           filter, filterSize * sizeof(int));
  }

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 1] = pool->segment;
    pool->pool[lastSegment][lastOffset + 2] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += reqspace;

  free(block);
  if(filter) free(filter);
  return newPointer;
}

long compressAndAddTfOnly(PostingsPool* pool, unsigned int* data,
    unsigned int* tf, unsigned int len, long tailPointer) {
  int lastSegment = -1;
  unsigned int lastOffset = 0;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  // Construct a Bloom filter if required
  unsigned int* filter = 0;
  unsigned int filterSize = 0;
  if(pool->bloomEnabled) {
    filterSize = computeBloomFilterLength(len, pool->bitsPerElement);
    filter = (unsigned int*) calloc(filterSize, sizeof(unsigned int));
    int i;
    for(i = 0; i < len; i++) {
      insertIntoBloomFilter(filter, filterSize, pool->nbHash, data[i]);
    }
  }

  unsigned int maxDocId = data[len - 1];
  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* tfblock = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);
  unsigned int tfcsize = OPT4(tf, len, tfblock, 0);

  int reqspace = csize + tfcsize + filterSize + 9;
  if(reqspace > (MAX_INT_VALUE - pool->offset)) {
    pool->segment++;
    pool->offset = 0;
  }

  pool->pool[pool->segment][pool->offset] = reqspace;
  pool->pool[pool->segment][pool->offset + 1] = UNKNOWN_SEGMENT;
  pool->pool[pool->segment][pool->offset + 2] = 0;
  pool->pool[pool->segment][pool->offset + 3] = maxDocId;
  pool->pool[pool->segment][pool->offset + 4] = csize + tfcsize + 8;
  pool->pool[pool->segment][pool->offset + 5] = len;
  pool->pool[pool->segment][pool->offset + 6] = csize;

  memcpy(&pool->pool[pool->segment][pool->offset + 7],
         block, csize * sizeof(int));

  pool->pool[pool->segment][pool->offset + 7 + csize] = tfcsize;
  memcpy(&pool->pool[pool->segment][pool->offset + 8 + csize],
         tfblock, tfcsize * sizeof(int));

  if(filter) {
    pool->pool[pool->segment][pool->offset + csize + tfcsize + 8] = filterSize;
    memcpy(&pool->pool[pool->segment][pool->offset + 9 + csize + tfcsize],
           filter, filterSize * sizeof(int));
  }

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 1] = pool->segment;
    pool->pool[lastSegment][lastOffset + 2] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += reqspace;

  free(block);
  free(tfblock);
  if(filter) free(filter);

  return newPointer;
}

long compressAndAddPositional(PostingsPool* pool, unsigned int* data,
    unsigned int* tf, unsigned int* positions,
    unsigned int len, unsigned int plen, long tailPointer) {
  int lastSegment = -1;
  unsigned int lastOffset = 0;
  if(tailPointer != UNDEFINED_POINTER) {
    lastSegment = DECODE_SEGMENT(tailPointer);
    lastOffset = DECODE_OFFSET(tailPointer);
  }

  // Construct a Bloom filter if required
  unsigned int* filter = 0;
  unsigned int filterSize = 0;
  if(pool->bloomEnabled) {
    filterSize = computeBloomFilterLength(len, pool->bitsPerElement);
    filter = (unsigned int*) calloc(filterSize, sizeof(unsigned int));
    int i;
    for(i = 0; i < len; i++) {
      insertIntoBloomFilter(filter, filterSize, pool->nbHash, data[i]);
    }
  }

  unsigned int maxDocId = data[len - 1];
  int pblocksize = 3 * ((plen / BLOCK_SIZE) + 1) * BLOCK_SIZE;
  unsigned int* block = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* tfblock = (unsigned int*) calloc(BLOCK_SIZE*2, sizeof(unsigned int));
  unsigned int* pblock = (unsigned int*) calloc(pblocksize, sizeof(unsigned int));
  unsigned int csize = OPT4(data, len, block, 1);
  unsigned int tfcsize = OPT4(tf, len, tfblock, 0);

  // compressing positions
  unsigned int pcsize = 0;
  int nb = plen / BLOCK_SIZE;
  int res = plen % BLOCK_SIZE;
  int i = 0;

  for(i = 0; i < nb; i++) {
    int tempPcsize = OPT4(&positions[i * BLOCK_SIZE], BLOCK_SIZE, &pblock[pcsize+1], 0);
    pblock[pcsize] = tempPcsize;
    pcsize += tempPcsize + 1;
  }

  if(res > 0) {
    unsigned int* a = (unsigned int*) calloc(BLOCK_SIZE, sizeof(unsigned int));
    memcpy(a, &positions[nb * BLOCK_SIZE], res * sizeof(unsigned int));
    int tempPcsize = OPT4(a, res, &pblock[pcsize+1], 0);
    pblock[pcsize] = tempPcsize;
    pcsize += tempPcsize + 1;
    i++;
    free(a);
  }
  // end compressing positions

  int reqspace = csize + tfcsize + pcsize + filterSize + 11;
  if(reqspace > (MAX_INT_VALUE - pool->offset)) {
    pool->segment++;
    pool->offset = 0;
  }

  pool->pool[pool->segment][pool->offset] = reqspace;
  pool->pool[pool->segment][pool->offset + 1] = UNKNOWN_SEGMENT;
  pool->pool[pool->segment][pool->offset + 2] = 0;
  pool->pool[pool->segment][pool->offset + 3] = maxDocId;
  pool->pool[pool->segment][pool->offset + 4] = csize + tfcsize + pcsize + 10;
  pool->pool[pool->segment][pool->offset + 5] = len;
  pool->pool[pool->segment][pool->offset + 6] = csize;

  memcpy(&pool->pool[pool->segment][pool->offset + 7],
         block, csize * sizeof(int));

  pool->pool[pool->segment][pool->offset + 7 + csize] = tfcsize;
  memcpy(&pool->pool[pool->segment][pool->offset + 8 + csize],
         tfblock, tfcsize * sizeof(int));

  pool->pool[pool->segment][pool->offset + 8 + csize + tfcsize] = plen;
  pool->pool[pool->segment][pool->offset + 9 + csize + tfcsize] = i;
  memcpy(&pool->pool[pool->segment][pool->offset + 10 + csize + tfcsize],
         pblock, pcsize * sizeof(int));

  if(filter) {
    pool->pool[pool->segment][pool->offset + csize + tfcsize + pcsize + 10] = filterSize;
    memcpy(&pool->pool[pool->segment][pool->offset + 11 + csize + tfcsize + pcsize],
         filter, filterSize * sizeof(int));
  }

  if(lastSegment >= 0) {
    pool->pool[lastSegment][lastOffset + 1] = pool->segment;
    pool->pool[lastSegment][lastOffset + 2] = pool->offset;
  }

  long newPointer = ENCODE_POINTER(pool->segment, pool->offset);
  pool->offset += reqspace;

  free(block);
  free(tfblock);
  free(pblock);
  if(filter) free(filter);

  return newPointer;
}

/**
 * Given the current pointer, this function returns
 * the next pointer. If the current pointer points to
 * the last block (i.e., there is no "next" block),
 * then this function returns UNDEFINED_POINTER.
 */
long nextPointer(PostingsPool* pool, long pointer) {
  if(pointer == UNDEFINED_POINTER) {
    return UNDEFINED_POINTER;
  }
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
  unsigned int* block = &pool->pool[pSegment][pOffset + 7];
  detailed_p4_decode(outBlock, block, aux, 1);

  return pool->pool[pSegment][pOffset + 5];
}

int decompressTfBlock(PostingsPool* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int csize = pool->pool[pSegment][pOffset + 6];
  unsigned int* block = &pool->pool[pSegment][pOffset + csize + 8];
  detailed_p4_decode(outBlock, block, aux, 0);

  return pool->pool[pSegment][pOffset + 5];
}

/**
 * Retrieved the number of positions stored in the block
 * pointed to by "pointer".
 */
int numberOfPositionBlocks(PostingsPool* pool, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int csize = pool->pool[pSegment][pOffset + 6];
  unsigned int tfsize = pool->pool[pSegment][pOffset + 7 + csize];
  return pool->pool[pSegment][pOffset + csize + tfsize + 9];
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
  unsigned int csize = pool->pool[pSegment][pOffset + 6];
  unsigned int tfsize = pool->pool[pSegment][pOffset + 7 + csize];
  unsigned int nb = pool->pool[pSegment][pOffset + csize + tfsize + 9];

  int i;
  unsigned int index = pOffset + csize + tfsize + 10;
  for(i = 0; i < nb; i++) {
    unsigned int sb = pool->pool[pSegment][index];
    unsigned int* block = &pool->pool[pSegment][index + 1];
    detailed_p4_decode(&outBlock[i * BLOCK_SIZE], block, aux, 0);
    memset(aux, 0, BLOCK_SIZE * 4 * sizeof(unsigned int));
    index += sb + 1;
  }
  return pool->pool[pSegment][pOffset + csize + tfsize + 8];
}

int containsDocid(PostingsPool* pool, unsigned int docid, long* pointer) {
  if(*pointer == UNDEFINED_POINTER) {
    return 0;
  }
  int pSegment = DECODE_SEGMENT(*pointer);
  unsigned int pOffset = DECODE_OFFSET(*pointer);

  while(pool->pool[pSegment][pOffset + 3] < docid) {
    int nSegment = pool->pool[pSegment][pOffset + 1];
    int nOffset = pool->pool[pSegment][pOffset + 2];
    pSegment = nSegment;
    pOffset = nOffset;
    if(pSegment == UNKNOWN_SEGMENT) {
      (*pointer) = UNDEFINED_POINTER;
      return 0;
    }
  }

  if(pool->pool[pSegment][pOffset + 3] == docid) {
    return 1;
  }

  unsigned int bloomOffset = pool->pool[pSegment][pOffset + 4];
  (*pointer) == ENCODE_POINTER(pSegment, pOffset);
  return containsBloomFilter(&pool->pool[pSegment][pOffset + bloomOffset + 1],
                             pool->pool[pSegment][pOffset + bloomOffset],
                             pool->nbHash, docid);
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
    long pos = ((pSegment * (unsigned long) MAX_INT_VALUE) + pOffset) * 4 + 20;

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

void readBloomStats(FILE* fp, int* bloomEnabled,
                    unsigned int* nbHash, unsigned int* bitsPerElement) {
  unsigned int temp;
  fread(&temp, sizeof(unsigned int), 1, fp);
  fread(&temp, sizeof(unsigned int), 1, fp);
  fread(bloomEnabled, sizeof(int), 1, fp);
  fread(nbHash, sizeof(unsigned int), 1, fp);
  fread(bitsPerElement, sizeof(unsigned int), 1, fp);
}

#endif
