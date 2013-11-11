//****************************************************************************
// Copyright (C) 2001-2006  PEAK System-Technik GmbH
//
// linux@peak-system.com 
// www.peak-system.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Maintainer(s): Klaus Hitschler (klaus.hitschler@gmx.de)
//****************************************************************************

//****************************************************************************
//
// receivetest.c - a small program to test the receive features of pcan driver 
//                 and the supporting shared library
//
// $Id: receivetest.c 413 2006-09-06 09:22:21Z edouard $
//
//****************************************************************************

//----------------------------------------------------------------------------
// set here current release for this program
#define CURRENT_RELEASE "Release_20060501_a"  

//****************************************************************************
// INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   // exit
#include <signal.h>
#include <string.h>
#include <stdlib.h>   // strtoul
#include <fcntl.h>    // O_RDWR
#include <libpcan.h>
#include <src/common.h>

#ifdef XENOMAI
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>
#endif

//****************************************************************************
// DEFINES
#ifdef XENOMAI
#define STATE_FILE_OPENED         1
#define STATE_TASK_CREATED        2
#endif
#define DEFAULT_NODE "/dev/pcan0"
#ifndef bool
#define bool  int
#define true  1
#define false 0
#endif

//****************************************************************************
// GLOBALS
#ifdef XENOMAI
unsigned int current_state = 0;
int shutdownnow = 0;
int nExtended = CAN_INIT_TYPE_ST;
RT_TASK reading_task;
RTIME my_task_period_ns =   5000llu;
#endif
HANDLE h = NULL;
char *current_release;

//****************************************************************************
// LOCALS

//****************************************************************************
// CODE 
static void hlpMsg(void)
{
  printf("receivetest - a small test program which receives and prints CAN messages.\n");
  printf("usage:   receivetest {[-f=devicenode] | {[-t=type] [-p=port [-i=irq]]}} [-b=BTR0BTR1] [-e] [-?]\n");
  printf("options: -f - devicenode - path to devicefile, default=%s\n", DEFAULT_NODE);
  printf("         -t - type of interface, e.g. 'pci', 'sp', 'epp', 'isa', 'pccard' or 'usb' (default: pci).\n");
  printf("         -p - port in hex notation if applicable, e.g. 0x378 (default: 1st port of type).\n");
  printf("         -i - irq in dec notation if applicable, e.g. 7 (default: irq of 1st port).\n");
  printf("         -b - BTR0BTR1 code in hex, e.g. 0x001C (default: 500 kbit).\n");
  printf("         -e - accept extended frames. (default: standard frames)\n");
  printf("         -? or --help - this help\n");
  printf("\n");
}

#ifdef XENOMAI

//----------------------------------------------------------------------------
// cleaning up
void cleanup_all(void)
{
  if (current_state & STATE_FILE_OPENED)
  {
    print_diag("receivetest");
    CAN_Close(h);
    current_state &= ~STATE_FILE_OPENED;
  }
  if (current_state & STATE_TASK_CREATED)
  {
    rt_task_delete(&reading_task);
    current_state &= ~STATE_TASK_CREATED;
  }
}

void catch_signal(int sig)
{
  signal(SIGTERM, catch_signal);
  signal(SIGINT, catch_signal);
  shutdownnow = 1;
  cleanup_all();
  printf("receivetest: finished(0)\n");
}

//----------------------------------------------------------------------------
// real time task
void reading_task_proc(void *arg)
{
  TPCANMsg m;
  __u32 status;

  while (1)
  {
    if ((errno = CAN_Read(h, &m)))
      shutdownnow = 1;
    else
    {
      // check if a CAN status is pending
      if (m.MSGTYPE & MSGTYPE_STATUS)
      {
        status = CAN_Status(h);
        if ((int)status < 0)
          shutdownnow = 1;
      }
    }
    if (shutdownnow == 1) break;
  }
}

//----------------------------------------------------------------------------
// all is done here
int main(int argc, char *argv[])
{
  char *ptr;
  int i;
  int nType = HW_PCI;
  __u32 dwPort = 0;
  __u16 wIrq = 0;
  __u16 wBTR0BTR1 = 0;
  char  *szDevNode = DEFAULT_NODE;
  bool bDevNodeGiven = false;
  bool bTypeGiven = false;
  char txt[VERSIONSTRING_LEN];

  mlockall(MCL_CURRENT | MCL_FUTURE);

  errno = 0;

  current_release = CURRENT_RELEASE;
  disclaimer("receivetest");

  // decode command line arguments
  for (i = 1; i < argc; i++)
  {
    char c;

    ptr = argv[i];

    if (*ptr == '-')
    {
      while (*ptr == '-')
        ptr++;

      c = *ptr;
      ptr++;

      if (*ptr == '=')
      ptr++;

      switch(tolower(c))
      {
        case 'f':
          szDevNode = ptr;
          bDevNodeGiven = true;
          break;
        case 't':
          nType = getTypeOfInterface(ptr);
          if (!nType)
          {
            errno = EINVAL;
            printf("receivetest: unknown type of interface!\n");
            goto error;
          }
          bTypeGiven = true;
          break;
        case 'p':
          dwPort = strtoul(ptr, NULL, 16);
          break;
        case 'i':
          wIrq   = (__u16)strtoul(ptr, NULL, 10);
         break;
        case 'e':
          nExtended = CAN_INIT_TYPE_EX;
          break;
        case '?':
        case 'h':
          hlpMsg();
          errno = 0;
          goto error;
          break;
        case 'b':
          wBTR0BTR1 = (__u16)strtoul(ptr, NULL, 16);
          break;
        default:
          errno = EINVAL;
          perror("receivetest: unknown command line argument!\n");
          goto error;
          break;
        }
     }
  }

  if (bDevNodeGiven && bTypeGiven)
  {
    errno = EINVAL;
    perror("receivetest: device node and type together is useless");
    goto error;
  }

  // tell some information to user
  if (!bTypeGiven)
  {
    printf("receivetest: device node=\"%s\"\n", szDevNode);
  }
  else
  {
    printf("receivetest: type=%s", getNameOfInterface(nType));
    if (nType == HW_USB)
    {
      printf(", Serial Number=default, Device Number=%d\n", dwPort); 
    }
    else
    {
      if (dwPort)
      {
        if (nType == HW_PCI)
          printf(", %d. PCI device", dwPort);
        else
          printf(", port=0x%08x", dwPort);
      }
      else
        printf(", port=default");
      if ((wIrq) && !(nType == HW_PCI))
        printf(" irq=0x%04x\n", wIrq);
      else
        printf(", irq=default\n");
    }
  }

  if (nExtended == CAN_INIT_TYPE_EX)
    printf("             Extended frames are sent");
  else
    printf("             Only standard frames are sent");
  if (wBTR0BTR1)
    printf(", init with BTR0BTR1=0x%04x\n", wBTR0BTR1);
  else
    printf(", init with 500 kbit/sec.\n");

  /* install signal handler for manual break */
  signal(SIGTERM, catch_signal);
  signal(SIGINT, catch_signal);

  /* start timer */
  errno = rt_timer_set_mode(TM_ONESHOT);
  if (errno)
  {
    printf("receivetest: error while configuring timer\n");
    goto error;
  }

  /* open the CAN port */
  if ((bDevNodeGiven) || (!bDevNodeGiven && !bTypeGiven))
  {
    h = LINUX_CAN_Open(szDevNode, O_RDWR);
    if (h)
      current_state |= STATE_FILE_OPENED;
    else
    {
      printf("receivetest: can't open %s\n", szDevNode);
      goto error;
    }
  }
  else
  {
    // please use what is appropriate  
    // HW_DONGLE_SJA 
    // HW_DONGLE_SJA_EPP 
    // HW_ISA_SJA 
    // HW_PCI 
    h = CAN_Open(nType, dwPort, wIrq);
    if (h)
      current_state |= STATE_FILE_OPENED;
    else
    {
      printf("receivetest: can't open %s device.\n", getNameOfInterface(nType));
      goto error;
    }
  }

  /* clear status */
  CAN_Status(h);

  /* get version info */
  errno = CAN_VersionInfo(h, txt);
  if (!errno)
    printf("receivetest: driver version = %s\n", txt);
  else
  {
    printf("receivetest: Error while getting version info\n");
    goto error;
  }

  /* init to a user defined bit rate */
  if (wBTR0BTR1)
  {
    errno = CAN_Init(h, wBTR0BTR1, nExtended);
    if (errno)
    {
      printf("receivetest: Error while initializing CAN device\n");
      goto error;
    }
  }

  //create reading_task
  errno = rt_task_create(&reading_task,"receivetest",0,50,0);
  if (errno) {
    printf("receivetest: Failed to create rt task, code %d\n",errno);
    goto error;
  }
  current_state |= STATE_TASK_CREATED;

  printf("receivetest: reading data from CAN ... (press Ctrl-C to exit)\n");

  // start reading_task
  errno = rt_task_start(&reading_task,&reading_task_proc,NULL);
  if (errno) {
    printf("receivetest: Failed to start rt task, code %d\n",errno);
    goto error;
  }

  pause();
  return 0;

error:
  cleanup_all();
  return errno;
}

#else

// centralized entry point for all exits
static void my_private_exit(int error)
{
  if (h)
  {
    print_diag("receivetest");
    CAN_Close(h); 
  }
  printf("receivetest: finished (%d).\n\n", error);
  exit(error);
}

// handles CTRL-C user interrupt
static void signal_handler(int signal)
{
  my_private_exit(0);
}

// here all is done
int main(int argc, char *argv[])
{
  char *ptr;
  int i;
  int nType = HW_PCI;
  __u32 dwPort = 0;
  __u16 wIrq = 0;
  __u16 wBTR0BTR1 = 0;
  int   nExtended = CAN_INIT_TYPE_ST;
	char  *szDevNode = DEFAULT_NODE;
	bool bDevNodeGiven = false;
	bool bTypeGiven    = false;
  
  errno = 0;
  
  current_release = CURRENT_RELEASE;
  disclaimer("receivetest");

  // decode command line arguments
  for (i = 1; i < argc; i++)
  {
    char c;
    
    ptr = argv[i];
    
    while (*ptr == '-')
      ptr++;
      
    c = *ptr;
    ptr++;
    
    if (*ptr == '=')
      ptr++;
      
    switch(tolower(c))
    {
			case 'f':
				szDevNode = ptr;
				bDevNodeGiven = true;
				break;
      case 't':
        nType = getTypeOfInterface(ptr);
	      if (!nType)
	      {
	        errno = EINVAL;
		      printf("receivetest: unknown type of interface!\n");
			    my_private_exit(errno);
				}
				bTypeGiven = true;
        break;
	    case 'p':
	      dwPort = strtoul(ptr, NULL, 16);
	      break;
	    case 'i':
	      wIrq   = (__u16)strtoul(ptr, NULL, 10);
	      break;
	    case 'e':
	      nExtended = CAN_INIT_TYPE_EX;
	      break;
      case '?': 
      case 'h':
        hlpMsg();
	      my_private_exit(0);
	      break;
	    case 'b':
	      wBTR0BTR1 = (__u16)strtoul(ptr, NULL, 16);
	      break;
	    default:
	      errno = EINVAL;
	      perror("receivetest: unknown command line argument!\n");
	      my_private_exit(errno);
	      break;
	   }
  }
  
	// simple command input check
	if (bDevNodeGiven && bTypeGiven)
	{
    errno = EINVAL;
    perror("receivetest: device node and type together is useless");
    my_private_exit(errno);
	}
	
  // give some information back
	if (!bTypeGiven)
	{
		printf("receivetest: device node=\"%s\"\n", szDevNode);
	}
	else
	{
		printf("receivetest: type=%s", getNameOfInterface(nType));
		if (nType == HW_USB)
		{
			if (dwPort)
				printf(", %d. device\n", dwPort);
			else
				printf(", standard device\n"); 
		}
		else
		{
			if (dwPort)
			{
				if (nType == HW_PCI)
					printf(", %d. PCI device", dwPort);
				else
					printf(", port=0x%08x", dwPort);
			}
			else
				printf(", port=default");
			if ((wIrq) && !(nType == HW_PCI))
				printf(" irq=0x%04x\n", wIrq);
			else
				printf(", irq=default\n");
		}
	}
    
  if (nExtended == CAN_INIT_TYPE_EX)
    printf("             Extended frames are accepted");
  else
    printf("             Only standard frames are accepted");
  if (wBTR0BTR1)
    printf(", init with BTR0BTR1=0x%04x\n", wBTR0BTR1);
  else
    printf(", init with 500 kbit/sec.\n");
    
  
  // install signal handler for manual break
  signal(SIGINT, signal_handler);
 
  // open the CAN port
	if ((bDevNodeGiven) || (!bDevNodeGiven && !bTypeGiven))
	{
    h = LINUX_CAN_Open(szDevNode, O_RDWR);  
	}
	else
	{
		// please use what is appropriate  
		// HW_DONGLE_SJA 
		// HW_DONGLE_SJA_EPP 
		// HW_ISA_SJA 
		// HW_PCI 
		// HW_USB
		h = CAN_Open(nType, dwPort, wIrq);
	}
  
  if (h)
  {
    char txt[VERSIONSTRING_LEN];
    
    // get version info
    errno = CAN_VersionInfo(h, txt);
    if (!errno)
      printf("receivetest: driver version = %s\n", txt);
    else
    {
      perror("receivetest: CAN_VersionInfo()");
      my_private_exit(errno);
    }
      
    // init to a user defined bit rate
    if (wBTR0BTR1)
    {
      errno = CAN_Init(h, wBTR0BTR1, nExtended);
      if (errno)
      {
        perror("receivetest: CAN_Init()");
	      my_private_exit(errno);
	    }
	  }
      
    // read in endless loop until Ctrl-C
    while (1)
    {
      TPCANMsg m;
      __u32 status;
      
      if ((errno = CAN_Read(h, &m)))
      {
        perror("receivetest: CAN_Read()");
	      my_private_exit(errno);
	    }
	    else
	    {
	      print_message(&m); 

	      // check if a CAN status is pending	     
	      if (m.MSGTYPE & MSGTYPE_STATUS)
	      {
	        status = CAN_Status(h);
		      if ((int)status < 0)
		      {
		        errno = nGetLastError();
		        perror("receivetest: CAN_Status()");
			      my_private_exit(errno);
			    }
			    else
			      printf("receivetest: pending CAN status 0x%04x read.\n", (__u16)status);
	      } 
	    }
    }
  }
  else
  {
    errno = nGetLastError();
    perror("receivetest: CAN_Open() or LINUX_CAN_Open()");
    my_private_exit(errno);
  }
    
  return errno;
}
#endif
