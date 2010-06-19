/*
	<linux/compat/system.h>

	some compatibility macros for asm/system.h

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: system.h,v 1.9 2000/07/31 10:47:36 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_SYSTEM_H
# define _LINUX_COMPAT_SYSTEM_H

# include <linux/compat/version.h>
# include <asm/system.h>


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
#  define rmb() mb()
#  define wmb() mb()

#  define __cli() cli()
#  define __sti() sti()

#  define __save_flags(x) save_flags(x)
#  define __restore_flags(x) restore_flags(x)
# endif


/* xchg() macro was buggy! */
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#  define xchg_compat(ptr,v) \
	({ __typeof__(*(ptr)) volatile t = xchg((ptr),(v)); t; })
# else
#  define xchg_compat(ptr,v) xchg((ptr),(v))
# endif


#endif	/* _LINUX_COMPAT_SYSTEM_H */

