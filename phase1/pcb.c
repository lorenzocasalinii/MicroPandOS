#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];  // Array of process control blocks
LIST_HEAD(pcbFree_h);  // Head of the free process list
static int next_pid = 1;  // PID counter for new processes

/**
 * @brief Initializes the free process list by adding all PCB entries to it.
 */
void initPcbs() {
    for(int i = 0; i < MAXPROC; i++){
        list_add(&pcbTable[i].p_list, &pcbFree_h);
    }
}

/**
 * @brief Adds a process back to the free process list.
 * 
 * @param p Pointer to the process to be freed
 */
void freePcb(pcb_t *p) {
    list_add_tail(&(p->p_list), &pcbFree_h);
}

/**
 * @brief Allocates a new process if available, initializes its values,
 *        and removes it from the free process list.
 * 
 * @return Pointer to the allocated PCB, or NULL if none are available.
 */
pcb_t *allocPcb() {
    if(list_empty(&pcbFree_h))
        return NULL;  // No available PCBs
    else{
        pcb_PTR tempPcb = container_of(pcbFree_h.next, pcb_t, p_list); // Get first free PCB
        list_del(pcbFree_h.next);  // Remove from free list
        INIT_LIST_HEAD(&tempPcb->p_list);  // Initialize process list pointers
        INIT_LIST_HEAD(&tempPcb->p_child);
        INIT_LIST_HEAD(&tempPcb->p_sib);
        INIT_LIST_HEAD(&tempPcb->msg_inbox);
        tempPcb->p_parent = NULL;
        tempPcb->p_time = 0;
        tempPcb->p_supportStruct = NULL;
        tempPcb->p_pid = next_pid++;  // Assign and increment PID
        tempPcb->p_s.status = ALLOFF;  // Set process status to default
        return tempPcb;
    }
}

/**
 * @brief Initializes a process queue sentinel node.
 * 
 * @param head Pointer to the sentinel node of the queue
 */
void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

/**
 * @brief Checks if the process queue is empty.
 * 
 * @param head Pointer to the queue sentinel node
 * @return 1 if the queue is empty, 0 otherwise
 */
int emptyProcQ(struct list_head *head) {
    return list_empty(head);
}

/**
 * @brief Inserts a process at the end of the process queue.
 * 
 * @param head Pointer to the queue sentinel node
 * @param p Pointer to the process to insert
 */
void insertProcQ(struct list_head *head, pcb_t *p) {
    list_add_tail(&p->p_list, head);
}

/**
 * @brief Returns the first process in the process queue without removing it.
 * 
 * @param head Pointer to the queue sentinel node
 * @return Pointer to the first process, or NULL if empty
 */
pcb_t *headProcQ(struct list_head *head) {
    if(emptyProcQ(head))
        return NULL;
    else
        return container_of(head->next, pcb_t, p_list);
}

/**
 * @brief Removes and returns the first process from the process queue.
 * 
 * @param head Pointer to the queue sentinel node
 * @return Pointer to the removed process, or NULL if the queue is empty
 */
pcb_t *removeProcQ(struct list_head *head) {
    if(emptyProcQ(head))
        return NULL;
    else {
        pcb_PTR temp = headProcQ(head);
        list_del(&temp->p_list);
        return temp;
    }
}

/**
 * @brief Removes a specific process from the process queue.
 * 
 * @param head Pointer to the queue sentinel node
 * @param p Pointer to the process to remove
 * @return Pointer to the removed process, or NULL if not found
 */
pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
    pcb_PTR temp;
    list_for_each_entry(temp, head, p_list) {
        if(temp == p) {
            list_del(&p->p_list);
            return p;
        }
    }
    return NULL;
}

/**
 * @brief Checks if a given PCB is in the free process list.
 * 
 * @param p Pointer to the PCB to check
 * @return 1 if found, 0 otherwise
 */
int isInPCBFree_h(pcb_t *p) {
    pcb_PTR iter;
    list_for_each_entry(iter, &pcbFree_h, p_list) {
        if(iter == p) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks if a PCB is part of a specific list.
 * 
 * @param head Pointer to the list sentinel node
 * @param p Pointer to the PCB to check
 * @return 1 if found, 0 otherwise
 */
int isInList(struct list_head *head, pcb_t *p) {
    pcb_PTR iter;
    list_for_each_entry(iter, head, p_list) {
        if(iter == p) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks if a process has no child processes.
 * 
 * @param p Pointer to the process to check
 * @return 1 if it has no children, 0 otherwise
 */
int emptyChild(pcb_t *p) {
    return list_empty(&p->p_child) ? 1 : 0;
}

/**
 * @brief Inserts a process as a child of another process.
 * 
 * @param prnt Pointer to the parent process
 * @param p Pointer to the child process
 */
void insertChild(pcb_t *prnt, pcb_t *p) {
    p->p_parent = prnt;
    list_add_tail(&p->p_sib, &prnt->p_child);
}

/**
 * @brief Removes and returns the first child of a given process.
 * 
 * @param p Pointer to the parent process
 * @return Pointer to the removed child, or NULL if no children exist
 */
pcb_t *removeChild(pcb_t *p) {
    if (!list_empty(&p->p_child)){
        pcb_PTR tempPcb = container_of(p->p_child.next, pcb_t, p_sib);
        tempPcb->p_parent = NULL;
        list_del(&tempPcb->p_sib);
        return tempPcb;
    }
    else
        return NULL;
}

/**
 * @brief Detaches a process from its parent and returns it.
 * 
 * @param p Pointer to the process to detach
 * @return Pointer to the detached process, or NULL if it has no parent
 */
pcb_t *outChild(pcb_t *p) {
    if (p->p_parent != NULL){
        list_del(&p->p_sib);
        p->p_parent = NULL;
        return p;
    }
    else
        return NULL;
}
