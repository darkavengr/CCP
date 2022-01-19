MUTEX *mutexes=NULL;

int create_mutex(void) {
 MUTEX *next;
 MUTEX *last;

 next=mutexes;			/* point to mutex list */

 while(next != NULL) {		/* find end */
  last=next;			/* second to last entry */

  next=next->next;
 }

 last->next=kernelalloc(sizeof(MUTEX));	/* add to end */
 if(last->next == NULL) return(-1);	/* error */

 next->
