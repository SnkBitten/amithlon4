/*
	<linux/compat/hardirq.h>

	some compatibility <asm/hardirq.h> macros

	$Id: hardirq.h,v 1.4 2000/05/21 13:43:55 fedorov Exp $

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su
*/

#ifndef  _LINUX_COMPAT_HARDIRQ_H
# define _LINUX_COMPAT_HARDIRQ_H

# include <linux/compat/version.h>

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,30)
#  include <asm/hardirq.h>
# else
#  ifndef __SMP__
#   include <linux/interrupt.h>
#   define in_interrupt() (intr_count)
#  else  /* def __SMP__ */
#   error 2.0.x SMP not supported
#  endif /* ndef __SMP__ */
# endif

#endif	/* _LINUX_COMPAT_HARDIRQ_H */

