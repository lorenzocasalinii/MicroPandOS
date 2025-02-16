#include "sysSupport.h"
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;

/**
 * @brief Handles exceptions at the support level
 */
void supportExceptionHandler() {
    // Request the support structure of the current process from the SSI 
    support_t *supPtr;
    ssi_payload_t payload = {
    .service_code = GETSUPPORTPTR,
    .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &supPtr, 0);

    // Get the processor state at the time of the exception
    state_t *supExceptionState = &(supPtr->sup_exceptState[GENERALEXCEPT]);

    // Extract the EXECCODE from the cause register
    int supExceptionCause = (supExceptionState->cause & GETEXECCODE) >> CAUSESHIFT;

    // If EXECCODE is 8, it's a syscall; invoke the syscall handler
    if(supExceptionCause == SYSEXCEPTION) {
        supportSyscallHandler(supExceptionState);
    }
    // Otherwise, treat it as a program trap and proceed with termination
    else {
        supportTrapHandler(supExceptionState);
    }

    // Increment the PC by 4 to avoid an infinite loop
    supExceptionState->pc_epc += WORDLEN;

    // Reload the processor state
    LDST(supExceptionState);
}

/**
 * @brief Handles system calls requested by Uprocs
 * 
 * @param supExceptionState Processor state at the time of the exception
 */
void supportSyscallHandler(state_t *supExceptionState) {
    // Check the syscall type from reg_a0
    switch(supExceptionState->reg_a0) {
        // If reg_a0 is 1, it's a sendMsg syscall
        case SENDMSG:
            sendMsg(supExceptionState);
            break;
        // If reg_a0 is 2, it's a receiveMsg syscall
        case RECEIVEMSG:
            receiveMsg(supExceptionState);
            break;
        // Default case: Handle as a trap (should not be reached)
        default:
            supportTrapHandler(supExceptionState);
            break;  
    }    
}

/**
 * @brief USYS1: Sends a message to a specific recipient process.
 * If a1 contains PARENT, the message is sent to its SST.
 * 
 * @param supExceptionState Processor state at the time of the exception
 */
void sendMsg(state_t *supExceptionState) {
    if(supExceptionState->reg_a1 == PARENT) {
      SYSCALL(SENDMESSAGE, (unsigned int)current_process->p_parent, supExceptionState->reg_a2, 0);
    } else {
      SYSCALL(SENDMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
    }
}

/**
 * @brief USYS2: Extracts a message from the inbox or waits for a message if the inbox is empty.
 * 
 * @param supExceptionState Processor state at the time of the exception
 */
void receiveMsg(state_t *supExceptionState) {
    SYSCALL(RECEIVEMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
}

/**
 * @brief Handles program traps at the support level
 * 
 * @param supExceptionState Processor state at the time of the exception
 */
void supportTrapHandler(state_t *supExceptionState) {
    // If the process had a mutex, release it by sending a message to swapMutex
    if(current_process == mutexHolderProcess)
        SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);   

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };

    // Terminate the process by sending a termination request to the SSI
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}
