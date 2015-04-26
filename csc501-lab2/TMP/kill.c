/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
	int i,j;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
			// Release all locks held by this process
			/*for (i=0; i <NLOCK; i++)
			{
				if (pptr->locks[i] == 1)
					releaseall(1,i);
			} no need of it here since it anyway falls to PRREADY */

	case PRREADY:	dequeue(pid);
			// Release all locks held by this process
			for (i=0; i <NLOCK; i++)
                        {
                                if (pptr->locks[i] == 1)
				{
					pptr->locks[i]=0;
					pptr->plock = -1;
					locktable[pid].procArray[pid]=0;
                                        releaseall(1,i);// release all will not work because as per definition it should release only the locks held by the 
							// Current process, since it is a different process calling the release all it is bound to fail.
				}
                        }
			// Reset the proc table flags for this lock
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
