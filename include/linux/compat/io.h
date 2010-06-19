/*
	<linux/compat/io.h>

	compatibility ioremap macros

	ioremap, iounmap, ioremap_nocache,
	virt_to_phys, phys_to_virt, virt_to_bus, bus_to_virt

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: io.h,v 1.12 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef   _LINUX_COMPAT_IO_H
# define  _LINUX_COMPAT_IO_H

# include <linux/compat/version.h>

# include <asm/io.h>

# if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)
#  if LINUX_VERSION_CODE <= KERNEL_VERSION(1,3,0)
#   define ioremap(offset,size)  ((char *)(offset))
#   define iounmap(addr)
#  else  /* 2.0.x */
#   include <linux/mm.h> /* vremap, vfree */
#   define ioremap(offset,size)  vremap(offset,size)
#   define iounmap(addr)         vfree(addr)
#  endif /* <= 1.3.0 */
#  define ioremap_nocache(offset,size)  ioremap(offset,size)
# endif  /* <= 2.1.0 */

#endif   /* _LINUX_COMPAT_IO_H */

