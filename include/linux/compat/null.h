/*
	<linux/compat/null.h>

	$Id: null.h,v 1.2 2000/05/21 13:43:55 fedorov Exp $

	force right NULL
*/

# undef NULL
# if defined __GNUG__ && \
		(__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#  define NULL __null
# else
#  if !defined(__cplusplus)
#   define NULL ((void*)0)
#  else
#   define NULL 0
#  endif
# endif

