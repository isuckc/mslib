/*
 * Author: neurd
 * Date: 16th November 2026
 *
 * MSList: a Mini and Simple List Implementation
 *
 * This implementation of dynamic lists are with circular xor linked lists.
 * We used malloc for this one, so expect to use heap allocation for this
 * implementation
 */

#ifndef MSLIST_H
#define MSLIST_H

#include <stddef.h>
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

typedef struct ListNode list_node_t;

typedef struct ListNode {
  void *data;
  list_node_t *link;
} list_node_t;

MSH_INLINE list_node_t *new_node(void *data) {
  list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));

  new_node->data = data;
  new_node->link = NULL;

  return new_node;
}

typedef struct LinkedList linked_list_t;

typedef struct LinkedList {
  list_node_t *head;
  list_node_t *tail;
  size_t size;
} linked_list_t;

MSH_INLINE list_node_t *XOR(list_node_t *a, list_node_t *b) {
  return (list_node_t *)((uintptr_t)(a) ^ (uintptr_t)(b));
}

MSH_INLINE linked_list_t *linked_list_init() {
  linked_list_t *new_list = (linked_list_t *)malloc(sizeof(linked_list_t));
  if (new_list == NULL) {
    return NULL;
  }

  new_list->head = NULL;
  new_list->tail = NULL;
  new_list->size = 0;

  return new_list;
}

MSH_INLINE void linked_list_push_front(linked_list_t *list, void *data) {}

MSH_INLINE void linked_list_push_back(linked_list_t *list, void *data) {}

MSH_INLINE void linked_list_insert_at(linked_list_t *list, void *data,
                                      size_t idx) {}

MSH_INLINE void linked_list_delete(linked_list_t *list, void *data) {}

MSH_INLINE void *linked_list_pop_front(linked_list_t *list) {
  if (list == NULL)
    return NULL;

  if (list->size <= 0)
    return NULL;

  list_node_t *curr = list->head;
  list_node_t *prev = list->tail;
  list_node_t *next;

  next = XOR(curr->link, list->tail);
  next->link = XOR(prev->link, NULL);

  return next;
}

MSH_INLINE void *linked_list_pop_back(linked_list_t *list) {}

MSH_INLINE void linked_list_destroy(linked_list_t *list) {
  if (list == NULL)
    return;

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

#endif // MSLIST_H
