/*
	<linux/compat/init.h>

	compatibility init section macros

	__init_text, __init_data, __init_func

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: init.h,v 1.11 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_INIT_H
# define _LINUX_COMPAT_INIT_H

# include <linux/compat/version.h>


# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,16)
#  include <linux/init.h>
       /* I like this names */
#  define __init_text __init
#  define __init_data __initdata
#  define __init_func(__arg) __initfunc(__arg)  /* deprecated */
# else
#  define __init_text
#  define __init_data
#  define __init_func(__arg) __arg              /* deprecated */

#  define __init
#  define __initdata
#  define __initfunc(__arg)  __arg              /* deprecated */
# endif

#endif	/* _LINUX_COMPAT_INIT_H */

