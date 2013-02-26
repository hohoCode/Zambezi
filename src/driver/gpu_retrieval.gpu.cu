#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "pfordelta/opt_p4.h"
#include "dictionary/Dictionary.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "util/ParseCommandLine.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "Config.h"
#include "InvertedIndex.h"
#include "intersection/SvS.h"
#include "intersection/WAND.h"


#ifndef RETRIEVAL_ALGO_ENUM_GUARD
#define RETRIEVAL_ALGO_ENUM_GUARD
/*typedef enum Algorithm Algorithm;
//enum Algorithm {
  SVS = 0,
  WAND = 1
};*/
#endif

#define THREADS_PER_BLOCK 512 
#define THREADS_PER_BLOCK_GLOBALPAIRS 64
#define LINEARBLOCK 100



/*************************************************************/
/* for fast unpacking of integers of fixed bit length */
/*************************************************************/

__device__ void cunpack0(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i++)  p[i] = 0;
}


__device__ void cunpack1(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 1)
  {
    p[0] = (w[0] >> 31);
    p[1] = (w[0] >> 30) & 1;
    p[2] = (w[0] >> 29) & 1;
    p[3] = (w[0] >> 28) & 1;
    p[4] = (w[0] >> 27) & 1;
    p[5] = (w[0] >> 26) & 1;
    p[6] = (w[0] >> 25) & 1;
    p[7] = (w[0] >> 24) & 1;
    p[8] = (w[0] >> 23) & 1;
    p[9] = (w[0] >> 22) & 1;
    p[10] = (w[0] >> 21) & 1;
    p[11] = (w[0] >> 20) & 1;
    p[12] = (w[0] >> 19) & 1;
    p[13] = (w[0] >> 18) & 1;
    p[14] = (w[0] >> 17) & 1;
    p[15] = (w[0] >> 16) & 1;
    p[16] = (w[0] >> 15) & 1;
    p[17] = (w[0] >> 14) & 1;
    p[18] = (w[0] >> 13) & 1;
    p[19] = (w[0] >> 12) & 1;
    p[20] = (w[0] >> 11) & 1;
    p[21] = (w[0] >> 10) & 1;
    p[22] = (w[0] >> 9) & 1;
    p[23] = (w[0] >> 8) & 1;
    p[24] = (w[0] >> 7) & 1;
    p[25] = (w[0] >> 6) & 1;
    p[26] = (w[0] >> 5) & 1;
    p[27] = (w[0] >> 4) & 1;
    p[28] = (w[0] >> 3) & 1;
    p[29] = (w[0] >> 2) & 1;
    p[30] = (w[0] >> 1) & 1;
    p[31] = (w[0]) & 1;
  }
}


__device__ void cunpack2(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 2)
  {
    p[0] = (w[0] >> 30);
    p[1] = (w[0] >> 28) & 3;
    p[2] = (w[0] >> 26) & 3;
    p[3] = (w[0] >> 24) & 3;
    p[4] = (w[0] >> 22) & 3;
    p[5] = (w[0] >> 20) & 3;
    p[6] = (w[0] >> 18) & 3;
    p[7] = (w[0] >> 16) & 3;
    p[8] = (w[0] >> 14) & 3;
    p[9] = (w[0] >> 12) & 3;
    p[10] = (w[0] >> 10) & 3;
    p[11] = (w[0] >> 8) & 3;
    p[12] = (w[0] >> 6) & 3;
    p[13] = (w[0] >> 4) & 3;
    p[14] = (w[0] >> 2) & 3;
    p[15] = (w[0]) & 3;
    p[16] = (w[1] >> 30);
    p[17] = (w[1] >> 28) & 3;
    p[18] = (w[1] >> 26) & 3;
    p[19] = (w[1] >> 24) & 3;
    p[20] = (w[1] >> 22) & 3;
    p[21] = (w[1] >> 20) & 3;
    p[22] = (w[1] >> 18) & 3;
    p[23] = (w[1] >> 16) & 3;
    p[24] = (w[1] >> 14) & 3;
    p[25] = (w[1] >> 12) & 3;
    p[26] = (w[1] >> 10) & 3;
    p[27] = (w[1] >> 8) & 3;
    p[28] = (w[1] >> 6) & 3;
    p[29] = (w[1] >> 4) & 3;
    p[30] = (w[1] >> 2) & 3;
    p[31] = (w[1]) & 3;
  }
}


__device__ void cunpack3(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 3)
  {
    p[0] = (w[0] >> 29);
    p[1] = (w[0] >> 26) & 7;
    p[2] = (w[0] >> 23) & 7;
    p[3] = (w[0] >> 20) & 7;
    p[4] = (w[0] >> 17) & 7;
    p[5] = (w[0] >> 14) & 7;
    p[6] = (w[0] >> 11) & 7;
    p[7] = (w[0] >> 8) & 7;
    p[8] = (w[0] >> 5) & 7;
    p[9] = (w[0] >> 2) & 7;
    p[10] = (w[0] << 1) & 7;
    p[10] |= (w[1] >> 31);
    p[11] = (w[1] >> 28) & 7;
    p[12] = (w[1] >> 25) & 7;
    p[13] = (w[1] >> 22) & 7;
    p[14] = (w[1] >> 19) & 7;
    p[15] = (w[1] >> 16) & 7;
    p[16] = (w[1] >> 13) & 7;
    p[17] = (w[1] >> 10) & 7;
    p[18] = (w[1] >> 7) & 7;
    p[19] = (w[1] >> 4) & 7;
    p[20] = (w[1] >> 1) & 7;
    p[21] = (w[1] << 2) & 7;
    p[21] |= (w[2] >> 30);
    p[22] = (w[2] >> 27) & 7;
    p[23] = (w[2] >> 24) & 7;
    p[24] = (w[2] >> 21) & 7;
    p[25] = (w[2] >> 18) & 7;
    p[26] = (w[2] >> 15) & 7;
    p[27] = (w[2] >> 12) & 7;
    p[28] = (w[2] >> 9) & 7;
    p[29] = (w[2] >> 6) & 7;
    p[30] = (w[2] >> 3) & 7;
    p[31] = (w[2]) & 7;
  }
}


__device__ void cunpack4(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 4)
  {
    p[0] = (w[0] >> 28);
    p[1] = (w[0] >> 24) & 15;
    p[2] = (w[0] >> 20) & 15;
    p[3] = (w[0] >> 16) & 15;
    p[4] = (w[0] >> 12) & 15;
    p[5] = (w[0] >> 8) & 15;
    p[6] = (w[0] >> 4) & 15;
    p[7] = (w[0]) & 15;
    p[8] = (w[1] >> 28);
    p[9] = (w[1] >> 24) & 15;
    p[10] = (w[1] >> 20) & 15;
    p[11] = (w[1] >> 16) & 15;
    p[12] = (w[1] >> 12) & 15;
    p[13] = (w[1] >> 8) & 15;
    p[14] = (w[1] >> 4) & 15;
    p[15] = (w[1]) & 15;
    p[16] = (w[2] >> 28);
    p[17] = (w[2] >> 24) & 15;
    p[18] = (w[2] >> 20) & 15;
    p[19] = (w[2] >> 16) & 15;
    p[20] = (w[2] >> 12) & 15;
    p[21] = (w[2] >> 8) & 15;
    p[22] = (w[2] >> 4) & 15;
    p[23] = (w[2]) & 15;
    p[24] = (w[3] >> 28);
    p[25] = (w[3] >> 24) & 15;
    p[26] = (w[3] >> 20) & 15;
    p[27] = (w[3] >> 16) & 15;
    p[28] = (w[3] >> 12) & 15;
    p[29] = (w[3] >> 8) & 15;
    p[30] = (w[3] >> 4) & 15;
    p[31] = (w[3]) & 15;
  }
}


__device__ void cunpack5(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 5)
  {
    p[0] = (w[0] >> 27);
    p[1] = (w[0] >> 22) & 31;
    p[2] = (w[0] >> 17) & 31;
    p[3] = (w[0] >> 12) & 31;
    p[4] = (w[0] >> 7) & 31;
    p[5] = (w[0] >> 2) & 31;
    p[6] = (w[0] << 3) & 31;
    p[6] |= (w[1] >> 29);
    p[7] = (w[1] >> 24) & 31;
    p[8] = (w[1] >> 19) & 31;
    p[9] = (w[1] >> 14) & 31;
    p[10] = (w[1] >> 9) & 31;
    p[11] = (w[1] >> 4) & 31;
    p[12] = (w[1] << 1) & 31;
    p[12] |= (w[2] >> 31);
    p[13] = (w[2] >> 26) & 31;
    p[14] = (w[2] >> 21) & 31;
    p[15] = (w[2] >> 16) & 31;
    p[16] = (w[2] >> 11) & 31;
    p[17] = (w[2] >> 6) & 31;
    p[18] = (w[2] >> 1) & 31;
    p[19] = (w[2] << 4) & 31;
    p[19] |= (w[3] >> 28);
    p[20] = (w[3] >> 23) & 31;
    p[21] = (w[3] >> 18) & 31;
    p[22] = (w[3] >> 13) & 31;
    p[23] = (w[3] >> 8) & 31;
    p[24] = (w[3] >> 3) & 31;
    p[25] = (w[3] << 2) & 31;
    p[25] |= (w[4] >> 30);
    p[26] = (w[4] >> 25) & 31;
    p[27] = (w[4] >> 20) & 31;
    p[28] = (w[4] >> 15) & 31;
    p[29] = (w[4] >> 10) & 31;
    p[30] = (w[4] >> 5) & 31;
    p[31] = (w[4]) & 31;
  }
}


__device__ void cunpack6(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 6)
  {
    p[0] = (w[0] >> 26);
    p[1] = (w[0] >> 20) & 63;
    p[2] = (w[0] >> 14) & 63;
    p[3] = (w[0] >> 8) & 63;
    p[4] = (w[0] >> 2) & 63;
    p[5] = (w[0] << 4) & 63;
    p[5] |= (w[1] >> 28);
    p[6] = (w[1] >> 22) & 63;
    p[7] = (w[1] >> 16) & 63;
    p[8] = (w[1] >> 10) & 63;
    p[9] = (w[1] >> 4) & 63;
    p[10] = (w[1] << 2) & 63;
    p[10] |= (w[2] >> 30);
    p[11] = (w[2] >> 24) & 63;
    p[12] = (w[2] >> 18) & 63;
    p[13] = (w[2] >> 12) & 63;
    p[14] = (w[2] >> 6) & 63;
    p[15] = (w[2]) & 63;
    p[16] = (w[3] >> 26);
    p[17] = (w[3] >> 20) & 63;
    p[18] = (w[3] >> 14) & 63;
    p[19] = (w[3] >> 8) & 63;
    p[20] = (w[3] >> 2) & 63;
    p[21] = (w[3] << 4) & 63;
    p[21] |= (w[4] >> 28);
    p[22] = (w[4] >> 22) & 63;
    p[23] = (w[4] >> 16) & 63;
    p[24] = (w[4] >> 10) & 63;
    p[25] = (w[4] >> 4) & 63;
    p[26] = (w[4] << 2) & 63;
    p[26] |= (w[5] >> 30);
    p[27] = (w[5] >> 24) & 63;
    p[28] = (w[5] >> 18) & 63;
    p[29] = (w[5] >> 12) & 63;
    p[30] = (w[5] >> 6) & 63;
    p[31] = (w[5]) & 63;
  }
}


__device__ void cunpack7(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 7)
  {
    p[0] = (w[0] >> 25);
    p[1] = (w[0] >> 18) & 127;
    p[2] = (w[0] >> 11) & 127;
    p[3] = (w[0] >> 4) & 127;
    p[4] = (w[0] << 3) & 127;
    p[4] |= (w[1] >> 29);
    p[5] = (w[1] >> 22) & 127;
    p[6] = (w[1] >> 15) & 127;
    p[7] = (w[1] >> 8) & 127;
    p[8] = (w[1] >> 1) & 127;
    p[9] = (w[1] << 6) & 127;
    p[9] |= (w[2] >> 26);
    p[10] = (w[2] >> 19) & 127;
    p[11] = (w[2] >> 12) & 127;
    p[12] = (w[2] >> 5) & 127;
    p[13] = (w[2] << 2) & 127;
    p[13] |= (w[3] >> 30);
    p[14] = (w[3] >> 23) & 127;
    p[15] = (w[3] >> 16) & 127;
    p[16] = (w[3] >> 9) & 127;
    p[17] = (w[3] >> 2) & 127;
    p[18] = (w[3] << 5) & 127;
    p[18] |= (w[4] >> 27);
    p[19] = (w[4] >> 20) & 127;
    p[20] = (w[4] >> 13) & 127;
    p[21] = (w[4] >> 6) & 127;
    p[22] = (w[4] << 1) & 127;
    p[22] |= (w[5] >> 31);
    p[23] = (w[5] >> 24) & 127;
    p[24] = (w[5] >> 17) & 127;
    p[25] = (w[5] >> 10) & 127;
    p[26] = (w[5] >> 3) & 127;
    p[27] = (w[5] << 4) & 127;
    p[27] |= (w[6] >> 28);
    p[28] = (w[6] >> 21) & 127;
    p[29] = (w[6] >> 14) & 127;
    p[30] = (w[6] >> 7) & 127;
    p[31] = (w[6]) & 127;
  }
}


__device__ void cunpack8(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 8)
  {
    p[0] = (w[0] >> 24);
    p[1] = (w[0] >> 16) & 255;
    p[2] = (w[0] >> 8) & 255;
    p[3] = (w[0]) & 255;
    p[4] = (w[1] >> 24);
    p[5] = (w[1] >> 16) & 255;
    p[6] = (w[1] >> 8) & 255;
    p[7] = (w[1]) & 255;
    p[8] = (w[2] >> 24);
    p[9] = (w[2] >> 16) & 255;
    p[10] = (w[2] >> 8) & 255;
    p[11] = (w[2]) & 255;
    p[12] = (w[3] >> 24);
    p[13] = (w[3] >> 16) & 255;
    p[14] = (w[3] >> 8) & 255;
    p[15] = (w[3]) & 255;
    p[16] = (w[4] >> 24);
    p[17] = (w[4] >> 16) & 255;
    p[18] = (w[4] >> 8) & 255;
    p[19] = (w[4]) & 255;
    p[20] = (w[5] >> 24);
    p[21] = (w[5] >> 16) & 255;
    p[22] = (w[5] >> 8) & 255;
    p[23] = (w[5]) & 255;
    p[24] = (w[6] >> 24);
    p[25] = (w[6] >> 16) & 255;
    p[26] = (w[6] >> 8) & 255;
    p[27] = (w[6]) & 255;
    p[28] = (w[7] >> 24);
    p[29] = (w[7] >> 16) & 255;
    p[30] = (w[7] >> 8) & 255;
    p[31] = (w[7]) & 255;
  }
}


__device__ void cunpack9(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 9)
  {
    p[0] = (w[0] >> 23);
    p[1] = (w[0] >> 14) & 511;
    p[2] = (w[0] >> 5) & 511;
    p[3] = (w[0] << 4) & 511;
    p[3] |= (w[1] >> 28);
    p[4] = (w[1] >> 19) & 511;
    p[5] = (w[1] >> 10) & 511;
    p[6] = (w[1] >> 1) & 511;
    p[7] = (w[1] << 8) & 511;
    p[7] |= (w[2] >> 24);
    p[8] = (w[2] >> 15) & 511;
    p[9] = (w[2] >> 6) & 511;
    p[10] = (w[2] << 3) & 511;
    p[10] |= (w[3] >> 29);
    p[11] = (w[3] >> 20) & 511;
    p[12] = (w[3] >> 11) & 511;
    p[13] = (w[3] >> 2) & 511;
    p[14] = (w[3] << 7) & 511;
    p[14] |= (w[4] >> 25);
    p[15] = (w[4] >> 16) & 511;
    p[16] = (w[4] >> 7) & 511;
    p[17] = (w[4] << 2) & 511;
    p[17] |= (w[5] >> 30);
    p[18] = (w[5] >> 21) & 511;
    p[19] = (w[5] >> 12) & 511;
    p[20] = (w[5] >> 3) & 511;
    p[21] = (w[5] << 6) & 511;
    p[21] |= (w[6] >> 26);
    p[22] = (w[6] >> 17) & 511;
    p[23] = (w[6] >> 8) & 511;
    p[24] = (w[6] << 1) & 511;
    p[24] |= (w[7] >> 31);
    p[25] = (w[7] >> 22) & 511;
    p[26] = (w[7] >> 13) & 511;
    p[27] = (w[7] >> 4) & 511;
    p[28] = (w[7] << 5) & 511;
    p[28] |= (w[8] >> 27);
    p[29] = (w[8] >> 18) & 511;
    p[30] = (w[8] >> 9) & 511;
    p[31] = (w[8]) & 511;
  }
}


__device__ void cunpack10(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 10)
  {
    p[0] = (w[0] >> 22);
    p[1] = (w[0] >> 12) & 1023;
    p[2] = (w[0] >> 2) & 1023;
    p[3] = (w[0] << 8) & 1023;
    p[3] |= (w[1] >> 24);
    p[4] = (w[1] >> 14) & 1023;
    p[5] = (w[1] >> 4) & 1023;
    p[6] = (w[1] << 6) & 1023;
    p[6] |= (w[2] >> 26);
    p[7] = (w[2] >> 16) & 1023;
    p[8] = (w[2] >> 6) & 1023;
    p[9] = (w[2] << 4) & 1023;
    p[9] |= (w[3] >> 28);
    p[10] = (w[3] >> 18) & 1023;
    p[11] = (w[3] >> 8) & 1023;
    p[12] = (w[3] << 2) & 1023;
    p[12] |= (w[4] >> 30);
    p[13] = (w[4] >> 20) & 1023;
    p[14] = (w[4] >> 10) & 1023;
    p[15] = (w[4]) & 1023;
    p[16] = (w[5] >> 22);
    p[17] = (w[5] >> 12) & 1023;
    p[18] = (w[5] >> 2) & 1023;
    p[19] = (w[5] << 8) & 1023;
    p[19] |= (w[6] >> 24);
    p[20] = (w[6] >> 14) & 1023;
    p[21] = (w[6] >> 4) & 1023;
    p[22] = (w[6] << 6) & 1023;
    p[22] |= (w[7] >> 26);
    p[23] = (w[7] >> 16) & 1023;
    p[24] = (w[7] >> 6) & 1023;
    p[25] = (w[7] << 4) & 1023;
    p[25] |= (w[8] >> 28);
    p[26] = (w[8] >> 18) & 1023;
    p[27] = (w[8] >> 8) & 1023;
    p[28] = (w[8] << 2) & 1023;
    p[28] |= (w[9] >> 30);
    p[29] = (w[9] >> 20) & 1023;
    p[30] = (w[9] >> 10) & 1023;
    p[31] = (w[9]) & 1023;
  }
}


__device__ void cunpack11(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 11)
  {
    p[0] = (w[0] >> 21);
    p[1] = (w[0] >> 10) & 2047;
    p[2] = (w[0] << 1) & 2047;
    p[2] |= (w[1] >> 31);
    p[3] = (w[1] >> 20) & 2047;
    p[4] = (w[1] >> 9) & 2047;
    p[5] = (w[1] << 2) & 2047;
    p[5] |= (w[2] >> 30);
    p[6] = (w[2] >> 19) & 2047;
    p[7] = (w[2] >> 8) & 2047;
    p[8] = (w[2] << 3) & 2047;
    p[8] |= (w[3] >> 29);
    p[9] = (w[3] >> 18) & 2047;
    p[10] = (w[3] >> 7) & 2047;
    p[11] = (w[3] << 4) & 2047;
    p[11] |= (w[4] >> 28);
    p[12] = (w[4] >> 17) & 2047;
    p[13] = (w[4] >> 6) & 2047;
    p[14] = (w[4] << 5) & 2047;
    p[14] |= (w[5] >> 27);
    p[15] = (w[5] >> 16) & 2047;
    p[16] = (w[5] >> 5) & 2047;
    p[17] = (w[5] << 6) & 2047;
    p[17] |= (w[6] >> 26);
    p[18] = (w[6] >> 15) & 2047;
    p[19] = (w[6] >> 4) & 2047;
    p[20] = (w[6] << 7) & 2047;
    p[20] |= (w[7] >> 25);
    p[21] = (w[7] >> 14) & 2047;
    p[22] = (w[7] >> 3) & 2047;
    p[23] = (w[7] << 8) & 2047;
    p[23] |= (w[8] >> 24);
    p[24] = (w[8] >> 13) & 2047;
    p[25] = (w[8] >> 2) & 2047;
    p[26] = (w[8] << 9) & 2047;
    p[26] |= (w[9] >> 23);
    p[27] = (w[9] >> 12) & 2047;
    p[28] = (w[9] >> 1) & 2047;
    p[29] = (w[9] << 10) & 2047;
    p[29] |= (w[10] >> 22);
    p[30] = (w[10] >> 11) & 2047;
    p[31] = (w[10]) & 2047;
  }
}


__device__ void cunpack12(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 12)
  {
    p[0] = (w[0] >> 20);
    p[1] = (w[0] >> 8) & 4095;
    p[2] = (w[0] << 4) & 4095;
    p[2] |= (w[1] >> 28);
    p[3] = (w[1] >> 16) & 4095;
    p[4] = (w[1] >> 4) & 4095;
    p[5] = (w[1] << 8) & 4095;
    p[5] |= (w[2] >> 24);
    p[6] = (w[2] >> 12) & 4095;
    p[7] = (w[2]) & 4095;
    p[8] = (w[3] >> 20);
    p[9] = (w[3] >> 8) & 4095;
    p[10] = (w[3] << 4) & 4095;
    p[10] |= (w[4] >> 28);
    p[11] = (w[4] >> 16) & 4095;
    p[12] = (w[4] >> 4) & 4095;
    p[13] = (w[4] << 8) & 4095;
    p[13] |= (w[5] >> 24);
    p[14] = (w[5] >> 12) & 4095;
    p[15] = (w[5]) & 4095;
    p[16] = (w[6] >> 20);
    p[17] = (w[6] >> 8) & 4095;
    p[18] = (w[6] << 4) & 4095;
    p[18] |= (w[7] >> 28);
    p[19] = (w[7] >> 16) & 4095;
    p[20] = (w[7] >> 4) & 4095;
    p[21] = (w[7] << 8) & 4095;
    p[21] |= (w[8] >> 24);
    p[22] = (w[8] >> 12) & 4095;
    p[23] = (w[8]) & 4095;
    p[24] = (w[9] >> 20);
    p[25] = (w[9] >> 8) & 4095;
    p[26] = (w[9] << 4) & 4095;
    p[26] |= (w[10] >> 28);
    p[27] = (w[10] >> 16) & 4095;
    p[28] = (w[10] >> 4) & 4095;
    p[29] = (w[10] << 8) & 4095;
    p[29] |= (w[11] >> 24);
    p[30] = (w[11] >> 12) & 4095;
    p[31] = (w[11]) & 4095;
  }
}


__device__ void cunpack13(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 13)
  {
    p[0] = (w[0] >> 19);
    p[1] = (w[0] >> 6) & 8191;
    p[2] = (w[0] << 7) & 8191;
    p[2] |= (w[1] >> 25);
    p[3] = (w[1] >> 12) & 8191;
    p[4] = (w[1] << 1) & 8191;
    p[4] |= (w[2] >> 31);
    p[5] = (w[2] >> 18) & 8191;
    p[6] = (w[2] >> 5) & 8191;
    p[7] = (w[2] << 8) & 8191;
    p[7] |= (w[3] >> 24);
    p[8] = (w[3] >> 11) & 8191;
    p[9] = (w[3] << 2) & 8191;
    p[9] |= (w[4] >> 30);
    p[10] = (w[4] >> 17) & 8191;
    p[11] = (w[4] >> 4) & 8191;
    p[12] = (w[4] << 9) & 8191;
    p[12] |= (w[5] >> 23);
    p[13] = (w[5] >> 10) & 8191;
    p[14] = (w[5] << 3) & 8191;
    p[14] |= (w[6] >> 29);
    p[15] = (w[6] >> 16) & 8191;
    p[16] = (w[6] >> 3) & 8191;
    p[17] = (w[6] << 10) & 8191;
    p[17] |= (w[7] >> 22);
    p[18] = (w[7] >> 9) & 8191;
    p[19] = (w[7] << 4) & 8191;
    p[19] |= (w[8] >> 28);
    p[20] = (w[8] >> 15) & 8191;
    p[21] = (w[8] >> 2) & 8191;
    p[22] = (w[8] << 11) & 8191;
    p[22] |= (w[9] >> 21);
    p[23] = (w[9] >> 8) & 8191;
    p[24] = (w[9] << 5) & 8191;
    p[24] |= (w[10] >> 27);
    p[25] = (w[10] >> 14) & 8191;
    p[26] = (w[10] >> 1) & 8191;
    p[27] = (w[10] << 12) & 8191;
    p[27] |= (w[11] >> 20);
    p[28] = (w[11] >> 7) & 8191;
    p[29] = (w[11] << 6) & 8191;
    p[29] |= (w[12] >> 26);
    p[30] = (w[12] >> 13) & 8191;
    p[31] = (w[12]) & 8191;
  }
}


__device__ void cunpack16(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 16)
  {
    p[0] = (w[0] >> 16);
    p[1] = (w[0]) & 65535;
    p[2] = (w[1] >> 16);
    p[3] = (w[1]) & 65535;
    p[4] = (w[2] >> 16);
    p[5] = (w[2]) & 65535;
    p[6] = (w[3] >> 16);
    p[7] = (w[3]) & 65535;
    p[8] = (w[4] >> 16);
    p[9] = (w[4]) & 65535;
    p[10] = (w[5] >> 16);
    p[11] = (w[5]) & 65535;
    p[12] = (w[6] >> 16);
    p[13] = (w[6]) & 65535;
    p[14] = (w[7] >> 16);
    p[15] = (w[7]) & 65535;
    p[16] = (w[8] >> 16);
    p[17] = (w[8]) & 65535;
    p[18] = (w[9] >> 16);
    p[19] = (w[9]) & 65535;
    p[20] = (w[10] >> 16);
    p[21] = (w[10]) & 65535;
    p[22] = (w[11] >> 16);
    p[23] = (w[11]) & 65535;
    p[24] = (w[12] >> 16);
    p[25] = (w[12]) & 65535;
    p[26] = (w[13] >> 16);
    p[27] = (w[13]) & 65535;
    p[28] = (w[14] >> 16);
    p[29] = (w[14]) & 65535;
    p[30] = (w[15] >> 16);
    p[31] = (w[15]) & 65535;
  }
}


__device__ void cunpack20(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 20)
  {
    p[0] = (w[0] >> 12);
    p[1] = (w[0] << 8) & ((1<<20)-1);
    p[1] |= (w[1] >> 24);
    p[2] = (w[1] >> 4) & ((1<<20)-1);
    p[3] = (w[1] << 16) & ((1<<20)-1);
    p[3] |= (w[2] >> 16);
    p[4] = (w[2] << 4) & ((1<<20)-1);
    p[4] |= (w[3] >> 28);
    p[5] = (w[3] >> 8) & ((1<<20)-1);
    p[6] = (w[3] << 12) & ((1<<20)-1);
    p[6] |= (w[4] >> 20);
    p[7] = (w[4]) & ((1<<20)-1);
    p[8] = (w[5] >> 12);
    p[9] = (w[5] << 8) & ((1<<20)-1);
    p[9] |= (w[6] >> 24);
    p[10] = (w[6] >> 4) & ((1<<20)-1);
    p[11] = (w[6] << 16) & ((1<<20)-1);
    p[11] |= (w[7] >> 16);
    p[12] = (w[7] << 4) & ((1<<20)-1);
    p[12] |= (w[8] >> 28);
    p[13] = (w[8] >> 8) & ((1<<20)-1);
    p[14] = (w[8] << 12) & ((1<<20)-1);
    p[14] |= (w[9] >> 20);
    p[15] = (w[9]) & ((1<<20)-1);
    p[16] = (w[10] >> 12);
    p[17] = (w[10] << 8) & ((1<<20)-1);
    p[17] |= (w[11] >> 24);
    p[18] = (w[11] >> 4) & ((1<<20)-1);
    p[19] = (w[11] << 16) & ((1<<20)-1);
    p[19] |= (w[12] >> 16);
    p[20] = (w[12] << 4) & ((1<<20)-1);
    p[20] |= (w[13] >> 28);
    p[21] = (w[13] >> 8) & ((1<<20)-1);
    p[22] = (w[13] << 12) & ((1<<20)-1);
    p[22] |= (w[14] >> 20);
    p[23] = (w[14]) & ((1<<20)-1);
    p[24] = (w[15] >> 12);
    p[25] = (w[15] << 8) & ((1<<20)-1);
    p[25] |= (w[16] >> 24);
    p[26] = (w[16] >> 4) & ((1<<20)-1);
    p[27] = (w[16] << 16) & ((1<<20)-1);
    p[27] |= (w[17] >> 16);
    p[28] = (w[17] << 4) & ((1<<20)-1);
    p[28] |= (w[18] >> 28);
    p[29] = (w[18] >> 8) & ((1<<20)-1);
    p[30] = (w[18] << 12) & ((1<<20)-1);
    p[30] |= (w[19] >> 20);
    p[31] = (w[19]) & ((1<<20)-1);
  }
}


__device__ void cunpack32(unsigned int *p, unsigned int *w)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i += 32, p += 32, w += 32)
  {
    p[0] = w[0];
    p[1] = w[1];
    p[2] = w[2];
    p[3] = w[3];
    p[4] = w[4];
    p[5] = w[5];
    p[6] = w[6];
    p[7] = w[7];
    p[8] = w[8];
    p[9] = w[9];
    p[10] = w[10];
    p[11] = w[11];
    p[12] = w[12];
    p[13] = w[13];
    p[14] = w[14];
    p[15] = w[15];
    p[16] = w[16];
    p[17] = w[17];
    p[18] = w[18];
    p[19] = w[19];
    p[20] = w[20];
    p[21] = w[21];
    p[22] = w[22];
    p[23] = w[23];
    p[24] = w[24];
    p[25] = w[25];
    p[26] = w[26];
    p[27] = w[27];
    p[28] = w[28];
    p[29] = w[29];
    p[30] = w[30];
    p[31] = w[31];
  }
}

/*modified p4decode */
__device__ unsigned int *detailed_p4_decode_new(unsigned int *_p, unsigned int *_w,  unsigned int * all_array, int delta)
{

  int i, s;
  unsigned int x;
  int flag = _w[0];
  (_w)++;

  unsigned int *_ww,*_pp;
  unsigned int b = ((flag>>10) & 31);
  unsigned int e_n = (flag & 1023) ;

  //(unpack[b])(_p, _w);

  if(b <= 13 ){
	b = (int)b;
  }else if (b == 14){
    b = 16;
  } else if (b == 15){
    b = 20;
  } else if (b == 16) {
    b = 32;
  }
  
  switch(b) { 
	case 0: cunpack0(_p, _w); break;
	case 1: cunpack1(_p, _w); break;
	case 2: cunpack2(_p, _w);break;
	case 3: cunpack3(_p, _w);break;
	case 4: cunpack4(_p, _w);break;
	case 5: cunpack5(_p, _w);break;
	case 6: cunpack6(_p, _w);break;
	case 7: cunpack7(_p, _w);break;
	case 8: cunpack8(_p, _w);break;
	case 9: cunpack9(_p, _w);break;
	case 10: cunpack10(_p, _w);break;
	case 11: cunpack11(_p, _w);break;
	case 12: cunpack12(_p, _w);break;
	case 13: cunpack13(_p, _w);break;
	case 16: cunpack16(_p, _w);break;
	case 20: cunpack20(_p, _w);break;
	case 32: cunpack32(_p, _w);break; 
  }

  //b = cnum[b];
  _w += ((b * BLOCK_SIZE)>>5);
  unsigned int _k = 0;
  unsigned int psum = 0;
  if(e_n != 0 )
  {
    for (_pp = all_array, _ww = (unsigned int *)(_w); _pp < &(all_array[e_n*2]);)
    {
      S16_DECODE(_ww, _pp);
    }

    _w += (_ww - _w);
    psum = all_array[0];

    for(i=0;i<e_n;i++)
    {
      _p[psum] += (all_array[e_n+i]<<b);
      psum += all_array[ i + 1] + 1;
    }
  }

  if(delta) {
    for(i = 1; i < BLOCK_SIZE && _p[i] != 0; i++) {
      _p[i] += _p[i - 1];
    }
  }

  return(_w);
}

__device__ int decompressDocidBlock_GPU(int* pool, unsigned int* outBlock, long pointer) {
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  unsigned int aux[BLOCK_SIZE*4];
  unsigned int* block = (unsigned int*) &pool[pOffset + 5];
  detailed_p4_decode_new(outBlock, block, aux, 1);

  return pool[pOffset + 3];
}

__device__ long nextPointer_GPU(int* pool, long pointer) {
  if(pointer == UNDEFINED_POINTER) {
    return UNDEFINED_POINTER;
  }
  int pSegment = DECODE_SEGMENT(pointer);
  unsigned int pOffset = DECODE_OFFSET(pointer);

  if(pool[pOffset + 1] == UNKNOWN_SEGMENT) {
    return UNDEFINED_POINTER;
  }

  return ENCODE_POINTER(pool[pOffset + 1],
                        pool[pOffset + 2]);
}

__device__ int* intersectPostingsLists_SvS_GPU(int* pool, long a, long b, int minDf) {
  int* set = (int*) malloc(minDf * sizeof(int));
  memset(set, 0, minDf * sizeof(int));

  unsigned int* dataA = (unsigned int*) malloc(BLOCK_SIZE * 2 * sizeof(unsigned int));
  unsigned int* dataB = (unsigned int*) malloc(BLOCK_SIZE * 2 * sizeof(unsigned int));
  memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
  memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));

  int cA = decompressDocidBlock_GPU(pool, dataA, a);
  int cB = decompressDocidBlock_GPU(pool, dataB, b);
  int iSet = 0, iA = 0, iB = 0;

  while(a != UNDEFINED_POINTER && b != UNDEFINED_POINTER) {
    if(dataB[iB] == dataA[iA]) {
      set[iSet++] = dataA[iA];
      iA++;
      iB++;
    }

    if(iA == cA) {
      a = nextPointer_GPU(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cA = decompressDocidBlock_GPU(pool, dataA, a);
      iA = 0;
    }
    if(iB == cB) {
      b = nextPointer_GPU(pool, b);
      if(b == UNDEFINED_POINTER) {
        break;
      }
      memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      cB = decompressDocidBlock_GPU(pool, dataB, b);
      iB = 0;
    }

    if(dataA[iA] < dataB[iB]) {
      if(dataA[cA - 1] < dataB[iB]) {
        iA = cA - 1;
      }
      while(dataA[iA] < dataB[iB]) {
        iA++;
        if(iA == cA) {
          a = nextPointer_GPU(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(dataA, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cA = decompressDocidBlock_GPU(pool, dataA, a);
          iA = 0;
        }
        if(dataA[cA - 1] < dataB[iB]) {
          iA = cA - 1;
        }
      }
    } else {
      if(dataB[cB - 1] < dataA[iA]) {
        iB = cB - 1;
      }
      while(dataB[iB] < dataA[iA]) {
        iB++;
        if(iB == cB) {
          b = nextPointer_GPU(pool, b);
          if(b == UNDEFINED_POINTER) {
            break;
          }
          memset(dataB, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          cB = decompressDocidBlock_GPU(pool, dataB, b);
          iB = 0;
        }
        if(dataB[cB - 1] < dataA[iA]) {
          iB = cB - 1;
        }
      }
    }
  }

  if(iSet < minDf) {
    set[iSet] = TERMINAL_DOCID;
  }

  free(dataA);
  free(dataB);

  return set;
}

__device__ int intersectSetPostingsList_SvS_GPU(int* pool, long a, int* currentSet, int len) {
  unsigned int* data = (unsigned int*) malloc(BLOCK_SIZE * 2 * sizeof(unsigned int));
  memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
  
  int c = decompressDocidBlock_GPU(pool, data, a);
  int iSet = 0, iCurrent = 0, i = 0;

  while(a != UNDEFINED_POINTER && iCurrent < len) {
    if(currentSet[iCurrent] == TERMINAL_DOCID) {
      break;
    }
    if(data[i] == currentSet[iCurrent]) {
      currentSet[iSet++] = currentSet[iCurrent];
      iCurrent++;
      i++;
    }

    if(i == c) {
      a = nextPointer_GPU(pool, a);
      if(a == UNDEFINED_POINTER) {
        break;
      }
      memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      c = decompressDocidBlock_GPU(pool, data, a);
      i = 0;
    }
    if(iCurrent == len) {
      break;
    }
    if(currentSet[iCurrent] == TERMINAL_DOCID) {
      break;
    }

    if(data[i] < currentSet[iCurrent]) {
      if(data[c - 1] < currentSet[iCurrent]) {
        i = c - 1;
      }
      while(data[i] < currentSet[iCurrent]) {
        i++;
        if(i == c) {
          a = nextPointer_GPU(pool, a);
          if(a == UNDEFINED_POINTER) {
            break;
          }
          memset(data, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
          c = decompressDocidBlock_GPU(pool, data, a);
          i = 0;
        }
        if(data[c - 1] < currentSet[iCurrent]) {
          i = c - 1;
        }
      }
    } else {
      while(currentSet[iCurrent] < data[i]) {
        iCurrent++;
        if(iCurrent == len) {
          break;
        }
        if(currentSet[iCurrent] == TERMINAL_DOCID) {
          break;
        }
      }
    }
  }

  if(iSet < len) {
    currentSet[iSet] = TERMINAL_DOCID;
  }

  free(data);
  return iSet;
}

__device__ int* intersectSvS_GPU(int* pool, long* startPointers, int len, int minDf) {
  if(len < 2) {
    unsigned int* block = (unsigned int*) malloc(BLOCK_SIZE * 2 *sizeof(unsigned int));
    memset(block, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
    
    int* set = (int*) malloc(minDf * sizeof(int));
    memset(set, 0, minDf* sizeof(unsigned int));
    
    int iSet = 0;
    long t = startPointers[0];
    while(t != UNDEFINED_POINTER) {
      memset(block, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      int c = decompressDocidBlock_GPU(pool, block, t);
      memcpy(&set[iSet], block, c * sizeof(int));
      iSet += c;
      t = nextPointer_GPU(pool, t);
    }
    free(block);
    return set;
  }

  int* set = intersectPostingsLists_SvS_GPU(pool, startPointers[0], startPointers[1], minDf);
  int i;
  for(i = 2; i < len; i++) {
    intersectSetPostingsList_SvS_GPU(pool, startPointers[i], set, minDf);
  }
  return set;
}

__global__ void SvS_GPU(
	int* queryLength_counter,
	unsigned int queryLength_vocabSize,
	DefaultValue queryLength_defaultValue,	
	int* idToIndexMap_counter,
	unsigned int idToIndexMap_vocabSize,
	DefaultValue idToIndexMap_defaultValue,		
	int* index_df_counter,
	unsigned int index_df_vocabSize,
	DefaultValue index_df_defaultValue,		
	long* index_pointer_counter,
	unsigned int index_pointer_vocabSize,
	DefaultValue index_pointer_defaultValue,	
	int* index_pool_firstseg, //index->pool->pool[0]
	unsigned int index_pool_offset,
	unsigned int index_pool_segment,	
	unsigned int* linearQ,
	int* linearQ_count,
	int totalQuery){

	int id = threadIdx.x + THREADS_PER_BLOCK * blockIdx.x;
	if(id >= queryLength_vocabSize) {
	  return;
	}
	
	if(queryLength_counter[id] == queryLength_defaultValue) {
	  return;
	}

	 printf("Start QID = %d\n", id);
	  // Measure elapsed time
	  int i, j;
	  int qlen = queryLength_counter[id];
	  int qindex = idToIndexMap_counter[id];
	  if (qindex > totalQuery){
	  	printf("Exceed the range!\n");
		return;
	  }
	  
	  unsigned int* qdf = (unsigned int*) malloc(qlen * sizeof(unsigned int));
	  memset(qdf, 0, qlen * sizeof(unsigned int));
	  
	  int* sortedDfIndex = (int*) malloc(qlen * sizeof(int));
	  memset(sortedDfIndex, 0, qlen * sizeof(unsigned int));

	  long* qStartPointers = (long*) malloc(qlen * sizeof(long));
	  memset(qStartPointers, 0, qlen * sizeof(unsigned int));
	  
	  int end = linearQ_count[qindex];
	  int start = 0;
	  if (qindex > 0){
		start = linearQ_count[qindex-1];
	  }
	  
	  if (linearQ[start]>= index_df_vocabSize ){
		printf("DF range exceeded\n");
		return;
	  }
	  qdf[0] = index_df_counter[linearQ[start]];//getDf(index->pointers, queries[qindex][0]);
	  unsigned int minimumDf = qdf[0];
	  for(i = 1; i < qlen; i++) {
	  	if(start+i > end){
			printf("out of range 1 \n");
			return;
	  	}
		  if (linearQ[start+i]>= index_df_vocabSize ){
			printf("DF range exceeded - Inside Loop - Not possible!\n");
			return;
		  }
		qdf[i] = index_df_counter[linearQ[start+i]];//getDf(index->pointers, queries[qindex][i]);
		if(qdf[i] < minimumDf) {
			  minimumDf = qdf[i];
		}
	  }	
	
	  // Sort query terms w.r.t. df
	  for(i = 0; i < qlen; i++) {
		unsigned int minDf = 0xFFFFFFFF;
		for(j = 0; j < qlen; j++) {
		  if(qdf[j] < minDf) {
			minDf = qdf[j];
			sortedDfIndex[i] = j;
		  }
		}
		qdf[sortedDfIndex[i]] = 0xFFFFFFFF;
	  }
	
	  for(i = 0; i < qlen; i++) {
	  	if(start+sortedDfIndex[i] > end){
			printf("out of range 2\n");
			return;
	  	}
		if (linearQ[start+sortedDfIndex[i]]>= index_pointer_vocabSize){
			printf("Pointer range exceeded - Inside Second Loop - Not possible!\n");
			return;
		}
		qStartPointers[i] = index_pointer_counter[linearQ[start+sortedDfIndex[i]]]; //getStartPointer(index->pointers, queries[qindex][sortedDfIndex[i]]);
		if (linearQ[start+sortedDfIndex[i]]>= index_df_vocabSize ){
			printf("DF range exceeded - Inside Second Loop - Not possible!\n");
			return;
		}
		qdf[i] = index_df_counter[linearQ[start+sortedDfIndex[i]]];
		//qdf[i] = getDf(index->pointers, queries[qindex][sortedDfIndex[i]]);
	  }
	
	  // Compute intersection set (or in disjunctive mode, top-k)
	  int* set;	  
	  int hits = minimumDf;
	  set = intersectSvS_GPU(index_pool_firstseg, qStartPointers, qlen, minimumDf);
	  	
	  // If output is specified, write the retrieved set to output
	  /*if(outputPath) {
		printf("Output\n");
		for(i = 0; i < hits && set[i] != TERMINAL_DOCID; i++) {
		  fprintf(fp, "q: %d no: %u\n", id, set[i]);
		}
	  } else {*/
		for(i = 0; i < hits && set[i] != TERMINAL_DOCID; i++) {
			printf("q: %d no: %u\n", id, set[i]);
		}
	  //}
	
	  // Free the allocated memory
	  free(set);
	  free(qdf);
	  free(sortedDfIndex);
	  free(qStartPointers);
}

void SvS_GPU_Entry(
	FixedIntCounter* queryLength, 
	FixedIntCounter* idToIndexMap, 
	char* outputPath, 
	InvertedIndex* index, 
	FILE * fp,
	int totalQuery,
	unsigned int* linearQ,
	int* linearQ_count,
	int tt){
	
	int i, j;
	int id = -1;
	int fqlen, pos, termid;	
	int hits = 1000;
	//Algorithm algorithm = SVS;

	printf("INside SvS GPU Entry!!!\n");
	if(queryLength==NULL || idToIndexMap == NULL || index == NULL){
		printf("NULLL\n");
	}	

	fprintf(stderr, "Start SvS Data Transfer\n");

	struct timeval transferstart, transferend, gpustart, gpuend;
	gettimeofday(&transferstart, NULL);
	int* queryLength_counter;
	int* idToIndexMap_counter;
	int* index_df_counter;
	long* index_pointer_counter;
	int* index_pool_firstseg;
	unsigned int* linearQ_cuda;
	int* linearQ_count_cuda;

	cudaMalloc((void**)&(queryLength_counter), 32768*sizeof(int));
	cudaMalloc((void**)&(idToIndexMap_counter), 32768*sizeof(int));
	cudaMalloc((void**)&(index_df_counter), DEFAULT_VOCAB_SIZE*sizeof(int));
	cudaMalloc((void**)&(index_pointer_counter), DEFAULT_VOCAB_SIZE*sizeof(long));
	cudaMalloc((void**)&(index_pool_firstseg), index->pool->offset*sizeof(int));	
	cudaMalloc((void**)&(linearQ_cuda), tt*sizeof(unsigned int));
	cudaMalloc((void**)&(linearQ_count_cuda), totalQuery*sizeof(int));

	cudaMemcpy(queryLength_counter, queryLength->counter, 32768*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(idToIndexMap_counter, idToIndexMap->counter, 32768*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(index_df_counter, index->pointers->df->counter, DEFAULT_VOCAB_SIZE*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(index_pointer_counter, index->pointers->startPointers->counter, DEFAULT_VOCAB_SIZE*sizeof(long), cudaMemcpyHostToDevice);
	cudaMemcpy(index_pool_firstseg, index->pool->pool[0], index->pool->offset*sizeof(int), cudaMemcpyHostToDevice);

	cudaMemcpy(linearQ_cuda, linearQ, tt*sizeof(unsigned int), cudaMemcpyHostToDevice);
	cudaMemcpy(linearQ_count_cuda, linearQ_count, totalQuery*sizeof(int), cudaMemcpyHostToDevice);

	gettimeofday(&transferend, NULL);

	gettimeofday(&gpustart, NULL);
	dim3  block(THREADS_PER_BLOCK, 1);
	dim3  grid((totalQuery + THREADS_PER_BLOCK - 1)/THREADS_PER_BLOCK, 1);
      pirntf("Query Number: %d - Block Number %d\n", totalQuery, grid.x);
      
	SvS_GPU<<<grid, block>>>(
		queryLength_counter,
		queryLength->vocabSize,//queryLength_vocabSize,
		queryLength->defaultValue,//queryLength_defaultValue,	
		idToIndexMap_counter,
		idToIndexMap->vocabSize,
		idToIndexMap->defaultValue,		
		index_df_counter,
		index->pointers->df->vocabSize,//_df_vocabSize,
		index->pointers->df->defaultValue,		
		index_pointer_counter,
		index->pointers->startPointers->vocabSize,
		index->pointers->startPointers->defaultValue,
		index_pool_firstseg, //index->pool->pool[0]
		index->pool->offset,
		index->pool->segment,
		linearQ_cuda,
		linearQ_count_cuda,
		totalQuery);

	gettimeofday(&gpuend, NULL);

	printf("Transfer Timing: %10.0f\n",
		   ((float) ((transferend.tv_sec * 1000000 + transferend.tv_usec) -
					 (transferstart.tv_sec * 1000000 + transferstart.tv_usec))));
	printf("GPU Timing: %10.0f\n",
		   ((float) ((gpuend.tv_sec * 1000000 + gpuend.tv_usec) -
					 (gpustart.tv_sec * 1000000 + gpustart.tv_usec))));

}



int main (int argc, char** args) {
  // Index path
  char* inputPath = getValueCL(argc, args, "-index");
  // Query path
  char* queryPath = getValueCL(argc, args, "-query");
  // Output path (optional)
  char* outputPath = getValueCL(argc, args, "-output");
  // Hits
  int hits = 1000;
  if(isPresentCL(argc, args, "-hits")) {
    hits = atoi(getValueCL(argc, args, "-hits"));
  }
  // Algorithm
  //char* intersectionAlgorithm = getValueCL(argc, args, "-algorithm");
  //Algorithm algorithm = SVS;

  // Algorithm is limited to the following list (case sensitive):
  // - SvS (conjunctive)
  // - WAND (disjunctive)
  /*if(!strcmp(intersectionAlgorithm, "SvS")) {
    algorithm = SVS;
  } else if(!strcmp(intersectionAlgorithm, "WAND")) {
    algorithm = WAND;
  } else {
    printf("Invalid algorithm (Options: SvS | WAND)\n");
    return;
  }*/

  // Read the inverted index
  printf("Start reading!\n");
  InvertedIndex* index = readInvertedIndex(inputPath);
  printf("Done reading!\n");
  // Read queries. Query file must be in the following format:
  // - First line: <number of queries: integer>
  // - <query id: integer> <query length: integer> <query text: string>
  // Note that, if a query term does not have a corresponding postings list,
  // then we drop the query term from the query. Empty queries are not evaluated.
  FixedIntCounter* queryLength = createFixedIntCounter(32768, ZERO);
  FixedIntCounter* idToIndexMap = createFixedIntCounter(32768, ZERO);
  FILE* fp = fopen(queryPath, "r");
  int totalQueries = 0, id, qlen, fqlen, j, pos, termid, i;
  char query[1024];
  fscanf(fp, "%d", &totalQueries);
  //unsigned int** queries = (unsigned int**) malloc(totalQueries * sizeof(unsigned int*));
  unsigned int* linearQ = (unsigned int*) malloc(100 * totalQueries * sizeof(unsigned int));
  int* linearQ_count = (int*) malloc(totalQueries * sizeof(unsigned int));

  int totalLen = 0;
  for(i = 0; i < totalQueries; i++) {
    fscanf(fp, "%d %d", &id, &qlen);
    //queries[i] = (unsigned int*) malloc(qlen * sizeof(unsigned int));
    pos = 0;
    fqlen = qlen;
    for(j = 0; j < qlen; j++) {
      fscanf(fp, "%s", query);
      termid = getTermId(index->dictionary, query);
      if(termid >= 0) {
        if(getStartPointer(index->pointers, termid) != UNDEFINED_POINTER) {
			linearQ[totalLen] = termid;
			totalLen++;
          //queries[i][pos++] = termid;
        } else {
          fqlen--;
        }
      } else {
        fqlen--;
      }
    }
    setFixedIntCounter(idToIndexMap, id, i);
    setFixedIntCounter(queryLength, id, fqlen);
	linearQ_count[i] = totalLen;
  }
  fclose(fp);

  if(outputPath) {
    fp = fopen(outputPath, "w");
  }

  // Evaluate queries by iterating over the queries that are not empty
  id = -1;

/////////////////////// CUDA Entry
  SvS_GPU_Entry(
  	queryLength, 
  	idToIndexMap, 
  	outputPath, 
  	index, 
  	fp, 
  	totalQueries,
  	linearQ,
  	linearQ_count,
  	totalLen);
//////////////////////

  if(outputPath) {
    fclose(fp);
  }
  /*for(i = 0; i < totalQueries; i++) {
    if(queries[i]) {
      free(queries[i]);
    }
  }
  free(queries);*/
  destroyFixedIntCounter(queryLength);
  destroyFixedIntCounter(idToIndexMap);
  destroyInvertedIndex(index);
  return 0;
}


