/*
	<linux/compat/sched.h>

	some compatibility macros for linux/sched.h

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: sched.h,v 1.10 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_SCHED_H
# define _LINUX_COMPAT_SCHED_H

# include <linux/compat/version.h>
# include <linux/compat/null.h>
# include <linux/sched.h>


/*
You are using a 2.2.x kernel? Then use
  if (signal_pending(current))
instead of your code. current->blocked has been changed to an array
to make more than 32 signals possible, and ~ can't be used on that.
*/

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,57)
#  define signal_pending(p)  ((p)->signal & ~(p)->blocked)
# endif


/* + unstable */
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,110)
#  define NEED_RESCHED (need_resched)
# else
#  define NEED_RESCHED (current->need_resched)	/* need lock? $$$ */
#endif
/* - unstable */


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,127)
#  define MAX_SCHEDULE_TIMEOUT	LONG_MAX

/* returns rest of timer */
long schedule_timeout(long timeout);

#  include <linux/compat/wait.h>	/* wait_queue_head_t */

/* returns rest of timer */
long interruptible_sleep_on_timeout(wait_queue_head_t* p, long timeout);

# endif


/* + unstable names */
/* Open file table structure. */

/* returns -EMFILE if amount of files exceeded or fd arg. */
extern inline int check_max_fd(struct files_struct* files, int fd)
{
	return
#if   LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,12)
		(fd >= files->max_fdset || fd >= files->max_fds)
#elif LINUX_VERSION_CODE >  KERNEL_VERSION(2,1,0)
		(fd >= files->max_fds)
#else	/* 2.0.x */
		(fd >= NR_OPEN)
#endif
		? -EMFILE : fd;
}

extern inline ulong open_fds_bits(struct files_struct* files, int fd_set_nr)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,12)
	return files->open_fds->fds_bits[fd_set_nr];
#else
	return files->open_fds.fds_bits[fd_set_nr];
#endif
}

/* - unstable names */


#endif	/* _LINUX_COMPAT_SCHED_H */

