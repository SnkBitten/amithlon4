/*
	irq - driver to delivery hardware interrupts to user space.

	Interface definition.

	(C) Dmitry A. Fedorov, fedorov@inp.nsk.su
	Copying-policy: LGPL

	$Id: irq.h,v 1.18 1999/09/29 06:47:56 fedorov Exp $

	tab size = 4
*/

#ifndef  _IRQ_H
# define _IRQ_H

#include "amithlon_pci.h"

# ifdef __KERNEL__
#  include <asm/ioctl.h>
# else
#  include <sys/ioctl.h>
# endif

# ifdef __cplusplus
   extern "C" {
# endif

typedef struct {
  struct task_struct *task;
  unsigned long      addr;
  unsigned long      handler;
} fpf_data;

typedef struct {
  unsigned long addr;
  unsigned long count;
  unsigned short port;
} ioport_data;

/* + ioctl definitions */

/* free slot from the linux/Documentation/ioctl-number.txt */
# define __IRQ_IOCTL_TYPE_LETTER 'i'

enum
{
	__IRQ_REQUEST_IOCTL_NUMBER=1,
	__IRQ_FREE_IOCTL_NUMBER,

	__IRQ_ENABLE_IOCTL_NUMBER,
	__IRQ_DISABLE_IOCTL_NUMBER,

	__IRQ_SKIPPING_ON_IOCTL_NUMBER,
	__IRQ_SKIPPING_OFF_IOCTL_NUMBER,

	__IRQ_SKIPPING_CONSENT_ON_IOCTL_NUMBER,
	__IRQ_SKIPPING_CONSENT_OFF_IOCTL_NUMBER,

	__IRQ_STAT_IOCTL_NUMBER,

	__IRQ_SIMULATE_IOCTL_NUMBER,
	
	__UAE_SET_TIMER_NUMBER,
	__UAE_GET_RANGE_NUMBER,
	__UAE_SET_FPF_NUMBER,
	__UAE_GET_FPF_EIP_NUMBER,
	__UAE_READ_IOPORT_NUMBER,
	__UAE_PCI_OP_NUMBER,
	__UAE_SET_IRQHANDLER_NUMBER,
	__UAE_STOP_IRQHANDLER_NUMBER,
	__UAE_GET_ZEROPAGE_NUMBER,
};


#define __IRQ_REQUEST_IOCTL \
	_IOW(__IRQ_IOCTL_TYPE_LETTER, __IRQ_REQUEST_IOCTL_NUMBER, \
		struct __irq_request_arg )

#define __IRQ_FREE_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_FREE_IOCTL_NUMBER)

#define __IRQ_ENABLE_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_ENABLE_IOCTL_NUMBER)
#define __IRQ_DISABLE_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_DISABLE_IOCTL_NUMBER)

#define __IRQ_SKIPPING_ON_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_SKIPPING_ON_IOCTL_NUMBER)
#define __IRQ_SKIPPING_OFF_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_SKIPPING_OFF_IOCTL_NUMBER)

#define __IRQ_SKIPPING_CONSENT_ON_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_SKIPPING_CONSENT_ON_IOCTL_NUMBER)
#define __IRQ_SKIPPING_CONSENT_OFF_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_SKIPPING_CONSENT_OFF_IOCTL_NUMBER)

#define __IRQ_STAT_IOCTL \
	_IOR(__IRQ_IOCTL_TYPE_LETTER, __IRQ_STAT_IOCTL_NUMBER, \
		struct irq_stat )

#define __IRQ_SIMULATE_IOCTL \
	_IO (__IRQ_IOCTL_TYPE_LETTER, __IRQ_SIMULATE_IOCTL_NUMBER)

#define __UAE_SET_TIMER_IOCTL \
	_IOW(__IRQ_IOCTL_TYPE_LETTER, __UAE_SET_TIMER_NUMBER, \
		unsigned long long )

#define __UAE_GET_RANGE_IOCTL \
	_IOWR(__IRQ_IOCTL_TYPE_LETTER, __UAE_GET_RANGE_NUMBER, \
		unsigned long)

#define __UAE_SET_FPF_IOCTL \
	_IOW(__IRQ_IOCTL_TYPE_LETTER, __UAE_SET_FPF_NUMBER, \
		fpf_data)

#define __UAE_GET_FPF_EIP_IOCTL \
	_IOR(__IRQ_IOCTL_TYPE_LETTER, __UAE_SET_FPF_NUMBER, \
		unsigned long)

#define __UAE_READ_IOPORT_IOCTL \
	_IOW(__IRQ_IOCTL_TYPE_LETTER, __UAE_READ_IOPORT_NUMBER, \
		ioport_data)

#define __UAE_PCI_OP_IOCTL \
	_IOWR(__IRQ_IOCTL_TYPE_LETTER, __UAE_PCI_OP_NUMBER, \
		pcidata)

#define __UAE_SET_IRQHANDLER_IOCTL \
	_IOWR(__IRQ_IOCTL_TYPE_LETTER, __UAE_SET_IRQHANDLER_NUMBER, \
		unsigned long)

#define __UAE_STOP_IRQHANDLER_IOCTL \
	_IOWR(__IRQ_IOCTL_TYPE_LETTER, __UAE_STOP_IRQHANDLER_NUMBER, \
		unsigned long)

#define __UAE_GET_ZEROPAGE_IOCTL \
	_IOR(__IRQ_IOCTL_TYPE_LETTER, __UAE_GET_ZEROPAGE_NUMBER, \
		unsigned long )
/* - ioctl definitions */


enum irq_flags
{
	IRQ_SHARED=1, IRQ_SKIPPING=4, IRQ_SKIPPING_CONSENT=8
};


struct __irq_request_arg
{
	int sig;
	int flags;
};

struct irq_stat
{
	size_t kstat_interrupts;
	size_t irq_handler_interrupts;
	size_t irq_handler_sent_interrupts;
	size_t irq_count;
	size_t sig_count;
};


/* + ioctl user interface */

# ifndef __KERNEL__

extern __inline__ int irq_request(int fd, int sig, int flags)
{
	struct __irq_request_arg arg = { sig, flags };
	return ioctl(fd, __IRQ_REQUEST_IOCTL, &arg);
}

extern __inline__ int irq_free(int fd)
	{ return ioctl(fd, __IRQ_FREE_IOCTL); }

extern __inline__ int irq_enable(int fd)
	{ return ioctl(fd, __IRQ_ENABLE_IOCTL); }

extern __inline__ int irq_disable(int fd)
	{ return ioctl(fd, __IRQ_DISABLE_IOCTL); }

extern __inline__ int irq_skipping_on(int fd)
	{ return ioctl(fd, __IRQ_SKIPPING_ON_IOCTL); }

extern __inline__ int irq_skipping_off(int fd)
	{ return ioctl(fd, __IRQ_SKIPPING_OFF_IOCTL); }

extern __inline__ int irq_skipping_consent(int fd)
	{ return ioctl(fd, __IRQ_SKIPPING_CONSENT_ON_IOCTL); }

extern __inline__ int irq_skipping_inconsent(int fd)
	{ return ioctl(fd, __IRQ_SKIPPING_CONSENT_OFF_IOCTL); }

extern __inline__ int irq_stat(int fd, struct irq_stat* stat)
	{ return ioctl(fd, __IRQ_STAT_IOCTL, stat); }

extern __inline__ int irq_simulate(int fd)
	{ return ioctl(fd, __IRQ_SIMULATE_IOCTL); }

# endif	//ndef __KERNEL__

/* - ioctl user interface */


# if 0
struct kernel_stat
{
	unsigned int interrupts[NR_IRQS];
};
extern struct kernel_stat kstat;
# endif

# ifdef __cplusplus
}
# endif

#endif	/* _IRQ_H */


/*
$Log: irq.h,v $
Revision 1.18  1999/09/29 06:47:56  fedorov
IRQ_ONESHOT flag removed to simplify driver.

Revision 1.17  1999/09/22 11:42:16  fedorov
comment style changed; internal names changed; binary compatibility keeped.

Revision 1.16  1999/08/11 11:39:21  fedorov
Header and history log formats changed.
Extra protection of multiple headers inclusion removed.
Some cosmetic changes.

Revision 1.15  1999/02/04 13:40:11  fedorov
indentation

Revision 1.14  1999/02/02 06:41:33  fedorov
C extern inline

Revision 1.13  1999/01/22 12:12:24  fedorov
conflict user/kernel ioctl headers resolved

Revision 1.12  1999/01/22 12:04:17  fedorov
conflict user/kernel ioctl headers resolved

Revision 1.11  1998/12/05 08:04:23  fedorov
*** empty log message ***

Revision 1.10  1998/04/11 13:27:21  fedorov
IRQ_IOCTL_TYPE_LETTER don't depend on Makefile

Revision 1.9  1998/04/10 11:21:09  fedorov
indentation

Revision 1.8  1998/03/25 22:42:01  fedorov
kernel space irq_simulate() declaration removed

Revision 1.7  1998/03/18 15:03:11  fedorov
kernel space irq_simulate() added

Revision 1.6  1998/03/16 10:48:43  fedorov
irq_simulate() added; indentation

Revision 1.5  1998/01/10 14:26:00  fedorov
skipping mode added, flags changed, shared mode deferred by
enable/disable now

Revision 1.4  1997/12/28 14:36:58  fedorov
extern C braces added

Revision 1.3  1997/12/15 13:19:26  fedorov
debugging reimplemented, task_chan has array of pointers to irq_chan now,
not a list, select() added.

Revision 1.2  1997/12/09 09:43:26  fedorov
something

Revision 1.1  1997/12/06 23:10:19  fedorov
Put under CVS control
*/

