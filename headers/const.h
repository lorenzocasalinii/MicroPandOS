#ifndef PANDOS_CONST_H_INCLUDED
#define PANDOS_CONST_H_INCLUDED

/****************************************************************************
 *
 * This header file contains utility constants & macro definitions.
 *
 ****************************************************************************/

#include <umps/const.h>


#define SEMDEVLEN 49   // Number of semaphores for device management 
#define RECVD     5    // Received message identifier

/* Hardware & software constants */
#define PAGESIZE  4096  // Page size in bytes
#define WORDLEN   4     // Word size in bytes

/* Timer, time scale, TOD-LO, and other bus registers */
#define RAMBASEADDR   0x10000000  // Base address of RAM
#define RAMBASESIZE   0x10000004  // Size of RAM
#define TODLOADDR     0x1000001C  // Time-of-day clock register
#define INTERVALTMR   0x10000020  // Interval timer register
#define TIMESCALEADDR 0x10000024  // Time scale register

/* Memory-related constants */
#define KSEG0        0x00000000  // Kernel segment 0
#define KSEG1        0x20000000  // Kernel segment 1 (uncached)
#define KSEG2        0x40000000  // Kernel segment 2
#define KUSEG        0x80000000  // User segment
#define RAMSTART     0x20000000  // RAM starting address
#define BIOSDATAPAGE 0x0FFFF000  // BIOS data page location
#define PASSUPVECTOR 0x0FFFF900  // Pass-up vector location

/* Exception-related constants */
#define PGFAULTEXCEPT 0  // Page fault exception
#define GENERALEXCEPT 1  // General exception

/* Process-related constants */
#define MAXPROC       50   // Maximum number of processes
#define MAXMESSAGES   50   // Maximum number of messages
#define ANYMESSAGE    0    // Identifier for any message
#define MSGNOGOOD    -1    // Invalid message error
#define DEST_NOT_EXIST -2  // Destination process does not exist
#define SENDMESSAGE  -1    // SYSCALL send message
#define RECEIVEMESSAGE -2  // SYSCALL receive message

#define SENDMSG 1          // USYSCALL send message
#define RECEIVEMSG 2       // USYSCALL receive message

#define PARENT 0           // Parent process

/* System service calls */
#define CREATEPROCESS  1
#define TERMPROCESS    2
#define DOIO           3
#define GETTIME        4
#define CLOCKWAIT      5
#define GETSUPPORTPTR  6
#define GETPROCESSID   7

/* System service identifiers */
#define GET_TOD       1  // Get time-of-day
#define TERMINATE     2  // Terminate process
#define WRITEPRINTER  3  // Write to printer
#define WRITETERMINAL 4  // Write to terminal

/* Status register constants */
#define ALLOFF      0x00000000  // All flags off
#define USERPON     0x00000008  // User mode on
#define KUCON       0x00000002  // Kernel/user mode control
#define IEPON       0x00000004  // Interrupt enable previous on
#define IECON       0x00000001  // Interrupt enable current on
#define IMON        0x0000FF00  // Interrupt mask
#define TEBITON     0x08000000  // Timer enable bit on
#define DISABLEINTS 0xFFFFFFFE  // Disable interrupts

/* Cause register constants */
#define GETEXECCODE       0x0000007C  // Extract execution code
#define CLEAREXECCODE     0xFFFFFF00  // Clear execution code
#define INTPROCINTERRUPT  0x00000000  // Interrupt base
#define LOCALTIMERINT     0x00000200  // Local timer interrupt
#define TIMERINTERRUPT    0x00000400  // Timer interrupt
#define DISKINTERRUPT     0x00000800  // Disk interrupt
#define FLASHINTERRUPT    0x00001000  // Flash memory interrupt
#define NETWORKINTERRUPT  0x00002000  // Network interrupt
#define PRINTINTERRUPT    0x00004000  // Printer interrupt
#define TERMINTERRUPT     0x00008000  // Terminal interrupt
#define IOINTERRUPTS      0  // I/O Interrupt base value
#define INTDEVBITMAP      0x10000040  // Interrupt device bitmap address
#define TLBINVLDL         2  // TLB invalid load exception
#define TLBINVLDS         3  // TLB invalid store exception
#define SYSEXCEPTION      8  // System call exception
#define BREAKEXCEPTION    9  // Break instruction exception
#define PRIVINSTR        10  // Privileged instruction exception
#define CAUSESHIFT        2  // Shift amount for cause register

/* EntryLO register (NDVG) constants */
#define DIRTYON  0x00000400  // Dirty bit enabled
#define VALIDON  0x00000200  // Valid bit enabled
#define GLOBALON 0x00000100  // Global bit enabled

/* EntryHI register constants */
#define GETPAGENO     0x3FFFF000  // Extract page number
#define GETSHAREFLAG  0xC0000000  // Extract shared segment flag
#define VPNSHIFT      12  // VPN shift amount
#define VPNMASK       0xFFFFF000  // VPN mask
#define ASIDSHIFT     6   // ASID shift amount
#define SHAREDSEGFLAG 30  // Shared segment flag bit position

/* Index register constants */
#define PRESENTFLAG 0x80000000

/* Device register constants */
#define DEV0ON 0x00000001
#define DEV1ON 0x00000002
#define DEV2ON 0x00000004
#define DEV3ON 0x00000008
#define DEV4ON 0x00000010
#define DEV5ON 0x00000020
#define DEV6ON 0x00000040
#define DEV7ON 0x00000080


/* Device operation constants */
#define OKCHARTRANS  5  // Character successfully transmitted
#define TRANSMITCHAR 2  // Character transmission command
#define RECEIVECHAR  2  // Character reception command
#define PRINTCHR     2  // Print character command

/* Disk operation constants */
#define SEEKTOCYL  2  // Seek to cylinder
#define DISKREAD   3  // Read from disk
#define DISKWRITE  4  // Write to disk
#define FLASHREAD  2  // Read from flash memory
#define FLASHWRITE 3  // Write to flash memory
#define BACKREAD   1  // Background read
#define BACKWRITE  2  // Background write

/* Memory constants */
#define UPROCSTARTADDR 0x800000B0  // User process start address
#define USERSTACKTOP   0xC0000000  // Top of user stack
#define KERNELSTACK    0x20001000  // Kernel stack address

#define SHARED  0x3  // Shared memory
#define PRIVATE 0x2  // Private memory


/* General utility constants */
#define ON         1
#define OFF        0
#define OK         0
#define NOPROC    -1
#define BYTELENGTH 8

#define PSECOND    100000  // Pseudo-second value
#define TIMESLICE  5000    // Process time slice
#define NEVER      0x7FFFFFFF  // Never-ending time value
#define SECOND     1000000  // One second in microseconds
#define STATESIZE  0x8C  // Processor state size
#define DEVICECNT  (DEVINTNUM * DEVPERINT)  // Total device count
#define MAXSTRLENG 128  // Maximum string length

#define DELAYASID    (UPROCMAX + 1)
#define KUSEG3SECTNO 0

#define VMDISK        0  /* Virtual memory disk identifier */
#define MAXPAGES      32  /* Maximum number of pages per process */
#define USERPGTBLSIZE MAXPAGES  /* User page table size, set to MAXPAGES */
#define OSFRAMES      32  /* Number of frames reserved for the OS */

#define FLASHPOOLSTART (RAMSTART + (OSFRAMES * PAGESIZE)) /* Start address of the flash pool */
#define DISKPOOLSTART  (FLASHPOOLSTART + (DEVPERINT * PAGESIZE)) /* Start address of the disk pool */
#define FRAMEPOOLSTART (DISKPOOLSTART + (DEVPERINT * PAGESIZE)) /* Start address of the frame pool */

#define SWAP_POOL_AREA 0x20020000  // Address of the swap pool area 

#define RAMTOP(T) ((T) = ((*((int *)RAMBASEADDR)) + (*((int *)RAMBASESIZE))))  
/* Macro to compute the top of RAM by reading the base address and size */

/*
 * This function takes the CAUSE register (Section 3.3 of POPS) and extracts the bits corresponding to interrupt pending (IP).
 * The parameter "il_no" represents all possible devices (as per /umps3/umps/arch.h, line 68).
 * The function allows checking which device is currently active.
 * If a device is active, CAUSE_IP_GET returns 1; otherwise, it returns 0.
 */
#define CAUSE_IP_GET(cause, il_no) ((cause) & (1 << ((il_no) + 8))) 
/* Extracts interrupt pending bit for a specific device */


/* Semaphore constants */
#define NRSEMAPHORES 49  /* Number of device semaphores plus the pseudo-clock semaphore */
#define NSUPPSEM 48  /* Number of semaphores used for the support level */

#define DISKBACK     1  /* Disk backing store */
#define FLASHBACK    0  /* Flash backing store */
#define BACKINGSTORE FLASHBACK  /* Default backing store is flash memory */

#define UPROCMAX 8  /* Maximum number of user processes */
#define POOLSIZE (UPROCMAX * 2)  /* Size of the resource pool */

#define CHARRECV 5		/* Character received*/

/* Start address of device registers */
#define START_DEVREG 0x10000054 

#define MAXDEV 8  /* Maximum number of device instances */
#define ENDIO 8  /* System service interrupt to end DOIO operation */
#define TRUE 1  /* Boolean true value */
#define FALSE 0  /* Boolean false value */

/* Base addresses for I/O devices */
#define TERM0ADDR 0x10000254  /* Base address for terminal 0 */
#define PRINTER0ADDR 0x100001D4  /* Base address for printer 0 */

/* Terminal and printer status masks */
#define TERMSTATMASK 0xFF  /* Mask to check terminal status */
#define READY 1  /* Device ready status */

#endif
