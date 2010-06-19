/*
	<linux/compat/wait.h>

	compatibility wait queue macros

	to do: init_waitqueue_entry()

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: wait.h,v 1.9 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_WAIT_H
# define _LINUX_COMPAT_WAIT_H

# include <linux/compat/version.h>
# include <linux/compat/null.h>
# include <linux/wait.h>


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,1)

typedef struct wait_queue wait_queue_t;
typedef wait_queue_t* wait_queue_head_t;

extern inline void init_waitqueue_head(wait_queue_head_t *q)
{
	*q = NULL;
}

#  define wait_queue_active waitqueue_active

#  define DECLARE_WAITQUEUE(name, task) wait_queue_t name = { task, NULL }

#  define DECLARE_WAIT_QUEUE_HEAD(name) wait_queue_head_t name = NULL

# endif

#endif	/* _LINUX_COMPAT_WAIT_H */

