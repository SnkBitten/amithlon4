/*
	<linux/compat/file.h>

	compatibility wrapper functions for accessing the file_struct fd array.

	fcheck_task(), fcheck(), fd_install(), fput_compat(), __fput_compat()

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: file.h,v 1.14 2000/07/23 11:55:59 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_FILE_H
# define _LINUX_COMPAT_FILE_H

# include <linux/compat/version.h>
# include <linux/compat/sched.h>	/* task_struct */
# include <linux/file.h>


# if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)

/*
	Check whether the specified task has the fd open.
	Since the task may not have a files_struct,
	we must test for p->files != NULL.
 */
extern inline struct file* fcheck_task(struct task_struct* p, unsigned int fd)
{
	return (fd<NR_OPEN && p->files) ? p->files->fd[fd] : (struct file*)NULL;
}


/* Check whether the specified fd has an open file. */
extern inline struct file* fcheck(unsigned int fd)
{
	return (fd<NR_OPEN) ? current->files->fd[fd] : (struct file*)NULL;
}


/* Install a file pointer in the fd array. */
extern inline void fd_install(unsigned int fd, struct file* file)
{
	current->files->fd[fd] = file;
}

# endif	/* LINUX_VERSION_CODE > KERNEL_VERSION(2,1,0) */


extern inline void fput_compat(struct file *file)
{
# if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)
	fput(file, file->f_inode);
# else
	fput(file);
# endif
}

extern inline void __fput_compat(struct file *file)
{
# if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)
	__fput(file, file->f_inode);
# else
	__fput(file);
# endif
}


#endif	/* _LINUX_COMPAT_FILE_H */

