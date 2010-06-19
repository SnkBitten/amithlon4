/*
	<linux/compat/bitops.h>

	compatibility bitops macros

	$Id: bitops.h,v 1.13 2000/05/21 13:43:55 fedorov Exp $

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	to do: hweight{8|16|32}
*/

#ifndef  _LINUX_COMPAT_BITOPS_H
# define _LINUX_COMPAT_BITOPS_H

# include <linux/compat/version.h>
# include <asm/bitops.h>

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,37)
#  define test_and_set_bit(nr, addr)       set_bit(nr, addr)
#  define test_and_clear_bit(nr, addr)   clear_bit(nr, addr)
#  define test_and_change_bit(nr, addr) change_bit(nr, addr)
#
#  define ffs(word) __builtin_ffs(word)
# endif

#endif	/* _LINUX_COMPAT_BITOPS_H */

