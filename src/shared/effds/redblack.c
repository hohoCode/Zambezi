/* Author J. Zobel, April 2001.
   Permission to use this code is freely granted, provided that this
   statement is retained. */

#include <stdio.h>

typedef struct wordrec
{
    char	*word;
    struct wordrec *left, *right;
    struct wordrec *par;
    char	colour;
} TREEREC;

typedef struct ansrec
{
    struct wordrec *root;
    struct wordrec *ans;
} ANSREC;

#define RED		0
#define BLACK		1


/* mainloop showing redblackinsert, etc in use */
main()
{
    void	redblacksearch(ANSREC *, char *);
    void	redblackinsert(ANSREC *, char *);
    void	printtree(ANSREC *);
    ANSREC	ans;
    char	buf[100];

    ans.root = ans.ans = NULL;

    while( gets(buf) != NULL )
    {
	redblackinsert(&ans, buf);
    }

    printtree(&ans);

    exit(0);
}


/* Find word in a redblack tree */
void
redblacksearch(ANSREC *ans, char *word)
{
    TREEREC    *curr = ans->root;
    int		val;

    if( ans->root != NULL )
    {
	while( curr != NULL && (val = scmp(word, curr->word)) != 0 )
	{
	    if( val > 0 )
		curr = curr->right;
	    else
		curr = curr->left;
	}
    }

    ans->ans = curr;

    return;
}


/* Rotate the right child of par upwards */
/* Could be written as a macro, but not really necessary
as it is only called on insertion */
void
leftrotate(ANSREC *ans, TREEREC *par)
{
    TREEREC	*curr, *gpar;

    if( ( curr = par->right ) != NULL )
    {
	par->right = curr->left;
	if( curr->left != NULL )
	    curr->left->par = par;
	curr->par = par->par;
	if( ( gpar=par->par ) == NULL )
	    ans->root = curr;
	else
	{
	    if( par==gpar->left )
		gpar->left = curr;
	    else
		gpar->right = curr;
	}
	curr->left = par;
	par->par = curr;
    }
}


/* Rotate the left child of par upwards */
void
rightrotate(ANSREC *ans, TREEREC *par)
{
    TREEREC	*curr, *gpar;

    if( ( curr = par->left ) != NULL )
    {
	par->left = curr->right;
	if( curr->right != NULL )
	    curr->right->par = par;
	curr->par = par->par;
	if( ( gpar=par->par ) == NULL )
	    ans->root = curr;
	else
	{
	    if( par==gpar->left )
		gpar->left = curr;
	    else
		gpar->right = curr;
	}
	curr->right = par;
	par->par = curr;
    }
}


/* Insert word into a redblack tree */
void
redblackinsert(ANSREC *ans, char *word)
{
    TREEREC    *curr = ans->root, *par, *gpar, *prev = NULL, *wcreate();
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

    ans->ans = curr;

    if( curr == NULL )
	/* Insert a new node, rotate up if necessary */
    {
	if( val > 0 )
	    curr = prev->right = wcreate(word, prev);
	else
	    curr = prev->left = wcreate(word, prev);

	curr->colour = RED;
	while( (par = curr->par) != NULL
		&& ( gpar = par->par ) != NULL
		&& curr->par->colour == RED )
	{
	    if( par == gpar->left )
	    {
		if( gpar->right!=NULL && gpar->right->colour == RED )
		{
		    par->colour = BLACK;
		    gpar->right->colour = BLACK;
		    gpar->colour = RED;
		    curr = gpar;
		}
		else
		{
		    if( curr == par->right )
		    {
			curr = par;
			leftrotate(ans, curr);
			par = curr->par;
		    }
		    par->colour = BLACK;
		    if( ( gpar=par->par ) != NULL )
		    {
			gpar->colour = RED;
			rightrotate(ans, gpar);
		    }
		}
	    }
	    else
	    {
		if( gpar->left!=NULL && gpar->left->colour == RED )
		{
		    par->colour = BLACK;
		    gpar->left->colour = BLACK;
		    gpar->colour = RED;
		    curr = gpar;
		}
		else
		{
		    if( curr == par->left )
		    {
			curr = par;
			rightrotate(ans, curr);
			par = curr->par;
		    }
		    par->colour = BLACK;
		    if( ( gpar=par->par ) != NULL )
		    {
			gpar->colour = RED;
			leftrotate(ans, gpar);
		    }
		}
	    }
	}
	if( curr->par == NULL )
	    ans->root = curr;
	ans->root->colour = BLACK;
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
