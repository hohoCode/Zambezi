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
  unsigned int** value;
  unsigned int* valueLength;
  unsigned int* valuePosition;
  unsigned long* tailPointer;
  unsigned int capacity;
};

DynamicBuffer* createDynamicBuffer(unsigned int initialSize) {
  DynamicBuffer* buffer = (DynamicBuffer*)
    malloc(sizeof(DynamicBuffer));
  buffer->value = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->capacity = initialSize;

  buffer->valueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->valuePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->tailPointer = (unsigned long*) calloc(initialSize, sizeof(unsigned long));
  return buffer;
}

void destroyDynamicBuffer(DynamicBuffer* buffer) {
  int i;
  for(i = 0; i < buffer->capacity; i++) {
    if(buffer->value[i]) {
      free(buffer->value[i]);
    }
  }
  free(buffer->value);
  free(buffer->valueLength);
  free(buffer->valuePosition);
  free(buffer->tailPointer);
  free(buffer);
}

void expandDynamicBuffer(DynamicBuffer* buffer) {
  unsigned int** tempValue = (unsigned int**) realloc(buffer->value,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int* tempValueLength = (unsigned int*) realloc(buffer->valueLength,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned int* tempValuePosition = (unsigned int*) realloc(buffer->valuePosition,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned long* tempTailPointer = (unsigned long*) realloc(buffer->tailPointer,
      buffer->capacity * 2 * sizeof(unsigned long));
  int i;
  for(i = buffer->capacity; i < buffer->capacity * 2; i++) {
    tempValue[i] = NULL;
    tempValueLength[i] = 0;
    tempValuePosition[i] = 0;
    tempTailPointer[i] = UNDEFINED_POINTER;
  }

  buffer->capacity *= 2;
  buffer->value = tempValue;
  buffer->valueLength = tempValueLength;
  buffer->valuePosition = tempValuePosition;
  buffer->tailPointer = tempTailPointer;
}

int containsKeyDynamicBuffer(DynamicBuffer* buffer, int k) {
  return buffer->value[k] != NULL;
}

int* getDynamicBuffer(DynamicBuffer* buffer, int k) {
  while(k > buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  return buffer->value[k];
}

void putDynamicBuffer(DynamicBuffer* buffer, int k, int* v, int vlen) {
  while(k > buffer->capacity) {
    expandDynamicBuffer(buffer);
  }

  buffer->value[k] = v;
  buffer->valueLength[k] = vlen;
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
