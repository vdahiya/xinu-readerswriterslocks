#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef	NLOCK
#define	NLOCK		50	/* number of locks, if not defined	*/
#endif

// Re-define the states
#define	LFREE	'\01'		/* this lock is free		*/
#define	LUSED	'\02'		/* this lock is used, set during intialize		*/
#define READ_LOCKED '\03'		/* Currently has only reader locks */
#define WRITE_LOCKED '\04'		/* Currently has only Writer locks */
#define READ 	(1)
#define WRITE 	(2)

#define lastType(tail)	(q[q[(tail)].qprev].qtype)

struct	lentry	{		/* lock table entry			*/
	char	lstate;		/* the state LFREE or LUSED		*/
	int	rdrcount;	/* count for this lock 			*/
	int	lqhead;		/* q index of head of list		*/
	int	lqtail;		/* q index of tail of list		*/
	int 	rdlock;         /* Read mutex				*/
	int 	wrlock; 	/* Write mutex 				*/
	int 	procArray[NPROC];
	int 	lprio; 		// Highest Lock priority
};

extern struct  lentry  locktable[];    /* lock table                           */
extern int     nextlock;               /* next lock slot to use in lcreate     */

#define	isbadlock(s)	(s<0 || s>=NLOCK)

SYSCALL lcreate(void);
void linit(void);
int ldelete(int);
int lock(int, int, int );
SYSCALL writeLockRelease (int);
SYSCALL readLockAcquire(int, int);
SYSCALL readLockRelease (int );
SYSCALL writeLockAcquire (int, int );
int insertlock(int , int , int , int );
void priority_inheritance(int ldes, int pid);
void resetpriority();
int selectpriority(int pid);
#endif

