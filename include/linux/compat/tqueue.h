/*
	<linux/compat/tqueue.h>

	compatibility task queue handling (bottom-halves) macros

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: tqueue.h,v 1.7 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_TQUEUE_H
# define _LINUX_COMPAT_TQUEUE_H

# include <linux/compat/version.h>
# include <linux/compat/null.h>
# include <linux/tqueue.h>


# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,0)

void queue_task_irq    (struct tq_struct*, task_queue*);
void queue_task_irq_off(struct tq_struct*, task_queue*);

#  define queue_task_irq(bh,tq)     queue_task(bh,tq)
#  define queue_task_irq_off(bh,tq) queue_task(bh,tq)

# endif


#endif	/* _LINUX_COMPAT_TQUEUE_H */

