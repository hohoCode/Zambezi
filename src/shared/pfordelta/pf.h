#ifndef P4_H_GUARD
#define p4_H_GUARD

#include <stdlib.h>
#include "s16head.h"
#include "unpack.h"


#define BLOCK_SIZE 128
#define FRAC 0.10
#define S 16
#define PCHUNK 128

void pack(unsigned int *v, unsigned int b, unsigned int n, unsigned int *w);


int detailed_p4_encode(unsigned int **w, unsigned int* p, int num , int *chunk_size, int * exception_n)
{
  int i, j, t, s;

  unsigned int b = cnum[num];
  int bb_e;
  int bb_p;
  int p_low;
  unsigned int e_n = 0;
  int max_p = 0;
  int max_e = 0;

  unsigned int* out = (unsigned*)malloc(sizeof(unsigned)*PCHUNK*2);
  unsigned int* ex = (unsigned*)malloc(sizeof(unsigned)*PCHUNK*2);
  unsigned int* po = (unsigned*)malloc(sizeof(unsigned)*PCHUNK*2);

  unsigned int* tp = NULL;
  unsigned int *_pp, *_ww;

  if (b == 32)
  {
    (*w)[0] = ((b<<10)) + (0);
    *w +=1;
    for (i = 0; i < PCHUNK ; i++)  (*w)[i] = p[i];
    *w += (PCHUNK);
    (*chunk_size) = 1 + BLOCK_SIZE;

    free(out);
    free(ex);
    free(po);
    return 0;
  }

  for (i = 0; i < PCHUNK ; i++)
  {
    if ( p[i] >= (1<<b) )    //exception
    {
      p_low = p[i] & ((1<<b)-1);
      out[i] = p_low;
      ex[e_n] = (p[i] >> b);
      po[(e_n++)] = i;
    }
    else
      out[i] = p[i];
  }

  if (1)    // force to pass every time
  {
    /*get the gap of position*/
    for(j = e_n-1;j>0;j--)
    {
      po[j] = po[j] - po[j-1] ;
      po[j] --;
    }

    s = ((b * PCHUNK)>>5);
    tp = (*w);
    (*w)[0] = ((num<<10))+e_n;      // record b and number of exceptions into this value, in the other version we pick this value out and did not count it
    (*w) += 1;
    for (i = 0; i < s; i++)  (*w)[i] = 0;
    pack(out, b, PCHUNK , *w);
    *w += s;

    unsigned int *all_array = (unsigned*)malloc(sizeof(unsigned)*PCHUNK*4) ;
    for(j=0;j<e_n;j++)
    {
      all_array[j] = po[j];
      all_array[e_n+j] =ex[j];
    }
    for (_pp = all_array, _ww = (*w); _pp < &(all_array[2*e_n]); )
      s16_encode(&_ww, &_pp, &(all_array[2*e_n]) - _pp);

    (*chunk_size) = 1 + s + (_ww - (*w)) ;

    (*w) += (_ww - (*w)) ;

    (*exception_n) = e_n;

    free(out);
    free(ex);
    free(po);
    free(all_array);
    return (e_n);

  }
}


void pack(unsigned int *v, unsigned int b, unsigned int n, unsigned int *w)
{
  int i, bp, wp, s;

  for (bp = 0, i = 0; i < n; i++, bp += b)
  {
    wp = bp>>5;
    s = 32 - b - (bp & 31);
    if (s >= 0)
      w[wp] |= (v[i]<<s);
    else
    {
      s = -s;
      w[wp] |= (v[i]>>s);
      w[wp+1] = (v[i]<<(32-s));
    }
  }
}

/*modified p4decode */
__host__ __device__ unsigned int *detailed_p4_decode(unsigned int *_p, unsigned int *_w,  unsigned int * all_array, int delta)
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
  
  switch(b) { */
	case 0: unpack0(_p, _w); break;
	case 1: unpack1(_p, _w); break;
	case 2: unpack2(_p, _w);break;
	case 3: unpack3(_p, _w);break;
	case 4: unpack4(_p, _w);break;
	case 5: unpack5(_p, _w);break;
	case 6: unpack6(_p, _w);break;
	case 7: unpack7(_p, _w);break;
	case 8: unpack8(_p, _w);break;
	case 9: unpack9(_p, _w);break;
	case 10: unpack10(_p, _w);break;
	case 11: unpack11(_p, _w);break;
	case 12: unpack12(_p, _w);break;
	case 13: unpack13(_p, _w);break;
	case 16: unpack16(_p, _w);break;
	case 20: unpack20(_p, _w);break;
	case 32: unpack32(_p, _w);break; 
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

#endif
