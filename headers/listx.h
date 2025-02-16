/* Subset of the Linux Kernel source file: "include/linux/list.h" CPLv2 */
#ifndef LISTX_H_INCLUDED
#define LISTX_H_INCLUDED

#ifndef NULL
#define NULL ((void *)0)
#endif
typedef unsigned int size_tt;

/*
    Macro that retrieves a pointer to the structure instance containing
    a specific list_head field. It does this by using the pointer to list_head,
    the type of the structure that contains it, and the name of the field.

    __mptr is simply a copy of ptr with the same type as the member field;
    the address of the structure is then obtained by subtracting the offset
    of the field from __mptr using the offsetof macro.

    ptr: Pointer to the list_head within the data structure.
    type: The type of the structure containing list_head.
    member: Name of the list_head field in the structure.

    return: Pointer to the structure containing the list_head referenced by ptr.
*/
#define container_of(ptr, type, member)                                                                                \
    ({                                                                                                                 \
        const typeof(((type *)0)->member) *__mptr = (ptr);                                                             \
        (type *)((char *)__mptr - offsetof(type, member));                                                             \
    })

/*
    Macro that returns the byte offset of a field from the beginning
    of a structure.

    It assumes a structure of type TYPE allocated at address 0
    and determines the offset by inspecting the field's address.
    If the structure starts at address 0, the field's offset matches its address.

    TYPE: Structure type containing the field.
    MEMBER: Name of the field.

    return: Offset in bytes from the start of TYPE to MEMBER.
*/
#define offsetof(TYPE, MEMBER) ((size_tt)(&((TYPE *)0)->MEMBER))

/*
    The list_head structure is a simple pair of pointers used to implement
    a doubly linked list. To create linked lists of arbitrary structures,
    include a list_head field within them.
*/
struct list_head {
    struct list_head *next, *prev;
};

/*
    Macro that initializes an empty list by setting both pointers
    to reference itself.

    name: Name of the list variable to initialize.

    return: A list_head structure initialized as empty.

    Example:
    struct list_head my_list = LIST_HEAD_INIT(my_list);
*/
#define LIST_HEAD_INIT(name)                                                                                           \
    { &(name), &(name) }

/*
    Macro that declares and initializes a new list. Unlike LIST_HEAD_INIT,
    this macro also declares the variable.

    name: Name of the list variable to be declared.
*/
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/*
    Inline function that initializes a list as empty by setting both
    pointers to reference itself. 

    Unlike LIST_HEAD_INIT, which creates an anonymous structure,
    INIT_LIST_HEAD initializes the fields of an existing structure.

    list: Pointer to the list to initialize.

    return: void
*/
static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

/*
    Inserts a new element between prev and next.

    new: Element to insert.
    prev: Element that should precede new.
    next: Element that should follow new.

    return: void
*/
static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
    next->prev = new;
    new->next  = next;
    new->prev  = prev;
    prev->next = new;
}

/*
    Inserts a new element at the head of the list.

    new: Element to insert.
    head: List in which to insert new.

    return: void
*/
static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

/*
    Inserts a new element at the tail of the list.

    new: Element to insert.
    head: List in which to insert new.

    return: void
*/
static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

/*
    Removes the elements between prev and next by linking them directly.

    prev: Start of the removal range.
    next: End of the removal range.

    return: void
*/
static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

/*
    Removes an element from the list it belongs to.

    entry: Element to remove.

    return: void
*/
static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
}

/*
    Checks if the list has reached the last element.

    list: Element of the list.
    head: Head of the list.

    return: 0 if list is not the last element, 1 otherwise.
*/
static inline int list_is_last(const struct list_head *list, const struct list_head *head) {
    return list->next == head;
}

/*
    Checks if the list is empty.

    head: List to check.

    return: 1 if the list is empty, 0 otherwise.
*/
static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

/*
    Returns the next element in the list.

    current: Current list element.

    return: current->next if the list is not empty, NULL otherwise.
*/
static inline struct list_head *list_next(const struct list_head *current) {
    if (list_empty(current))
        return NULL;
    else
        return current->next;
}

/*
    Returns the previous element in the list.

    current: Current list element.

    return: current->prev if the list is not empty, NULL otherwise.
*/
static inline struct list_head *list_prev(const struct list_head *current) {
    if (list_empty(current))
        return NULL;
    else
        return current->prev;
}

/*
    Macro that creates a for-loop to iterate over each element
    in a list starting from head.

    In each iteration, the variable pos points to the list_head field
    of an element in the list, proceeding in order.

    Example:
    struct list_head* iter;
    list_for_each(iter, &head) {
        my_struct_t* item = container_of(iter, my_struct_t, list);
        printf("Element: %d\n", item->value);
    }

    pos: Pointer used to iterate over the list.
    head: Start of the list (sentinel element).
*/
#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

/*
    Macro similar to list_for_each but iterates in reverse order.

    pos: Pointer used to iterate over the list.
    head: Start of the list (sentinel element).
*/
#define list_for_each_prev(pos, head) for (pos = (head)->prev; pos != (head); pos = pos->prev)

/*
    Macro that creates a for-loop to iterate over the content
    of a list. Unlike list_for_each, the pos variable points
    to the structure containing the list_head field instead of
    the field itself.

    pos: Pointer used to iterate over the list's content.
    head: Start of the list (sentinel element).
    member: Name of the list_head field inside the structure.
*/
#define list_for_each_entry(pos, head, member)                                                                         \
    for (pos = container_of((head)->next, typeof(*pos), member); &pos->member != (head);                               \
         pos = container_of(pos->member.next, typeof(*pos), member))

/*
    Macro similar to list_for_each_entry but iterates in reverse order.

    pos: Pointer used to iterate over the list's content.
    head: Start of the list (sentinel element).
    member: Name of the list_head field inside the structure.
*/
#define list_for_each_entry_reverse(pos, head, member)                                                                 \
    for (pos = container_of((head)->prev, typeof(*pos), member); &pos->member != (head);                               \
         pos = container_of(pos->member.prev, typeof(*pos), member))

#endif
