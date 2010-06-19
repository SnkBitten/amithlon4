/*
	<linux/compat/module.h>

	some compatibility macros for linux/module.h

	(C) Dmitry Fedorov, fedorov@inp.nsk.su, 2000

	$Id: module.h,v 1.1 2000/06/24 18:43:30 fedorov Exp $
*/


/*
usage:

#include <linux/compat/module.h>

[...]

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18)
static struct symbol_table xx_syms =
{
# include <linux/symtab_begin.h>
#endif

	EXPORT_SYMBOL(symbol1)        EXPORT_SYMBOL_END

	[...]

	EXPORT_SYMBOL(symboln)        EXPORT_SYMBOL_END

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18)
# include <linux/symtab_end.h>
};
#endif

[...]

int xx_init(void)
{
	[...]

	if ( (rc=register_chrdev(major, name, &xx_fops)) < 0 )
		{ ... }

	[...]

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18)
	if ( (rc=register_symtab(&xx_syms)) !=0 )
	{
		printk(KERN_ERR "register_symtab() error, errno=%d\n", -rc);
		unregister_chrdev(major, name);
		return rc;
	}
# endif

	[...]
}

*/


#ifndef  _LINUX_COMPAT_MODULE_H
# define _LINUX_COMPAT_MODULE_H

# include <linux/compat/version.h>
# include <linux/module.h>

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18)
#  define MODULE_AUTHOR(name)
#  define MODULE_DESCRIPTION(desc)
#  define MODULE_SUPPORTED_DEVICE(name)
#  define MODULE_PARM(var,type)
#  define MODULE_PARM_DESC(var,desc)

#  define EXPORT_SYMBOL(symbol) X(symbol)
#  define EXPORT_SYMBOL_NOVERS(symbol) XNOVERS(symbol)
#  define EXPORT_SYMBOL_END ,
# else
#  define EXPORT_SYMBOL_END ;
#  define register_symtab(symtab)
# endif

#endif	/* _LINUX_COMPAT_MODULE_H */

