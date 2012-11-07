/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

/* Binary trees for strings, the way it should be done in principle.
   During traversal, track is kept of the immediate left and right
   bounds of the search string.  Any lead characters these have in
   common do not need to be checked, thus saving some work in
   string comparisons.  However, there are some extra tests involved,
   so there may be no saving in practice.

   I've put in two versions of "bstcsearch" and "bstcinsert".
   The first is a simple one, in the second I have unrolled the loop
   to reduce the volume of tests and arithmetic required during
   traversal.
*/

#include <stdio.h>


typedef struct wordrec
{
    char	*word;
    struct wordrec *left, *right;
} TREEREC;


typedef struct ansrec
{
    struct wordrec *root;
    struct wordrec *ans;
} ANSREC;


/* mainloop showing bstcinsert, etc in use */
main()
{
    void	bstcsearch(ANSREC *, char *);
    void	bstcinsert(ANSREC *, char *);
    void	printtree(ANSREC *);
    ANSREC	ans;
    char	buf[100];

    ans.root = ans.ans = NULL;

    while( gets(buf) != NULL )
    {
	bstcinsert(&ans, buf);
    }

    printtree(&ans);

    exit(0);
}



/* Search for word in a bst.  As the search proceeds, omit comparisons
   on the parts of strings that are known to be identical. */
void
bstcsearch(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root;
    char	*lbound = NULL, *rbound = NULL;
    int		val, off = 0;

    if( ans->root == NULL )
    {
	ans->ans = NULL;
	return;
    }

    while( curr != NULL && (val = scmp(word+off, curr->word+off)) != 0 )
    {
	if( val > 0 )
	{
	    lbound = curr->word;
	    curr = curr->right;
	}
	else
	{
	    rbound = curr->word;
	    curr = curr->left;
	}
	    /* strictly speaking this should be a while loop, to keep
	       counting up until the chars of lbound and rbound differ,
	       but they are more expensive to set up and almost never
	       go through more than once */
	if( lbound!=NULL && rbound!=NULL && *(lbound+off) == *(rbound+off) )
	    off++;
    }

    ans->ans = curr;

    return;
}


/* Search for a word in a bst.  As the search proceeds, omit comparisons
   on the parts of strings that are known to be identical.  Faster
   than the previous version due to partial unrolling of the main
   iteration. */
void
bstcsearchgood(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root;
    char	*w, *lbound = NULL, *rbound = NULL;
    int		val, off;

    if( ans->root == NULL )
    {
	ans->ans = NULL;
	return;
    }

    while( curr != NULL && (val = scmp(word, curr->word)) != 0
				&& ( lbound==NULL || rbound==NULL ) )
    {
	prev = curr;
	if( val > 0 )
	{
	    lbound = curr->word;
	    curr = curr->right;
	}
	else
	{
	    rbound = curr->word;
	    curr = curr->left;
	}
    }

    if( val!=0 )
    {
	off = 0;
	w = word;
	while( curr != NULL && (val = scmp(w, curr->word+off)) != 0 )
	{
	    prev = curr;
	    if( val > 0 )
	    {
		lbound = curr->word;
		curr = curr->right;
	    }
	    else
	    {
		rbound = curr->word;
		curr = curr->left;
	    }
	    if( *(lbound+off) == *(rbound+off) )
	    {
		off++;
		w++;
	    }
	}
    }

    ans->ans = curr;

    return;
}


/* Insert word into a bst.  As the search proceeds, omit comparisons
   on the parts of strings that are known to be identical. */
void
bstcinsert(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root, *prev = NULL, *wcreate();
    char	*lbound = NULL, *rbound = NULL;
    int		val, off = 0;

    if( ans->root == NULL )
    {
	ans->ans = ans->root = wcreate(word, NULL);
	return;
    }

    while( curr != NULL && (val = scmp(word+off, curr->word+off)) != 0 )
    {
	prev = curr;
	if( val > 0 )
	{
	    lbound = curr->word;
	    curr = curr->right;
	}
	else
	{
	    rbound = curr->word;
	    curr = curr->left;
	}
	if( lbound!=NULL && rbound!=NULL && *(lbound+off) == *(rbound+off) )
	{
	    off++;
	}
    }

    if( curr == NULL )
    {
	if( val > 0 )
	    curr = prev->right = wcreate(word, prev);
	else
	    curr = prev->left = wcreate(word, prev);
    }

    ans->ans = curr;

    return;
}


/* Insert word into a bst.  As the search proceeds, omit comparisons
   on the parts of strings that are known to be identical.  Faster
   than the previous version due to partial unrolling of the main
   iteration. */
void
bstcinsertgood(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root, *prev = NULL, *wcreate();
    char	*w, *lbound = NULL, *rbound = NULL;
    int		val, off;

    if( ans->root == NULL )
    {
	ans->ans = ans->root = wcreate(word, NULL);
	return;
    }

    while( curr != NULL && (val = scmp(word, curr->word)) != 0
				&& ( lbound==NULL || rbound==NULL ) )
    {
	prev = curr;
	if( val > 0 )
	{
	    lbound = curr->word;
	    curr = curr->right;
	}
	else
	{
	    rbound = curr->word;
	    curr = curr->left;
	}
    }

    if( val!=0 )
    {
	off = 0;
	w = word;
	while( curr != NULL && (val = scmp(w, curr->word+off)) != 0 )
	{
	    prev = curr;
	    if( val > 0 )
	    {
		lbound = curr->word;
		curr = curr->right;
	    }
	    else
	    {
		rbound = curr->word;
		curr = curr->left;
	    }
	    if( *(lbound+off) == *(rbound+off) )
	    {
		off++;
		w++;
	    }
	}
    }

    if( curr == NULL )
    {
	if( val > 0 )
	    curr = prev->right = wcreate(word, prev);
	else
	    curr = prev->left = wcreate(word, prev);
    }

    ans->ans = curr;

    return;
}


/* Create a node to hold a word */
TREEREC *
wcreate(char *word)
{
    TREEREC    *tmp;

    tmp = (TREEREC *) malloc(sizeof(TREEREC));
    tmp->word = (char *) malloc(strlen(word) + 1);
    strcpy(tmp->word, word);
    tmp->left = tmp->right = NULL;

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
