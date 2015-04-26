#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

// FIX ME , NO RETURN VALUE IN THE FUNCTION
int lock (int ldes1, int type, int priority)
{
	//kprintf("Entered lock function , type = %d\n", type);
	struct  lentry  *lptr;
	struct	pentry	*pptr;
	int i;
	STATWORD ps;
        if (isbadlock(ldes1) || (lptr= &locktable[ldes1])->lstate==LFREE) {
                return(SYSERR);
        }
	disable(ps);
	lptr= &locktable[ldes1];
	if ( READ == type )
	{
		//kprintf("Entered lock read part of the LOCK func");
		if (lptr->lstate == LUSED)
		{
			lptr->lstate= READ_LOCKED;
			// updating the Process table with the lock descriptor
			(pptr = &proctab[currpid])->locks[ldes1] =1;
			//Indicates which process currently holds the lock
			lptr->procArray[currpid] = 1;
			//printf(" Acquired Read lock \n");
			restore(ps);
			return (OK);
		}
		else if ( lptr->lstate == READ_LOCKED)
		{
			// There is already a process which has acquired the lock, so move it to wait queue or put on the lock wait queue
			if ( priority > maxwritepriority(ldes1))
			{
				// Give the process the lock
				(pptr = &proctab[currpid])->locks[ldes1] =1;
				lptr->procArray[currpid] = 1;	
				restore(ps);
				return (OK);
			}
			else
			{
				// There is a higher priority write process waiting, put the lock in the queue

				(pptr = &proctab[currpid])->pstate = PRWAIT;
				pptr->plock = ldes1;
				lptr->lprio = max(lptr->lprio,proctab[currpid].pprio);
				priority_inheritance(ldes1,currpid);
				insertlock(currpid,lptr->lqhead,priority,READ);
                		pptr->pwaitret = OK;
                		resched();
				lptr->procArray[currpid] = 1;
                                proctab[currpid].locks[ldes1] = 1;
				for(i=0;i<NPROC;i++)
               			{
                        		if(proctab[i].plock == ldes1)   //now update current process holding this lock, with this proc details
                        		{
                                		priority_inheritance(proctab[i].plock,i);
                        		}

                		}
                		restore(ps);
                		return pptr->pwaitret;
			}
		}
		else // lock currently held by writer process, put in the lock queue
		{
			(pptr = &proctab[currpid])->pstate = PRWAIT;
			pptr->plock = ldes1;
			lptr->lprio = max(lptr->lprio,proctab[currpid].pprio);
			priority_inheritance(ldes1,currpid);
                        insertlock(currpid,lptr->lqhead,priority,READ);
                        pptr->pwaitret = OK;
                        resched();
			lptr->procArray[currpid] = 1;
			proctab[currpid].locks[ldes1] = 1;
			for(i=0;i<NPROC;i++)
               		{
                        	if(proctab[i].plock == ldes1)
                        	{
                                	priority_inheritance(proctab[i].plock,i);
                        	}

                	}
                        restore(ps);
                        return pptr->pwaitret;
		}

	}
	else  //type = WRITE
	{
		//printf("Trying to Acquire Write Lock, current state = %d \n", lptr->lstate);
		if (lptr->lstate == LUSED)
		{
			lptr->lstate= WRITE_LOCKED;
			(pptr = &proctab[currpid])->locks[ldes1] =1;
			lptr->procArray[currpid] = 1;
                        restore(ps);
                        return (OK);
		}
		else
		{
			// Lock already held, Add to Lock wait queue
			(pptr = &proctab[currpid])->pstate = PRWAIT;
			pptr->plock = ldes1;
			lptr->lprio = max(lptr->lprio,proctab[currpid].pprio);
			//printf("Lock already held,pptr->plock = %d , lptr->lprio =%d \n", pptr->plock,lptr->lprio);
			priority_inheritance(ldes1,currpid);
			//printf("AFter Priority inheritance, process inherited prio = %d \n", pptr->pinh);
                        insertlock(currpid,lptr->lqhead,priority,WRITE);
                        pptr->pwaitret = OK;
                        resched();
			lptr->procArray[currpid] = 1;
			proctab[currpid].locks[ldes1] = 1;
			for(i=0;i<NPROC;i++)
                        {
                                if(proctab[i].plock == ldes1)
                                {
                                        priority_inheritance(proctab[i].plock,i);
                                }

                        }
                        restore(ps);
                        return pptr->pwaitret;		
		}
		//writeLockAcquire(ldes1, priority);
	}
	
}

int insertlock(int proc, int head, int key, int type)
{
	int	next;			/* runs through list		*/
	int	prev;

	next = q[head].qnext;
	while (q[next].qkey < key)	/* tail has maxint as key	*/
		next = q[next].qnext;
	while ( (q[next].qtype == READ) && (q[next].qkey == key) )
		next = q[next].qnext;
	q[proc].qnext = next;
	q[proc].qprev = prev = q[next].qprev;
	q[proc].qkey  = key;
	q[proc].qtype = type;
	q[prev].qnext = proc;
	q[next].qprev = proc;
	return(OK);
}

int  maxwritepriority(int ldes)
{
	int prev;
	register struct lentry  *lptr;
	lptr= &locktable[ldes];
	if (nonempty(lptr->lqhead))
	{
		prev = q[lptr->lqtail].qprev;
		while (q[prev].qtype !=WRITE)
			prev = q[prev].qprev;
		if (q[prev].qtype == WRITE)
			return q[prev].qkey;	
		else
			return 0;
	}
	return 0;
}

void priority_inheritance(int ldes, int pid)
{
	// REMOVE
	//return;
	int i;
	if(ldes==-1)
	{
		//printf("priority_inheritance, LDES is -1 \n");
       		return;
	}
	for(i=0;i<NPROC;i++)
	{

        	if(locktable[ldes].procArray[i] > 0)
        	{
			// this lock is held by some other process
                	//process i holds the lock ldes, so see if you need to update it priority.

                	if(proctab[pid].pinh > 0) //check if the current process waiting to acquire the lock already has some inherited priority
                	{
                        	if(proctab[i].pinh > 0) // if the other process already has some inherited priority, comapre against that
                        	{
                                	if(proctab[pid].pinh > proctab[i].pinh)   // if your inherited priority is greater than his inherited prio, swap
                                        	proctab[i].pinh = proctab[pid].pinh;
                        	}

                        	else // the other process does not have any inherited priority
                        	{
                                	if(proctab[pid].pinh > proctab[i].pprio)
                                        	proctab[i].pinh = proctab[pid].pinh;

                        	}

                	}
                	else // ur current process does not have any inherited prioroty
                	{
                        	if(proctab[i].pinh > 0) // if the other process has inherited prio, compare it with ur base prio and swap
                        	{
                                	if(proctab[pid].pprio > proctab[i].pinh)
                                        	proctab[i].pinh = proctab[pid].pprio;
                        	}
                        	else
                        	{
     		                	  if(proctab[pid].pprio > proctab[i].pprio)
                                        	proctab[i].pinh = proctab[pid].pprio;
                        	}

                	}
			//printf("\nUpdated pinh for process %d is %d\n",i,proctab[i].pinh);
                	priority_inheritance(proctab[i].plock,i);

        	}

	} // End of for	
//printf ("Exiting Priority INHERITANCE \n");
return ;

}
