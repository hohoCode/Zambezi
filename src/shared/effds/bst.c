/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

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


/* mainloop showing bstinsert, etc in use */
main()
{
    void	bstsearch(ANSREC *, char *);
    void	bstinsert(ANSREC *, char *);
    void	printtree(ANSREC *);
    ANSREC	ans;
    char	buf[100];

    ans.root = ans.ans = NULL;

    while( gets(buf) != NULL )
    {
	bstinsert(&ans, buf);
    }

    printtree(&ans);

    exit(0);
}



/* Search for word in a bst. */
void
bstsearch(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root;
    int		val;

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

    return;
}


/* Insert word into a bst.  */
void
bstinsert(ANSREC *ans, char *word)
{
    TREEREC	*curr = ans->root, *prev = NULL, *wcreate();
    int		val;

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
