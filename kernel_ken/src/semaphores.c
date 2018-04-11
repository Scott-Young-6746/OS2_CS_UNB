#include "common.h"
#include "semaphores.h"
#include "task.h"

#define VALID_SEM(sid) (sid >= 1 && sid <= 32)
#define sid_index(sid) (sid-1)

semaphore_t semaphores[32];
uint32_t semaphores_used = 0;

int sem_is_set(int sid){
  if(VALID_SEM(sid))
    return semaphores_used & (uint32_t)(1<<sid_index(sid));
  return 0;
}

void set_semaphore(int sid){
  if(VALID_SEM(sid))
    semaphores_used |= (uint32_t)(1<<sid_index(sid));
}

void unset_semaphore(int sid){
  if(VALID_SEM(sid))
    semaphores_used &= ((uint32_t)0xffffffff - (uint32_t)(1<<sid_index(sid)));
}

int get_first_unset_sem(){
  int i;
  for(i=1; i<=32; i++){
    if(!sem_is_set(i))
      return i;
  }
  return 0;
}

int pop_queue(int sid){
  if(!VALID_SEM(sid)) return 0;
  semaphore_t* sem = &semaphores[sid];
  int to_pop = sem->queue[0];
  int i;
  for(i=1; i<sem->queue_size; i++){
    sem->queue[i-1] = sem->queue[i];
  }
}

semaphore_t new_semaphore(int sid, int c){
  semaphore_t sem;
  sem.sid = sid;
  sem.value = c;
  sem.queue_size = 0;
  int i;
  for(i=0; i<16; i++)
    sem.queue[i] = -1;
  return sem;
}

void initialise_semaphores(){

}

int internal_open_sem(int n){
  __asm__ volatile("cli");
  int sid = get_first_unset_sem();
  if(sid){
    semaphores[sid_index(sid)] = new_semaphore(sid, n);
    set_semaphore(sid);
  }
  __asm__ volatile("sti");
  return sid;
}

int internal_close_sem(int s){
  __asm__ volatile("cli");
  int sid = 0;
  if(VALID_SEM(s)){
    sid = s;
    set_semaphore(sid);
  }
  __asm__ volatile("sti");
  return sid;
}

int internal_wait(int s){
  __asm__ volatile("cli");
  int sid = 0;
  if(VALID_SEM(s) && sem_is_set(s)){
    sid = s;
    semaphore_t* sem = &semaphores[sid_index(sid)];
    sem->value--;
    if(sem<1){
      int pid = get_pid();
      queue(pid);
    }
  }
  __asm__ volatile("sti");
  task_switch();
  return sid;
}

int internal_signal(int s){
  __asm__ volatile("cli");
  int sid = 0;
  if(VALID_SEM(s) && sem_is_set(s)){
    sid = s;
    semaphore_t* sem = &semaphores[sid_index(sid)];
    if(sem<1){
      int pid = pop_queue(sid);
      dequeue(pid);
    }
    sem->value++;
  }
  __asm__ volatile("sti");
  task_switch();
  return sid;
}
