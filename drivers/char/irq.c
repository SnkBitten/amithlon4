/*
	irq - driver to delivery hardware interrupts to user space.

	(C) Dmitry A. Fedorov, fedorov@inp.nsk.su
	Copying-policy: GPL

	$Id: irq.c,v 1.67 2000/07/31 10:59:07 fedorov Exp $

	tab size = 4
*/


#include <linux/types.h>

#ifndef __KERNEL__
# error This is a Linux kernel device driver
#endif

#ifndef  IRQ_MAJOR
# define IRQ_MAJOR 60			// experimental
#endif

#define DISALLOW_SHARED_DISABLE 0

#if defined(MODVERSIONS) && !defined(__GENKSYMS__)
# include <linux/modversions.h>
#endif

#include <linux/config.h>

//#ifdef			 MODULE
# include <linux/module.h>
//#endif

#if defined(MODVERSIONS) && defined(CONFIG_IRQ_EXPORT_SIMULATE) && !defined(__GENKSYMS__)
# include "irq.ver"
#endif

#include <linux/mm.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/smp_lock.h>
#include <linux/highuid.h>

#include <linux/compat/uaccess.h>
#include <linux/compat/init.h>
#include <linux/compat/atomic.h>
#include <linux/compat/bitops.h>
#include <linux/compat/system.h>		// xchg_compat()
#include <linux/compat/wait.h>			// wait_queue_head
#include <linux/compat/spinlock.h>
#include <linux/compat/fs.h>			// file_operations
#include <linux/compat/poll.h>			// poll_wait()
#include <linux/compat/sched.h> 		// NEED_RESCHED
#include <linux/compat/errno.h> 		// ENOTSUP

#include <linux/types.h>
#include <linux/slab.h>				// kmalloc(), kfree()
#include <asm/irq.h>					// NR_IRQS
#include <linux/major.h>				// MAX_CHRDEV

#include <linux/df/pr.h>				// pr_error(), pr_endl()
#include <linux/df/assert.h>			// assert_retvoid(), assert_retval()
#include <linux/df/save_flags_and_cli.h>
#include <linux/df/strdup.h>

#include <asm/io.h>
#include <asm/system.h>

#include "./irq.h"						// API definitions

#include "amithlon_pci.h"

#define REGPARM(n) __attribute__(( regparm(n) ))

#define UNUSED __attribute__(( unused ))


/* UAE memory blocks. Why here, you ask? Because it's easy. */
typedef struct {
  int start;
  int end;
} range;

#define UAE_BLOCKS_N 16
static range uae_blocks[UAE_BLOCKS_N];
static int blockindex=0;

extern unsigned long process_to_elevate;
extern int do_elevate_process;
extern unsigned long zeropage;

void add_uae_block(int start, int end)
{
  if (blockindex==UAE_BLOCKS_N)
    return;
  uae_blocks[blockindex].start=start;
  uae_blocks[blockindex].end=end;
  blockindex++;
}

/* UAE fast page fault handling */

static fpf_data fpf;
static unsigned long fpf_eip;

int uae_handle_fault(struct pt_regs *regs, unsigned long error_code,
		     unsigned long address)
{
  error_code=5;
  if (current==fpf.task && 
      address==fpf.addr &&
      fpf.handler &&
      (error_code&1) &&  /* protection fault, rather than page fault */
      (error_code&4)) {  /* In user mode */
    /* Do it */
    unsigned long x;
    fpf_eip=regs->eip;
    if (COPY_FROM_USER(&x,(const void*)(fpf_eip+6), sizeof(x)))
      return 0;
    regs->eip=x;
    return 1;
  }
  return 0;
}

/* UAE high precision timers. Kludge in here like nothing you ever dreamt of */
unsigned long long uae_alert=~0; /* When to start checking timers */
unsigned long long uae_nextevent=~0; /* When to next trigger */



//==== declarations ====================================================

static int major = IRQ_MAJOR;	// 0 - auto - it works, but useless

static const char      name[]             = "irq";

static const char   version[] __init_data =
	"$Revision: 1.67 $ $Date: 2000/07/31 10:59:07 $";

static const char copyright[] __init_data =
	"(C) Dmitry A. Fedorov, 1997, fedorov@inp.nsk.su";

typedef enum { false=0, true=1, Off=false, On=true } bool;
typedef struct __irq_request_arg __irq_request_arg;
typedef struct inode Inode;
typedef struct file File;
typedef struct task_struct task_struct;



//==== utilities =======================================================

# include <linux/df/strdup.imp>

static int nomem(void)
{
	pr_error("%s: Out of memory!\n", name);
	return -ENOMEM;
}

extern inline bool has_readwrite_access(const File* file)
{
	return ( (file->f_flags&O_ACCMODE) != O_RDWR ) ? false : true;
}


#if 0
static uint trace_count=0;

# define TRACE()	\
	printk("%u: " __FILE__ ":%u: " __FUNCTION__ "()\n", \
		trace_count++, __LINE__)

# define TRACEF(fmt, arg...)	\
	do \
	{ \
		printk("%u: " __FILE__ ":%u: " __FUNCTION__ "(): ", \
			trace_count++, __LINE__); \
		printk(fmt, ##arg); \
	} while(0)
#endif

/***************** PCI handling ****************************************/

#define PCI_MINOR 255

typedef struct pci_list_t {
  struct pci_dev* dev;
  unsigned char* releasecode;
  char* name;
  struct pci_list_t* next;
  struct pci_list_t** prev_p;
} pci_list;

pci_list* devlist=NULL;


static int find_code_size(unsigned char* ud)
{
  int size=0;

  while (1) {
    unsigned char c;
    COPY_FROM_USER(&c,ud+size+2,1);
    switch(c) {
    case 0xa0: size+=4; break;
    case 0xe0: size+=6; break;
    case 0x90: size+=4; break;
    case 0xd0: size+=6; break;
    case 0x80: size+=6; break;
    case 0xc0: size+=8; break;
    default: return size;
    }
    size+=2;
  }
}

u32 dummy=0;

static void execute_release_code(pci_list* x)
{
  int pos;
  u8* releasecode;

  if (!x->releasecode)
    return;
  pos=0;
  releasecode=x->releasecode;

  cli();
  while (1) {
    int basenum=releasecode[pos];
    int type=releasecode[pos+2];
    unsigned int base=x->dev->resource[basenum].start;
    int io=(x->dev->resource[basenum].flags&1);
    unsigned int offset;
    unsigned int val;
    int len;
    unsigned int i;
    int op=releasecode[pos+1];

    pos+=2; /* skip base/op */
    if (type&0x40) {
      offset=((u32)releasecode[pos+2]<<8)|
	((u32)releasecode[pos+3]<<0);
      pos+=2;
    }
    else {
      offset=releasecode[pos+1];
    }      
    pos+=2; /* skip type/offset */
    switch(type) {
    case 0x80:
    case 0xc0: len=4; break;
    case 0x90:
    case 0xd0: len=2; break;
    case 0xa0:
    case 0xe0: len=1; break;
    default:
      sti();
      return;
    }
    val=0;
    for (i=0;i<len;i++)
      val=(val<<8)+releasecode[pos+i];

    printk("op %d: base %x, offset %x, val %x, io=%d, len=%d\n",op,base,offset,val,io,len);

    switch(op) {
    case 0x00: 
      {/* write */
	if (io) {
	  switch(len) {
	  case 4: outl(val,base+offset); break;
	  case 2: outw(val,base+offset); break;
	  case 1: outb(val,base+offset); break;
	  }
	}
	else { /* memory */
	  unsigned long paddr=(unsigned long)bus_to_virt(base+offset);
	  void* addr=ioremap(paddr,4);

	  switch(len) {
	  case 4: *((u32*)addr)=val; break;
	  case 2: *((u16*)addr)=val; break;
	  case 1: *((u8*)addr)=val; break;
	  }
	  iounmap(addr);
	}
      }
      break;

    case 0x01: 
      { /* read */
	if (io) {
	  switch(len) {
	  case 4: dummy+=inl(base+offset); break;
	  case 2: dummy+=inw(base+offset); break;
	  case 1: dummy+=inb(base+offset); break;
	  }
	}
	else { /* memory */
	  unsigned long paddr=(unsigned long)bus_to_virt(base+offset);
	  void* addr=ioremap(paddr,4);

	  switch(len) {
	  case 4: dummy+=*((u32*)addr); break;
	  case 2: dummy+=*((u16*)addr); break;
	  case 1: dummy+=*((u8*)addr); break;
	  }
	  iounmap(addr);
	}
      }
      break;

    case 0x02: 
      { /* sleep */
	for (i=0;i<val;i++)
	  outb(0,0x80);
      }
      break;

    default:
      break;
    }
    
    if (len==1)
      pos+=2;
    else
      pos+=len;
  }
  sti();
}

static void release_device(struct pci_dev* dev, int runcode)
{
  pci_list* l=devlist;

  while (l && l->dev!=dev)
    l=l->next;
  if (!l)
    return;
  if (runcode)
    execute_release_code(l);
  pci_release_regions(l->dev);

  if (l->next)
    l->next->prev_p=l->prev_p;
  *(l->prev_p)=l->next;
  kfree(l->name);
  if (l->releasecode)
    kfree(l->releasecode);
  kfree(l);
}

static void release_all_devices(void)
{
  pci_list* l=devlist;

  while (l) {
    pci_list* next=l->next;

    release_device(l->dev,1);
    l=next;
  }
}

static int pcicommand(pcidata* x)
{
  struct pci_dev* sh=(struct pci_dev*)x->starthandle;

  switch(x->command) {
  case CMD_FIND_SLOT:
    x->handle=(unsigned long)pci_find_slot(x->busnum,
					   PCI_DEVFN(x->devnum,x->funnum));
    return 0;
  case CMD_FIND_SUBSYS:
    x->handle=(unsigned long)pci_find_subsys(x->vendor,
					     x->device,
					     x->subsys_vendor,
					     x->subsys_device,
					     sh);
    return 0;
  case CMD_FIND_DEVICE:
    x->handle=(unsigned long)pci_find_device(x->vendor,
					     x->device,
					     sh);
    return 0;
  case CMD_FIND_CLASS:
    x->handle=(unsigned long)pci_find_class(x->class,
					    sh);
    return 0;
  case CMD_FIND_CAPAB:
    x->cappos=pci_find_capability(sh,
				  x->capability);
    return 0;
  case CMD_SET_POWER:
    x->oldpowerstate=pci_set_power_state(sh,
					 x->powerstate);
    return 0;
  case CMD_ENABLE:
    x->result=pci_enable_device(sh);
    return 0;
  case CMD_DISABLE:
    pci_disable_device(sh);
    return 0;
  case CMD_RELEASE:
    release_device(sh,0);
    return 0;
  case CMD_REQUEST:
    {
      char* name;
      
      if (x->res_name) {
	int len;
	char c;
	int i;

	len=0;
	while (!COPY_FROM_USER(&c, x->res_name+len,1)) {
	  if (!c)
	    break;
	  len++;
	}
	name=kmalloc(len+1,GFP_KERNEL);
	for (i=0;i<len;i++) {
	  name[i]=0;
	  COPY_FROM_USER(name+i, x->res_name+i,1);
	}
	name[i]=0;
      }
      else {
	name=kmalloc(22,GFP_KERNEL);
	strcpy(name,"amithlon pci system");
      }

      x->result=pci_request_regions(sh,name);
      if (!x->result) { /* Successful */
	pci_list* n=kmalloc(sizeof(pci_list),GFP_KERNEL);
	n->dev=sh;
	if (x->releasecode) {
	  int size=find_code_size(x->releasecode);
	  n->releasecode=kmalloc(size,GFP_KERNEL);
	  COPY_FROM_USER(n->releasecode,x->releasecode,size);
	}
	else 
	  n->releasecode=NULL;
	n->name=name;
	n->next=devlist;
	n->prev_p=&devlist;
	if (devlist)
	  devlist->prev_p=&(n->next);
	devlist=n;
      }
      else {
	kfree(name);
      }
    }
    return 0;
    
  case CMD_READBYTE:
    x->confdata=0;
    x->result=pci_read_config_byte(sh,
				   x->offset,
				   (u8*)&(x->confdata));
    return 0;

  case CMD_READWORD:
    x->confdata=0;
    x->result=pci_read_config_word(sh,
				   x->offset,
				   (u16*)&(x->confdata));
    return 0;
  case CMD_READLONG:
    x->confdata=0;
    x->result=pci_read_config_dword(sh,
				    x->offset,
				    (u32*)&(x->confdata));
    return 0;
  case CMD_WRITEBYTE:
    x->result=pci_write_config_byte(sh,
				    x->offset,
				    (u8)(x->confdata));
    return 0;
  case CMD_WRITEWORD:
    x->result=pci_write_config_word(sh,
				    x->offset,
				    (u16)(x->confdata));
    return 0;
  case CMD_WRITELONG:
    x->result=pci_write_config_dword(sh,
				     x->offset,
				     (u32)(x->confdata));
    return 0;
    
  case CMD_GETBASE:
    x->start=sh->resource[x->basenum].start;
    x->end=sh->resource[x->basenum].end;
    x->flags=sh->resource[x->basenum].flags;
    return 0;

  case CMD_GETINFO:
    x->irq=sh->irq;
    x->devnum=PCI_SLOT(sh->devfn);
    x->funnum=PCI_FUNC(sh->devfn);
    x->busnum=sh->bus->number;
    return 0;

  case CMD_GETNAME:
    {
      int len=0;
      do {  
	if (COPY_TO_USER((void*)(x->res_name+len),(void*)(sh->name+len),1))
	  return -EFAULT;
      } while (sh->name[len++]);
    }
    return 0;
  default:
    return -EINVAL;
  }
}

//==== irq utilities ===================================================

extern inline uint irq_minor(ushort minor)
{
	return minor;	// /dev/irqN has minor N
}

extern inline uint irq_inode(const Inode* inode)
{
	return irq_minor(INODE_MINOR(inode));
}

extern inline uint irq_file(const File* file)
{
	return irq_minor(FILE_MINOR(file));
}


//==== declarations ====================================================

//+ init/cleanup
static int _irq_init(void);
#ifdef MODULE
int init_module(void);
void cleanup_module(void);
#else
int irq_init(void);
#endif
//- init/cleanup


//+ file_operations
static int irq_open(Inode* inode, File* file);
static FILE_OPERATIONS_CLOSE_TYPE irq_close(Inode* inode, File* file);

static FILE_OPERATIONS_SEEK_PROTOTYPE(irq_seek,inode,file,offset,origin);

static int irq_ioctl(Inode* inode, File* file, uint cmd, ulong arg);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,23)
static int irq_select(Inode* inode, File* file, int sel_type, select_table* wt);
#else
static uint irq_poll(struct file* file, poll_table* pt);
#endif
//- file_operations


//+ data structures and methods

typedef struct irq_channel irq_chan;
typedef struct task_channel task_chan;



// interrupt delivery channel
struct irq_channel
{
#ifndef NDEBUG
	ulong signature;		// assertions
# define ICH_SIG 0xdeadbeef
#endif

	File* file;
	uint irq;				// access speed-up
	task_chan* tch;			// my container
	int sig;				// sig num for delivery, -1 for select(2) only
	int flags;				// enum irq_flags
	char* devname;			// for info in /proc/interrupt

	wait_queue_head_t wqh;

	size_t irq_count;		// received interrupts
	size_t sig_count;		// signals sent
	size_t sel_count;		// interrupt counter for irq_select()
	size_t skipping_count;	// local skipping mode counter
};

static irq_chan* ich_ctor(File* file, int sig, int flags);
static void ich_dtor(irq_chan* ich);
static void ich_attach_tch(irq_chan* ich, task_chan* tch);
static void ich_detach_tch(irq_chan* ich);




// user task's interrupt delivery channel
struct task_channel
{
#ifndef NDEBUG
	ulong signature;			// assertions
# define TCH_SIG 0xbeefdead
#endif

	task_chan* next;			// list
	task_chan* prev;			//

	irq_chan* ichs[NR_IRQS];	// interrupt delivery channels
	size_t nr_ich;

	task_struct* owner_task;	// interrupt requester/owner
	pid_t owner_pid;			//
};

static task_chan* tch_ctor(task_struct* task);
static void       tch_dtor(task_chan* tch);
static void       tch_link(task_chan* tch);
static void       tch_unlink(task_chan* tch);
static task_chan* tch_find(const task_struct* task);
static task_chan* tch_new_link(task_struct* task);
#if 0
static task_chan* tch_find_new_link(task_struct* task);
#endif
static void tch_attach_ich(task_chan* tch, irq_chan* ich);
static void tch_detach_ich(task_chan* tch, uint irq);


// are all of interrupt delivery channels empty?
extern inline bool tch_is_empty(const task_chan* tch)
{
	assert_retval(tch!=NULL, false);
	assert_retval(tch->signature==TCH_SIG, false);
	return tch->nr_ich==0;
}

// delete self if all of interrupt delivery channels are empty.
// returns empty flag.
extern inline bool tch_dtor_if_empty(task_chan* tch)
{
	return tch_is_empty(tch) ? (tch_dtor(tch), true) : false;
}


static bool check_ownership(const irq_chan* ich)			REGPARM(1);

static int check_null_and_ownership(const irq_chan* ich)	REGPARM(1);

//- data structures and methods


//+
static int do_request(File* file, int sig, int flags);

static int register_irq_handler(irq_chan* ich)	REGPARM(1);
static bool free_irq_ich_tch(irq_chan* ich)		REGPARM(1);

static void skipping_on (irq_chan* ich)	REGPARM(1);
static int	skipping_off(irq_chan* ich)	REGPARM(1);
static void skipping_sub(irq_chan* ich)	REGPARM(1);

static void irq_simulate(uint irq);

static void irq_handler(int irq, void* dev_id, struct pt_regs* ptregs);
static void do_irq(irq_chan* ich) REGPARM(1);
//-



//==== common variables & tables =======================================

// file_operations switcher
static struct file_operations fops = {
  owner:  NULL,
  llseek: irq_seek,		// for error reporting only
  poll:   irq_poll,
  ioctl:  irq_ioctl,
  open:   irq_open,
  release: irq_close,
  };


static size_t irq_channels_registered[NR_IRQS] = { [0 ... NR_IRQS-1] = 0 };

// list of tasks with opened interrupt delivery channels
static task_chan* tchlist=NULL;
static spinlock_t tchlist_lock = SPIN_LOCK_UNLOCKED; //used in simulate() only

// skipping mode counter
static size_t skipping_irq_counts[NR_IRQS] = { [0 ... NR_IRQS-1] = 0 };

static size_t disabled_irq_counts[NR_IRQS] = { [0 ... NR_IRQS-1] = 0 };


#ifdef CONFIG_IRQ_EXPORT_SIMULATE
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18)
static struct symbol_table irq_syms =
{
#  include <linux/symtab_begin.h>
	X(irq_simulate),
#  include <linux/symtab_end.h>
};
# else
EXPORT_SYMBOL(irq_simulate);
# endif
#endif


//======================================================================

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,1,0)

static void REGPARM(1) DISABLE_IRQ(uint irq)
{
	assert_retvoid(irq<NR_IRQS);
	disabled_irq_counts[irq] = 1;
	disable_irq(irq);
}

static void REGPARM(1) ENABLE_IRQ(uint irq)
{
	assert_retvoid(irq<NR_IRQS);
	disabled_irq_counts[irq] = 0;
	enable_irq(irq);
}

#else

static void REGPARM(1) DISABLE_IRQ(uint irq)
{
	assert_retvoid(irq<NR_IRQS);
	++disabled_irq_counts[irq];
	disable_irq(irq);
}

static void REGPARM(1) ENABLE_IRQ(uint irq)
{
	assert_retvoid(irq<NR_IRQS);
	if (disabled_irq_counts[irq] > 0)
	{
		--disabled_irq_counts[irq];
		enable_irq(irq);
	}
}

#endif


//==== debugging =======================================================

static void _prefix(const char* file, int line, const char* func)
{
	printk("%s:%d %s(): ", file, line, func);
}

#define prefix() _prefix(__FILE__,__LINE__,__FUNCTION__)


#ifdef DEBUG

static size_t msg_counter = 0;

static void print_msg_counter(void)
{
	printk("%u: ", msg_counter++);
}

static void pfx(uint irq)
{
	printk("%s[%u]: ", name, irq);
}

# define PFX(irq)		pfx(irq)
# define PREFIX()		prefix()
# define PRDEBUG(fmt,arg...)	printk(fmt,##arg)
# define PRMSGCOUNTER()	print_msg_counter()
# define PRENDL() pr_endl()
#
#else
#
# define PFX(irq)
# define PREFIX()
# define PRDEBUG(fmt,arg...)
# define PRMSGCOUNTER()
# define PRENDL()
#
#endif



#ifdef DEBUG

static void ICH_PRINT(const irq_chan* ich)
{
	ulong flags;
	irq_chan tich;

	if (ich==NULL)
	{
		printk("!NULL!");
		return;
	}

	flags=save_flags_and_cli();
	memcpy(&tich, ich, sizeof(tich));
	restore_flags(flags);

	printk( "(devname=%s irq=%d tch=%p sig=%d flags=0x%08X "
			"irq_count=%u sig_count=%u)",
		tich.devname, tich.irq, tich.tch, tich.sig, tich.flags,
		tich.irq_count, tich.sig_count);
}

static void ICHLIST_PRINT(const task_chan* tch)
{
	int i;

	if (tch==NULL)
		printk("!NULL!");

	assert_retvoid(tch->signature==TCH_SIG);

	printk("[");
	for (i=0; i<NR_IRQS; i++)
	{
		assert_retvoid(tch->ichs[i]==NULL || tch->ichs[i]->signature==ICH_SIG);
		printk(" %p",tch->ichs[i]);
	}
	printk(" ]");
}

static void TCHLIST_PRINT(void)
{
	task_chan* tch;

	for(tch=tchlist; tch!=NULL; tch=tch->next)
	{
		assert_break(tch->signature==TCH_SIG);

		printk("%s %s(): tch=%p next=%p, ichlist: ",
			name, __FUNCTION__, tch, tch->next);
		ICHLIST_PRINT(tch);

		pr_endl();
	}
}

#else	// ndef DEBUG

# define ICHLIST_PRINT(tch)
# define TCHLIST_PRINT()
# define ICH_PRINT(ich)

#endif	// def DEBUG



//==== implementation ==================================================

//+ data structures and methods

// allocate memory for interrupt delivery channel structure and initialize.
// returns NULL on memory exhausted.
static irq_chan* ich_ctor(File* file, int sig, int flags)
{
	register irq_chan* ich;

	assert_retval(irq_file(file) < NR_IRQS, NULL);

	ich=kmalloc(sizeof(irq_chan), GFP_KERNEL);

	if (ich!=NULL)
	{
#ifndef NDEBUG
		ich->signature=ICH_SIG;
#endif

		ich->file=file;
		file->private_data = ich;
		ich->irq=irq_file(file);
		ich->tch=NULL;
		ich->sig=sig;
		ich->flags=flags;
		ich->devname=NULL;

		init_waitqueue_head(&ich->wqh);

		ich->irq_count=0;
		ich->sig_count=0;
		ich->sel_count=0;
		ich->skipping_count=0;
	}

	PREFIX(); PRDEBUG("ich=%p ich=", ich); ICH_PRINT(ich); PRENDL();
	return ich;
}

static void REGPARM(1) ich_dtor(irq_chan* ich)
{
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(ich->tch==NULL);
	PREFIX(); ICH_PRINT(ich); PRENDL();

	ich->file->private_data = NULL;

#ifndef NDEBUG
	ich->signature=0;
	ich->file=NULL;
	ich->tch=NULL;
#endif

	kfree(ich->devname);

#ifndef NDEBUG
	ich->devname=NULL;
#endif

	kfree(ich);
}


static void ich_attach_tch(irq_chan* ich, task_chan* tch)
{
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(tch!=NULL);
	assert_retvoid(tch->signature==TCH_SIG);

	ich->tch=tch;
}

static void ich_detach_tch(irq_chan* ich)
{
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(ich->tch!=NULL);
	assert_retvoid(ich->tch->signature==TCH_SIG);

	ich->tch=NULL;
}



// allocate memory for task channel structure and initialize.
// returns NULL on memory exhausted.
static task_chan* tch_ctor(task_struct* task)
{
	register task_chan* tch=kmalloc(sizeof(task_chan),GFP_KERNEL);

	if (tch!=NULL)
	{
#ifndef NDEBUG
		tch->signature=TCH_SIG;
#endif
		tch->next=NULL;
		tch->prev=NULL;
		memset(tch->ichs, 0, sizeof(tch->ichs));
		tch->nr_ich=0;
		tch->owner_task=task;
		tch->owner_pid=task->pid;
	}
	return tch;
}

static void tch_dtor(task_chan* tch)
{
	assert_retvoid(tch!=NULL);
	assert_retvoid(tch->signature==TCH_SIG);

	tch_unlink(tch);
	if (tch_is_empty(tch))
	{
		assert_retvoid(tch->next==NULL);
		assert_retvoid(tch->prev==NULL);
		assert_retvoid(tch->nr_ich==0);

#ifndef NDEBUG
		memset(&tch->ichs, 0, sizeof(tch->ichs));
		tch->owner_task=NULL;
		tch->owner_pid=-1;
#endif
		kfree(tch);
	}
	else
	{
		prefix(); printk("task_chan* %p is not empty!\n", tch);
	}
}

// insert new and cleared task channel at begin of list.
static void tch_link(task_chan* tch)
{
	assert_retvoid(tch!=NULL);
	assert_retvoid(tch->signature==TCH_SIG);
	assert_retvoid(tch->next==NULL);
	assert_retvoid(tch->prev==NULL);

	if (tchlist!=NULL)
	{
		assert_retvoid(tchlist->signature==TCH_SIG);

		tch->next=tchlist;
		tchlist->prev=tch;
	}
	tchlist=tch;
}

static void tch_unlink(task_chan* tch)
{
	assert_retvoid(tch!=NULL);
	assert_retvoid(tch->signature==TCH_SIG);

	assert_retvoid(tchlist==NULL || tchlist->signature==TCH_SIG);

	assert_retvoid(tch->next==NULL || tch->next->signature==TCH_SIG);
	assert_retvoid(tch->prev==NULL || tch->prev->signature==TCH_SIG);

	assert_retvoid(tch->next==NULL || tch->next->prev == tch);
	assert_retvoid(tch->prev==NULL || tch->prev->next == tch);

	if (tch->next!=NULL)		// I am not a last member in list
		tch->next->prev = tch->prev;

	if (tch->prev!=NULL)		// I am not a first member in list
		tch->prev->next = tch->next;
	else						// I am a first member in list
		tchlist=tch->next;
}

// find task channel.
static task_chan* tch_find(const task_struct* task)
{
	task_chan* tch;

	for(tch=tchlist; tch!=NULL; tch=tch->next)
	{
		assert_break(tch->signature==TCH_SIG);

		if (tch->owner_task==task && tch->owner_pid==task->pid)
			return tch;
	}

	return NULL;
}

// create new task channel and add it to list.
// returns NULL on ENOMEM.
static task_chan* tch_new_link(task_struct* task)
{
	task_chan* tch;

	if ((tch=tch_ctor(task))==NULL)
		return NULL;
	assert_retval(tch->signature==TCH_SIG, NULL);
	tch_link(tch);
	return tch;
}

#if 0
// find task channel, if isn't create new and add it to list.
// returns NULL on ENOMEM.
static task_chan* tch_find_new_link(task_struct* task)
{
	task_chan* tch=tch_find(task);
	if (tch!=NULL) return tch;

	return tch_new_link(task);
}
#endif

static void tch_attach_ich(task_chan* tch, irq_chan* ich)
{
	assert_retvoid(tch!=NULL);
	assert_retvoid(tch->signature==TCH_SIG);
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);

	assert_retvoid(ich->irq<NR_IRQS);
	assert_retvoid(tch->ichs[ich->irq]==NULL);

	tch->ichs[ich->irq] = ich;
	tch->nr_ich++;
	ich_attach_tch(ich,tch);
}

static void tch_detach_ich(task_chan* tch, uint irq)
{
	assert_retvoid(tch!=NULL);
	assert_retvoid(tch->signature==TCH_SIG);
	assert_retvoid(irq<NR_IRQS);
	assert_retvoid(tch->ichs[irq] != NULL);
	assert_retvoid(tch==tch->ichs[irq]->tch);
	assert_retvoid(tch->nr_ich>0);

	ich_detach_tch(tch->ichs[irq]);
	tch->ichs[irq] = NULL;
	tch->nr_ich--;
}



//returns error flag
static bool check_ownership(const irq_chan* ich)
{
	assert_retval(ich!=NULL, true);
	assert_retval(ich->signature==ICH_SIG, true);
	assert_retval(ich->tch!=NULL, true);
	assert_retval(ich->tch->signature==TCH_SIG, true);

	return (ich->tch->owner_pid != current->pid);
}

//returns -errno or 0
static int check_null_and_ownership(const irq_chan* ich)
{
	if (ich==NULL)
		return -EPERM;

	if (check_ownership(ich))
		return -EACCES;

	return 0;
}

//- data structures and methods


//+ init/cleanup

static int __init_text _irq_init(void)
{
	int rc;

	pr_info("%s driver %s\n%s    %s\n",
		name,version, KERN_INFO,copyright);
	pr_info("%s: ", name);
	if ( (rc=register_chrdev(major,name,&fops)) <0 )
	{
		printk("registration failed: ");
		if (rc==-EBUSY)
			if (major==0)
				printk("no free major slots");
			else
				printk("major %d is busy",major);
		else if (rc==-EINVAL)
				printk("major number %d > %d", major, MAX_CHRDEV-1);
		else
			printk("rc = %d",rc);
		pr_endl();
		return rc;
	}


#if defined(MODULE) && defined(CONFIG_IRQ_EXPORT_SIMULATE)
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18)
	if( (rc=register_symtab(&irq_syms))!=0)
	{
		printk("register_symtab() error\n");
		unregister_chrdev(major,name);
		return rc;
	}
# endif
#endif

	if (major==0) major=rc;
	printk("registered with major %u\n", major);

	// static object constructors

	return 0;
}

#ifdef MODULE
int __init_text init_module(void)
{
	return _irq_init();
}

void cleanup_module(void)
{
	int rc=unregister_chrdev(major,name);
	pr_info("%s unloaded ",name);
	if (rc==0)
		printk("successfully");
	else
		printk("with error: ");
		if (rc==-EINVAL)
			printk("major number %d is not registered"
					" with the matching name %s",
				major, name);
		else
			printk("%d",rc);
	pr_endl();
}

#else	//def MODULE

int __init_text irq_init(void)
{
	return _irq_init();
}
#endif	//MODULE

//- init/cleanup


//+ file_operations routines

static int irq_open(Inode* inode, File* file)
{
	PREFIX(); PFX(irq_inode(inode)); PRDEBUG("opening by %d...\n",
		current->pid);

	if (irq_inode(inode)>=NR_IRQS && irq_inode(inode)!=PCI_MINOR)
		return -ENXIO;
#if 0
	if (file->f_flags & ~O_ACCMODE)
		return -EINVAL;
#endif
	file->private_data=NULL;

	MOD_INC_USE_COUNT;
	PREFIX(); PFX(irq_inode(inode)); PRDEBUG("opened by %d\n", current->pid);
	PREFIX(); TCHLIST_PRINT();
	return 0;
}

static FILE_OPERATIONS_CLOSE_TYPE irq_close(Inode* UNUSED inode, File* file)
{
	register irq_chan* ich=file->private_data;

	if (irq_inode(inode)==PCI_MINOR) 
	  release_all_devices();
	PREFIX(); PFX(irq_inode(inode)); PRDEBUG("closing by %d...\n",
		current->pid);
	PREFIX(); TCHLIST_PRINT();

	if (ich!=NULL)
	{
		assert_goto(ich->signature==ICH_SIG, out);
		assert_goto(ich->tch!=NULL, out);
		assert_goto(ich->tch->signature==TCH_SIG, out);

		PREFIX(); PRDEBUG("%d found, tch=%p\n", current->pid, ich->tch);
		PREFIX(); TCHLIST_PRINT();

		free_irq_ich_tch(ich);

		PREFIX(); TCHLIST_PRINT();
	}

out:
	MOD_DEC_USE_COUNT;

	PREFIX(); PFX(irq_inode(inode)); PRDEBUG("closed by %d", current->pid);
	if (ich==NULL)		//no registered handler
	{
		PRDEBUG(" (w/o irq channels)");
		;
	}
	PRENDL();

	return FILE_OPERATIONS_CLOSE_VALUE(0);
}

static FILE_OPERATIONS_SEEK_PROTOTYPE(irq_seek, UNUSED inode, UNUSED file,
	UNUSED offset, UNUSED origin)
{
	return -ESPIPE;
}

extern unsigned long cpu_khz;
extern void schedule_hiprec(void);

#define rdtsc(low,high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

static int irq_ioctl(Inode* inode, File* file, uint cmd, ulong arg)
{
  uint irq=irq_inode(inode);
  int rc=0;
  __irq_request_arg req_arg;
  unsigned long long when;
  irq_chan* ich=file->private_data;

  if (irq==PCI_MINOR) { /* ANY ioctl on the pci one will release the devices */
    release_all_devices();
    return 0;
  }
  assert_retval(irq<NR_IRQS, -EBADFD);
  PREFIX(); PFX(irq); PRDEBUG("ioctl by %d, cmd: 0x%08X, arg: 0x%08lX\n",
			      current->pid, cmd, arg);
  PREFIX(); TCHLIST_PRINT();

  switch (cmd)
    {
    case __IRQ_REQUEST_IOCTL:
      if ( COPY_FROM_USER(&req_arg, (const void*)arg, sizeof(req_arg)) )
	return -EFAULT;

      PREFIX(); PFX(irq);
      PRDEBUG("ioctl IRQ_REQUEST: pid=%d sig=%d flags=0x%08X\n",
	      current->pid, req_arg.sig, req_arg.flags);

      if ( ich!=NULL && check_ownership(ich) )
	return -EACCES;

      return do_request(file, req_arg.sig, req_arg.flags);

    case __IRQ_FREE_IOCTL:
      PREFIX(); PFX(irq); PRDEBUG("ioctl IRQ_FREE: pid=%d sig=%ld\n",
				  current->pid, arg);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;

      free_irq_ich_tch(ich);
      return 0;

    case __IRQ_ENABLE_IOCTL:
      PREFIX(); PFX(irq); PRDEBUG("ioctl IRQ_ENABLE: pid=%d\n",
				  current->pid);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;
#if DISALLOW_SHARED_DISABLE
      if (ich->flags&IRQ_SHARED)
	return -EPERM;
#endif

      ENABLE_IRQ(irq);
      break;

    case __IRQ_DISABLE_IOCTL:
      PREFIX(); PFX(irq); PRDEBUG("ioctl IRQ_DISABLE: pid=%d\n",
				  current->pid);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;
#if DISALLOW_SHARED_DISABLE
      if (ich->flags&IRQ_SHARED)
	return -EPERM;
#endif

      DISABLE_IRQ(irq);
      break;

    case __IRQ_SKIPPING_ON_IOCTL:
      PREFIX(); PFX(irq); PRDEBUG("ioctl IRQ_SKIPPING_ON: pid=%d\n",
				  current->pid);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;

      skipping_on(ich);
      break;

    case __IRQ_SKIPPING_OFF_IOCTL:
      PREFIX(); PFX(irq); PRDEBUG("ioctl IRQ_SKIPPING_OFF: pid=%d\n",
				  current->pid);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;

      return skipping_off(ich);

    case __IRQ_SKIPPING_CONSENT_ON_IOCTL:
      PREFIX(); PFX(irq);
      PRDEBUG("ioctl IRQ_SKIPPING_CONSENT_ON: pid=%d\n",
	      current->pid);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;

      ich->flags|=IRQ_SKIPPING_CONSENT;
      break;

    case __IRQ_SKIPPING_CONSENT_OFF_IOCTL:
      PREFIX(); PFX(irq);
      PRDEBUG("ioctl IRQ_SKIPPING_CONSENT_OFF: pid=%d\n",
	      current->pid);

      rc=check_null_and_ownership(ich);
      if (rc!=0)
	return rc;

      ich->flags &= ~IRQ_SKIPPING_CONSENT;
      break;

    case __IRQ_SIMULATE_IOCTL:
      PREFIX(); PFX(irq); PRDEBUG("ioctl IRQ_SIMULATE: pid=%d\n",
				  current->pid);

      if (!has_readwrite_access(file))
	return -EACCES;

      irq_simulate(irq);
      break;

    case __IRQ_STAT_IOCTL:
      return -ENOTSUP;

    case __UAE_SET_TIMER_IOCTL:
      if ( COPY_FROM_USER(&when, (const void*)arg, sizeof(when)) )
	return -EFAULT;
      uae_nextevent=when;
      uae_alert=when-cpu_khz/HZ*2000;
      if (uae_alert>when)
	uae_alert=0;
#if 0
      printk("uae_nextevent=%u/%u, uae_alert=%u/%u\n",
	     (unsigned int)(uae_nextevent>>32),
	     (unsigned int)uae_nextevent,
	     (unsigned int)(uae_alert>>32),
	     (unsigned int)uae_alert);
#endif
      schedule_hiprec();
      return 0;

    case __UAE_GET_RANGE_IOCTL:
      {
	long index;
	int se;

	if ( COPY_FROM_USER(&index, (const void*)arg, sizeof(index)) )
	  return -EFAULT;
	if (index==-1) {
	  index=blockindex;
	  if (COPY_TO_USER((void*) arg,&index,sizeof(index)))
	    return -EFAULT;
	  return 0;
	}
	se=index&1;
	index/=2;
	if (index<0 || index>=UAE_BLOCKS_N)
	  return -EFAULT;
	if (se) 
	  index=uae_blocks[index].start;
	else
	  index=uae_blocks[index].end;
	if (COPY_TO_USER((void*) arg,&index,sizeof(index)))
	  return -EFAULT;
	return 0;
      }

    case __UAE_SET_FPF_IOCTL:
      {
	long index;
	int se;

	if ( COPY_FROM_USER(&fpf, (const void*)arg, sizeof(fpf)) )
	  return -EFAULT;
	fpf.task=current;
      }
      return 0;

    case __UAE_GET_FPF_EIP_IOCTL:
      {
	long index;
	int se;

	if ( COPY_TO_USER((void*)arg, &fpf_eip, sizeof(fpf_eip)) )
	  return -EFAULT;
      }
      return 0;

    case __UAE_READ_IOPORT_IOCTL:
      {
	ioport_data id;
	int count;
	unsigned short* data;
	int dummy;
	unsigned int now;
	unsigned char x;

	if ( COPY_FROM_USER(&id, (const void*)arg, sizeof(id)) )
	  return -EFAULT;
	count=id.count;
	data=(unsigned short*)(id.addr);

	cli();
	while (count--) {
	  x=inb(id.port);
	  rdtsc(now,dummy);
	  now&=~0x000f;
	  x&=0x08;
	  now|=x;
	  *data++=now;
	}
	sti();
      }
      return 0;

    case __UAE_PCI_OP_IOCTL:
      {
	pcidata pd;
	int answer;
	if ( COPY_FROM_USER(&pd, (const void*)arg, sizeof(pd)) )
	  return -EFAULT;
	answer=pcicommand(&pd);
	if (answer)
	  return answer;
	if ( COPY_TO_USER((void*)arg, &pd, sizeof(pd)) )
	  return -EFAULT;
      }
      return 0;

    case __UAE_SET_IRQHANDLER_IOCTL:
      {
	unsigned long x;
	if ( COPY_FROM_USER(&x, (const void*)arg, sizeof(x)) )
	  return -EFAULT;
	process_to_elevate=x;
	return 0;
      }
	
    case __UAE_STOP_IRQHANDLER_IOCTL:
      {
	do_elevate_process=0;
	xchg_compat( &NEED_RESCHED, 1 );
	return 0;
      }

    case __UAE_GET_ZEROPAGE_IOCTL:
      {
	unsigned long x=zeropage;
	if (COPY_TO_USER((void*)arg,&x,sizeof(x)))
	  return -EFAULT;
	return 0;
      }

    default:
      return -ENOSYS;
    }
  
  return 0;
}


# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,23)

// sel_type: SEL_IN || SEL_OUT || SEL_EX
// return: 0 if wait, 1 if ready
static int irq_select(Inode* UNUSED inode, File* file,
	int sel_type, select_table* wt)
{
	irq_chan* ich=file->private_data;

	if (ich==NULL) return 1;	// unrequested irq

	assert_retval(ich->signature==ICH_SIG, 1);

	if (sel_type==SEL_IN)
	{
		if ( xchg_compat( &ich->sel_count, 0 ) == 0 )
			select_wait(&ich->wqh, wt);
		else
		{
			PREFIX(); PRMSGCOUNTER(); PFX(ich->irq);
				PRDEBUG("sel_count was %d\n", ich->sel_count);

			return 1;
		}
	}
	return 0;
}

#else	// LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,23)

static uint irq_poll(File* file, poll_table* pt)
{
	irq_chan* ich=file->private_data;

	if (ich==NULL)	// unrequested irq
		return POLLERR|POLLNVAL;

	assert_retval(ich->signature==ICH_SIG, POLLERR|POLLNVAL);

	// queue current process into any wait queue that
	// may awaken in the future (read and write)
	poll_wait(file, &ich->wqh, pt);

	// we have an input event only
	return xchg_compat( &ich->sel_count, 0 ) ? POLLIN|POLLRDNORM : 0;
}

#endif	// LINUX_VERSION_CODE < KERNEL_VERSION(2,1,23)

//- file_operations routines


static int do_request(File* file, int sig, int flags)
{
	task_chan* tch;
	irq_chan* ich;
	uint irq;
	int rc;

	if (!has_readwrite_access(file))
		return -EACCES;

	if ( sig != -1 && (sig<1 || sig>_NSIG) )
		return -EINVAL;

	if ( flags & ~(IRQ_SHARED|IRQ_SKIPPING|IRQ_SKIPPING_CONSENT) )
		return -EINVAL;

	irq=irq_file(file);
	assert_retval(irq<NR_IRQS, -EBADFD);

	if ( (tch=tch_find(current)) != NULL )
	{
		if ( tch->ichs[irq] != NULL )// this task already owned IRQ
			return -EBUSY;
	}
	else
		if ( (tch=tch_new_link(current)) == NULL )
			return nomem();

	if ((ich=ich_ctor(file, sig, flags))==NULL)
	{
		tch_dtor_if_empty(tch);		// clean up
		return nomem();
	}

	tch_attach_ich(tch,ich);

	PREFIX(); PFX(irq); PRDEBUG("ich=%p ich=", ich); ICH_PRINT(ich); PRENDL();
	PREFIX(); TCHLIST_PRINT();

	rc=register_irq_handler(ich);
	if (rc!=0)
	{	// clean up
		tch_detach_ich(tch,irq);
		ich_dtor(ich);
		tch_dtor_if_empty(tch);
	}

	return rc;
}

static int register_irq_handler(irq_chan* ich)
{
	int rc;
	char buf[128];
	char* devname;
	uint irq;

	assert_retval(ich!=NULL, -EBADFD);
	assert_retval(ich->signature==ICH_SIG, -EBADFD);

	irq=ich->irq;
	assert_retval(irq<NR_IRQS, -EBADFD);

	//+ info for /proc/interrupts
	if (ich->sig!=-1)		//signal
		sprintf(buf,"%s:%d.%d", name, current->pid, ich->sig);
	else					//select() only
		sprintf(buf,"%s:%d", name, current->pid);

	devname=strdup(buf);
	if (devname==NULL)	return nomem();
	ich->devname=devname;
	//- info for /proc/interrupts

	if (ich->flags&IRQ_SKIPPING)
		skipping_on(ich);

	rc=request_irq(irq, irq_handler,
		(ich->flags&IRQ_SHARED) ? SA_SHIRQ : 0, devname, ich);
	if (rc!=0)
	{	// clean up
		skipping_sub(ich);
		return rc;
	}
	++irq_channels_registered[irq];


	PREFIX(); PRDEBUG("%s: ich=%p ich=",name,ich); ICH_PRINT(ich); PRENDL();
	PREFIX(); TCHLIST_PRINT();
	return 0;
}

// free IRQ and its structures.
// returns destroyed tch flag.
static bool free_irq_ich_tch(irq_chan* ich)
{
	task_chan* tch;
	uint irq;

	if (ich==NULL)	return false;

	assert_retval(ich->signature==ICH_SIG, false);

	tch=ich->tch;

	assert_retval(tch!=NULL, false);
	assert_retval(tch->signature==TCH_SIG, false);

	irq=ich->irq;
	assert_retval(irq<NR_IRQS, false);
	assert_retval(tch->ichs[irq]==ich, false);

	free_irq(irq,ich); //no more interrupts on this channel

	--irq_channels_registered[irq];

	skipping_sub(ich);

	tch_detach_ich(tch, irq);
	ich_dtor(ich);
	return tch_dtor_if_empty(tch);
}


static void skipping_on(register irq_chan* ich)
{
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(ich->tch!=NULL);
	assert_retvoid(ich->tch->signature==TCH_SIG);
	assert_retvoid(ich->irq<NR_IRQS);

	ich->skipping_count++;
	skipping_irq_counts[ich->irq]++;
}

static int skipping_off(register irq_chan* ich)
{
	assert_retval(ich!=NULL, -EBADFD);
	assert_retval(ich->signature==ICH_SIG, -EBADFD);
	assert_retval(ich->tch!=NULL, -EBADFD);
	assert_retval(ich->tch->signature==TCH_SIG, -EBADFD);
	assert_retval(ich->irq<NR_IRQS, -EBADFD);

	if (ich->skipping_count && skipping_irq_counts[ich->irq])
	{
		ich->skipping_count--;
		skipping_irq_counts[ich->irq]--;
		return 0;
	}
	else
		return -EPERM;
}

static void skipping_sub(register irq_chan* ich)
{
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(ich->tch!=NULL);
	assert_retvoid(ich->tch->signature==TCH_SIG);
	assert_retvoid(ich->irq<NR_IRQS);

	skipping_irq_counts[ich->irq] -= ich->skipping_count;
	ich->skipping_count=0;
}


static void irq_simulate(uint irq)
{
	task_chan* tch;
	irq_chan* ich;

#ifdef CONFIG_IRQ_EXPORT_SIMULATE
    ulong flags;
#endif

	assert_retvoid(irq<NR_IRQS);

#ifdef CONFIG_IRQ_EXPORT_SIMULATE
	spin_lock_irqsave(&tchlist_lock, flags);
#endif

	for(tch=tchlist; tch!=NULL; tch=tch->next)
	{
		assert_break(tch->signature==TCH_SIG);
		ich=tch->ichs[irq];

		if (ich!=NULL)
		{
			assert_break(ich->signature==ICH_SIG);

			if ( (ich->flags&IRQ_SKIPPING_CONSENT && skipping_irq_counts[irq])
				|| disabled_irq_counts[irq] )
				continue;

#ifndef CONFIG_IRQ_EXPORT_SIMULATE
			DISABLE_IRQ(irq);
#endif

			do_irq(ich);

#ifndef CONFIG_IRQ_EXPORT_SIMULATE
			ENABLE_IRQ(irq);
#endif
		}
	}

#ifdef CONFIG_IRQ_EXPORT_SIMULATE
	spin_unlock_irqrestore(&tchlist_lock, flags);
#endif
}


// "slow" interrupt handler
static void irq_handler(int irq, void* dev_id, struct pt_regs* UNUSED ptregs)
{
	register irq_chan* ich = (irq_chan*)dev_id;

	PREFIX(); PRDEBUG("IRQ %d interrupt handler\n", irq);

	assert_retvoid((uint)irq<NR_IRQS);
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(ich==ich->file->private_data && (uint)irq==ich->irq);

	disable_irq_nosync(irq);
	++disabled_irq_counts[irq];

	ich->irq_count++;

	if ( ich->flags&IRQ_SKIPPING_CONSENT && skipping_irq_counts[irq] )
		return;

	do_irq(ich);
}

static void do_irq(register irq_chan* ich)
{
	assert_retvoid(ich!=NULL);
	assert_retvoid(ich->signature==ICH_SIG);
	assert_retvoid(ich->tch!=NULL);
	assert_retvoid(ich->tch->signature==TCH_SIG);
	assert_retvoid(ich->irq<NR_IRQS);

	PREFIX(); PFX(ich->irq); PRDEBUG("sending "); ICH_PRINT(ich); PRENDL();

	ich->sel_count++;
	wake_up_interruptible(&ich->wqh);

	if (ich->sig != -1)
		if (send_sig(ich->sig, ich->tch->owner_task, 1)==0)
		{
			ich->sig_count++;
			PREFIX(); PFX(ich->irq); PRDEBUG("sent ");
			ICH_PRINT(ich); PRENDL();
			do_elevate_process=1;  /* Get a fast response */
		}
	xchg_compat( &NEED_RESCHED, 1 );
}



/* UAE hackery */
void uae_trigger(void)
{
  irq_simulate(31);
  uae_alert=~0;
  uae_nextevent=~0;
}

/*
$Log: irq.c,v $
Revision 1.67  2000/07/31 10:59:07  fedorov
xchg_compat() used, xchg() bug fixed

Revision 1.66  2000/05/24 10:39:28  fedorov
english corrected

Revision 1.65  2000/05/18 04:16:14  fedorov
---

Revision 1.64  2000/05/10 14:05:02  fedorov
using of sys_sched_yield() was buggy, atomic xchg(&NEED_RESCHED, 1) used;
<linux/compat/errno.h> used

Revision 1.63  1999/12/19 09:42:58  fedorov
sys_sched_yield() used to pre-empt current task in interrupt delivery.

Revision 1.62  1999/12/17 15:23:55  fedorov
NEED_RESCHED compatibility macro used.

Revision 1.61  1999/11/27 09:27:42  fedorov
ENOTSUP compatibility

Revision 1.60  1999/10/27 07:07:21  fedorov
do_irq(): set need_resched if preemtion needed only.

Revision 1.59  1999/10/07 08:09:19  fedorov
sorted; assertions added; unsigned int for IRQ number;
de-bugged on 2.2.12 kernel.

Revision 1.58  1999/10/05 07:29:45  fedorov
2.0 - 2.2 compatibility completed, compilation test passed.

Revision 1.57  1999/10/02 13:45:00  fedorov
select() bug fixed: handle ich->sel_count by xchg() atomically;
poll() added - no compilation test available.

Revision 1.56  1999/10/02 12:31:00  fedorov
minor lookup macros from <linux/compat/fs.h> used.

Revision 1.55  1999/10/02 10:58:56  fedorov
<linux/compat/fs.h> (FILE_OPERATIONS_* macros) used -
irq_seek(), irq_close(), fops affected.

Revision 1.54  1999/09/30 13:23:00  fedorov
irq_simulate() can be called from interrupt/bottom-half handlers.

Revision 1.53  1999/09/29 10:54:16  fedorov
ich_{att,det}ach_tch(), tch_{att,det}ach_ich() added;
file->private_data handling moved to ich ctor/dtor.

Revision 1.52  1999/09/29 09:32:16  fedorov
static init of static objects;
disabled irqs handled by irq_simulate;
skip_counts -> skipping_irq_counts; skip_count -> skipping_count.

Revision 1.51  1999/09/29 06:58:38  fedorov
IRQ_ONESHOT flag removed to simplify driver - locks out;
sorted;
do_free() removed - fre_irq_ich_tch() used;

Revision 1.50  1999/09/26 14:43:22  fedorov
tchlist locking started

Revision 1.49  1999/09/26 11:11:07  fedorov
sorted

Revision 1.48  1999/09/26 10:23:08  fedorov
*** empty log message ***

Revision 1.47  1999/09/23 14:39:15  fedorov
irq_channels_registered become atomic_t;
irq_wait_queues become static.

Revision 1.46  1999/09/22 14:43:51  fedorov
compatibility wait_queue used

Revision 1.45  1999/09/22 14:34:06  fedorov
compatibility COPY_FROM_USER() used

Revision 1.44  1999/09/22 14:23:36  fedorov
__init attributes applied

Revision 1.43  1999/09/22 14:04:53  fedorov
compatibility headers included;
old EXPORT_SIMULATE symbol bug fixed;
VERBOSE_DEBUG -> DEBUG;
debug print functions uppercased.

Revision 1.42  1999/09/22 13:32:24  fedorov
EBADFD errno value used on assertion failure.

Revision 1.41  1999/09/22 11:42:08  fedorov
internal names changed; binary compatibility saved.

Revision 1.40  1999/09/22 10:29:29  fedorov
many of utilities moved to <linux/df/ *.h> headers;
my kernel assertions used instead of *CHECK* macros.

Revision 1.39  1999/08/15 14:16:28  fedorov
tab replaced by spaces in string

Revision 1.38  1999/08/11 11:30:13  fedorov
English version of man draft created. English comments.
Header and history log formats changed.
Extra protection of multiple headers inclusion removed.
Some cosmetic changes.

Revision 1.37  1999/07/07 14:34:24	fedorov
"empty log messages" removed

Revision 1.36  1999/03/06 14:32:41	fedorov
process ownership checking added

Revision 1.35  1999/03/06 10:09:37	fedorov
some function names changed; REGPARM1 macro created & used

Revision 1.34  1999/03/06 08:25:11	fedorov
printkEOL() -> pr_endl()

Revision 1.33  1999/03/06 07:20:36	fedorov
my printk() quirks removed

Revision 1.32  1999/02/04 13:40:11	fedorov
indentation

Revision 1.31  1999/02/04 09:06:32	fedorov
bool redefined

Revision 1.30  1999/02/02 06:41:32	fedorov
C extern inline

Revision 1.27  1998/04/25 12:07:13	fedorov
indentation; irq_read(),irq_write() removed

Revision 1.26  1998/04/20 12:23:59	fedorov
never mind

Revision 1.25  1998/04/18 08:35:47	fedorov
never mind

Revision 1.24  1998/04/10 11:21:09	fedorov
indentation

Revision 1.23  1998/04/03 10:52:23	fedorov
indentation

Revision 1.22  1998/03/29 17:20:30	fedorov
irqnum renamed to irq anywhere;
task_struct pointer is not const somewhere now
	to avoid type casting to non-const pointer;
irq_simulate() defer skipping mode;

Revision 1.21  1998/03/25 22:43:37	fedorov
modversions & genksym used

Revision 1.20  1998/03/18 15:47:21	fedorov
irq_simulate() created and added to own part of kernel symbol_table.
need_resched=1 added to end of bh_handler() for irq_simulate()
from syscall.
int irqnum added to struct irq_channel for access speed-up.
irqnum_minor(int minor) created for minor->irqnum translation
	independly from real mapping; minor superseeded by irqnum elsewere.
pfx() and PFX() accept irqnum now, was minor.
more [un]register_chrdev() error codes printed on init/cleanup module.
extra have_readwrite_access() calls removed.
obsolete for(;;) loop eliminated from tch_is_empty().
print_overrun() added for save space from irq_handler() & bh_handler().
save some space by using repeated words strings.
pfx() comment-out conditionally to save space.
unused tch_find_new_link() comment-out for save space.

Revision 1.19  1998/03/10 08:51:15	fedorov
something

Revision 1.18  1998/02/01 15:24:59	fedorov
cpp error message changed

Revision 1.17  1998/01/12 13:53:41	fedorov
tch_new_link() added

Revision 1.15  1998/01/11 14:58:24	fedorov
send_sig() fail processing corrected

Revision 1.14  1998/01/11 13:55:47	fedorov
skipping_off check underflow

Revision 1.13  1998/01/10 16:13:32	fedorov
nested skipping_{on|off} allowed

Revision 1.12  1998/01/10 14:26:00	fedorov
skipping mode added, flags changed, shared mode deferred by
enable/disable now

Revision 1.11  1997/12/23 15:48:55	fedorov
access mode must be O_RDWR now

Revision 1.10  1997/12/17 14:38:12	fedorov
unused variables removed

Revision 1.9  1997/12/17 11:48:39  fedorov
copyright minor change

Revision 1.8  1997/12/15 13:19:26  fedorov
debugging reimplemented, task_chan has array of pointers to irq_chan now,
not a list, select() added.

Revision 1.7  1997/12/09 14:15:52  fedorov
test passed!

Revision 1.6  1997/12/09 11:33:20  fedorov
turn irq on

Revision 1.5  1997/12/09 09:42:32  fedorov
ich_link() bug fixed. debugging reimplemented

Revision 1.4  1997/12/07 13:38:22  fedorov
never mind

Revision 1.3  1997/12/07 11:52:12  fedorov
never mind

Revision 1.2  1997/12/07 11:41:30  fedorov
signature checks implemented

Revision 1.1  1997/12/06 23:10:19  fedorov
Put under CVS control
*/

