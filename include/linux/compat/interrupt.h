/*
	<linux/compat/interrupt.h>

	compatibility <linux/interrupt.h> macros

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: interrupt.h,v 1.2 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_INTERRUPT_H
# define _LINUX_COMPAT_INTERRUPT_H

# include <linux/compat/version.h>
# include <linux/interrupt.h>

# if LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)
#  define in_interrupt()	(intr_count)
# endif

#endif	/* _LINUX_COMPAT_INTERRUPT_H */

