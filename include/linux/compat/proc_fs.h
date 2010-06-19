/*
	<linux/compat/proc_fs.h>

	proc_register_dynamic() compatibility

	Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: proc_fs.h,v 1.3 2000/07/31 10:45:25 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_PROC_FS_H
# define _LINUX_COMPAT_PROC_FS_H

# include <linux/compat/version.h>
# include <linux/proc_fs.h>

# if LINUX_VERSION_CODE > KERNEL_VERSION(2,1,0)
#  ifdef CONFIG_PROC_FS

extern inline int proc_register_dynamic(
	struct proc_dir_entry* parent,
	struct proc_dir_entry* child)
{
	child->low_ino = 0;
	return proc_register(parent, child);
}

#  endif
# endif

#endif	/* _LINUX_COMPAT_PROC_FS_H */

