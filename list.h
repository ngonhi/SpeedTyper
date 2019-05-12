#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#ifndef __LIST_H__
#define __LIST_H__

typedef struct node {
  pthread_t thread;
  struct node* next;
} node_t;

typedef struct linkedlist {
  pthread_mutex_t lst_m;
  node_t * first;
  int length;
} linkedlist_t;

node_t * createNode(pthread_t thread);

linkedlist_t * listInit();

void addNode(linkedlist_t* list, pthread_t thread);

void destroyList(linkedlist_t* list);

#endif
