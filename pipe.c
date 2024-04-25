
/*
 * Create pipe
 *
 * In:  Nothing
 *
 * Returns: -1 on error, file handle on success.
 *
 */
size_t pipe(void) {
	openfiles_last->next=kernelalloc(sizeof(OPENFILE));	/* add file descriptor */
	if(openfileslast->next == NULL) return(-1);

	openfiles_last=openfiles_last->next;

	openfiles_last->FILE_FIFO;
	openfiles_last->handle=highest_handle++;

	openfiles_last->pipe.buffer=NULL;		/* empty, for now */
	openfiles_last->pipe.size=0;

	return(openfiles_last->handle);
}

