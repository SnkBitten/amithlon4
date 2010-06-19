/*
	<linux/compat/fs.h>

	compatibility <linux/fs.h> macros

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su

	$Id: fs.h,v 1.8 2000/05/21 13:43:55 fedorov Exp $
*/

#ifndef  _LINUX_COMPAT_FS_H
# define _LINUX_COMPAT_FS_H

# include <linux/compat/version.h>
# include <linux/fs.h>


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,31)
#  define FILE_OPERATIONS_CLOSE_TYPE void
#  define FILE_OPERATIONS_CLOSE_VALUE(rc)
# else
#  define FILE_OPERATIONS_CLOSE_TYPE int
#  define FILE_OPERATIONS_CLOSE_VALUE(rc) (rc)
# endif


/* + unstable */

# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,23)
#  define FILE_OPERATIONS_SEEK_PROTOTYPE(name,_inode,_file,_offset,_origin) \
	int name (struct inode* _inode, struct file* _file, \
		off_t _offset, int _origin)
# else
#  define FILE_OPERATIONS_SEEK_PROTOTYPE(name,_inode,_file,_offset,_origin) \
	loff_t name (struct file* _file, \
		loff_t _offset, int _origin)
# endif


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,23)

#  define COMPAT_FILE_OPERATIONS_INIT( \
	seek,read,write,readdir, \
	select,poll, \
	ioctl,mmap,open, \
	flush, \
	release,fsync,fasync,\
	check_media_change,revalidate, \
	lock) \
\
	{\
		seek,read,write,readdir, select, ioctl,mmap,open, \
		release,fsync,fasync,check_media_change,revalidate \
	}

# else

#  define COMPAT_FILE_OPERATIONS_INIT( \
	seek,read,write,readdir, \
	select,poll, \
	ioctl,mmap,open, \
	flush, \
	release,fsync,fasync,\
	check_media_change,revalidate, \
	lock) \
\
	{\
		seek,read,write,readdir, poll, ioctl,mmap,open, \
		flush, release,fsync,fasync,check_media_change,revalidate, lock \
	}

# endif

/* - unstable */


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,43)
#  define FILE_INODE(file) ((file)->f_inode)
# else
#  define FILE_INODE(file) ((file)->f_dentry->d_inode)
# endif

# define INODE_MINOR(inode) (MINOR((inode)->i_rdev))
# define INODE_MAJOR(inode) (MAJOR((inode)->i_rdev))

# define FILE_MINOR(file) (INODE_MINOR(FILE_INODE(file)))
# define FILE_MAJOR(file) (INODE_MAJOR(FILE_INODE(file)))


# if  LINUX_VERSION_CODE < KERNEL_VERSION(2,0,31)
#  define FILE_OWNER_PID(file) ((file)->f_owner)
# else
#  define FILE_OWNER_PID(file) ((file)->f_owner.pid)
# endif


#endif	/* _LINUX_COMPAT_FS_H */

