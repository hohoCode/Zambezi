#ifndef SHORT_TERM_DICTIONARY_H_GUARD
#define SHORT_TERM_DICTIONARY_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "Vocab.h"
#include "Hash.h"

#define SHORT_LOAD_FACTOR 0.75
#define TRUE 1
#define FALSE 0

typedef struct ShortTermDictionary ShortTermDictionary;

struct ShortTermDictionary {
  unsigned char* used;
  unsigned long* key;
  unsigned int* value;
  unsigned int capacity;
  unsigned int size;
  unsigned int mask;
};

void STD_write(ShortTermDictionary* dic, FILE* fp) {
  fwrite(&dic->size, sizeof(unsigned int), 1, fp);
  fwrite(&dic->mask, sizeof(unsigned int), 1, fp);
  fwrite(&dic->capacity, sizeof(unsigned int), 1, fp);

  int i = 0, j;
  for(j = dic->size; j-- != 0;) {
    while(!dic->used[i]) i++;

    fwrite(&i, sizeof(int), 1, fp);
    fwrite(&dic->value[i], sizeof(unsigned int), 1, fp);
    fwrite(&dic->key[i], sizeof(unsigned long), 1, fp);
    i++;
  }
}

ShortTermDictionary* STD_read(FILE* fp) {
  ShortTermDictionary* dic = (ShortTermDictionary*)
    malloc(sizeof(ShortTermDictionary));

  fread(&dic->size, sizeof(unsigned int), 1, fp);
  fread(&dic->mask, sizeof(unsigned int), 1, fp);
  fread(&dic->capacity, sizeof(unsigned int), 1, fp);

  dic->used = (unsigned char*) malloc(dic->capacity * sizeof(unsigned char));
  memset(dic->used, FALSE, dic->capacity);
  dic->key = (unsigned long*) calloc(dic->capacity, sizeof(unsigned long));
  dic->value = (unsigned int*) calloc(dic->capacity, sizeof(unsigned int));

  int i = 0, j;
  for(j = dic->size; j-- != 0;) {
    fread(&i, sizeof(int), 1, fp);
    fread(&dic->value[i], sizeof(unsigned int), 1, fp);
    fread(&dic->key[i], sizeof(unsigned long), 1, fp);
    dic->used[i] = TRUE;
  }

  return dic;
}

ShortTermDictionary* STD_create(unsigned int initialSize) {
  ShortTermDictionary* dic = (ShortTermDictionary*)
    malloc(sizeof(ShortTermDictionary));
  dic->used = (unsigned char*) malloc(initialSize * sizeof(unsigned char));
  memset(dic->used, FALSE, initialSize);
  dic->key = (unsigned long*) calloc(initialSize, sizeof(unsigned long));
  dic->value = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  dic->size = 0;
  dic->mask = initialSize - 1;
  dic->capacity = initialSize;
  return dic;
}

void STD_destroy(ShortTermDictionary* dic) {
  free(dic->used);
  free(dic->key);
  free(dic->value);
  free(dic);
}

ShortTermDictionary* STD_expand(ShortTermDictionary* dic) {
  ShortTermDictionary* copy = STD_create(dic->capacity*2);
  copy->size = dic->size;

  int i = 0, j, pos;
  for(j = dic->size; j-- != 0;) {
    while(!dic->used[i]) i++;
    pos = (int) murmurHash3(dic->key[i]) & copy->mask;
    while(copy->used[pos]) {
      pos = (pos + 1) & copy->mask;
    }
    copy->key[pos] = dic->key[i];
    copy->value[pos] = dic->value[i];
    copy->used[pos] = TRUE;
    i++;
  }
  STD_destroy(dic);
  return copy;
}

unsigned int STD_putIfNotPresent(ShortTermDictionary* dic,
                                 char* chars, unsigned int len,
                                 unsigned int id) {
  unsigned long code = encodeInLong(chars, len, 0);
  int pos = (int) murmurHash3(code) & dic->mask;
  while(dic->used[pos]) {
    if(dic->key[pos] == code) {
      return dic->value[pos];
    }
    pos = (pos + 1) & dic->mask;
  }
  dic->used[pos] = TRUE;
  dic->value[pos] = id;
  dic->key[pos] = code;
  dic->size++;
  return id;
}

unsigned int STD_get(ShortTermDictionary* dic,
                     char* chars, unsigned int len) {
  unsigned long code = encodeInLong(chars, len, 0);
  int pos = (int) murmurHash3(code) & dic->mask;
  while(dic->used[pos]) {
    if(dic->key[pos] == code) {
      return dic->value[pos];
    }
    pos = (pos + 1) & dic->mask;
  }
  return -1;
}

#endif
