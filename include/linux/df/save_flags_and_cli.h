/*
	<linux/df/save_flags_and_cli.h>
	$Id: save_flags_and_cli.h,v 1.9 2000/07/13 12:56:44 fedorov Exp $

	(C) Dmitry A. Fedorov, fedorov@inp.nsk.su, 1999
	Copying policy: GNU LGPL
*/

#ifndef  _LINUX_DF_SAVE_FLAGS_AND_CLI_H
# define _LINUX_DF_SAVE_FLAGS_AND_CLI_H

# include <asm/system.h>

/* assembler output of this will be optimized very well */
extern inline unsigned long save_flags_and_cli(void)
{
	unsigned long flags;
	save_flags(flags);
	cli();
	return flags;
}

#endif	/* _LINUX_DF_SAVE_FLAGS_AND_CLI_H */

