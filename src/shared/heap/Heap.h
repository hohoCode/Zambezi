#ifndef HEAP_H_GUARD
#define HEAP_H_GUARD

#include <stdlib.h>
#include <float.h>

typedef struct Heap Heap;

struct Heap {
  int* docid;
  float* score;
  int size;
  int index;
};

Heap* initHeap(int size) {
  Heap* heap = (Heap*) malloc(sizeof(Heap));
  heap->index = 0;
  heap->size = size + 2;
  heap->docid = (int*) malloc(heap->size * sizeof(int));
  heap->score = (float*) malloc(heap->size * sizeof(float));
  heap->score[0] = -FLT_MAX;
  return heap;
}

void destroyHeap(Heap* heap) {
  free(heap->docid);
  free(heap->score);
  free(heap);
}

int isFullHeap(Heap* heap) {
  return heap->index >= heap->size - 2;
}

int deleteMinHeap(Heap* heap) {
  int minElement,lastElement,child,now;
  float lastScore;
  minElement = heap->docid[1];
  lastScore = heap->score[heap->index];
  lastElement = heap->docid[heap->index--];

  for(now = 1; now*2 <= heap->index; now = child) {
    child = now*2;
    if(child != heap->index && heap->score[child+1] < heap->score[child] ) {
      child++;
    }
    if(lastScore > heap->score[child]) {
      heap->docid[now] = heap->docid[child];
      heap->score[now] = heap->score[child];
    } else {
      break;
    }
  }
  heap->docid[now] = lastElement;
  heap->score[now] = lastScore;
  return minElement;
}

void insertHeap(Heap* heap, int docid, float score) {

  heap->index++;
  heap->docid[heap->index] = docid;
  heap->score[heap->index] = score;

  int now = heap->index;
  while(heap->score[now/2] > score) {
    heap->docid[now] = heap->docid[now/2];
    heap->score[now] = heap->score[now/2];
    now /= 2;
  }
  heap->docid[now] = docid;
  heap->score[now] = score;

  if(heap->index == heap->size - 1) {
    deleteMinHeap(heap);
  }
}

float minScoreHeap(Heap* heap) {
  return heap->score[1];
}

int minDocidHeap(Heap* heap) {
  return heap->docid[1];
}

#endif
