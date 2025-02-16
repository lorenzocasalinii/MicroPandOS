#include "vmSupport.h"
#include "./sysSupport.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;
extern swpo_t swap_pool[POOLSIZE];

/**
 * @brief Handles the case where an attempt is made to access a virtual address
 *        that doesn't have a corresponding entry in the TLB (Translation Lookaside Buffer).
 */
void uTLB_RefillHandler() {

    // Get the exception state from the BIOS data page
    state_t *exception_state = (state_t *) BIOSDATAPAGE;
    
    // Extract the page number from entryHi
    int p = (exception_state->entry_hi & GETPAGENO) >> VPNSHIFT;
    if (p == 0x3FFFF) {
        p = 31;  // Adjust for special case of page number 0x3FFFF
    }

    // Retrieve the page table entry for convenience
    pteEntry_t *pgTblEntry = &(current_process->p_supportStruct->sup_privatePgTbl[p]);

    // Prepare to insert the page into the TLB
    setENTRYHI(pgTblEntry->pte_entryHI);
    setENTRYLO(pgTblEntry->pte_entryLO);

    // Insert the entry into the TLB
    TLBWR();

    // Return control to the exception state
    LDST(exception_state);
}

/**
 * @brief Implements the paging algorithm. Handles page faults by swapping pages
 *        between the memory and the backing store.
 */
void pager() {
    // Retrieve the support structure for the current process from the SSI
    support_t *support_PTR;
    ssi_payload_t payload = {
    .service_code = GETSUPPORTPTR,
    .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &support_PTR, 0);

    // Get the cause of the exception
    int exceptCause = support_PTR->sup_exceptState[PGFAULTEXCEPT].cause;

    // If the cause is TLB-Modification, handle it as a trap
    if ((exceptCause & GETEXECCODE) >> CAUSESHIFT == 1) {
        supportTrapHandler(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
    }
    else {
        // Ensure mutual exclusion on the swap pool by sending a message to the swap mutex process
        if (current_process != mutexHolderProcess) {
            SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);
            SYSCALL(RECEIVEMESSAGE, (unsigned int)swapMutexProcess, 0, 0);
        }

        // Extract the page number from entryHi
        int p = (support_PTR->sup_exceptState[PGFAULTEXCEPT].entry_hi & GETPAGENO) >> VPNSHIFT;
        if (p == 0x3FFFF) {
            p = 31;
        }

        // Select a frame to replace
        unsigned int i = selectFrame();

        // If the frame already contains a page, invalidate it
        if (swap_pool[i].swpo_asid != NOPROC) {
            invalidateFrame(i, support_PTR);
        }

        // Read the page from the backing store into the selected frame
        int blockToUpload = (swap_pool[i].swpo_pte_ptr->pte_entryHI & GETPAGENO) >> VPNSHIFT;
        if(blockToUpload == 0x3FFFF){
            blockToUpload = 31;
        }
        memaddr frameAddr = (memaddr)SWAP_POOL_AREA + (i * PAGESIZE);
        dtpreg_t *flashDevReg = (dtpreg_t *)DEV_REG_ADDR(FLASHINT, support_PTR->sup_asid - 1);
        int status = readWriteBackingStore(flashDevReg, frameAddr, p, FLASHREAD);
        
        // Handle failed read operation as a trap
        if (status != 1) {
            supportTrapHandler(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
        }
        
        // Update the swap pool entry
        swap_pool[i].swpo_asid = support_PTR->sup_asid;
        swap_pool[i].swpo_page = p;
        swap_pool[i].swpo_pte_ptr = &(support_PTR->sup_privatePgTbl[p]);

        // Disable interrupts
        setSTATUS(getSTATUS() & (~IECON));

        // Update the page table entry for the process
        support_PTR->sup_privatePgTbl[p].pte_entryLO |= VALIDON;
        support_PTR->sup_privatePgTbl[p].pte_entryLO |= DIRTYON;
        support_PTR->sup_privatePgTbl[p].pte_entryLO &= 0xFFF;
        support_PTR->sup_privatePgTbl[p].pte_entryLO |= (frameAddr);

        // Update the TLB with the new page table entry
        updateTLB(&support_PTR->sup_privatePgTbl[p]);

        // Re-enable interrupts
        setSTATUS(getSTATUS() | IECON);

        // Release the mutex
        SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);

        // Return control to the current process
        LDST(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
    }
}

/**
 * @brief Selects the frame to replace (FIFO - First In, First Out).
 * @return The index of the frame to replace
 */
static unsigned int selectFrame() {

    // Scan through all the frames in the swap pool
    for (int i = 0; i < POOLSIZE; i++) {

        // If there is an empty frame, return it
        if (swap_pool[i].swpo_asid == NOPROC)
            return i;
    }

    // Otherwise, return the index of the frame containing the oldest page
    static int last = -1;
    return last = (last + 1) % POOLSIZE;
}

/**
 * @brief Invalidates a page in the swap pool by clearing its valid bit.
 * @param frame The index of the frame to invalidate
 * @param support_PTR The pointer to the support structure of the current process
 */
void invalidateFrame(unsigned int frame, support_t *support_PTR){
    // Disable interrupts
    setSTATUS(getSTATUS() & (~IECON));

    // Clear the valid bit in the page table entry
    swap_pool[frame].swpo_pte_ptr->pte_entryLO &= (~VALIDON);

    // Update the TLB
    updateTLB(swap_pool[frame].swpo_pte_ptr);
    
    // Re-enable interrupts
    setSTATUS(getSTATUS() | IECON);

    // Update the backing store by writing the page back
    int blockToUpload = (swap_pool[frame].swpo_pte_ptr->pte_entryHI & GETPAGENO) >> VPNSHIFT;
    if(blockToUpload == 0x3FFFF){
        blockToUpload = 31;
    }
    memaddr frameAddr = (memaddr) SWAP_POOL_AREA + (frame * PAGESIZE);
    dtpreg_t *flashDevReg = (dtpreg_t *)DEV_REG_ADDR(FLASHINT, support_PTR->sup_asid - 1);
    int status = readWriteBackingStore(flashDevReg, frameAddr, blockToUpload, FLASHWRITE);

    // Handle failed write operation as a trap
    if (status != 1) {
        supportTrapHandler(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
    }
}

/**
 * @brief Updates a specific entry in the TLB.
 * @param entry The pointer to the page table entry to update in the TLB
 */
void updateTLB(pteEntry_t *entry) {

    // Set entryHi to the entryHi of the entry being updated
    setENTRYHI(entry->pte_entryHI);

    // Start a probe with TLBP(). If a corresponding entry is found in the TLB,
    // it will be indicated via the index
    TLBP();

    // If found, set entryLo to update the entry in the TLB
    if ((getINDEX() & PRESENTFLAG) == 0) {
        setENTRYHI(entry->pte_entryHI);
        setENTRYLO(entry->pte_entryLO);
        // Update both entryHi and entryLo of the TLB entry currently pointed to by the index
        TLBWI();
    }

}

/**
 * @brief Reads from or writes to the backing store (flash device).
 * 
 * @param flashDevReg   The device register of the flash device involved in the operation.
 * @param dataMemAddr   The starting memory address of the block in RAM involved in the operation.
 * @param devBlockNo    The block number on the flash device to be read or written.
 * @param opType        FLASHREAD: the block devBlockNo on the flash device will be read into dataMemAddr.
 *                      FLASHWRITE: the contents of dataMemAddr will be written to the block devBlockNo on the flash device.
 * 
 * @return              The result of the read or write operation.
 */
int readWriteBackingStore(dtpreg_t *flashDevReg, memaddr dataMemAddr, unsigned int devBlockNo, unsigned int opType) {
    // Load the data0 register of the flash device with the address of the memory block
    flashDevReg->data0 = dataMemAddr;

    // Prepare the structures for the SSI operation
    ssi_do_io_t doIO = {
        .commandAddr = (memaddr*)&flashDevReg->command,
        .commandValue = opType | (devBlockNo << 8)
    };
    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &doIO
    };

    // Send the message to the SSI
    unsigned int status;
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    return status;
}
