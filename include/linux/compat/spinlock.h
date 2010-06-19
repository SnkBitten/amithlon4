/*
	<linux/compat/spinlock.h>

	compatibility spin lock macros

	$Id: spinlock.h,v 1.5 2000/05/21 13:43:55 fedorov Exp $

	compiled from kernel sources by Dmitry Fedorov, fedorov@inp.nsk.su
*/

#ifndef  _LINUX_COMPAT_SPINLOCK_H
# define _LINUX_COMPAT_SPINLOCK_H

# include <linux/compat/version.h>

# if LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)

#  ifndef __SMP__

/* Gcc-2.7.x has a nasty bug with empty initializers. */
#   if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
     typedef struct { } spinlock_t;
#    define SPIN_LOCK_UNLOCKED (spinlock_t) { }
#   else
     typedef struct { int gcc_is_buggy; } spinlock_t;
#    define SPIN_LOCK_UNLOCKED (spinlock_t) { 0 }
#   endif

#   define spin_lock_init(lock)   do { } while(0)
#   define spin_lock(lock)        do { } while(0)
#   define spin_trylock(lock)     (1)
#   define spin_unlock_wait(lock) do { } while(0)
#   define spin_unlock(lock)      do { } while(0)
#   define spin_lock_irq(lock)    cli()
#   define spin_unlock_irq(lock)  sti()

#   define spin_lock_irqsave(lock, flags) \
        do { save_flags(flags); cli(); } while (0)

#   define spin_unlock_irqrestore(lock, flags) \
        restore_flags(flags)

/*
 * Read-write spinlocks, allowing multiple readers
 * but only one writer.
 *
 * NOTE! it is quite common to have readers in interrupts
 * but no interrupt writers. For those circumstances we
 * can "mix" irq-safe locks - any writer needs to get a
 * irq-safe write-lock, but readers can get non-irqsafe
 * read-locks.
 *
 * Gcc-2.7.x has a nasty bug with empty initializers.
 */
#   if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
     typedef struct { } rwlock_t;
#    define RW_LOCK_UNLOCKED (rwlock_t) { }
#   else
     typedef struct { int gcc_is_buggy; } rwlock_t;
#    define RW_LOCK_UNLOCKED (rwlock_t) { 0 }
#   endif

#   define read_lock(lock)        do { } while(0)
#   define read_unlock(lock)      do { } while(0)
#   define write_lock(lock)       do { } while(0)
#   define write_unlock(lock)     do { } while(0)
#   define read_lock_irq(lock)    cli()
#   define read_unlock_irq(lock)  sti()
#   define write_lock_irq(lock)   cli()
#   define write_unlock_irq(lock) sti()

#   define read_lock_irqsave(lock, flags) \
        do { save_flags(flags); cli(); } while (0)

#   define read_unlock_irqrestore(lock, flags) \
        restore_flags(flags)

#   define write_lock_irqsave(lock, flags)	\
        do { save_flags(flags); cli(); } while (0)

#   define write_unlock_irqrestore(lock, flags) \
        restore_flags(flags)

#  else  /* def __SMP__ */

#   error 2.0.x SMP not supported

#  endif /* ndef __SMP__ */

# else   /* LINUX_VERSION_CODE > KERNEL_VERSION(2,1,0) */
#  include <linux/spinlock.h>
# endif

#endif	/* _LINUX_COMPAT_SPINLOCK_H */

