#include <conf.h>
#include <kernel.h>
#include <sleep.h>
#include <i386.h>
#include <stdio.h>
#include <q.h>
#include <lock.h>

void linit()
{
	nextlock = NLOCK - 1;
	struct  lentry  *lptr;
	int i;
	for ( i=0; i<NLOCK ; i++) { /*initialize locks */
                (lptr = &locktable[i])->lstate = LFREE;
                lptr->lqtail = 1 + (lptr->lqhead = newqueue());
		lptr->lprio = -1;
        }
}
