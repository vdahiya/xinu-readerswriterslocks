/* create.c - create, newpid */
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>
#include <q.h>
int releaseall (nargs, args)
	int nargs;
{
// releaseall means release all locks held by a particular process
	STATWORD 	ps; 
	unsigned long	*a;
	struct  lentry  *lptr;
	int retval = OK;	
	int ldes;
	int i, j=0, flag=0, lock_still_in_use=0;
	a = (unsigned long *)(&args) + (nargs-1); /* last argument	*/
	for ( ; nargs > 0 ; nargs--)	/* machine dependent; copy args	*/
	{
		ldes=*a--;
		//printf("releaseall, lock descriptor = %d\n",ldes);
		// check for Error Condition
		if (isbadlock(ldes) || (lptr= &locktable[ldes])->lstate==LFREE || (lptr= &locktable[ldes])->lstate==LUSED) {
                	retval = SYSERR;
			continue;
        	}
		if ( proctab[currpid].locks[ldes] != 1 )
		{
			retval=SYSERR;
			continue;
		}
		disable(ps);
		lptr= &locktable[ldes];
		//for ( i=0; i<NLOCK; i++)
		{
			proctab[currpid].locks[ldes]=0; // reset the proctab map saying u no longer hold the lock
		}
		//for (i=0; i <NLOCK; i++)
			lptr->procArray[currpid] = 0; // reset the lock table
		for (i=0; i <NPROC; i++)
		{
			if (lptr->procArray[i] ==1)
				lock_still_in_use =1;
		}
		if (lock_still_in_use)
		{
			resetpriority();
			lptr->lprio = maxWaitQueue();
			resched();
			continue;
		}
		if (nonempty(lptr->lqhead))
                {
			do
			{
				flag =0;
				if (lastType(lptr->lqtail) == READ)
				{
					lptr->lstate = READ_LOCKED;
					flag =1;
				}
				else
					lptr->lstate = WRITE_LOCKED;
				//printf("releaseall, Changed State = %d \n", lptr->lstate);
				proctab[q[(lptr->lqtail)].qprev].plock=-1;
			//printf("releaseall, curreent Proc ID : %d \n", q[(lptr->lqtail)].qprev);	
				ready(getlast(lptr->lqtail), RESCHNO);
				// reset the plock of the process now that the process will get the lock it was waiting for
			} while (flag && (lastType(lptr->lqtail) == READ));
                }
		else // NO process waiting for this lock
		{
			for ( i=0; i <NPROC; i++)
			{
				if ( proctab[i].locks[ldes] == 1 )
					j++;
			}
			if ( j < 1 )
				lptr->lstate = LUSED;
		}
		lptr->lprio = maxWaitQueue();
		resetpriority();
	}
	resched();
	return(retval);
}

void resetpriority()
{

//Needs to be done for the process which is releasing the lock.

        int maxlock=-1,i;
        //check all locks held by currpid
        for(i=0;i<NLOCK;i++)  //check all locks held by currpid
        {
                if(proctab[currpid].locks[i] > 0)
                {
                        //for each lock held by this process, find max process priority on each of their wait queues.
                        //i-> lock held by this process.
                     
                        maxlock = max(maxlock,locktable[i].lprio); //compare max prio of processes on wait queue of this lock.

                }
        }

        if(maxlock > 0)
                proctab[currpid].pinh = maxlock; //assign this value to pinh
        else
                proctab[currpid].pinh = -1; //this will allow us to go back to using pprio instead of pinh.

	priority_inheritance(proctab[currpid].plock, currpid);
}

int maxWaitQueue(int ldes)
{

	 struct lentry * lptr;
	 int maxprio=-1;int i;
	 lptr = &locktable[ldes];
	 i=q[lptr->lqhead].qnext;
	 while(i!=lptr->lqtail) //find max wait
         {
        	if(proctab[i].pprio > maxprio)
                    maxprio=proctab[i].pprio;
                i = q[i].qnext;
         }
 	 return maxprio;

}
