/*
	<linux/compat/uaccess.h>

	compatibility user space memory access macros

	GET_USER, PUT_USER, GET_USER_NOCHECK, PUT_USER_NOCHECK
	COPY_FROM_USER, COPY_TO_USER,
	COPY_FROM_USER_NOCHECK, COPY_TO_USER_NOCHECK,
	clear_user, strlen_user, strncpy_from_user

	to do: put_user_ret, get_user_ret,
	       __clear_user, __strncpy_from_user (no check)

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: uaccess.h,v 1.20 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_UACCESS_H
# define _LINUX_COMPAT_UACCESS_H

# include <linux/compat/version.h>
# include <linux/compat/null.h>



/*
 * These are the main single-value transfer routines.
 * "Overloaded" macros, automatically use the right size
 * if we just have the right pointer type.
 *
 * The "xxx_NOCHECK" versions of the user access functions are versions
 * that do not verify the address space.
 *
 * get_user and put_user has different "prototypes" in 2.0 and 2.1 kernels.
 *
 * err  GET_USER(x,addr);
 * err  PUT_USER(x,addr);
 * void GET_USER_NOCHECK(x,addr);
 * void PUT_USER_NOCHECK(x,addr);
 *
 * get_user and put_user returns 0 for success and -EFAULT for bad access.
 */


/*
 * Copy from/to userspace
 *
 * COPY_FROM_USER and COPY_TO_USER returns the number of bytes they failed
 * to copy (so zero is a successful return).
 */

extern inline unsigned long
COPY_FROM_USER(void* kdst, const void* usrc, unsigned long len);

extern inline unsigned long
COPY_TO_USER  (void* udst, const void* ksrc, unsigned long len);

extern inline void
COPY_FROM_USER_NOCHECK(void* kdst, const void* usrc, unsigned long len);

extern inline void
COPY_TO_USER_NOCHECK  (void* udst, const void* ksrc, unsigned long len);


/*
 * clear_user, strlen_user, strncpy_from_user
 *
 * to do: __clear_user, __strncpy_from_user (no check)
 *
 * These functions are absent in 2.0 kernels.
 *
 * Implementation of these functions for 2.0 kernels is very slow -
 * clear_user copy of each byte and string functions does verify_area
 * on each byte.
 *
 * extern unsigned long clear_user(void* dst, unsigned long len);
 *   Zeroing userspace.
 *   Return the number of bytes that failed to zeroing.
 *
 * extern long strlen_user(const char *str);
 *   Return the length of a string (including the ending 0).
 *   Return 0 for error.
 *
 * extern long strncpy_from_user(char *dst, const char *src, long count);
 *   Copy a null terminated string from userspace.
 *   Return length of string w/o ending 0 or -EFAULT for error.
 *
 */


/*
 * memset of user space ($$ my extension $$).
 * Return the number of bytes that failed to set.
 */
unsigned long memset_user(void* udst, int c, unsigned long len);



# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,4)

#  include <linux/mm.h>		/* verify_area, VERIFY_READ, VERIFY_WRITE */
#  include <asm/segment.h>	/* put_user, get_user, ... */


#  define GET_USER(x, addr) \
( \
 verify_area(VERIFY_READ, addr, sizeof(*(addr))) \
	? : (x=get_user(addr), 0) \
)

#  define PUT_USER(x, addr) \
( \
 verify_area(VERIFY_WRITE, addr, sizeof(*(addr))) \
	? : (put_user(x,addr), 0) \
)

#  define GET_USER_NOCHECK(x, addr) (x=get_user(addr))

#  define PUT_USER_NOCHECK(x, addr) (put_user(x,addr))



#  define COPY_FROM_USER(kdst, usrc, len) \
( \
 !verify_area(VERIFY_READ, usrc, len) \
 ? (memcpy_fromfs(kdst, usrc, len), 0UL) \
 : len \
)

#  define COPY_TO_USER(udst, ksrc, len) \
( \
 !verify_area(VERIFY_WRITE, udst, len) \
 ? (memcpy_tofs(udst, ksrc, len), 0UL) \
 : len \
)

#  define COPY_FROM_USER_NOCHECK(kdst, usrc, len) \
	memcpy_fromfs(kdst, usrc, len)

#  define COPY_TO_USER_NOCHECK(udst, ksrc, len) \
	memcpy_tofs(udst, ksrc, len)



extern long strncpy_from_user(char *kdst, const char *usrc, long count);
extern long strlen_user(const char *ustr);

extern inline unsigned long clear_user(void* udst, unsigned long len)
{
	return memset_user(udst, 0, len);
}


# else	/* LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,4) */

#  include <asm/uaccess.h>

#  define GET_USER(x,addr) get_user(x,addr)
#  define PUT_USER(x,addr) put_user(x,addr)
#  define GET_USER_NOCHECK(x,addr) __get_user(x,addr)
#  define PUT_USER_NOCHECK(x,addr) __put_user(x,addr)
#
#  define COPY_FROM_USER(kdst,usrc,len) copy_from_user(kdst,usrc,len)
#  define COPY_TO_USER(udst,ksrc,len)   copy_to_user(udst,ksrc,len)
#  define COPY_FROM_USER_NOCHECK(kdst,usrc,len) __copy_from_user(kdst,usrc,len)
#  define COPY_TO_USER_NOCHECK(udst,ksrc,len)   __copy_to_user(udst,ksrc,len)

# endif	/* LINUX_VERSION_CODE < KERNEL_VERSION(2,1,4) */


#endif	/* _LINUX_COMPAT_UACCESS_H */

