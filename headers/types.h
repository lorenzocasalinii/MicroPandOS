#ifndef PANDOS_TYPES_H_INCLUDED
#define PANDOS_TYPES_H_INCLUDED

/****************************************************************************
 *
 * This header file contains utility types definitions.
 *
 ****************************************************************************/

#include <umps/types.h>
#include "./const.h"
#include "./listx.h"

typedef signed int cpu_t;   // Represents CPU time
typedef unsigned int memaddr; // Represents a memory address

/* Page Table Entry descriptor */
typedef struct pteEntry_t {
    unsigned int pte_entryHI; // High part of the PTE entry 
    unsigned int pte_entryLO; // Low part of the PTE entry 
} pteEntry_t;

/* Swap pool entry descriptor */
typedef struct swpo_t {
    int swpo_asid;              // Address Space ID (ASID) of the owning process
    unsigned int swpo_page;     // Virtual Page Number (VPN)
    pteEntry_t *swpo_pte_ptr;   // Pointer to the corresponding PTE in the process's page table
} swpo_t;

/* Support-level context for exception handling */
typedef struct context_t {
    unsigned int stackPtr; // Stack pointer
    unsigned int status;   // Processor status
    unsigned int pc;       // Program counter
} context_t;


/* Support-level descriptor */
typedef struct support_t {
    int        sup_asid;                        // Process ID (ASID)
    state_t    sup_exceptState[2];              // Old exception states
    context_t  sup_exceptContext[2];            // New contexts for handling exceptions
    pteEntry_t sup_privatePgTbl[USERPGTBLSIZE]; // User-level private page table
    struct list_head s_list;                    // Linked list entry for support structures
} support_t;


/* Process Control Block (PCB) descriptor */
typedef struct pcb_t {
    /* Process queue linkage */
    struct list_head p_list; // Linked list node for queue management

    /* Process tree fields */
    struct pcb_t *p_parent;   // Pointer to parent process
    struct list_head p_child; // Head of the child process list
    struct list_head p_sib;   // Linked list node for sibling processes

    /* Process execution state */
    state_t p_s;  // Processor state
    cpu_t p_time; // CPU time used by the process

    /* Message queue for inter-process communication */
    struct list_head msg_inbox; // Head of the message queue

    /* Pointer to the support structure (if any) */
    support_t *p_supportStruct;

    /* Process ID */
    int p_pid;
} pcb_t, *pcb_PTR;


/* Message descriptor for inter-process communication */
typedef struct msg_t {
    struct list_head m_list;  // Linked list node for message queue
    struct pcb_t *m_sender;   // Pointer to the sender process
    unsigned int m_payload;   // Message payload 
} msg_t, *msg_PTR;

/* Payload structure for SSI messages */
typedef struct ssi_payload_t {
    int service_code; // Service request code
    void *arg;        // Pointer to additional arguments 
} ssi_payload_t, *ssi_payload_PTR;

/* SSI structure for process creation requests */
typedef struct ssi_create_process_t {
    state_t *state;     // Initial processor state of the new process
    support_t *support; // Pointer to the support structure 
} ssi_create_process_t, *ssi_create_process_PTR;

/* SSI structure for I/O operations */
typedef struct ssi_do_io_t {
    memaddr* commandAddr;    // Address of the I/O command register
    unsigned int commandValue; // Value to write to the I/O register
} ssi_do_io_t, *ssi_do_io_PTR;

/* SSI structure for printing requests */
typedef struct sst_print_t {
    int length;  // Length of the string
    char *string; // Pointer to the string to print
} sst_print_t, *sst_print_PTR;

/* Swap pool information structure */
typedef struct swap_t {
    int         sw_asid;   // ASID number of the process using the swap entry
    int         sw_pageNo; // Virtual page number being swapped
    pteEntry_t *sw_pte;    // Pointer to the corresponding PTE in the page table
} swap_t;

#endif
