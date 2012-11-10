#ifndef DYNAMIC_BUFFER_H_GUARD
#define DYNAMIC_BUFFER_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "dictionary/Hash.h"
#include "PostingsPool.h"

#define BUFFER_LOAD_FACTOR 0.75
#define TRUE 1
#define FALSE 0

typedef struct DynamicBuffer DynamicBuffer;

struct DynamicBuffer {
  unsigned int** docid;
  unsigned int** tf;
  unsigned int** position;
  unsigned long* tailPointer;
  unsigned int* valueLength;
  unsigned int* valuePosition;
  unsigned int* pvalueLength;
  unsigned int* pvaluePosition;
  unsigned int capacity;
};

DynamicBuffer* createDynamicBuffer(unsigned int initialSize) {
  DynamicBuffer* buffer = (DynamicBuffer*)
    malloc(sizeof(DynamicBuffer));
  buffer->docid = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->tf = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->position = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->capacity = initialSize;

  buffer->valueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->valuePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->pvalueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->pvaluePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->tailPointer = (unsigned long*) calloc(initialSize, sizeof(unsigned long));
  return buffer;
}

void destroyDynamicBuffer(DynamicBuffer* buffer) {
  int i;
  for(i = 0; i < buffer->capacity; i++) {
    if(buffer->docid[i]) {
      free(buffer->docid[i]);
      free(buffer->tf[i]);
      free(buffer->position[i]);
    }
  }
  free(buffer->docid);
  free(buffer->tf);
  free(buffer->position);
  free(buffer->valueLength);
  free(buffer->valuePosition);
  free(buffer->pvalueLength);
  free(buffer->pvaluePosition);
  free(buffer->tailPointer);
  free(buffer);
}

void expandDynamicBuffer(DynamicBuffer* buffer) {
  unsigned int** tempDocid = (unsigned int**) realloc(buffer->docid,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int** tempTf = (unsigned int**) realloc(buffer->tf,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int** tempPosition = (unsigned int**) realloc(buffer->position,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int* tempValueLength = (unsigned int*) realloc(buffer->valueLength,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned int* tempValuePosition = (unsigned int*) realloc(buffer->valuePosition,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned int* tempPValueLength = (unsigned int*) realloc(buffer->pvalueLength,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned int* tempPValuePosition = (unsigned int*) realloc(buffer->pvaluePosition,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned long* tempTailPointer = (unsigned long*) realloc(buffer->tailPointer,
      buffer->capacity * 2 * sizeof(unsigned long));
  int i;
  for(i = buffer->capacity; i < buffer->capacity * 2; i++) {
    tempDocid[i] = NULL;
    tempTf[i] = NULL;
    tempPosition[i] = NULL;
    tempValueLength[i] = 0;
    tempValuePosition[i] = 0;
    tempPValueLength[i] = 0;
    tempPValuePosition[i] = 0;
    tempTailPointer[i] = UNDEFINED_POINTER;
  }

  buffer->capacity *= 2;
  buffer->docid = tempDocid;
  buffer->tf = tempTf;
  buffer->position = tempPosition;
  buffer->valueLength = tempValueLength;
  buffer->valuePosition = tempValuePosition;
  buffer->pvalueLength = tempPValueLength;
  buffer->pvaluePosition = tempPValuePosition;
  buffer->tailPointer = tempTailPointer;
}

int containsKeyDynamicBuffer(DynamicBuffer* buffer, int k) {
  return buffer->docid[k] != NULL;
}

int* getDocidDynamicBuffer(DynamicBuffer* buffer, int k) {
  while(k > buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->docid[k];
}

int* getTfDynamicBuffer(DynamicBuffer* buffer, int k) {
  while(k > buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->tf[k];
}

int* getPositionDynamicBuffer(DynamicBuffer* buffer, int k) {
  while(k > buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->position[k];
}

int nextIndexDynamicBuffer(DynamicBuffer* buffer, int pos, int minLength) {
  pos++;
  while(buffer->valueLength[pos] < minLength) {
    pos++;
    if(pos >= buffer->capacity) {
      return -1;
    }
  }
  return pos;
}

#endif
