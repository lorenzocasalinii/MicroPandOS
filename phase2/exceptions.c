#include "exceptions.h"

#include "syscall.h"

extern state_t *currentState;
extern pcb_PTR current_process;
extern void interruptHandler();

/**
 * @brief Handles all other types of exceptions.
 * The function processes different exception codes and dispatches them to their corresponding handlers.
 */
void exceptionHandler() {
    switch((getCAUSE() & GETEXECCODE) >> CAUSESHIFT) {
        case IOINTERRUPTS:
            // External Device Interrupt - handle interrupts from external devices
            interruptHandler();
            break;
        case 1 ... 3:
            // TLB Exception - handle TLB modifications or invalidation
            passUpOrDie(PGFAULTEXCEPT);  // Handle page fault exception
            break;
        case 4 ... 7:
            // Program Traps p1: Address and Bus Error Exceptions
            passUpOrDie(GENERALEXCEPT);  // Handle general exception
            break;
        case SYSEXCEPTION: 
            // Syscalls - handle system calls
            syscallHandler();
            break;
        case 9 ... 12:
            // Breakpoint Calls, Program Traps p2 - handle program breakpoints
            passUpOrDie(GENERALEXCEPT);  // Handle general exception
            break;
        default: 
            // Wrong ExcCode - unrecognized exception code
            passUpOrDie(GENERALEXCEPT);  // Handle general exception
            break;
    }
}

/*
    Exception Code Table:
    Number - Code - Description
    0 - Int - External Device Interrupt
    1 - Mod - TLB-Modification Exception
    2 - TLBL - TLB Invalid Exception: on a Load instruction or instruction fetch
    3 - TLBS - TLB Invalid Exception: on a Store instruction
    4 - AdEL - Address Error Exception: on a Load or instruction fetch
    5 - AdES - Address Error Exception: on a Store instruction
    6 - IBE - Bus Error Exception: on an instruction fetch
    7 - DBE - Bus Error Exception: on a Load/Store data access
    8 - Sys - Syscall Exception
    9 - Bp - Breakpoint Exception
    10 - RI - Reserved Instruction Exception
    11 - CpU - Coprocessor Unusable Exception
    12 - OV - Arithmetic Overflow Exception
*/
