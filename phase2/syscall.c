#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head pseudoclock_blocked_list;
extern pcb_PTR ssi_pcb;
extern state_t *currentState;
extern void terminateProcess(pcb_t *proc);
extern int isInDevicesLists(pcb_t *p);
extern void copyRegisters(state_t *dest, state_t *src);

/**
 * @brief Handles the request for a send or receive system call.
 * This function checks the current mode (kernel or user) and performs actions based on the system call type.
 */
void syscallHandler() {
    // If in user mode, set the cause.ExcCode to PRIVINSTR and invoke the Trap handler
    if ((currentState->status & KUCON) >> 1) {
        currentState->cause &= CLEAREXECCODE;
        currentState->cause |= (PRIVINSTR << CAUSESHIFT);
        passUpOrDie(GENERALEXCEPT);
    } else {
        // If in kernel mode, invoke the corresponding system call handler
        switch(currentState->reg_a0) {
            case SENDMESSAGE:
                sendMessage();
                LDST(currentState);  // Load the state after sending the message
                break;
            case RECEIVEMESSAGE:
                receiveMessage();
                LDST(currentState);  // Load the state after receiving the message
                break;
            default:
                passUpOrDie(GENERALEXCEPT);  // If unrecognized, handle as a general exception
                break;  
        }
    }
}

/**
 * @brief Sends a message to a specific recipient process.
 * This function checks if the recipient is valid (in PCB list, in ready queue, or in a waiting list).
 * If the recipient is not in any of these lists, it puts the message in the inbox and wakes the process.
 */
void sendMessage() {
    pcb_PTR receiver = (pcb_PTR)currentState->reg_a1;
    unsigned int payload = currentState->reg_a2;
    int messagePushed = FALSE, found = FALSE;

    // Check if the receiver is in the free PCB list
    if(isInPCBFree_h(receiver)) {
        found = TRUE;
        currentState->reg_v0 = DEST_NOT_EXIST;  // Receiver does not exist
    }

    // Check if the receiver is currently running or in any waiting list (ready, pseudoclock, or devices)
    if (!found && (receiver == current_process || isInList(&ready_queue, receiver) || isInList(&pseudoclock_blocked_list, receiver) || isInDevicesLists(receiver))) {
        found = TRUE;
        msg_PTR toPush = createMessage(current_process, payload);
        if (toPush != NULL) {
            insertMessage(&receiver->msg_inbox, toPush);  // Add the message to the receiver's inbox
            messagePushed = TRUE;
        }
    }

    // If the receiver was not found in the lists, push the message into their inbox and wake them up
    if(!found) {
        msg_PTR toPush = createMessage(current_process, payload);
        if (toPush != NULL) {
            insertMessage(&receiver->msg_inbox, toPush);  // Add the message to the receiver's inbox
            messagePushed = TRUE;
            insertProcQ(&ready_queue, receiver);  // Wake up the receiver by adding them to the ready queue
        }
    }

    // If the message was successfully pushed, set reg_v0 to OK
    if(messagePushed) {
        currentState->reg_v0 = OK;
    } else if(!isInPCBFree_h(receiver)) {
        // Otherwise, set reg_v0 to MSGNOGOOD if the receiver was not in the free PCB list
        currentState->reg_v0 = MSGNOGOOD;
    }

    // Increment PC to avoid infinite loops
    currentState->pc_epc += WORDLEN;
}

/**
 * @brief Extracts a message from the inbox or waits for a message if the inbox is empty.
 * This function handles the case where the process waits for a message if no message is available.
 */
void receiveMessage() {
    msg_PTR messageExtracted = NULL;
    pcb_PTR sender = (pcb_PTR)currentState->reg_a1;
    memaddr *payload = (memaddr*) currentState->reg_a2;

    // Try to extract a message from the current process's inbox
    messageExtracted = popMessage(&current_process->msg_inbox, sender);

    // If no message is found, block the process
    if(messageExtracted == NULL) {
        copyRegisters(&current_process->p_s, currentState);  // Save the current state
        current_process->p_time += (TIMESLICE - getTIMER());  // Adjust time
        current_process = NULL;
        schedule();  // Call the scheduler to handle context switch
    } 
    // If a message was found
    else {
        // Store the message payload in the location pointed by reg_a2
        if(payload != NULL) {
            *payload = messageExtracted->m_payload;
        }

        // Store the sender's address in reg_v0
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        freeMsg(messageExtracted);  // Free the message after processing
        currentState->pc_epc += WORDLEN;  // Increment PC to avoid infinite loops
    }
}

/**
 * @brief Handles the exception by either passing it up or terminating the process.
 * @param indexValue Determines whether it's a PGFAULTEXCEPT or GENERALEXCEPT.
 */
void passUpOrDie(int indexValue) {
    // Pass up if the process has a support structure
    if(current_process->p_supportStruct != NULL) {
        copyRegisters(&current_process->p_supportStruct->sup_exceptState[indexValue], currentState);

        unsigned int stackPtr, status, progCounter;
        stackPtr = current_process->p_supportStruct->sup_exceptContext[indexValue].stackPtr;
        status = current_process->p_supportStruct->sup_exceptContext[indexValue].status;
        progCounter = current_process->p_supportStruct->sup_exceptContext[indexValue].pc;

        LDCXT(stackPtr, status, progCounter);  // Load the context for exception handling
    }
    // Or terminate the process if no support structure exists
    else {
        terminateProcess(current_process);  // Terminate the process
        current_process = NULL;
        schedule();  // Call the scheduler for a new process
    }
}

/**
 * @brief Allocates a new message.
 * @param sender The sender of the message.
 * @param payload The payload of the message.
 * @return A pointer to the new message if successful, NULL otherwise.
 */
msg_PTR createMessage(pcb_PTR sender, unsigned int payload) {
    msg_PTR newMsg = allocMsg();  // Allocate memory for a new message
    if (newMsg != NULL) {
        newMsg->m_sender = sender;  // Set the sender of the message
        newMsg->m_payload = payload;  // Set the payload of the message
    }
    return newMsg;
}
