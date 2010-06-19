/*
	<linux/compat/dma.h>

	compatibility dma lock macros

	$Id: dma.h,v 1.3 2000/05/21 13:43:55 fedorov Exp $

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su
*/

#ifndef  _LINUX_COMPAT_DMA_H
# define _LINUX_COMPAT_DMA_H

# include <linux/compat/version.h>
# include <asm/dma.h>

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
#  ifndef __SMP__

extern __inline__ unsigned long claim_dma_lock(void)
{
	unsigned long flags;
	save_flags(flags); cli();
	return flags;
}

extern __inline__ void release_dma_lock(unsigned long flags)
{
	restore_flags(flags);
}

#  else
#   error 2.0.x SMP not supported
#  endif
# endif

#endif	/* _LINUX_COMPAT_DMA_H */

