#ifndef OPT_P4_H_GUARD
#define OPT_P4_H_GUARD

#include "pf.h"
#define BLOCK_SIZE 128

//int dnum[17] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,16,20,32};

void p4_encode(unsigned int *doc_id, int npos, int b,unsigned int *buf , int *size, int *ex_n)
{
  int i = 0;
  unsigned int *ww = buf;
  detailed_p4_encode(&ww, &(doc_id[i]), b, size,ex_n);
}

int estimateCompressedSize(unsigned int* doc_id, unsigned int size, int bits) {
  int maxNoExp = (1<<bits) - 1;
  int outputOffset = 32 + bits*BLOCK_SIZE;
  int expNum = 0;
  int i = 0;
  for(i = 0; i < BLOCK_SIZE; i++) {
    if(doc_id[i] > maxNoExp) {
      expNum++;
    }
  }
  outputOffset += (expNum<<5);
  return outputOffset;
}

/*
*  when list_size is too small, not good to use this function
*/
int OPT4(unsigned int *doc_id,unsigned int list_size,unsigned int *aux)
{
  int i,j,l;
  int size = 0;
  int ex_n = 0;
  int csize = 0;   // compressed size in bytes

  int chunk_size = 0;
  int b = -1, temp_en = 0;
  int offset = 0;
  for(j=0;j<list_size;j+=BLOCK_SIZE)        // for each chunk
  {
    chunk_size = 999999999;
    b = -1;
    // get the smallest chunk size by trying all b's
    for(l=1;l<16;l++)
        {
          /*
          p4_encode(doc_id+j, BLOCK_SIZE, l, aux+offset, &size, &ex_n);
          if(chunk_size > size * 4)      // int bytes
          {
            chunk_size = size *4;
            b = l;
            temp_en = ex_n;
          }
          */
          int tempSize = estimateCompressedSize(doc_id, list_size, l);
          if(chunk_size > tempSize) {
            chunk_size = tempSize;
            b = l;
          }
        }

        p4_encode(doc_id + j, BLOCK_SIZE, b, aux + offset, &size, &ex_n);
        csize += size * 4;
        offset += size;
  }

  return csize;
}

#endif
