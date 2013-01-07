#ifndef BM25_H_GUARD
#define BM25_H_GUARD

#include <math.h>

#define K1 0.5f
#define B 0.3f

float idf(int numDocs, int df) {
  return (float) log(((float) numDocs - (float) df + 0.5f)
                     / ((float) df + 0.5f));
}

float bm25tf(int tf, int docLen, float avgDocLen) {
  return ((1.0f + K1) * tf) / (K1 * (1.0f - B + B * docLen / avgDocLen) + tf);
}

float bm25(int tf, int df, int numDocs, int docLen, float avgDocLen) {
  return bm25tf(tf, docLen, avgDocLen) * idf(numDocs, df);
}

#endif
