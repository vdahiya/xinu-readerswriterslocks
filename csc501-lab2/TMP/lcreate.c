/* screate.c - screate, newlock */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();

/*------------------------------------------------------------------------
 *  * screate  --  create and initialize a lockaphore, returning its id
 *   *------------------------------------------------------------------------
 *    */
SYSCALL lcreate()
{
	STATWORD ps;    
	int	lock_fd;

	disable(ps);
	if ( (lock_fd=newlock() )==SYSERR) {
		restore(ps);
		return(SYSERR);
	}
	restore(ps);
	return(lock_fd);
}

/*------------------------------------------------------------------------
 *  * newlock  --  allocate an unused lockaphore and return its index
 *   *------------------------------------------------------------------------
 *    */
LOCAL int newlock()
{
	int	lock_fd;
	int	i;

	for (i=0 ; i<NLOCK ; i++) {
		lock_fd=nextlock--;
		//printf (" lock_fd = %d , nextlock = %d \n", lock_fd, nextlock);
		if (nextlock < 0)
			nextlock = NLOCK-1;
		if (locktable[lock_fd].lstate==LFREE) {
			locktable[lock_fd].rdlock = 1;
			locktable[lock_fd].wrlock = 1;
			locktable[lock_fd].lstate = LUSED;
			locktable[lock_fd].rdrcount = 0;
			locktable[lock_fd].lprio = -1;
			for (i=0; i <NPROC; i++)
				locktable[lock_fd].procArray[i] =0;
			//printf("returining lock_fd = %d \n", lock_fd);
			return(lock_fd);
		}
	}
	return(SYSERR);
}

