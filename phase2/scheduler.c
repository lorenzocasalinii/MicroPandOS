#include "scheduler.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;

/**
 * @brief Loads a process to be run, or blocks execution.
 */
void schedule() {
  // Dispatch the next process
  current_process = removeProcQ(&ready_queue);

  if (current_process != NULL) {
    // Load the PLT 
    setTIMER(TIMESLICE * (*((cpu_t *)TIMESCALEADDR)));
    // Perform Load Processor State 
    LDST(&current_process->p_s);
  } else if (process_count == 1) {
    // If only the SSI process is in the system, halt
    HALT();
  } else if (process_count > 0 && waiting_count > 0) {
    // If waiting for an interrupt, wait
    setSTATUS((IECON | IMON) & (~TEBITON));
    WAIT();
  } else if (process_count > 0 && waiting_count == 0) {
    // Deadlock condition, panic
    PANIC();
  }
}
