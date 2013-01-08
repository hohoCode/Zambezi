#ifndef BLOOM_INDEX_H_GUARD
#define BLOOM_INDEX_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PostingsPool.h"
#include "Config.h"
#include "bloom/BloomFilter.h"

typedef struct BloomIndex BloomIndex;

struct BloomIndex {
  unsigned int** filter;
  unsigned int* length;
  unsigned int capacity;

  unsigned int bitsPerElement;
  unsigned int nbHash;
};

void writeBloomIndex(BloomIndex* buffer, FILE* fp) {
  fwrite(&buffer->nbHash, sizeof(unsigned int), 1, fp);
  fwrite(&buffer->bitsPerElement, sizeof(unsigned int), 1, fp);
  fwrite(&buffer->capacity, sizeof(unsigned int), 1, fp);

  int i;
  for(i = 0; i < buffer->capacity; i++) {
    if(buffer->filter[i]) {
      fwrite(&i, sizeof(int), 1, fp);
      fwrite(&buffer->length[i], sizeof(unsigned int), 1, fp);
      fwrite(buffer->filter[i], sizeof(unsigned int), buffer->length[i], fp);
    }
  }
  i = -1;
  fwrite(&i, sizeof(int), 1, fp);
}

BloomIndex* readBloomIndex(FILE* fp) {
  BloomIndex* buffer = (BloomIndex*) malloc(sizeof(BloomIndex));
  fread(&buffer->nbHash, sizeof(unsigned int), 1, fp);
  fread(&buffer->bitsPerElement, sizeof(unsigned int), 1, fp);
  fread(&buffer->capacity, sizeof(unsigned int), 1, fp);

  buffer->length = (unsigned int*) calloc(buffer->capacity, sizeof(unsigned int));
  buffer->filter = (unsigned int**) malloc(buffer->capacity * sizeof(unsigned int*));
  int i;
  for(i = 0; i < buffer->capacity; i++) {
    buffer->filter[i] = NULL;
  }

  unsigned int length;
  while(1) {
    fread(&i, sizeof(int), 1, fp);
    if(i < 0) break;
    fread(&length, sizeof(unsigned int), 1, fp);
    buffer->filter[i] = (unsigned int*) calloc(length, sizeof(unsigned int));
    fread(buffer->filter[i], sizeof(unsigned int), length, fp);
    buffer->length[i] = length;
  }
  return buffer;
}

BloomIndex* createBloomIndex(unsigned int initialSize, unsigned int bitsPerElement,
                             unsigned int nbHash) {
  BloomIndex* buffer = (BloomIndex*) malloc(sizeof(BloomIndex));
  buffer->capacity = initialSize;
  buffer->filter = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->length = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->bitsPerElement = bitsPerElement;
  buffer->nbHash = nbHash;
  return buffer;
}

void destroyBloomIndex(BloomIndex* buffer) {
  int i;
  for(i = 0; i < buffer->capacity; i++) {
    if(buffer->filter[i]) {
      free(buffer->filter[i]);
    }
  }
  free(buffer->filter);
  free(buffer->length);
  free(buffer);
}

void expandBloomIndex(BloomIndex* buffer) {
  unsigned int** tempFilter = (unsigned int**) realloc(buffer->filter,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int* tempLength = (unsigned int*) realloc(buffer->length,
      buffer->capacity * 2 * sizeof(unsigned int));

  int i;
  for(i = buffer->capacity; i < buffer->capacity * 2; i++) {
    tempFilter[i] = NULL;
    tempLength[i] = 0;
  }

  buffer->filter = tempFilter;
  buffer->length = tempLength;
  buffer->capacity *= 2;
}

void createBloomFilterInIndex(BloomIndex* buffer, int termid, int df) {
  if(termid >= buffer->capacity) {
    expandBloomIndex(buffer);
  }

  buffer->length[termid] = computeBloomFilterLength(df, buffer->bitsPerElement);
  buffer->filter[termid] = (unsigned int*) calloc(buffer->length[termid],
                                                  sizeof(unsigned int));
}

void insertIntoBloomFilterInIndex(BloomIndex* buffer, int termid, int docid) {
  insertIntoBloomFilter(buffer->filter[termid], buffer->length[termid],
                        buffer->nbHash, docid);
}

// Performs a membership test on the Bloom filter of termid
int containsElementInIndex(BloomIndex* buffer, int termid, int docid) {
  return containsBloomFilter(buffer->filter[termid], buffer->length[termid],
                             buffer->nbHash, docid);
}

#endif
