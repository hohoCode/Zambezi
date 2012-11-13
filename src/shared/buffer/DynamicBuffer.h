#ifndef DYNAMIC_BUFFER_H_GUARD
#define DYNAMIC_BUFFER_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "PostingsPool.h"

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

DynamicBuffer* createDynamicBuffer(unsigned int initialSize,
                                   int positional) {
  DynamicBuffer* buffer = (DynamicBuffer*)
    malloc(sizeof(DynamicBuffer));
  buffer->capacity = initialSize;
  buffer->docid = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->valueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->valuePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->tailPointer = (unsigned long*) calloc(initialSize, sizeof(unsigned long));

  if(positional) {
    buffer->tf = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
    buffer->position = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
    buffer->pvalueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
    buffer->pvaluePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  } else {
    buffer->tf = NULL;
    buffer->position = NULL;
    buffer->pvalueLength = NULL;
    buffer->pvaluePosition = NULL;
  }
  return buffer;
}

void destroyDynamicBuffer(DynamicBuffer* buffer) {
  int i;
  if(buffer->tf) {
    for(i = 0; i < buffer->capacity; i++) {
      if(buffer->docid[i]) {
        free(buffer->docid[i]);
        free(buffer->tf[i]);
        free(buffer->position[i]);
      }
    }
    free(buffer->tf);
    free(buffer->position);
    free(buffer->pvalueLength);
    free(buffer->pvaluePosition);
  } else {
    for(i = 0; i < buffer->capacity; i++) {
      if(buffer->docid[i]) {
        free(buffer->docid[i]);
      }
    }
  }

  free(buffer->docid);
  free(buffer->valueLength);
  free(buffer->valuePosition);
  free(buffer->tailPointer);
  free(buffer);
}

void expandDynamicBuffer(DynamicBuffer* buffer) {
  unsigned int** tempDocid = (unsigned int**) realloc(buffer->docid,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int* tempValueLength = (unsigned int*) realloc(buffer->valueLength,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned int* tempValuePosition = (unsigned int*) realloc(buffer->valuePosition,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned long* tempTailPointer = (unsigned long*) realloc(buffer->tailPointer,
      buffer->capacity * 2 * sizeof(unsigned long));

  int i;
  for(i = buffer->capacity; i < buffer->capacity * 2; i++) {
    tempDocid[i] = NULL;
    tempValueLength[i] = 0;
    tempValuePosition[i] = 0;
    tempTailPointer[i] = UNDEFINED_POINTER;
  }

  buffer->docid = tempDocid;
  buffer->valueLength = tempValueLength;
  buffer->valuePosition = tempValuePosition;
  buffer->tailPointer = tempTailPointer;

  if(buffer->tf) {
    unsigned int** tempTf = (unsigned int**) realloc(buffer->tf,
        buffer->capacity * 2 * sizeof(unsigned int*));
    unsigned int** tempPosition = (unsigned int**) realloc(buffer->position,
        buffer->capacity * 2 * sizeof(unsigned int*));
    unsigned int* tempPValueLength = (unsigned int*) realloc(buffer->pvalueLength,
        buffer->capacity * 2 * sizeof(unsigned int));
    unsigned int* tempPValuePosition = (unsigned int*) realloc(buffer->pvaluePosition,
        buffer->capacity * 2 * sizeof(unsigned int));

    int j;
    for(j = buffer->capacity; j < buffer->capacity * 2; j++) {
      tempTf[j] = NULL;
      tempPosition[j] = NULL;
      tempPValueLength[j] = 0;
      tempPValuePosition[j] = 0;
    }

    buffer->tf = tempTf;
    buffer->position = tempPosition;
    buffer->pvalueLength = tempPValueLength;
    buffer->pvaluePosition = tempPValuePosition;
  }

  buffer->capacity *= 2;
}

int containsKeyDynamicBuffer(DynamicBuffer* buffer, int k) {
  return buffer->docid[k] != NULL;
}

int* getDocidDynamicBuffer(DynamicBuffer* buffer, int k) {
  if(k >= buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->docid[k];
}

int* getTfDynamicBuffer(DynamicBuffer* buffer, int k) {
  if(k >= buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->tf[k];
}

int* getPositionDynamicBuffer(DynamicBuffer* buffer, int k) {
  if(k >= buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->position[k];
}

int nextIndexDynamicBuffer(DynamicBuffer* buffer, int pos, int minLength) {
  pos++;
  if(pos >= buffer->capacity) {
    return -1;
  }
  while(buffer->valueLength[pos] < minLength) {
    pos++;
    if(pos >= buffer->capacity) {
      return -1;
    }
  }
  return pos;
}

#endif
