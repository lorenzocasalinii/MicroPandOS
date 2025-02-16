#include "./headers/msg.h"

static msg_t msgTable[MAXMESSAGES];  // Table holding all message structures
LIST_HEAD(msgFree_h);  // Head of the free message list

/**
 * @brief Initializes the list of unused messages.
 *        This function adds all messages from msgTable to the free list.
 */
void initMsgs() {
  for (int i = 0; i < MAXMESSAGES; i++) {
    list_add(&msgTable[i].m_list, &msgFree_h);
  }
}

/**
 * @brief Frees a message and returns it to the free message list.
 * 
 * @param m Pointer to the message to be freed
 */
void freeMsg(msg_t *m) {
  m->m_sender = NULL;  // Reset sender information
  m->m_payload = 0;    // Clear message content
  list_add_tail(&(m->m_list), &msgFree_h);  // Add message back to free list
}

/**
 * @brief Allocates an empty message.
 * 
 * @return Pointer to the allocated message if available, NULL otherwise.
 */
msg_t *allocMsg() {
  if (list_empty(&msgFree_h)) {
    return NULL;  // No available messages in the free list
  } else {
    msg_PTR m = container_of(msgFree_h.next, msg_t, m_list); // Get first free message
    list_del(msgFree_h.next); // Remove it from the free list
    INIT_LIST_HEAD(&m->m_list); // Initialize its list pointer
    m->m_sender = NULL; // Reset sender
    m->m_payload = 0; // Clear message content
    return m;
  }
}

/**
 * @brief Initializes a sentinel node for a message queue.
 * 
 * @param head Pointer to the sentinel node of the list to initialize
 */
void mkEmptyMessageQ(struct list_head *head) {
  INIT_LIST_HEAD(head);
}

/**
 * @brief Checks if the message queue is empty.
 * 
 * @param head Pointer to the sentinel node of the list to check
 * @return 1 if the queue is empty, 0 otherwise
 */
int emptyMessageQ(struct list_head *head) {
  return list_empty(head);
}

/**
 * @brief Inserts a message at the end of the message queue.
 * 
 * @param head Pointer to the sentinel node of the list
 * @param m Pointer to the message to insert
 */
void insertMessage(struct list_head *head, msg_t *m) {
  list_add_tail(&m->m_list, head);
}

/**
 * @brief Pushes a message to the front of the message queue.
 * 
 * @param head Pointer to the sentinel node of the list
 * @param m Pointer to the message to insert
 */
void pushMessage(struct list_head *head, msg_t *m) {
  list_add(&m->m_list, head);
}

/**
 * @brief Removes the first message in the queue sent by p_ptr.
 *        If p_ptr is NULL, removes the first message in the queue.
 * 
 * @param head Pointer to the sentinel node of the list
 * @param p_ptr Pointer to the sender PCB to match (or NULL to remove any)
 * @return Pointer to the removed message if found, NULL otherwise
 */
msg_t *popMessage(struct list_head *head, pcb_t *p_ptr) {
  if (list_empty(head)) {
    return NULL; // Queue is empty
  } else {
    if (p_ptr == NULL) {  
      // Remove the first message in the queue
      msg_PTR m = container_of(list_next(head), msg_t, m_list);
      list_del(list_next(head));
      return m;
    } else {
      msg_PTR i = NULL;
      // Iterate through the list to find a message from the given sender
      list_for_each_entry(i, head, m_list) {
        if (i->m_sender == p_ptr) {
          list_del(&i->m_list);
          return i;
        }
      }
      return NULL; // No matching message found
    }
  }
}

/**
 * @brief Retrieves the first message in the message queue without removing it.
 * 
 * @param head Pointer to the sentinel node of the list
 * @return Pointer to the first message in the queue, or NULL if empty
 */
msg_t *headMessage(struct list_head *head) {
  if (list_empty(head)) {
    return NULL;
  } else {
    return container_of(list_next(head), msg_t, m_list);
  }
}
