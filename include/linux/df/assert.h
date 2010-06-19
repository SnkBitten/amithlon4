/*
	<linux/df/assert.h>
	$Id: assert.h,v 1.10 2000/07/13 12:56:42 fedorov Exp $

	(C) Dmitry A. Fedorov, fedorov@inp.nsk.su, 1999
	Copying policy: GNU LGPL
*/

#ifndef  _LINUX_DF_ASSERT_H
# define _LINUX_DF_ASSERT_H


# ifdef NDEBUG

#  define assert(expr)            ((void) 0)
#  define assert_retvoid(expr)    ((void) 0)
#  define assert_retval(expr,val) ((void) 0)
#  define assert_goto(expr,label) ((void) 0)
#  define assert_break(expr)      ((void) 0)

# else	/* ndef NDEBUG */

#  include <linux/kernel.h>		/* printk */

#  define __assertion_failed(expr) \
	( printk(KERN_ERR "%s:%u: %s: Assertion `%s' failed.\n", \
		__FILE__, __LINE__, __PRETTY_FUNCTION__, #expr) )

#  define __assert(expr) \
	( (expr) ? 1 : (__assertion_failed(expr), 0) )

#  define assert(expr) ((void) __assert(expr))

#  define assert_retvoid(expr) \
	do { if (!__assert(expr)) return; } while(0)

#  define assert_retval(expr,val) \
	do { if (!__assert(expr)) return (val); } while(0)

#  define assert_goto(expr,label) \
	do { if (!__assert(expr)) goto label; } while(0)

#  define assert_break(expr) \
	if (!__assert(expr)) break

# endif	/* def NDEBUG */


#endif	/* _LINUX_DF_ASSERT_H */


/*
$Log: assert.h,v $
Revision 1.10  2000/07/13 12:56:42  fedorov
---

Revision 1.9  1999/10/09 03:47:28  fedorov
__assertion_failed() created and used.

Revision 1.8  1999/09/25 10:53:00  fedorov
---

Revision 1.7  1999/09/25 10:45:56  fedorov
do while(0) construct used instead of statement expression (GNU extension);
assert_goto() and assert_break() added.

Revision 1.6  1999/09/21 07:09:30  fedorov
comment style changed; dependence on pr.h removed.

Revision 1.5  1999/08/05 14:27:26  fedorov
header and history formats changed

Revision 1.4  1999/05/02 10:32:07  fedorov
extra multiple inclusion protection removed

Revision 1.3  1999/04/15 08:49:59  fedorov
include-once file macro names uppercased

Revision 1.2  1999/04/11 18:55:33  fedorov
assert_ret* created

Revision 1.1  1999/04/07 06:31:50  fedorov
created
*/

