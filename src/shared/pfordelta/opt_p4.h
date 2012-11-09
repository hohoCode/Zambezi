#ifndef OPT_P4_H_GUARD
#define OPT_P4_H_GUARD

#include "pf.h"
#define BLOCK_SIZE 128

int findBestB(unsigned int* docid) {
  unsigned int bits[16] = {0, 1, 3, 7, 15, 31, 63, 127, 255, 511,
                           1023, 2047, 4095, 8191, 65535, 1048575};
  int offset[17] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0};
  int i;
  for(i = 0; i < BLOCK_SIZE; i++) {
    offset[0] += (docid[i] > bits[0]);
    offset[1] += (docid[i] > bits[1]);
    offset[2] += (docid[i] > bits[2]);
    offset[3] += (docid[i] > bits[3]);
    offset[4] += (docid[i] > bits[4]);
    offset[5] += (docid[i] > bits[5]);
    offset[6] += (docid[i] > bits[6]);
    offset[7] += (docid[i] > bits[7]);
    offset[8] += (docid[i] > bits[8]);
    offset[9] += (docid[i] > bits[9]);
    offset[10] += (docid[i] > bits[10]);
    offset[11] += (docid[i] > bits[11]);
    offset[12] += (docid[i] > bits[12]);
    offset[13] += (docid[i] > bits[13]);
    offset[14] += (docid[i] > bits[14]);
    offset[15] += (docid[i] > bits[15]);
  }

  int bestB = 0;
  int bestOffset = 999999999;
  int temp;
  for(i = 0; i < 17; i++) {
    temp = (offset[i]<<5) + (cnum[i] * BLOCK_SIZE);
    if(temp < bestOffset) {
      bestB = i;
      bestOffset = temp;
    }
  }

  return bestB;
}

unsigned int OPT4(unsigned int *doc_id, unsigned int list_size, unsigned int *aux, int delta)
{
  unsigned int size = 0;
  int ex_n = 0;
  int i;

  if(delta) {
    for(i = list_size - 1; i > 0; i--) {
      doc_id[i] -= doc_id[i - 1];
    }
  }

  int b = findBestB(doc_id);
  unsigned int *ww = aux;
  detailed_p4_encode(&ww, doc_id, b, &size, &ex_n);

  return size;
}

#endif
