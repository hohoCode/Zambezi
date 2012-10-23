#ifndef DYNAMIC_BUFFER_H_GUARD
#define DYNAMIC_BUFFER_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "dictionary/Hash.h"

#define BUFFER_LOAD_FACTOR 0.75
#define TRUE 1
#define FALSE 0

typedef struct DynamicBuffer DynamicBuffer;

struct DynamicBuffer {
  unsigned char* used;
  unsigned int* key;
  unsigned int** value;
  unsigned int* valueLength;
  unsigned int capacity;
  unsigned int size;
  unsigned int mask;
};

DynamicBuffer* createDynamicBuffer(unsigned int initialSize) {
  DynamicBuffer* buffer = (DynamicBuffer*)
    malloc(sizeof(DynamicBuffer));
  buffer->used = (unsigned char*) malloc(initialSize * sizeof(unsigned char));
  memset(buffer->used, FALSE, initialSize);
  buffer->value = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->size = 0;
  buffer->mask = initialSize - 1;
  buffer->capacity = initialSize;

  buffer->key = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->valueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  return buffer;
}

void destroyDynamicBuffer(DynamicBuffer* buffer) {
  free(buffer->used);
  free(buffer->key);
  int i;
  for(i = 0; i < buffer->capacity; i++) {
    if(buffer->value[i]) {
      free(buffer->value[i]);
    }
  }
  free(buffer->value);
  free(buffer->valueLength);
  free(buffer);
}

void shallowDestroyDynamicBuffer(DynamicBuffer* buffer) {
  free(buffer->used);
  free(buffer->key);
  free(buffer->value);
  free(buffer->valueLength);
  free(buffer);
}

DynamicBuffer* expandDynamicBuffer(DynamicBuffer* buffer) {
  DynamicBuffer* copy = createDynamicBuffer(buffer->capacity * 2);
  copy->size = buffer->size;

  int i = 0, j, pos, k;
  for(j = buffer->size; j-- != 0;) {
    while(!buffer->used[i]) i++;
    pos = murmurHash3Int(buffer->key[i]) & copy->mask;
    while(copy->used[pos]) {
      pos = (pos + 1) & copy->mask;
    }
    copy->key[pos] = buffer->key[i];
    copy->value[pos] = buffer->value[i];
    copy->valueLength[pos] = buffer->valueLength[i];
    copy->used[pos] = TRUE;
    i++;
  }
  shallowDestroyDynamicBuffer(buffer);
  return copy;
}

int containsKeyDynamicBuffer(DynamicBuffer* buffer, int k) {
  int pos = murmurHash3Int(k) & buffer->mask;
  while(buffer->used[pos]) {
    if(buffer->key[pos] == k) {
      return TRUE;
    }
    pos = (pos + 1) & buffer->mask;
  }
  return FALSE;
}

int getDynamicBuffer(DynamicBuffer* buffer, int k, int** out) {
  int pos = murmurHash3Int(k) & buffer->mask;
  while(buffer->used[pos]) {
    if(buffer->key[pos] == k) {
      *out = buffer->value[pos];
      return buffer->valueLength[pos];
    }
    pos = (pos + 1) & buffer->mask;
  }
  return -1;
}

void putDynamicBuffer(DynamicBuffer** buffer, int k, int* v, int vlen) {
  int pos = murmurHash3Int(k) & (*buffer)->mask;
  while((*buffer)->used[pos]) {
    if((*buffer)->key[pos] == k) break;
    pos = (pos + 1) & (*buffer)->mask;
  }
  if(!(*buffer)->used[pos]) {
    (*buffer)->size++;
  } else {
    free((*buffer)->value[pos]);
  }
  (*buffer)->used[pos] = TRUE;
  (*buffer)->key[pos] = k;
  (*buffer)->value[pos] = v;
  (*buffer)->valueLength[pos] = vlen;

  if((*buffer)->size > BUFFER_LOAD_FACTOR * (*buffer)->capacity) {
    DynamicBuffer* temp = expandDynamicBuffer((*buffer));
    *buffer = temp;
  }
}

#endif
