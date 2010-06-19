/*
	<linux/compat/poll.h>

	compatibility <linux/poll.h> macros

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: poll.h,v 1.4 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_POLL_H
# define _LINUX_COMPAT_POLL_H

# include <linux/compat/version.h>

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,23)
#  include <linux/poll.h>

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,89)
#   define poll_wait(f,wq,w) poll_wait((wq),(w))
#  endif

# endif

#endif	/* _LINUX_COMPAT_POLL_H */

