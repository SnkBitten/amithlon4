/*
	<linux/compat/smp_lock.h>

	compatibility smp_lock macros

	$Id: smp_lock.h,v 1.5 2000/05/21 13:43:55 fedorov Exp $

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su
*/

#ifndef  _LINUX_COMPAT_SMPLOCK_H
# define _LINUX_COMPAT_SMPLOCK_H

# include <linux/compat/version.h>

# if LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)
#  ifndef __SMP__
#   define lock_kernel()                  do { } while(0)
#   define unlock_kernel()                do { } while(0)
#   define release_kernel_lock(task, cpu) do { } while(0)
#   define reacquire_kernel_lock(task)    do { } while(0)
#  else
#   error 2.0.x SMP not supported
#  endif
# else
#  include <linux/smp_lock.h>
# endif

#endif	/* _LINUX_COMPAT_SMPLOCK_H */

