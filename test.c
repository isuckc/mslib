#include "msilist.h"
#include "msvector.h"
#include <stdio.h>

struct task {
  int id;
  char *name;
  ilist_node link;
};

int main(void) {

  // Intrusive Doubly Linked list
  // ilist_node queue = {0};
  // ilist_init(&queue);
  //
  // struct task t1 = {.id = 0, .name = "Hello world!"};
  // struct task t2 = {.id = 1, .name = "This is task 2"};
  //
  // ilist_init(&t1.link);
  // ilist_init(&t2.link);
  //
  // ilist_push_back(&queue, &t1.link);
  // ilist_push_back(&queue, &t2.link);
  //
  // ilist_node *it;
  // ILIST_FOR_EACH(it, &queue) {
  //   struct task *t = ILIST_ENTRY(it, struct task, link);
  //   printf("Task todo:\n\tId: %d\n\tName: %s\n", t->id, t->name);
  // }
  //
  // struct task *todo = ILIST_ENTRY(ilist_pop_back(&queue), struct task, link);
  //
  // puts("Task to do next in the queue: ");
  // printf("%d: %s\n", todo->id, todo->name);
  //
  // ILIST_FOR_EACH(it, &queue) {
  //   struct task *t = ILIST_ENTRY(it, struct task, link);
  //   printf("Tasks Left todo:\n\tId: %d\n\tName: %s\n", t->id, t->name);
  // }

  float *v1 = msvec_new(32);
  float *v2 = msvec_new(32);

  // Vector 1 { 10, 20, 30 }
  msvec_push(v1, 10);
  msvec_push(v1, 20);
  msvec_push(v1, 30);

  // Vector 2 { 2, 3, 4 }
  msvec_push(v2, 2);
  msvec_push(v2, 3);
  msvec_push(v2, 4);

  float *result = msvec_new(32);
  msvec_add(v1, v2, result);
  float total = msvec_mul(v1, v2);

  puts("Testing vector operations!\n");

  printf("Vector 1: { %f, %f, %f }\n", v1[0], v1[1], v1[2]);
  printf("Vector 2: { %f, %f, %f }\n", v2[0], v2[1], v2[2]);

  printf("Result vector of the sum: { %f, %f, %f }\n", result[0], result[1],
         result[2]);

  printf("Total of the multiplication: %f\n", total);

  msvec_free(v1);
  msvec_free(v2);
  msvec_free(result);

  return 0;
}
