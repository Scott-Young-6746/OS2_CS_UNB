#ifndef SEMAPHORES_H
#define SEMAPHORES_H

typedef struct semaphore {
  int sid;
  int value;
  int queue_size;
  int queue[16];
} semaphore_t;

void initialise_semaphores();
int internal_open_sem(int n);
int internal_wait(int s);
int internal_signal(int s);
int internal_close_sem(int s);
#endif
