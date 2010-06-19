/*
	<linux/compat/processor.h>

	mm_segment_t compatibility

	(C) 2000, Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: processor.h,v 1.2 2000/07/13 12:47:07 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_PROCESSOR_H
# define _LINUX_COMPAT_PROCESSOR_H

# include <linux/compat/version.h>
# include <linux/compat/null.h>

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
   typedef unsigned long mm_segment_t;
# else
   /* asm/processor.h does not includes all of needed headers */
#  include <linux/sched.h>
#  include <asm/processor.h>	/* mm_segment_t */
# endif

#endif	/* _LINUX_COMPAT_PROCESSOR_H */

