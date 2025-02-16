/*
  Test and Support Level’s global variables
*/

#ifndef INITPROC_H
#define INITPROC_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

// processo che possiede attualmente la mutex
pcb_PTR mutexHolderProcess;
// processo mutex
pcb_PTR swapMutexProcess;
// state del processo mutex
state_t swapMutexState; 

// processo di test
pcb_PTR test_pcb;
// indirizzo di memoria corrente
memaddr addr;
// state degli U-proc
state_t uprocStates[UPROCMAX];
// state degli SST
state_t sstStates[UPROCMAX];
// strutture di supporto condivise
support_t supports[UPROCMAX];
// array dei processi SST
pcb_PTR sstArray[UPROCMAX];
// Swap pool 
swpo_t swap_pool[POOLSIZE];

void test();
static void initUproc();
static void initSST();
static void initSwapPool();
static void initSwapMutex();
void swapMutex();
static void initPageTableEntry(unsigned int asid, pteEntry_t *entry, int idx);

#endif