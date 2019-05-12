#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "list.h"

node_t * createNode(pthread_t thread) {
  node_t* cur = (node_t*) malloc(sizeof(node_t));
  cur->thread = thread;
  cur->next = NULL;
  return cur;
}

linkedlist_t * listInit() {
  linkedlist_t* cur = (linkedlist_t*) malloc(sizeof(linkedlist_t));
  cur->first = NULL;
  cur->length = 0;
  pthread_mutex_init(&(cur->lst_m), NULL);
  return cur;
}

void addNode(linkedlist_t* list, pthread_t thread) {
  pthread_mutex_lock(&list->lst_m);
  node_t* node = createNode(thread);
  if (list->first == NULL)
    list->first = node;
  else {
    node_t* cur = list->first;
    while (cur->next != NULL) {
      cur = cur->next;
    }

    cur->next = node;
  }
  list->length++;
  pthread_mutex_unlock(&list->lst_m);
}

void destroyList(linkedlist_t* list) {
  pthread_mutex_lock(&list->lst_m);
  node_t* cur = list->first;
  while(cur != NULL) {
    node_t * temp = cur;
    cur = cur->next;
    free(temp);
  }
  pthread_mutex_unlock(&list->lst_m);
  free(list);
}

