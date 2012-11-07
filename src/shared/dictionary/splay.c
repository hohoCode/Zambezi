/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

#include <stdio.h>

#define ROTATEFAC 11


typedef struct wordrec
{
    char	*word;
    struct wordrec *left, *right;
    struct wordrec *par;
} TREEREC;


typedef struct ansrec
{
    struct wordrec *root;
    struct wordrec *ans;
} ANSREC;


/* mainloop showing splayinsert, etc in use */
main()
{
    void	splaysearch(ANSREC *, char *);
    void	splayinsert(ANSREC *, char *);
    void	printtree(ANSREC *);
    ANSREC	ans;
    char	buf[100];

    ans.root = ans.ans = NULL;

    while( gets(buf) != NULL )
    {
	splayinsert(&ans, buf);
    }

    printtree(&ans);

    exit(0);
}


#define ONELEVEL(dir,rid) \
    { \
	par->dir = curr->rid; \
	if( par->dir!=NULL ) \
	    par->dir->par = par; \
	curr->rid = par; \
	par->par = curr; \
	curr->par = NULL; \
    }

#define ZIGZIG(dir,rid) \
    { \
	curr->par = gpar->par; \
	if( curr->par!=NULL ) \
	{ \
	    if( curr->par->dir==gpar ) \
		curr->par->dir = curr; \
	    else \
		curr->par->rid = curr; \
	} \
	gpar->dir = par->rid; \
	if( gpar->dir!=NULL ) \
	    gpar->dir->par = gpar; \
	par->dir = curr->rid; \
	if( curr->rid!=NULL ) \
	    curr->rid->par = par; \
	curr->rid = par; \
	par->par = curr; \
	par->rid = gpar; \
	gpar->par = par; \
    }

#define ZIGZAG(dir,rid) \
    { \
	curr->par = gpar->par; \
	if( curr->par!=NULL ) \
	{ \
	    if( curr->par->dir==gpar ) \
		curr->par->dir = curr; \
	    else \
		curr->par->rid = curr; \
	} \
	par->rid = curr->dir; \
	if( par->rid!=NULL ) \
	    par->rid->par = par; \
	gpar->dir = curr->rid; \
	if( gpar->dir!=NULL ) \
	    gpar->dir->par = gpar; \
	curr->dir = par; \
	par->par = curr; \
	curr->rid = gpar; \
	gpar->par = curr; \
    }


int    scount = ROTATEFAC;


/* Search for word in a splay tree.  If word is found, bring it to
   root, possibly intermittently.  Structure ans is used to pass
   in the root, and to pass back both the new root (which may or
   may not be changed) and the looked-for record. */
void
splaysearch(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root, *par, *gpar;
    int		val;

    scount--;

    if( ans->root == NULL )
    {
	ans->ans = NULL;
	return;
    }
    while( curr != NULL && (val = scmp(word, curr->word)) != 0 )
    {
	if( val > 0 )
	    curr = curr->right;
	else
	    curr = curr->left;
    }

    ans->ans = curr;

    if( curr==ans->root )
    {
	return;
    }

    if( scount<=0 && curr!=NULL )    /* Move node towards root */
    {
	scount = ROTATEFAC;

	while( (par = curr->par) != NULL )
	{
	    if( par->left==curr )
	    {
		if( (gpar = par->par) == NULL )
		    ONELEVEL(left,right)
		else if( gpar->left == par )
		    ZIGZIG(left,right)
		else
		    ZIGZAG(right,left)
	    }
	    else
	    {
		if( (gpar = par->par) == NULL )
		    ONELEVEL(right,left)
		else if( gpar->left == par )
		    ZIGZAG(left,right)
		else
		    ZIGZIG(right,left)
	    }
	}
	ans->root = curr;
    }

    return;
}


/* Insert word into a splay tree.  If word is already present, bring it to
   root, possibly intermittently.  Structure ans is used to pass
   in the root, and to pass back both the new root (which may or
   may not be changed) and the looked-for record. */
void
splayinsert(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root, *par, *gpar, *prev = NULL, *wcreate();
    int		val;

    scount--;

    if( ans->root == NULL )
    {
	ans->ans = ans->root = wcreate(word, NULL);
	return;
    }

    while( curr != NULL && (val = scmp(word, curr->word)) != 0 )
    {
	prev = curr;
	if( val > 0 )
	    curr = curr->right;
	else
	    curr = curr->left;
    }

    if( curr == NULL )
    {
	if( val > 0 )
	    curr = prev->right = wcreate(word, prev);
	else
	    curr = prev->left = wcreate(word, prev);
    }

    ans->ans = curr;

    if( scount<=0 )    /* Move node towards root */
    {
	scount = ROTATEFAC;

	while( (par = curr->par) != NULL )
	{
	    if( par->left==curr )
	    {
		if( (gpar = par->par) == NULL )
		    ONELEVEL(left,right)
		else if( gpar->left == par )
		    ZIGZIG(left,right)
		else
		    ZIGZAG(right,left)
	    }
	    else
	    {
		if( (gpar = par->par) == NULL )
		    ONELEVEL(right,left)
		else if( gpar->left == par )
		    ZIGZAG(left,right)
		else
		    ZIGZIG(right,left)
	    }
	}
	ans->root = curr;
    }

    return;
}


/* Create a node to hold a word */
TREEREC *
wcreate(char *word, TREEREC *par)
{
    TREEREC    *tmp;

    tmp = (TREEREC *) malloc(sizeof(TREEREC));
    tmp->word = (char *) malloc(strlen(word) + 1);
    strcpy(tmp->word, word);
    tmp->left = tmp->right = NULL;

    tmp->par = par;

    return(tmp);
}


int
scmp( char *s1, char *s2 )
{
    while( *s1 != '\0' && *s1 == *s2 )
    {
	s1++;
	s2++;
    }
    return( *s1-*s2 );
}


void
printtree(ANSREC *ans)
{
    void do_printtree(int, TREEREC *);

    if( ans->root!=NULL )
	do_printtree(0, ans->root);

    return;
}


void
do_printtree(int d, TREEREC *wt)
{
    int		i;

    if( wt->left!=NULL )
    {
	do_printtree(d+1, wt->left);
    }

    printf("%3d ", d);
    for( i=0 ; i<d ; i++ )
	printf(" ");
    printf("%s\n", wt->word);

    if( wt->right!=NULL )
    {
	do_printtree(d+1, wt->right);
    }

    return;
}
