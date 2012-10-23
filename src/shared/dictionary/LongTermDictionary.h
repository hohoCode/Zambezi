#ifndef LONG_TERM_DICTIONARY_H_GUARD
#define LONG_TERM_DICTIONARY_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "Vocab.h"
#include "Hash.h"

#define LONG_LOAD_FACTOR 0.75
#define TRUE 1
#define FALSE 0

#define MAX_LONGS 3
#define MAX_LONGS_LENGTH MAX_LONGS*LONG_CAPACITY

typedef struct LongTermDictionary LongTermDictionary;

struct LongTermDictionary {
  unsigned char* used;
  unsigned long* key;
  unsigned int* value;
  unsigned int capacity;
  unsigned int size;
  unsigned int mask;
};

void LTD_write(LongTermDictionary* dic, FILE* fp) {
  fwrite(&dic->size, sizeof(unsigned int), 1, fp);
  fwrite(&dic->mask, sizeof(unsigned int), 1, fp);
  fwrite(&dic->capacity, sizeof(unsigned int), 1, fp);

  int i = 0, j;
  for(j = dic->size; j-- != 0;) {
    while(!dic->used[i]) i++;

    fwrite(&i, sizeof(int), 1, fp);
    fwrite(&dic->value[i], sizeof(unsigned int), 1, fp);
    fwrite(&dic->key[i*MAX_LONGS], sizeof(unsigned long), MAX_LONGS, fp);
    i++;
  }
}

LongTermDictionary* LTD_read(FILE* fp) {
  LongTermDictionary* dic = (LongTermDictionary*)
    malloc(sizeof(LongTermDictionary));

  fread(&dic->size, sizeof(unsigned int), 1, fp);
  fread(&dic->mask, sizeof(unsigned int), 1, fp);
  fread(&dic->capacity, sizeof(unsigned int), 1, fp);

  dic->used = (unsigned char*) malloc(dic->capacity * sizeof(unsigned char));
  memset(dic->used, FALSE, dic->capacity);
  dic->value = (unsigned int*) calloc(dic->capacity, sizeof(unsigned int));
  dic->key = (unsigned long*) calloc(dic->capacity * MAX_LONGS,
                                     sizeof(unsigned long));

  int i = 0, j;
  for(j = dic->size; j-- != 0;) {
    fread(&i, sizeof(int), 1, fp);
    fread(&dic->value[i], sizeof(unsigned int), 1, fp);
    fread(&dic->key[i*MAX_LONGS], sizeof(unsigned long), MAX_LONGS, fp);
    dic->used[i] = TRUE;
  }

  return dic;
}

LongTermDictionary* LTD_create(unsigned int initialSize) {
  LongTermDictionary* dic = (LongTermDictionary*)
    malloc(sizeof(LongTermDictionary));
  dic->used = (unsigned char*) malloc(initialSize * sizeof(unsigned char));
  memset(dic->used, FALSE, initialSize);
  dic->value = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  dic->size = 0;
  dic->mask = initialSize - 1;
  dic->capacity = initialSize;

  dic->key = (unsigned long*) calloc(initialSize * MAX_LONGS,
                                     sizeof(unsigned long));
  return dic;
}

void LTD_destroy(LongTermDictionary* dic) {
  free(dic->used);
  free(dic->key);
  free(dic->value);
  free(dic);
}

LongTermDictionary* LTD_expand(LongTermDictionary* dic) {
  LongTermDictionary* copy = LTD_create(dic->capacity * 2);
  copy->size = dic->size;

  int i = 0, j, pos, k;
  for(j = dic->size; j-- != 0;) {
    while(!dic->used[i]) i++;
    pos = (int) murmurHash3(dic->key[i*MAX_LONGS]) & copy->mask;
    while(copy->used[pos]) {
      pos = (pos + 1) & copy->mask;
    }
    memcpy(&copy->key[pos*MAX_LONGS], &dic->key[i*MAX_LONGS],
           MAX_LONGS * sizeof(unsigned long));
    copy->value[pos] = dic->value[i];
    copy->used[pos] = TRUE;
    i++;
  }
  LTD_destroy(dic);
  return copy;
}

unsigned int LTD_putIfNotPresent(LongTermDictionary* dic,
                                 char* chars, unsigned int len,
                                 unsigned int id) {
  unsigned long codes[MAX_LONGS];
  codes[0] = encodeInLong(chars, len, 0);
  codes[1] = encodeInLong(chars, len, LONG_CAPACITY);
  codes[2] = encodeInLong(chars, len, LONG_CAPACITY * 2);

  int pos = (int) murmurHash3(codes[0]) & dic->mask;
  int kpos = pos * MAX_LONGS;
  while(dic->used[pos]) {
    if(dic->key[kpos] == codes[0]) {
      if(dic->key[kpos + 1] == codes[1]) {
        if(dic->key[kpos + 2] == codes[2]) {
          return dic->value[pos];
        }
      }
    }
    pos = (pos + 1) & dic->mask;
    kpos = pos * MAX_LONGS;
  }
  dic->used[pos] = TRUE;
  dic->value[pos] = id;
  memcpy(&dic->key[kpos], codes, MAX_LONGS*sizeof(unsigned long));
  dic->size++;
  return id;
}

int LTD_get(LongTermDictionary* dic,
                     char* chars, unsigned int len) {
  unsigned long codes[MAX_LONGS];
  codes[0] = encodeInLong(chars, len, 0);
  codes[1] = encodeInLong(chars, len, LONG_CAPACITY);
  codes[2] = encodeInLong(chars, len, LONG_CAPACITY * 2);

  int pos = (int) murmurHash3(codes[0]) & dic->mask;
  int kpos = pos * MAX_LONGS;
  while(dic->used[pos]) {
    if(dic->key[kpos] == codes[0]) {
      if(dic->key[kpos + 1] == codes[1]) {
        if(dic->key[kpos + 2] == codes[2]) {
          return dic->value[pos];
        }
      }
    }
    pos = (pos + 1) & dic->mask;
    kpos = pos * MAX_LONGS;
  }
  return -1;
}

#endif
