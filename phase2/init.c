#include "init.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern void uTLB_RefillHandler();
extern void exceptionHandler();
extern void SSIHandler();
extern void test();

/**
 * @brief Entry point of the operating system.
 * Initializes the kernel, instantiates the SSI and test processes, and loads the Interval Timer.
 */
void main() {
  // nucleus initialization
  initialize();

  // load Interval Timer 100ms
  LDIT(PSECOND);

  // instantiate the first process (SSI)
  ssi_pcb = allocPcb();
  ssi_pcb->p_s.status |= IEPON | IMON;
  RAMTOP(ssi_pcb->p_s.reg_sp);
  ssi_pcb->p_s.pc_epc = (memaddr) SSIHandler;
  ssi_pcb->p_s.reg_t9 = (memaddr) SSIHandler;
  insertProcQ(&ready_queue, ssi_pcb);
  process_count++;

  // instantiate the second process (test)
  p3test_pcb = allocPcb();
  p3test_pcb->p_s.status |= IEPON | IMON | TEBITON;
  p3test_pcb->p_s.reg_sp = ssi_pcb->p_s.reg_sp - (2 * PAGESIZE);
  p3test_pcb->p_s.pc_epc = p3test_pcb->p_s.reg_t9 = (memaddr) test;
  insertProcQ(&ready_queue, p3test_pcb);
  process_count++;

  // call the scheduler to start execution
  schedule();
}

/**
 * @brief Initializes the PassUp Vector, PCB/msg structures, and global variables.
 */
static void initialize() {
  // Pass Up Vector for Processor 0
  passupvector_t *passUpVec = (passupvector_t *) PASSUPVECTOR;
  passUpVec->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
  passUpVec->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
  passUpVec->exception_handler = (memaddr) exceptionHandler;
  passUpVec->exception_stackPtr = (memaddr) KERNELSTACK;

  // initialize level 2 structures
  initPcbs();
  initMsgs();

  // initialize global variables
  process_count = 0;
  waiting_count = 0;
  current_process = NULL;
  mkEmptyProcQ(&ready_queue);

  // initialize device blocked lists
  for (int i = 0; i < MAXDEV; i++) {
    // external devices
    mkEmptyProcQ(&external_blocked_list[0][i]);
    mkEmptyProcQ(&external_blocked_list[1][i]);
    mkEmptyProcQ(&external_blocked_list[2][i]);
    mkEmptyProcQ(&external_blocked_list[3][i]);
    // terminal devices
    mkEmptyProcQ(&terminal_blocked_list[0][i]);
    mkEmptyProcQ(&terminal_blocked_list[1][i]);
  }

  // initialize pseudoclock blocked list
  mkEmptyProcQ(&pseudoclock_blocked_list);

  // set current state to BIOS data page
  currentState = (state_t *)BIOSDATAPAGE;
  stateCauseReg = &currentState->cause;
}

/**
 * @brief Checks if the process is waiting for a device.
 * 
 * @param p pointer to the process to check
 * @return 1 if it is in a device waiting list, 0 otherwise
 */
int isInDevicesLists(pcb_t *p) {
  for (int i = 0; i < MAXDEV; i++) {
    // check for external devices
    if (isInList(&external_blocked_list[0][i], p)) return TRUE;
    if (isInList(&external_blocked_list[1][i], p)) return TRUE;
    if (isInList(&external_blocked_list[2][i], p)) return TRUE;
    if (isInList(&external_blocked_list[3][i], p)) return TRUE;
    // check for terminal devices
    if (isInList(&terminal_blocked_list[0][i], p)) return TRUE;
    if (isInList(&terminal_blocked_list[1][i], p)) return TRUE;
  }
  return FALSE;
}

/**
 * @brief Copies all the register values from one state to another.
 * 
 * @param dest state to copy the values to
 * @param src state to copy the values from
 */
void copyRegisters(state_t *dest, state_t *src) {
  dest->cause = src->cause;
  dest->entry_hi = src->entry_hi;
  dest->reg_at = src->reg_at;
  dest->reg_v0 = src->reg_v0;
  dest->reg_v1 = src->reg_v1;
  dest->reg_a0 = src->reg_a0;
  dest->reg_a1 = src->reg_a1;
  dest->reg_a2 = src->reg_a2;
  dest->reg_a3 = src->reg_a3;
  dest->reg_t0 = src->reg_t0;
  dest->reg_t1 = src->reg_t1;
  dest->reg_t2 = src->reg_t2;
  dest->reg_t3 = src->reg_t3;
  dest->reg_t4 = src->reg_t4;
  dest->reg_t5 = src->reg_t5;
  dest->reg_t6 = src->reg_t6;
  dest->reg_t7 = src->reg_t7;
  dest->reg_s0 = src->reg_s0;
  dest->reg_s1 = src->reg_s1;
  dest->reg_s2 = src->reg_s2;
  dest->reg_s3 = src->reg_s3;
  dest->reg_s4 = src->reg_s4;
  dest->reg_s5 = src->reg_s5;
  dest->reg_s6 = src->reg_s6;
  dest->reg_s7 = src->reg_s7;
  dest->reg_t8 = src->reg_t8;
  dest->reg_t9 = src->reg_t9;
  dest->reg_gp = src->reg_gp;
  dest->reg_sp = src->reg_sp;
  dest->reg_fp = src->reg_fp;
  dest->reg_ra = src->reg_ra;
  dest->reg_HI = src->reg_HI;
  dest->reg_LO = src->reg_LO;
  dest->hi = src->hi;
  dest->lo = src->lo;
  dest->pc_epc = src->pc_epc;
  dest->status = src->status;
}
