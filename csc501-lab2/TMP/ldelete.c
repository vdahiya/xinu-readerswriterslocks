/* ldelete.c - ldelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  * ldelete  --  delete a lock by releasing its table entry
 *   *------------------------------------------------------------------------
 *    */
SYSCALL ldelete(int lock_fd)
{
	STATWORD ps;    
	int	pid,i;
	struct	lentry	*lptr;

	disable(ps);
	if (isbadlock(lock_fd) || locktable[lock_fd].lstate==LFREE) {
		restore(ps);
		return(SYSERR);
	}
	lptr = &locktable[lock_fd];
	lptr->lstate = LFREE;
	for (i =0; i <NPROC; i++)
	{
		lptr->procArray[i]=0;
	}
	for (i =0; i <NLOCK; i++)
	{
		proctab[i].locks[lock_fd] =0;
	}
	if (nonempty(lptr->lqhead)) {
		while( (pid=getfirst(lptr->lqhead)) != EMPTY)
		  {
		    proctab[pid].pwaitret = DELETED;
		    proctab[pid].plock = -1;
		    ready(pid,RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}

