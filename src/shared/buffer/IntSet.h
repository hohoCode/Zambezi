#ifndef INT_SET_H_GUARD
#define INT_SET_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "dictionary/Hash.h"

#define INTSET_LOAD_FACTOR 0.75
#define TRUE 1
#define FALSE 0

typedef struct IntSet IntSet;

struct IntSet {
  unsigned char* used;
  unsigned int* key;
  unsigned int capacity;
  unsigned int size;
  unsigned int mask;
};

IntSet* createIntSet(unsigned int initialSize) {
  IntSet* set = (IntSet*) malloc(sizeof(IntSet));
  set->used = (unsigned char*) malloc(initialSize * sizeof(unsigned char));
  memset(set->used, FALSE, initialSize);
  set->mask = initialSize - 1;
  set->capacity = initialSize;

  set->key = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  return set;
}

void destroyIntSet(IntSet* set) {
  free(set->used);
  free(set->key);
  free(set);
}

IntSet* expandIntSet(IntSet* set) {
  IntSet* copy = createIntSet(set->capacity * 2);
  copy->size = set->size;

  int i = 0, j, pos, k;
  for(j = set->size; j-- != 0;) {
    while(!set->used[i]) i++;
    pos = set->key[i] & copy->mask;
    while(copy->used[pos]) {
      pos = (pos + 1) & copy->mask;
    }
    copy->key[pos] = set->key[i];
    copy->used[pos] = TRUE;
    i++;
  }
  destroyIntSet(set);
  return copy;
}

int containsKey(IntSet* set, int k) {
  int pos = k & set->mask;
  while(set->used[pos]) {
    if(set->key[pos] == k) return 1;
    pos = (pos + 1) & set->mask;
  }
  return 0;
}

int addIntSet(IntSet** set, int k) {
  int pos = k & (*set)->mask;
  while((*set)->used[pos]) {
    if((*set)->key[pos] == k) return 0;
    pos = (pos + 1) & (*set)->mask;
  }
  (*set)->size++;
  (*set)->used[pos] = TRUE;
  (*set)->key[pos] = k;

  if((*set)->size > INTSET_LOAD_FACTOR * (*set)->capacity) {
    IntSet* temp = expandIntSet((*set));
    *set = temp;
  }
  return 1;
}

void clearIntSet(IntSet* set) {
  memset(set->used, FALSE, set->capacity);
  set->size = 0;
}

int nextIndexIntSet(IntSet* set, int pos) {
  pos++;
  while(!set->used[pos]) {
    pos++;
    if(pos >= set->capacity) {
      return -1;
    }
  }
  return pos;
}
#endif
