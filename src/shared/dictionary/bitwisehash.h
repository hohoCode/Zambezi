/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

/* Bitwise hash function.  Note that tsize does not have to be prime. */
unsigned int bitwisehash(char *word, int tsize, unsigned int seed) {
  char c;
  unsigned int h;

  h = seed;
  for(; ( c=*word )!='\0' ; word++) {
    h ^= ((h << 5) + c + (h >> 2));
  }
  return((unsigned int)((h&0x7fffffff) % tsize));
}
