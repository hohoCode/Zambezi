#ifndef TERM_DICTIONARY_H_GUARD
#define TERM_DICTIONARY_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "Vocab.h"
#include "Term.h"

#define TERM_LOAD_FACTOR 0.75
#define TRUE 1
#define FALSE 0

typedef struct TermDictionary TermDictionary;

struct TermDictionary {
  unsigned char* used;
  Term** key;
  unsigned int* value;
  unsigned int capacity;
  unsigned int size;
  unsigned int mask;
};

void TD_write(TermDictionary* dic, FILE* fp) {
  fwrite(&dic->size, sizeof(unsigned int), 1, fp);
  fwrite(&dic->mask, sizeof(unsigned int), 1, fp);
  fwrite(&dic->capacity, sizeof(unsigned int), 1, fp);

  int i = 0, j;
  for(j = dic->size; j-- != 0;) {
    while(!dic->used[i]) i++;

    fwrite(&i, sizeof(int), 1, fp);
    fwrite(&dic->value[i], sizeof(unsigned int), 1, fp);
    term_write(dic->key[i], fp);
    i++;
  }
}

TermDictionary* TD_read(FILE* fp) {
  TermDictionary* dic = (TermDictionary*)
    malloc(sizeof(TermDictionary));

  fread(&dic->size, sizeof(unsigned int), 1, fp);
  fread(&dic->mask, sizeof(unsigned int), 1, fp);
  fread(&dic->capacity, sizeof(unsigned int), 1, fp);

  dic->used = (unsigned char*) malloc(dic->capacity * sizeof(unsigned char));
  memset(dic->used, FALSE, dic->capacity);
  dic->key = (Term**) calloc(dic->capacity, sizeof(Term*));
  dic->value = (unsigned int*) calloc(dic->capacity, sizeof(unsigned int));

  int i = 0, j;
  for(j = dic->size; j-- != 0;) {
    fread(&i, sizeof(int), 1, fp);
    fread(&dic->value[i], sizeof(unsigned int), 1, fp);
    dic->key[i] = term_read(fp);
    dic->used[i] = TRUE;
  }

  return dic;
}

TermDictionary* TD_create(unsigned int initialSize) {
  TermDictionary* dic = (TermDictionary*)
    malloc(sizeof(TermDictionary));
  dic->used = (unsigned char*) malloc(initialSize * sizeof(unsigned char));
  memset(dic->used, FALSE, initialSize);
  dic->key = (Term**) calloc(initialSize, sizeof(Term*));
  dic->value = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  dic->size = 0;
  dic->mask = initialSize - 1;
  dic->capacity = initialSize;
  return dic;
}

void TD_destroy(TermDictionary* dic) {
  free(dic->used);
  int i = 0;
  for(i = 0; i < dic->capacity; i++) {
    if(dic->key[i]) {
      free(dic->key[i]);
    }
  }
  free(dic->key);
  free(dic->value);
  free(dic);
}

void TD_shallowDestroy(TermDictionary* dic) {
  free(dic->used);
  free(dic->key);
  free(dic->value);
  free(dic);
}

TermDictionary* TD_expand(TermDictionary* dic) {
  TermDictionary* copy = TD_create(dic->capacity * 2);
  copy->size = dic->size;

  int i = 0, j, pos;
  for(j = dic->size; j-- != 0;) {
    while(!dic->used[i]) i++;
    pos = (int) dic->key[i]->hash & copy->mask;
    while(copy->used[pos]) {
      pos = (pos + 1) & copy->mask;
    }
    copy->key[pos] = dic->key[i];
    copy->value[pos] = dic->value[i];
    copy->used[pos] = TRUE;
    i++;
  }
  TD_shallowDestroy(dic);
  return copy;
}

unsigned int TD_putIfNotPresent(TermDictionary* dic,
                                char* chars, unsigned int len,
                                unsigned int id) {
  Term* term = createTerm(chars, len);
  int pos = (int) term->hash & dic->mask;
  while(dic->used[pos]) {
    if(equals(dic->key[pos], term)) {
      term_destroy(term);
      return dic->value[pos];
    }
    pos = (pos + 1) & dic->mask;
  }
  dic->used[pos] = TRUE;
  dic->value[pos] = id;
  dic->key[pos] = term;

  dic->size++;
  return id;
}

unsigned int TD_get(TermDictionary* dic,
                    char* chars, unsigned int len) {
  Term* term = createTerm(chars, len);
  int pos = (int) term->hash & dic->mask;
  while(dic->used[pos]) {
    if(equals(dic->key[pos], term)) {
      term_destroy(term);
      return dic->value[pos];
    }
    pos = (pos + 1) & dic->mask;
  }
  term_destroy(term);
  return -1;
}

#endif
