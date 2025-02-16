#include "ssi.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head external_blocked_list[4][MAXDEV];
extern struct list_head pseudoclock_blocked_list;
extern struct list_head terminal_blocked_list[2][MAXDEV];
extern void copyRegisters(state_t *dest, state_t *src);

/**
 * @brief Handles a request received from a process.
 * This function processes different service codes and responds accordingly.
 */
void SSIHandler() {
  while (TRUE) {
    ssi_payload_PTR p_payload;
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &p_payload, 0);
    unsigned int response = 0;

    // Perform the requested service based on the service code
    switch (p_payload->service_code) {
      case CREATEPROCESS:
        // Create a new process
        response = createProcess((ssi_create_process_PTR) p_payload->arg, sender);
        break;
      case TERMPROCESS:
        // Terminate an existing process
        if (p_payload->arg == NULL) {
          terminateProcess(sender);  // Terminate the sender itself
        } else {
          terminateProcess((pcb_PTR) p_payload->arg);  // Terminate the specified process
        }
        break;
      case DOIO:
        // Perform I/O operation
        blockForDevice((ssi_do_io_PTR) p_payload->arg, sender);
        break;
      case GETTIME:
        // Return the accumulated processor time
        response = (unsigned int) sender->p_time;
        break;
      case CLOCKWAIT:
        // Block the process for the pseudoclock
        insertProcQ(&pseudoclock_blocked_list, sender);
        waiting_count++;
        break;
      case GETSUPPORTPTR:
        // Return the support structure of the process
        response = (unsigned int) sender->p_supportStruct;
        break;
      case GETPROCESSID:
        // Return the process ID of the sender or its parent
        if (((unsigned int) p_payload->arg) == 0) {
          response = sender->p_pid;
        } else {
          if (sender->p_parent == NULL) {
            response = 0;  // Return 0 if no parent exists
          } else {
            response = sender->p_parent->p_pid;  // Return the parent's process ID
          }
        }
        break;
      case ENDIO:
        // Terminate the IO operation
        response = sender->p_s.reg_v0;
        break;
      default:
        // Invalid service code, terminate the requesting process and its progeny
        terminateProcess(sender);
        break;
    }
    if (p_payload->service_code != DOIO) {
      SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
    }
  }
}

/**
 * @brief Creates a new process as a child of the requesting process and inserts it into the ready queue.
 * @param arg Structure containing the state and optional support structure.
 * @param sender The requesting process.
 * @return Pointer to the created process, NOPROC otherwise.
 */
static unsigned int createProcess(ssi_create_process_t *arg, pcb_t *sender) {
  pcb_PTR p = allocPcb();  // Allocate a new PCB for the process
  if (p == NULL) {
    return (unsigned int) NOPROC;  // Return NOPROC if allocation fails
  } else {
    copyRegisters(&p->p_s, arg->state);  // Copy the state from the argument to the new process
    if (arg->support != NULL) p->p_supportStruct = arg->support;  // Set the support structure if provided
    insertChild(sender, p);  // Insert the new process as a child of the sender
    insertProcQ(&ready_queue, p);  // Insert the process into the ready queue
    process_count++;  // Increment the process count
    return (unsigned int) p;  // Return the process pointer
  }
}

/**
 * @brief Terminates a process and all of its progeny (children).
 * @param p The process to terminate.
 */
void terminateProcess(pcb_t *p) {
  outChild(p);  // Remove the process from the parent’s children list
  terminateProgeny(p);  // Recursively terminate the progeny
  destroyProcess(p);  // Destroy the process and free resources
}

/**
 * @brief Terminates all the children of a process recursively.
 * @param p The process whose children are to be terminated.
 */
void terminateProgeny(pcb_t *p) {
  while (!emptyChild(p)) {
    // Remove the first child
    pcb_t *child = removeChild(p);
    // If the child was removed successfully, terminate its progeny recursively
    if (child != NULL) {
      terminateProgeny(child);
      // After terminating the children, destroy the child process
      destroyProcess(child);
    }
  }
}

/**
 * @brief Destroys a process by removing it from various queues and freeing its resources.
 * @param p The process to destroy.
 */
void destroyProcess(pcb_t *p) {
  if (!isInPCBFree_h(p)) {
    // Look for the process in the ready queue
    if (outProcQ(&ready_queue, p) == NULL) {
      // If not in the ready queue, check the pseudoclock blocked list
      int found = FALSE;
      if (outProcQ(&pseudoclock_blocked_list, p) == NULL) {
        // If not in pseudoclock, check other blocked lists (e.g., devices)
        for (int i = 0; i < MAXDEV && found == FALSE; i++) {
          if (outProcQ(&external_blocked_list[0][i], p) != NULL) found = TRUE;
          if (outProcQ(&external_blocked_list[1][i], p) != NULL) found = TRUE;
          if (outProcQ(&external_blocked_list[2][i], p) != NULL) found = TRUE;
          if (outProcQ(&external_blocked_list[3][i], p) != NULL) found = TRUE;
          if (outProcQ(&terminal_blocked_list[0][i], p) != NULL) found = TRUE;
          if (outProcQ(&terminal_blocked_list[1][i], p) != NULL) found = TRUE;
        }
      } else {
        found = TRUE;
      }
      // Decrease waiting_count only if the process was blocked for IO or pseudoclock
      if (found) waiting_count--;
    }
    freePcb(p);  // Free the PCB
    process_count--;  // Decrement the process count
  }
}

/**
 * @brief Blocks a process in the device list for the specified IO operation.
 * @param arg Pointer to the DOIO structure.
 * @param toBlock The process to block.
 */
static void blockForDevice(ssi_do_io_t *arg, pcb_t *toBlock) {
  // Iterate over terminal devices
  for (int dev = 0; dev < MAXDEV; dev++) {
    // Calculate the base address for the device
    termreg_t *base_address = (termreg_t *)DEV_REG_ADDR(TERMINT, dev);
    if (arg->commandAddr == (memaddr) & (base_address->recv_command)) {
      // Insert the process into the respective blocked list for the terminal receive command
      insertProcQ(&terminal_blocked_list[1][dev], toBlock);
      waiting_count++;
      *arg->commandAddr = arg->commandValue;  // Set the command value for the operation
      return;
    } else if (arg->commandAddr == (memaddr) & (base_address->transm_command)) {
      // Insert the process into the respective blocked list for the terminal transmit command
      insertProcQ(&terminal_blocked_list[0][dev], toBlock);
      waiting_count++;
      *arg->commandAddr = arg->commandValue;  // Set the command value for the operation
      return;
    }
  }
  // Iterate over other devices
  for (int line = 3; line < 7; line++) {
    for (int dev = 0; dev < MAXDEV; dev++) {
      dtpreg_t *base_address = (dtpreg_t *)DEV_REG_ADDR(line, dev);
      if (arg->commandAddr == (memaddr) & (base_address->command)) {
        // Insert the process into the respective blocked list for the device
        insertProcQ(&external_blocked_list[line - 3][dev], toBlock);
      }
    }
  }

  waiting_count++;  // Increment waiting count as the process is blocked
  *arg->commandAddr = arg->commandValue;  // Set the command value for the operation
}
