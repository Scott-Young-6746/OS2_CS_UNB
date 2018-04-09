//
// task.h - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "paging.h"

#define KERNEL_STACK_SIZE 2048       // Use a 2kb kernel stack.

#define DEAD    0
#define WAITING 1
#define RUNNING 2
#define READY   3

// This structure defines a 'task' - a process.
typedef struct task
{
    int id;                // Process ID.
    uint32_t esp, ebp;       // Stack and base pointers.
    uint32_t eip;            // Instruction pointer.
    page_directory_t *page_directory; // Page directory.
    uint32_t kernel_stack;   // Kernel stack location.
    int32_t starting_priority;
    int32_t priority;
    int32_t status;
} task_t;

// Initialises the tasking system.
void initialise_processes();

// Called by the timer hook, this changes the running process.
void task_switch();

void switch_to_user_mode();

// Forks the current process, spawning a new one with a different
// memory space.
int fork_proc();

void task_exit();

int set_task_prio(int pid, int prio);

// Causes the current process' stack to be forcibly moved to a new location.
void move_stack(void *new_stack_start, uint32_t size);

// Returns the pid of the current process.
int get_pid();

#endif
