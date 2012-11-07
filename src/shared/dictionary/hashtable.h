/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

#include "bitwisehash.h"
#include "scmp.h"

#define TSIZE  134217728
#define SEED  1159241
#define HASHFN  bitwisehash

typedef struct hashrec {
  char  *word;
  int id;
  struct hashrec *next;
} Dictionary;


/* Create hash table, initialise ptrs to NULL */
Dictionary ** inithashtable() {
  int i;
  Dictionary **ht;
  ht = (Dictionary **) malloc( sizeof(Dictionary *) * TSIZE );

  for( i=0 ; i<TSIZE ; i++ )
    ht[i] = (Dictionary *) NULL;

  return(ht);
}

void destroyhashtable(Dictionary **ht) {
  int i;
  for(i = 0; i < TSIZE; i++) {
    Dictionary* htmp = ht[i];
    Dictionary* next = NULL;
    while(htmp != NULL) {
      free(htmp->word);
      next = htmp->next;
      free(htmp);
      htmp = next;
    }
  }
  free(ht);
}

/* Search hash table for given string, return record if found, else NULL */
int hashsearch(Dictionary **ht, char *w) {
  Dictionary  *htmp, *hprv;
  unsigned int hval = HASHFN(w, TSIZE, SEED);

  for( hprv = NULL, htmp=ht[hval]
         ; htmp != NULL && scmp(htmp->word, w) != 0
         ; hprv = htmp, htmp = htmp->next )
    {
      ;
    }

  if( hprv!=NULL && htmp!=NULL ) /* move to front on access */
    {
      hprv->next = htmp->next;
      htmp->next = ht[hval];
      ht[hval] = htmp;
    }

  if(htmp == NULL return -1;
  return htmp->id;
}


/* Search hash table for given string, insert if not found */
int hashinsert(Dictionary **ht, char *w, int id) {
  Dictionary  *htmp, *hprv;
  unsigned int hval = HASHFN(w, TSIZE, SEED);

  for( hprv = NULL, htmp=ht[hval]
         ; htmp != NULL && scmp(htmp->word, w) != 0
         ; hprv = htmp, htmp = htmp->next )
    {
      ;
    }

  if( htmp==NULL )
    {
      htmp = (Dictionary *) malloc( sizeof(Dictionary) );
      htmp->word = (char *) malloc( strlen(w) + 1 );
      strcpy(htmp->word, w);
      htmp->id = id;
      htmp->next = NULL;
      if( hprv==NULL )
        ht[hval] = htmp;
      else
        hprv->next = htmp;

  /* new records are not moved to front */
    }
  else
    {
      if( hprv!=NULL ) /* move to front on access */
        {
          hprv->next = htmp->next;
          htmp->next = ht[hval];
          ht[hval] = htmp;
        }
    }

  return htmp->id;
}

void writehashtable(Dictionary **ht, FILE* fp) {
  int terminal = -1;
  int i, l;
  Dictionary* ptr;

  for( i=0 ; i<TSIZE ; i++ ) {
    for(ptr=ht[i] ; ptr!=NULL ; ptr=ptr->next) {
      fwrite(&ptr->id, sizeof(int), 1, fp);
      l = strlen(ptr->word);
      fwrite(&l, sizeof(int), 1, fp);
      fwrite(&ptr->word, sizeof(char), strlen(ptr->word), fp);
    }
  }
  fwrite(&terminal, sizeof(int), 1, fp);
}

Dictionary** readhashtable(FILE* fp) {
  Dictionary** ht = inithash();
  int id, i, l;
  char term[1048576];
  fread(&id, sizeof(int), 1, fp);
  while(id != -1) {
    fread(&l, sizeof(int), 1, fp);
    fread(term, sizeof(char), l, fp);
    hashinsert(ht, term, id);
    fread(&id, sizeof(int), 1, fp);
  }
  return ht;
}
