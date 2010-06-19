/* 
 * mtuae - a sort of "initial ramdisk" that loads with the kernel image
 * $Id: mtuae.c,v 0.01 2001/06/01
 * Author: Bernd Meyer
 *
 * based heavily on mtdram.c, by
 * Alexander Larsson <alex@cendio.se> 
 *
 * Copyright (c) 1999 Alexander Larsson <alex@cendio.se>
 *               2001 Bernd Meyer <bmeyer@csse.monash.edu.au>
 * This code is GPL
 *
 */

#include <linux/config.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/mtd/compatmac.h>
#include <linux/mtd/mtd.h>

#define MTDUAE_TOTAL_SIZE=(1024*1024)
#define MTDUAE_ERASE_SIZE=(1024)
static char data[MTDUAE_TOTAL_SIZE]=
{0,1,2,3,4,5,6,7,255,254,253,252,251,250,249,248};

// We could store these in the mtd structure, but we only support 1 device..
static struct mtd_info *mtd_info;


static int
uae_erase(struct mtd_info *mtd, struct erase_info *instr)
{
  if (instr->addr + instr->len > mtd->size)
    return -EINVAL;
	
  memset((char *)mtd->priv + instr->addr, 0xff, instr->len);
  
  instr->state = MTD_ERASE_DONE;

  if (instr->callback)
    (*(instr->callback))(instr);
  return 0;
}

static int uae_point (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char **mtdbuf)
{
  if (from + len > mtd->size)
    return -EINVAL;
  
  *mtdbuf = mtd->priv + from;
  *retlen = len;
  return 0;
}

static void uae_unpoint (struct mtd_info *mtd, u_char *addr)
{
}

static int uae_read(struct mtd_info *mtd, loff_t from, size_t len,
	     size_t *retlen, u_char *buf)
{
  if (from + len > mtd->size)
    return -EINVAL;

  memcpy(buf, mtd->priv + from, len);

  *retlen=len;
  return 0;
}

static int uae_write(struct mtd_info *mtd, loff_t to, size_t len,
	      size_t *retlen, const u_char *buf)
{
  if (to + len > mtd->size)
    return -EINVAL;
  
  memcpy ((char *)mtd->priv + to, buf, len);

  *retlen=len;
  return 0;
}

#if LINUX_VERSION_CODE < 0x20300
#ifdef MODULE
#define init_mtdram init_module
#define cleanup_mtdram cleanup_module
#endif
#endif

//static void __exit cleanup_mtdram(void)
mod_exit_t cleanup_mtdram(void)
{
  if (mtd_info) {
    del_mtd_device(mtd_info);
    kfree(mtd_info);
  }
}

extern struct module __this_module;

mod_init_t init_mtdram(void)
{
   // Allocate some memory
   mtd_info = (struct mtd_info *)kmalloc(sizeof(struct mtd_info), GFP_KERNEL);
   if (mtd_info == 0)
      return 0;
   
   memset(mtd_info, 0, sizeof(*mtd_info));

   // Setup the MTD structure
   mtd_info->name = "mtdram test device";
   mtd_info->type = MTD_RAM;
   mtd_info->flags = MTD_CAP_RAM;
   mtd_info->size = MTDUAE_TOTAL_SIZE;
   mtd_info->erasesize = MTDUAE_ERASE_SIZE;
   mtd_info->priv = data;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
   mtd_info->module = THIS_MODULE;			
#endif

   if (!mtd_info->priv) {
     kfree(mtd_info);
     mtd_info = NULL;
     return -ENOMEM;
   }
   mtd_info->erase = uae_erase;
   mtd_info->point = uae_point;
   mtd_info->unpoint = uae_unpoint;
   mtd_info->read = uae_read;
   mtd_info->write = uae_write;

   if (add_mtd_device(mtd_info)) {
     vfree(mtd_info->priv);
     kfree(mtd_info);
     mtd_info = NULL;
     return -EIO;
   }
   
   return 0;
}

#if LINUX_VERSION_CODE > 0x20300
module_init(init_mtdram);
module_exit(cleanup_mtdram);
#endif
