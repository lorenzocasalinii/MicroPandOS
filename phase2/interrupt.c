#include "interrupt.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head external_blocked_list[4][MAXDEV];
extern struct list_head pseudoclock_blocked_list;
extern struct list_head terminal_blocked_list[2][MAXDEV];
extern pcb_PTR ssi_pcb;
extern state_t *currentState;
extern void copyRegisters(state_t *dest, state_t *src);
extern msg_PTR createMessage(pcb_PTR sender, unsigned int payload);

// Payload for direct message to SSI when IO operation ends
ssi_payload_t payloadDM = {
    .service_code = ENDIO,
    .arg = NULL,
};

/**
 * Handles all types of interrupts.
 * This function processes interrupts based on their line number and manages the execution flow.
 */
void interruptHandler() {
    unsigned int causeReg = getCAUSE();
    // Interrupts are globally enabled
    if(((currentState->status & IEPON) >> 2) == 1) {

        for(int line = 0; line < 8; line++) {
            // Check if the current interrupt line is active
            if(intPendingInLine(causeReg, line)) {
                switch(line) {
                    case 0:
                    break;
                    case 1:
                        if(intLineActive(1)) {
                            // Time slice for the current running process has expired
                            PLTInterruptHandler();
                        }
                    break;
                    case 2:
                        if(intLineActive(2)) {
                            // Interval timer interrupt (pseudoclock)
                            ITInterruptHandler();
                        }   
                    break;
                    case 5:
                    break;
                    default:
                        // If the current interrupt line is active, check for device-specific interrupts
                        if(intLineActive(line)) {

                            unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (line - 3)));

                            for(int dev = 0; dev < 8; dev++) {
                                if(intPendingOnDev(intLaneMapped, dev)) {
                                    
                                    pcb_PTR toUnblock;
                                    unsigned int devStatusReg;

                                    // Handle terminal device interrupts (both transmit and receive)
                                    if(line == 7) {
                                        toUnblock = termDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    // Handle external device interrupts
                                    else {
                                        toUnblock = extDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    
                                    // If there's a process to unblock, update its state and send a message to SSI
                                    if(toUnblock != NULL) {
                                        waiting_count--;
                                        toUnblock->p_s.reg_v0 = devStatusReg;

                                        // Create a message to SSI to unblock the process
                                        msg_PTR toPush = createMessage(toUnblock, (unsigned int) &payloadDM);
                                        if (toPush != NULL) {
                                            insertMessage(&ssi_pcb->msg_inbox, toPush);
                                            // If SSI is not executing or in readyQueue, move it there
                                            if (ssi_pcb != current_process && !isInList(&ready_queue, ssi_pcb)) {
                                                insertProcQ(&ready_queue, ssi_pcb);
                                            }
                                        } 
                                    }

                                    // Schedule the next process if needed
                                    if(current_process == NULL)
                                        schedule();
                                    else
                                        LDST(currentState);
                                }
                            }
                        }
                    break;
                }
            }
        }
    }
}

/**
 * Given the interrupt line and device interrupt line, calculate the base address of the device register.
 * 
 * @param intLine Interrupt line number
 * @param devIntLine Device interrupt line number
 * @return Base address of the device register
 */
memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine) {
    return DEV_REG_ADDR(intLine, devIntLine);
}

/**
 * Checks if a specific interrupt line is active or masked.
 * 
 * @param line Interrupt line number
 * @return 1 if active, 0 if masked
 */
unsigned short int intLineActive(unsigned short int line) {
    return (((currentState->status & IMON) >> (8 + line)) & 0x1) == 1; 
}

/**
 * Checks if there is a pending interrupt in a specific interrupt line.
 * 
 * @param causeReg Cause register value
 * @param line Interrupt line number
 * @return 1 if interrupt is pending, 0 if not
 */
unsigned short int intPendingInLine(unsigned int causeReg, unsigned short int line) {
    return (((causeReg & interruptConsts[line]) >> (8 + line))) == 1;
}

/**
 * Checks if a specific device has triggered an interrupt.
 * 
 * @param intLaneMapped Pointer to the interrupt lane map
 * @param dev Device number
 * @return 1 if interrupt is pending on the device, 0 if not
 */
unsigned short int intPendingOnDev(unsigned int *intLaneMapped, unsigned int dev) {
    return (((*intLaneMapped) & deviceConsts[dev]) >> dev) == 1;
}

/**
 * Handles the interrupt caused by the expiration of the time slice (Preemption).
 * Saves the current process state and moves it to the ready queue.
 */
void PLTInterruptHandler() {
    copyRegisters(&current_process->p_s, currentState);
    current_process->p_time += TIMESLICE;
    insertProcQ(&ready_queue, current_process);
    current_process = NULL;
    schedule();
}

/**
 * Handles the interrupt generated by the interval timer (pseudoclock).
 * Moves processes waiting for the pseudoclock to the ready queue.
 */
void ITInterruptHandler() {
    LDIT(PSECOND);
    // Move all processes waiting for the pseudoclock back to the ready queue
    while(!emptyProcQ(&pseudoclock_blocked_list)) {
        pcb_PTR toUnblock = removeProcQ(&pseudoclock_blocked_list);
        waiting_count--;
        insertProcQ(&ready_queue, toUnblock);
    }
    if(current_process == NULL)
        schedule();
    else
        LDST(currentState);
}

/**
 * Handles interrupts generated by terminal devices (both transmit and receive).
 * 
 * @param devStatusReg Device status register
 * @param line Interrupt line number
 * @param dev Device number
 * @return The process to unblock
 */
pcb_PTR termDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    termreg_t *devReg = (termreg_t *)getDevReg(line, dev);
    unsigned short int selector;

    // Check for transmit status
    if((devReg->transm_status & 0xFF) == OKCHARTRANS) {
        *devStatusReg = devReg->transm_status;
        devReg->transm_command = ACK;
        selector = 0;
    }
    // Check for receive status
    else if((devReg->recv_status & 0xFF) == OKCHARTRANS) {
        *devStatusReg = devReg->recv_status;
        devReg->recv_command = ACK;
        selector = 1;
    }

    // Use selector to determine whether to unblock the process from the transmitter or receiver queue
    return removeProcQ(&terminal_blocked_list[selector][dev]);
}

/**
 * Handles interrupts generated by external devices.
 * 
 * @param devStatusReg Device status register
 * @param line Interrupt line number
 * @param dev Device number
 * @return The process to unblock
 */
pcb_PTR extDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    dtpreg_t *devReg = (dtpreg_t *)getDevReg(line, dev);

    // Save the status register and acknowledge the interrupt
    *devStatusReg = devReg->status;
    devReg->command = ACK;

    // Unblock the process waiting for this external device
    return removeProcQ(&external_blocked_list[(line - 3)][dev]);
}
