#ifndef DICTIONARY_H_GUARD
#define DICTIONARY_H_GUARD

#include "ShortTermDictionary.h"
#include "LongTermDictionary.h"
#include "TermDictionary.h"
#include "Vocab.h"

typedef struct Dictionary Dictionary;

struct Dictionary {
  ShortTermDictionary* shortTermDic;
  LongTermDictionary* longTermDic;
  TermDictionary* other;
};

void writeDictionary(Dictionary* dic, FILE* fp) {
  STD_write(dic->shortTermDic, fp);
  LTD_write(dic->longTermDic, fp);
  TD_write(dic->other, fp);
}

Dictionary* readDictionary(FILE* fp) {
  Dictionary* dic = (Dictionary*) malloc(sizeof(Dictionary));
  dic->shortTermDic = STD_read(fp);
  dic->longTermDic = LTD_read(fp);
  dic->other = TD_read(fp);
  return dic;
}

Dictionary* createDictionary(int initialSize) {
  Dictionary* dic = (Dictionary*) malloc(sizeof(Dictionary));
  dic->shortTermDic = STD_create(initialSize);
  dic->longTermDic = LTD_create(initialSize);
  dic->other = TD_create(initialSize);

  return dic;
}

void destroyDictionary(Dictionary* dic) {
  STD_destroy(dic->shortTermDic);
  LTD_destroy(dic->longTermDic);
  TD_destroy(dic->other);
}

unsigned int putIfNotPresent(Dictionary* dic, char* chars,
                             unsigned int len, int id) {
  if(len <= LONG_CAPACITY) {
    int r = STD_putIfNotPresent(dic->shortTermDic, chars, len, id);
    if(dic->shortTermDic->size > SHORT_LOAD_FACTOR * dic->shortTermDic->capacity) {
      ShortTermDictionary* temp = STD_expand(dic->shortTermDic);
      dic->shortTermDic = temp;
    }
    return r;
  } else if(len <= MAX_LONGS_LENGTH) {
    int r = LTD_putIfNotPresent(dic->longTermDic, chars, len, id);
    if(dic->longTermDic->size > LONG_LOAD_FACTOR * dic->longTermDic->capacity) {
      LongTermDictionary* temp = LTD_expand(dic->longTermDic);
      dic->longTermDic = temp;
    }
    return r;
  } else {
    int r = TD_putIfNotPresent(dic->other, chars, len, id);
    if(dic->other->size > TERM_LOAD_FACTOR * dic->other->capacity) {
      TermDictionary* temp = TD_expand(dic->other);
      dic->other = temp;
    }
    return r;
  }
}

#endif
