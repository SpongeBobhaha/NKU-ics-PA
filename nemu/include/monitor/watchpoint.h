#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

  int oldValue;
  char e[32];
  int hitNum;
} WP;

bool new_wp(char *arg);
bool free_wp(int num);
void print_wp();
bool watch_wp();
#endif
