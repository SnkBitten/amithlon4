/*
	<linux/df/pr.h>
	$Id: pr.h,v 1.12 2000/07/13 12:56:44 fedorov Exp $

	(C) Dmitry A. Fedorov, fedorov@inp.nsk.su, 1999
	Copying policy: GNU LGPL
*/

#ifndef  _LINUX_DF_PR_H
# define _LINUX_DF_PR_H

# include <linux/kernel.h>		/* printk, pr_debug, pr_info */


# ifndef  pr_error
#  define pr_error(fmt,arg...)	printk(KERN_ERR fmt,##arg)
# endif

# ifndef  pr_warn
#  define pr_warn(fmt,arg...)	printk(KERN_WARNING fmt,##arg)
# endif

# ifndef  pr_notice
#  define pr_notice(fmt,arg...)	printk(KERN_NOTICE fmt,##arg)
# endif

# ifndef  pr_endl
static inline void pr_endl(void)
{
	printk("\n");
}
# endif


#endif	/* _LINUX_DF_PR_H */


/*
$Log: pr.h,v $
Revision 1.12  2000/07/13 12:56:44  fedorov
---

Revision 1.11  1999/09/25 10:53:00  fedorov
---

Revision 1.10  1999/09/21 07:10:57  fedorov
comment style changed; extern "C" removed; pr_endl() added.

Revision 1.9  1999/08/05 14:27:27  fedorov
header and history formats changed

Revision 1.8  1999/05/19 06:36:34  fedorov
pr_begin_* macros removed

Revision 1.7  1999/05/18 19:03:51  fedorov
pr_begin_* redefined as function macros

Revision 1.6  1999/05/02 10:32:07  fedorov
extra multiple inclusion protection removed

Revision 1.5  1999/04/15 08:49:17  fedorov
_ prepended to file macros

Revision 1.4  1999/03/02 15:47:51  fedorov
extern "C"

Revision 1.2  1999/02/07 10:55:50  fedorov
pr_begin_* added

Revision 1.1  1999/02/05 16:05:44  fedorov
created
*/

