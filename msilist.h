/*
 * Author: neurd
 * Date: 16th November 2026
 * MSList Header: A Mini and Simple List Implementation
 * This is a single header library for using lists on your utilities and
 * projects
 *
 * This particular implementations suppose that the user allows an intrusive
 * pattern in his application, otherwise, it is advised to NOT use this
 * implementation
 *
 * Intrusive circular doubly linked list node
 * Embed this inside your payload struct, and use a standalone instance
 * as the sentinel head of a list.
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

typedef struct IListNode ilist_node;

typedef struct IListNode {
  ilist_node *next, *prev;
} ilist_node;

MSH_INLINE void ilist_init(ilist_node *node) {
  node->next = node;
  node->prev = node;
}

MSH_INLINE int ilist_is_empty(const ilist_node *head) {
  return head->next == NULL;
}

/* A basic primitive for implementing the following functions*/
MSH_INLINE void ilist_insert_between(ilist_node *node, ilist_node *prev,
                                     ilist_node *next) {
  node->prev = prev;
  node->next = next;
  prev->next = node;
  next->prev = node;
}

MSH_INLINE void ilist_push_front(ilist_node *head, ilist_node *node) {
  ilist_insert_between(node, head, head->next);
}

MSH_INLINE void ilist_push_back(ilist_node *head, ilist_node *node) {
  ilist_insert_between(node, head->prev, head);
}

/* Unlinks a node and set it unlinked */
MSH_INLINE void ilist_remove(ilist_node *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  ilist_init(node);
}

MSH_INLINE ilist_node *ilist_pop_front(ilist_node *head) {
  if (ilist_is_empty(head)) {
    return NULL;
  }

  ilist_node *first = head->next;
  ilist_remove(first);
  return first;
}

MSH_INLINE ilist_node *ilist_pop_back(ilist_node *head) {
  if (ilist_is_empty(head)) {
    return NULL;
  }

  ilist_node *last = head->prev;
  ilist_remove(last);
  return last;
}

#define ILIST_ENTRY(ptr, type, member)                                         \
  ((type *)((char *)(ptr) - offsetof(type, member)))

#define ILIST_FOR_EACH(pos, head)                                              \
  for (ilist_node * (pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

#define ILIST_FOR_EACH_SAFE(pos, tmp, head)                                    \
  for (ilist_node * (pos) = (head)->next; *(tmp) != (pos)->next;               \
       (pos) != (head); (pos) = (tmp), (tmp) = (pos)->next)

#ifdef __cplusplus
}
#endif

#endif // MSILIST_H
