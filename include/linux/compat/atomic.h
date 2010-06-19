/*
	<linux/compat/atomic.h>

	compatibility atomic macros

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: atomic.h,v 1.9 2000/05/21 13:32:14 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_ATOMIC_H
# define _LINUX_COMPAT_ATOMIC_H

# include <linux/compat/version.h>
# include <asm/atomic.h>

# if LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)
#  define ATOMIC_INIT(i)			(i)
#  define atomic_read(vp)         (*(vp))
#  define atomic_set(vp,i)      ( (*(vp)) = (i) )
# endif

#endif	/* _LINUX_COMPAT_ATOMIC_H */

