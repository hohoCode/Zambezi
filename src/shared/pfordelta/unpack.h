#ifndef UNPACK_H_GUARD
#define UNPACK_H_GUARD

/*************************************************************/
/* macros for fast unpacking of integers of fixed bit length */
/*************************************************************/

#define BLOCK_SIZE 128

/* supported bit lengths */
int cnum[17] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,16,20,32};

__device__ __host__ void unpack0(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack1(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack2(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack3(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack4(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack5(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack6(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack7(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack8(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack9(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack10(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack11(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack12(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack13(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack16(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack20(unsigned int *p, unsigned int *w);

__host__ __device__ void unpack32(unsigned int *p, unsigned int *w);

typedef void (*pf)(unsigned int *p, unsigned int *w);
/*pf unpack[17] = {unpack0, unpack1, unpack2, unpack3, unpack4, unpack5,
                 unpack6, unpack7, unpack8, unpack9, unpack10, unpack11,
                 unpack12, unpack13, unpack16, unpack20, unpack32};*/

#endif
