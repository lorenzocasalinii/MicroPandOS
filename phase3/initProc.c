#include "initProc.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern void SSTInitialize();
extern void supportExceptionHandler();
extern void pager();

/**
 * Test function for phase 3
 * This function initializes different systems, waits for messages signaling the termination of U-processes, 
 * and terminates the test process by sending a termination message to SSI.
 */
void test() {
  test_pcb = current_process;
  RAMTOP(addr);
  // Move beyond the SSI and test processes
  addr -= (3 * PAGESIZE);

  // Initialize swap pool
  initSwapPool();

  // Initialize U-proc
  initUproc();

  // Initialize mutex for swap processes
  initSwapMutex();

  // Initialize the SST (System Support Tables)
  initSST();

  // Wait for the 8 messages that signal the termination of U-procs
  for (int i = 0; i < 8; i++) {
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
  }

  // Terminate the test process
  ssi_payload_t termPayload = {
    .service_code = TERMPROCESS,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &termPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, 0, 0);

  // If successful, this line should never be reached
  PANIC();
}

/**
 * Initializes the state for each U-proc (user process)
 */
static void initUproc() {
  for (int asid = 1; asid <= 8; asid++) {
    uprocStates[asid - 1].pc_epc = (memaddr) UPROCSTARTADDR;
    uprocStates[asid - 1].reg_t9 = (memaddr) UPROCSTARTADDR;
    uprocStates[asid - 1].reg_sp = (memaddr) USERSTACKTOP;
    uprocStates[asid - 1].status = ALLOFF | USERPON | IEPON | IMON | TEBITON;
    uprocStates[asid - 1].entry_hi = asid << ASIDSHIFT;

    // Initialize support structures
    supports[asid - 1].sup_asid = asid;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
    addr -= PAGESIZE;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].pc = (memaddr) supportExceptionHandler;
    addr -= PAGESIZE;

    // Initialize the page table entries for the U-proc
    for (int i = 0; i < USERPGTBLSIZE; i++) {
      initPageTableEntry(asid, &(supports[asid - 1].sup_privatePgTbl[i]), i);
    }
  }
}

/**
 * Initializes the swap pool by setting all entries to NOPROC and -1 (no assigned page)
 */
static void initSwapPool() {
  for (int i = 0; i < POOLSIZE; i++) {
    swap_pool[i].swpo_asid = NOPROC;
    swap_pool[i].swpo_page = -1;
  }
}

/**
 * Initializes and creates SST (System Support Tables) processes
 */
static void initSST() {
  for (int asid = 1; asid <= 8; asid++) {
    // Initialize the state for each SST
    sstStates[asid - 1].reg_sp = (memaddr) addr;
    sstStates[asid - 1].pc_epc = (memaddr) SSTInitialize;
    sstStates[asid - 1].reg_t9 = (memaddr) SSTInitialize;
    sstStates[asid - 1].status = ALLOFF | IEPON | IMON | TEBITON;
    sstStates[asid - 1].entry_hi = asid << ASIDSHIFT;

    // Create the process by sending a request to the SSI
    ssi_create_process_t create = {
      .state = &sstStates[asid - 1],
      .support = &supports[asid - 1],
    };
    ssi_payload_t createPayload = {
      .service_code = CREATEPROCESS,
      .arg = &create,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &createPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &sstArray[asid - 1], 0);
    
    addr -= PAGESIZE;
  }
}

/**
 * Initializes a page table entry for a U-proc with a given ASID and index.
 * This sets the proper entry HI and LO values based on the U-proc's configuration.
 */
static void initPageTableEntry(unsigned int asid, pteEntry_t *entry, int idx){
  if (idx < 31)
    entry->pte_entryHI = KUSEG + (idx << VPNSHIFT) + (asid << ASIDSHIFT);
  else
    entry->pte_entryHI = 0xBFFFF000 + (asid << ASIDSHIFT); 
  entry->pte_entryLO = DIRTYON;
}

/**
 * Initializes the swap mutex process that handles mutual exclusion
 */
static void initSwapMutex() {
  swapMutexState.reg_sp = (memaddr) addr;
  swapMutexState.pc_epc = swapMutexState.reg_t9 = (memaddr) swapMutex;
  swapMutexState.status = ALLOFF | IEPON | IMON | TEBITON;

  addr -= PAGESIZE;

  // Create the mutex process by sending a request to the SSI
  ssi_create_process_t create = {
      .state = &swapMutexState,
      .support = NULL,
  };
  ssi_payload_t createPayload = {
      .service_code = CREATEPROCESS,
      .arg = &create,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &createPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &swapMutexProcess, 0);
  
}

/**
 * Handles the mutual exclusion for swap processes
 * The swapMutex function ensures that only one process can hold the swap mutex at a time.
 */
void swapMutex() {
  while(TRUE) {
    unsigned int sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
    mutexHolderProcess = (pcb_t *)sender;
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
    // The process holding the mutex must release it
    SYSCALL(RECEIVEMESSAGE, sender, 0, 0);
    mutexHolderProcess = NULL;
  }
}
