#ifndef POSTINGS_LIST_H_GUARD
#define POSTINGS_LIST_H_GUARD

#include <stdio.h>
#include "InvertedIndex.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "pfordelta/opt_p4.h"

typedef struct PostingsList PostingsList;

struct PostingsList {
  InvertedIndex* index;
  int termid;
  int df;

  long pointer;
  unsigned int* docid;
  unsigned int* tf;
  int length;
  int position;
};

PostingsList* getPostingsList(InvertedIndex* index, int termid) {
  PostingsList* list = (PostingsList*) malloc(sizeof(PostingsList));
  list->index = index;
  list->termid = termid;
  list->df = getDf(index->pointers, termid);
  list->pointer = getStartPointer(index->pointers, termid);
  list->docid = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  list->tf = (unsigned int*) calloc(BLOCK_SIZE * 2, sizeof(unsigned int));
  list->position = -1;
  list->length = 0;

  if(list->pointer != UNDEFINED_POINTER) {
    list->length = decompressDocidBlock(index->pool, list->docid, list->pointer);
    decompressTfBlock(index->pool, list->tf, list->pointer);
  }
  return list;
}

void destroyPostingsList(PostingsList* list) {
  free(list->docid);
  free(list->tf);
  free(list);
}

void nextPosting(PostingsList* list) {
  if(list->pointer == UNDEFINED_POINTER) {
    return;
  }
  if(list->position == list->length - 1) {
    list->pointer = nextPointer(list->index->pool, list->pointer);
    if(list->pointer == UNDEFINED_POINTER) {
      return;
    }
    list->length = decompressDocidBlock(list->index->pool, list->docid, list->pointer);
    decompressTfBlock(list->index->pool, list->tf, list->pointer);
    list->position = -1;
  }
  list->position++;
}

int hasNext(PostingsList* list) {
  return (list->position < list->length - 1) ||
    (list->position == list->length - 1 &&
     nextPointer(list->index->pool, list->pointer) != UNDEFINED_POINTER);
}

int getDocumentId(PostingsList* list) {
  return list->docid[list->position];
}

int getTermFrequency(PostingsList* list) {
  return list->tf[list->position];
}

int getDocumentFrequency(PostingsList* list) {
  return list->df;
}

#endif
