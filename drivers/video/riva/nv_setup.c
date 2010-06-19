/* $XConsortium: nv_driver.c /main/3 1996/10/28 05:13:37 kaleb $ */
/*
 * Copyright 1996-1997  David J. McKay
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID J. MCKAY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * GPL licensing note -- nVidia is allowing a liberal interpretation of
 * the documentation restriction above, to merely say that this nVidia's
 * copyright and disclaimer should be included with all code derived
 * from this source.  -- Jeff Garzik <jgarzik@mandrakesoft.com>, 01/Nov/99 
 */

/* Hacked together from mga driver and 3.3.4 NVIDIA driver by Jarno Paananen
   <jpaana@s2.org> */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_setup.c,v 1.18 2002/08/0
5 20:47:06 mvojkovi Exp $ */

#include <linux/delay.h>
#include <linux/pci_ids.h>
#include "nv_type.h"
#include "rivafb.h"
#include "nvreg.h"


#ifndef CONFIG_PCI		/* sanity check */
#error This driver requires PCI support.
#endif

#define PFX "rivafb: "

static inline unsigned char MISCin(struct rivafb_info *rinfo)
{
	return (VGA_RD08(rinfo->riva.PVIO, 0x3cc));
}

static Bool 
riva_is_connected(struct rivafb_info *rinfo, Bool second)
{
	volatile U032 *PRAMDAC = rinfo->riva.PRAMDAC0;
	U032 reg52C, reg608;
	Bool present;

	if(second) PRAMDAC += 0x800;

	reg52C = PRAMDAC[0x052C/4];
	reg608 = PRAMDAC[0x0608/4];

	PRAMDAC[0x0608/4] = reg608 & ~0x00010000;

	PRAMDAC[0x052C/4] = reg52C & 0x0000FEEE;
	mdelay(1); 
	PRAMDAC[0x052C/4] |= 1;

	rinfo->riva.PRAMDAC0[0x0610/4] = 0x94050140;
	rinfo->riva.PRAMDAC0[0x0608/4] |= 0x00001000;

	mdelay(1);

	present = (PRAMDAC[0x0608/4] & (1 << 28)) ? TRUE : FALSE;

	rinfo->riva.PRAMDAC0[0x0608/4] &= 0x0000EFFF;

	PRAMDAC[0x052C/4] = reg52C;
	PRAMDAC[0x0608/4] = reg608;

	return present;
}

static void
riva_override_CRTC(struct rivafb_info *rinfo)
{
	printk(KERN_INFO PFX
		"Detected CRTC controller %i being used\n",
		rinfo->SecondCRTC ? 1 : 0);

	if(rinfo->forceCRTC != -1) {
		printk(KERN_INFO PFX
			"Forcing usage of CRTC %i\n", rinfo->forceCRTC);
		rinfo->SecondCRTC = rinfo->forceCRTC;
	}
}

static void
riva_is_second(struct rivafb_info *rinfo)
{
	if(rinfo->FlatPanel == 1) {
		switch(rinfo->Chipset) {
		/*case NV_CHIP_GEFORCE4_440_GO:
		case NV_CHIP_GEFORCE4_440_GO_M64:
		case NV_CHIP_GEFORCE4_420_GO:
		case NV_CHIP_GEFORCE4_420_GO_M32:
		case NV_CHIP_QUADRO4_500_GOGL:
			rinfo->SecondCRTC = TRUE;
			break;*/
		default:
			rinfo->SecondCRTC = FALSE;
			break;
		}
	} else {
		if(riva_is_connected(rinfo, 0)) {
			if(rinfo->riva.PRAMDAC0[0x0000052C/4] & 0x100)
				rinfo->SecondCRTC = TRUE;
			else
				rinfo->SecondCRTC = FALSE;
		} else
		if (riva_is_connected(rinfo, 1)) {
			if(rinfo->riva.PRAMDAC0[0x0000252C/4] & 0x100)
				rinfo->SecondCRTC = TRUE;
			else
				rinfo->SecondCRTC = FALSE;
		} else /* default */
			rinfo->SecondCRTC = FALSE;
	}

	riva_override_CRTC(rinfo);
}

void
riva_common_setup(struct rivafb_info *rinfo)
{
	rinfo->riva.EnableIRQ = 0;
	rinfo->riva.PRAMDAC0 = (unsigned *)(rinfo->ctrl_base + 0x00680000);
	rinfo->riva.PFB = (unsigned *)(rinfo->ctrl_base + 0x00100000);
	rinfo->riva.PFIFO = (unsigned *)(rinfo->ctrl_base + 0x00002000);
	rinfo->riva.PGRAPH = (unsigned *)(rinfo->ctrl_base + 0x00400000);
	rinfo->riva.PEXTDEV = (unsigned *)(rinfo->ctrl_base + 0x00101000);
	rinfo->riva.PTIMER = (unsigned *)(rinfo->ctrl_base + 0x00009000);
	rinfo->riva.PMC = (unsigned *)(rinfo->ctrl_base + 0x00000000);
	rinfo->riva.FIFO = (unsigned *)(rinfo->ctrl_base + 0x00800000);
	rinfo->riva.PCIO0 = (U008 *)(rinfo->ctrl_base + 0x00601000);
	rinfo->riva.PDIO0 = (U008 *)(rinfo->ctrl_base + 0x00681000);
	rinfo->riva.PVIO = (U008 *)(rinfo->ctrl_base + 0x000C0000);

	rinfo->riva.IO = (MISCin(rinfo) & 0x01) ? 0x3D0 : 0x3B0;

	if(rinfo->FlatPanel == -1) {
		switch(rinfo->Chipset) {
		/*case NV_CHIP_GEFORCE4_440_GO:
		case NV_CHIP_GEFORCE4_440_GO_M64:
		case NV_CHIP_GEFORCE4_420_GO:
		case NV_CHIP_GEFORCE4_420_GO_M32:
		case NV_CHIP_QUADRO4_500_GOGL:
		case NV_CHIP_GEFORCE2_GO:
			printk(KERN_INFO PFX
				"On a laptop.  Assuming Digital Flat Panel\n");
			rinfo->FlatPanel = 1;
			break; */
		default:
			break;
		}
	}
	
	switch (rinfo->Chipset & 0x0ff0) {
	case 0x0110:
		if (rinfo->Chipset == NV_CHIP_GEFORCE2_GO)
			rinfo->SecondCRTC = TRUE; 
#if defined(__powerpc__)
		if (rinfo->FlatPanel == 1)
			rinfo->SecondCRTC = TRUE;
#endif
		riva_override_CRTC(rinfo);
		break;
	case 0x0170:
	case 0x0180:
	case 0x01F0:
	case 0x0250:
	case 0x0280:
		riva_is_second(rinfo);
		break;
	default:
		break;
	}

	if (rinfo->SecondCRTC) {
		rinfo->riva.PCIO = rinfo->riva.PCIO0 + 0x2000;
		rinfo->riva.PCRTC = rinfo->riva.PCRTC0 + 0x800;
		rinfo->riva.PRAMDAC = rinfo->riva.PRAMDAC0 + 0x800;
		rinfo->riva.PDIO = rinfo->riva.PDIO0 + 0x2000;
	} else {
		rinfo->riva.PCIO = rinfo->riva.PCIO0;
		rinfo->riva.PCRTC = rinfo->riva.PCRTC0;
		rinfo->riva.PRAMDAC = rinfo->riva.PRAMDAC0;
		rinfo->riva.PDIO = rinfo->riva.PDIO0;
	}

	RivaGetConfig(&rinfo->riva, rinfo->Chipset);

	if (rinfo->FlatPanel == -1) {
		/* Fix me, need x86 DDC code */
		rinfo->FlatPanel = 0;
	}
	rinfo->riva.flatPanel = (rinfo->FlatPanel > 0) ? TRUE : FALSE;
}

