/*
 * Author: strommach
 * Date: 16th November 2026
 * mslist.h - v0.0.1: Single-header C99 heap-allocated circular XOR linked list.
 *
 * Overview:
 *   Dynamic doubly linked list using XOR-encoded neighbor pointers to store
 *   both prev and next links in a single pointer field per node. Nodes and
 *   list headers are heap-allocated via malloc().
 *
 * API:
 *   new_node               - allocate a standalone list node holding data
 *   XOR                    - compute XOR of two node pointers
 *   linked_list_init       - allocate and initialize an empty list
 *   linked_list_push_front - insert data at the front of the list (TODO)
 *   linked_list_push_back  - insert data at the back of the list (TODO)
 *   linked_list_insert_at  - insert data at index idx (TODO)
 *   linked_list_delete     - remove first node matching data (TODO)
 *   linked_list_pop_front  - remove and return the front element
 *   linked_list_pop_back   - remove and return the back element (TODO)
 *   linked_list_destroy    - free all nodes and the list header
 */

#ifndef MSLIST_H
#define MSLIST_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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

struct ListNode;
typedef struct ListNode list_node_t;

struct ListNode {
  void *data;
  list_node_t *link; /* XOR of previous and next node pointers */
};

struct LinkedList;
typedef struct LinkedList linked_list_t;

struct LinkedList {
  list_node_t *head;
  list_node_t *tail;
  size_t size;
};

/* Allocates a new list node holding data, or returns NULL on OOM. */
MSH_INLINE list_node_t *new_node(void *data) {
  list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
  if (!node) {
    return NULL;
  }
  node->data = data;
  node->link = NULL;
  return node;
}

/* Computes the bitwise XOR of two node pointers. */
MSH_INLINE list_node_t *XOR(list_node_t *a, list_node_t *b) {
  return (list_node_t *)((uintptr_t)(a) ^ (uintptr_t)(b));
}

/* Allocates and initializes an empty linked list, or returns NULL on OOM. */
MSH_INLINE linked_list_t *linked_list_init(void) {
  linked_list_t *list = (linked_list_t *)malloc(sizeof(linked_list_t));
  if (!list) {
    return NULL;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;

  return list;
}

/* Inserts data at the front of the list. (Unimplemented) */
MSH_INLINE void linked_list_push_front(linked_list_t *list, void *data) {
  /* TODO */
  (void)list;
  (void)data;
}

/* Inserts data at the back of the list. (Unimplemented) */
MSH_INLINE void linked_list_push_back(linked_list_t *list, void *data) {
  /* TODO */
  (void)list;
  (void)data;
}

/* Inserts data at position idx in the list. (Unimplemented) */
MSH_INLINE void linked_list_insert_at(linked_list_t *list, void *data,
                                      size_t idx) {
  /* TODO */
  (void)list;
  (void)data;
  (void)idx;
}

/* Deletes the first node matching data from the list. (Unimplemented) */
MSH_INLINE void linked_list_delete(linked_list_t *list, void *data) {
  /* TODO */
  (void)list;
  (void)data;
}

/* Removes and returns the front element, or NULL if the list is empty. */
MSH_INLINE void *linked_list_pop_front(linked_list_t *list) {
  if (!list || list->size == 0) {
    return NULL;
  }

  list_node_t *curr = list->head;
  list_node_t *prev = list->tail;
  list_node_t *next = XOR(curr->link, list->tail);
  next->link = XOR(prev->link, NULL);

  return next;
}

/* Removes and returns the back element, or NULL if empty. (Unimplemented) */
MSH_INLINE void *linked_list_pop_back(linked_list_t *list) {
  /* TODO */
  (void)list;
  return NULL;
}

/* Frees all nodes in the list and the list header itself. */
MSH_INLINE void linked_list_destroy(linked_list_t *list) {
  if (!list) {
    return;
  }

  list_node_t *curr = list->head;
  list_node_t *prev = list->tail;
  list_node_t *next;

  while (curr != NULL) {
    next = XOR(curr->link, prev);
    free(prev);
    prev = curr;
    curr = next;
  }

  free(list);
}

#ifdef __cplusplus
}
#endif

#endif /* MSLIST_H */
