/*
	<linux/compat/version.h>

	Kernel version macros

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: version.h,v 1.8 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_VERSION_H
# define _LINUX_COMPAT_VERSION_H

# include <linux/version.h>	/* LINUX_VERSION_CODE, KERNEL_VERSION() */

# ifndef  KERNEL_VERSION
#  define KERNEL_VERSION(v, p, s)	( ((v)<<16) | ((p)<<8) | (s) )
# endif


/* + unstable */

# ifndef  LVC_VERSION
#  define LVC_VERSION(lvc)     ( ((lvc)>>16) & 0xFF )
# endif

# ifndef  LVC_PATCHLEVEL
#  define LVC_PATCHLEVEL(lvc)  ( ((lvc)>>8) & 0xFF )
# endif

# ifndef  LVC_SUBLEVEL
#  define LVC_SUBLEVEL(lvc)    ( (lvc) & 0xFF )
# endif

/* - unstable */

#endif	/* _LINUX_COMPAT_KERNEL_VERSION_H */

