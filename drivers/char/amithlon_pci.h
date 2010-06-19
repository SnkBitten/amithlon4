#ifndef AMITHLON_PCI_H
#define AMITHLON_PCI_H

#include "linux/pci.h"

#define CMD_FIND_SLOT   1
#define CMD_FIND_SUBSYS 2
#define CMD_FIND_DEVICE 3
#define CMD_FIND_CLASS  4
#define CMD_FIND_CAPAB  5
#define CMD_SET_POWER   6
#define CMD_ENABLE      7
#define CMD_DISABLE     8
#define CMD_RELEASE     9
#define CMD_REQUEST    10

#define CMD_READBYTE   20
#define CMD_READWORD   21
#define CMD_READLONG   22
#define CMD_WRITEBYTE  23
#define CMD_WRITEWORD  24
#define CMD_WRITELONG  25

#define CMD_GETBASE    30
#define CMD_GETINFO    31
#define CMD_GETNAME    32

typedef struct {
  /* operation choice */
  int command;

  /* inputs */
  unsigned char busnum;
  unsigned char devnum;
  unsigned char funnum;

  unsigned int  vendor;
  unsigned int  device;
  unsigned int  subsys_vendor;
  unsigned int  subsys_device;
  unsigned int  class;
  unsigned long starthandle;
  int           capability;
  int           powerstate;
  int           basenum;
  int           offset;
  char*         res_name;
  unsigned char* releasecode;

  /* Outputs */
  int           cappos;
  unsigned long handle;
  int           oldpowerstate;
  int           result;
  unsigned long confdata;
  unsigned long start;
  unsigned long end;
  unsigned long flags;
  unsigned long irq;
} pcidata;

#endif
