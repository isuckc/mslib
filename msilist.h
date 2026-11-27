/*
 * Author: strommach
 * Date: 16th November 2026
 * msilist.h - v0.0.1: Single-header C99 intrusive circular doubly linked list.
 *
 * Usage:
 *   Embed an `ilist_node` inside your payload struct and keep a standalone
 *   `ilist_node` initialized with ilist_init() as the sentinel list head.
 *   Use ILIST_ENTRY() to recover the enclosing struct from a node pointer.
 *
 * Example:
 *   typedef struct { int value; ilist_node link; } Item;
 *
 *   ilist_node head;
 *   ilist_init(&head);
 *   Item a = { .value = 42 };
 *   ilist_push_back(&head, &a.link);
 *
 *   ILIST_FOR_EACH(pos, &head) {
 *     Item *it = ILIST_ENTRY(pos, Item, link);
 *     printf("%d\n", it->value);
 *   }
 *
 * API:
 *   ilist_init           - initialize a sentinel head or standalone node
 *   ilist_is_empty       - returns 1 if list has no elements, 0 otherwise
 *   ilist_insert_between - low-level primitive inserting between prev & next
 *   ilist_push_front     - insert node at the front of the list
 *   ilist_push_back      - insert node at the back of the list
 *   ilist_remove         - unlink a node and re-initialize it as standalone
 *   ilist_pop_front      - unlink and return the front node (or NULL)
 *   ilist_pop_back       - unlink and return the back node (or NULL)
 *   ILIST_ENTRY          - recover parent struct pointer from an ilist_node *
 *   ILIST_FOR_EACH       - iterate over all nodes in the list
 *   ILIST_FOR_EACH_SAFE  - iterate while safely removing nodes
 */

#ifndef MSILIST_H
#define MSILIST_H

#include <stddef.h>

#ifndef MSH_INLINE
#if defined(__cplusplus)
#define MSH_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSH_INLINE static inline __attribute__((unused))
#else
#define MSH_INLINE static inline
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct IListNode;
typedef struct IListNode ilist_node;

struct IListNode {
  ilist_node *next;
  ilist_node *prev;
};

/* Initializes a node (or sentinel head) so it points to itself. */
MSH_INLINE void ilist_init(ilist_node *node) {
  if (!node) {
    return;
  }
  node->next = node;
  node->prev = node;
}

/* Returns 1 if the list is empty (or head is NULL), 0 otherwise. */
MSH_INLINE int ilist_is_empty(const ilist_node *head) {
  return !head || head->next == head;
}

/* Inserts node between two adjacent nodes prev and next. */
MSH_INLINE void ilist_insert_between(ilist_node *node, ilist_node *prev,
                                     ilist_node *next) {
  node->prev = prev;
  node->next = next;
  prev->next = node;
  next->prev = node;
}

/* Inserts node at the front of the list (immediately after head). */
MSH_INLINE void ilist_push_front(ilist_node *head, ilist_node *node) {
  if (!head || !node) {
    return;
  }
  ilist_insert_between(node, head, head->next);
}

/* Inserts node at the back of the list (immediately before head). */
MSH_INLINE void ilist_push_back(ilist_node *head, ilist_node *node) {
  if (!head || !node) {
    return;
  }
  ilist_insert_between(node, head->prev, head);
}

/* Unlinks node from its current list and resets it to a standalone state. */
MSH_INLINE void ilist_remove(ilist_node *node) {
  if (!node) {
    return;
  }
  node->prev->next = node->next;
  node->next->prev = node->prev;
  ilist_init(node);
}

/* Unlinks and returns the first node in the list, or NULL if empty. */
MSH_INLINE ilist_node *ilist_pop_front(ilist_node *head) {
  if (ilist_is_empty(head)) {
    return NULL;
  }

  ilist_node *first = head->next;
  ilist_remove(first);
  return first;
}

/* Unlinks and returns the last node in the list, or NULL if empty. */
MSH_INLINE ilist_node *ilist_pop_back(ilist_node *head) {
  if (ilist_is_empty(head)) {
    return NULL;
  }

  ilist_node *last = head->prev;
  ilist_remove(last);
  return last;
}

/* Recovers a pointer to the enclosing struct containing member at ptr. */
#define ILIST_ENTRY(ptr, type, member)                                         \
  ((type *)((char *)(ptr) - offsetof(type, member)))

/* Iterates over every node in head (do not unlink pos during iteration). */
#define ILIST_FOR_EACH(pos, head)                                              \
  for (ilist_node *(pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

/* Iterates over every node in head, safe against removing pos during loop. */
#define ILIST_FOR_EACH_SAFE(pos, tmp, head)                                    \
  for (ilist_node *(pos) = (head)->next, *(tmp) = (pos)->next;                 \
       (pos) != (head); (pos) = (tmp), (tmp) = (pos)->next)

#ifdef __cplusplus
}
#endif

#endif /* MSILIST_H */
