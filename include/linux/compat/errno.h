/*
	<linux/compat/errno.h>

	$Id: errno.h,v 1.4 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_ERRNO_H
# define _LINUX_COMPAT_ERRNO_H

# include <linux/errno.h>


# ifndef  ENOTSUP
#  define ENOTSUP EOPNOTSUPP
# endif


# ifndef ENOMEDIUM /* ENOMEDIUM and EMEDIUMTYPE was defined at the same time */
#  if defined(_I386_ERRNO_H)
#   define ENOMEDIUM    123
#   define EMEDIUMTYPE  124
#  elif defined(__ASM_MIPS_ERRNO_H)
#   define ENOMEDIUM    159
#   define EMEDIUMTYPE  160
#  elif defined(_ALPHA_ERRNO_H)
#   define ENOMEDIUM    129
#   define EMEDIUMTYPE  130
#  elif defined(_M68K_ERRNO_H)
#   define ENOMEDIUM    123
#   define EMEDIUMTYPE  124
#  elif defined(_SPARC_ERRNO_H)
#   define ENOMEDIUM    125
#   define EMEDIUMTYPE  126
#  elif defined(_PPC_ERRNO_H)
#   define ENOMEDIUM    123
#   define EMEDIUMTYPE  124
#  elif defined(_SPARC64_ERRNO_H)
#   define ENOMEDIUM    125
#   define EMEDIUMTYPE  126
#  elif defined(_ARM_ERRNO_H)
#   define ENOMEDIUM    123
#   define EMEDIUMTYPE  124
#  elif defined(_S390_ERRNO_H)
#   define ENOMEDIUM    123
#   define EMEDIUMTYPE  124
#  endif
# endif

#endif	/* _LINUX_COMPAT_ERRNO_H */

