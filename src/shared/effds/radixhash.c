/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

/* Simple seeded radix hash */
unsigned int
radixhash_seed(char *s, int tsize, int seed)
{
    int         hval = 0;

#define LARGEPRIME 100000007

    for( ; *s != '\0' ; s++ )
    {
        hval = ( seed*hval + *s ) % LARGEPRIME;
    }
    hval = hval % tsize;
    if( hval < 0 )
	hval += tsize;

    return(hval);
}


/* Unseeded radix hash, from Sedgewick 1998 */
unsigned int
radixhash_Sedgewick(char *s, int tsize, int seed)
{
    int         hval = 0;
    int		r = 1;

#define OTHERLARGEPRIME 638767

    for( ; *s != '\0' ; s++ )
    {
	r = ( r * OTHERLARGEPRIME ) % (tsize-1);
        hval = ( r*hval + *s ) % tsize;
    }
    if( hval < 0 )
	hval += tsize;

    return(hval);
}
