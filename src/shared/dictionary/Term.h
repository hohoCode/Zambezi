#ifndef TERM_H_GUARD
#define TERM_H_GUARD

#include "Hash.h"
#include "Vocab.h"

typedef struct Term Term;

struct Term {
  unsigned long* codedTerm;
  unsigned int length;
  unsigned long hash;
};

void term_write(Term* term, FILE* fp) {
  fwrite(&term->hash, sizeof(unsigned long), 1, fp);
  fwrite(&term->length, sizeof(unsigned int), 1, fp);
  fwrite(term->codedTerm, sizeof(unsigned long), term->length, fp);
}

Term* term_read(FILE* fp) {
  Term* term = (Term*) malloc(sizeof(Term));
  fread(&term->hash, sizeof(unsigned long), 1, fp);
  fread(&term->length, sizeof(unsigned int), 1, fp);

  term->codedTerm = (unsigned long*) calloc(term->length, sizeof(unsigned long));
  fread(term->codedTerm, sizeof(unsigned long), term->length, fp);
  return term;
}

Term* createTerm(char* chars, unsigned int len) {
  Term* term = (Term*) malloc(sizeof(Term));
  term->length = (len + LONG_CAPACITY - 1) / LONG_CAPACITY;
  term->codedTerm = (unsigned long*) calloc(term->length, sizeof(unsigned long));
  int i = 0;
  term->hash = 0;
  for(i = 0; i < term->length; i++) {
    term->codedTerm[i] = encodeInLong(chars, len, i * LONG_CAPACITY);
    term->hash += murmurHash3(term->codedTerm[i]);
  }
  return term;
}

void term_destroy(Term* term) {
  free(term->codedTerm);
  free(term);
}

unsigned char equals(Term* a, Term* b) {
  if(a->length != b->length) {
    return 0;
  }
  int i = 0;
  for(i = 0; i < a->length; i++) {
    if(a->codedTerm[i] != b->codedTerm[i]) {
      return 0;
    }
  }
  return 1;
}

#endif
