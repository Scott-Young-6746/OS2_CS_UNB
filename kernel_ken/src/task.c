//
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"
#include "descriptor_tables.h"
#include "kheap.h"
#include "common.h"
#include "timer.h"

#define MAX_TASK 16

#define VALID_TASK(id) ((id >= 0 && id<MAX_TASK))

// The currently running task.
volatile task_t *current_task = 0;
volatile uint32_t ring_index = 0;
volatile task_t tasks[16];
volatile uint32_t pid_is_used;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern void alloc_frame(page_t*,int,int);
extern uint32_t initial_esp;
extern uint32_t read_eip();
extern void perform_task_switch(uint32_t, uint32_t, uint32_t, uint32_t);

#define is_usd_pid(pid) ((VALID_TASK(pid)) ? (pid_is_used & (0x1<<pid)) : 0 )

int fucking_sleep(unsigned int seconds){
  __asm__ volatile("cli");
  uint32_t end_tick = sleep_to_tick(seconds);
  if(can_wake(end_tick)) return 0;
  current_task->status = WAITING;
  current_task->wake_tick = end_tick;
  __asm__ volatile("sti");
  int res = task_switch();
  while(res == WAITING){
    res = task_switch();
    if(res != RUNNING)
      __asm__ volatile("hlt");
  }
  return 0;
}

void queue(int pid){
  task_t* task = &tasks[pid-1];
  task->status = IN_SEM_QUEUE;
}

void dequeue(int pid){
  task_t* task = &tasks[pid-1];
  task->status = READY;
}

short is_used_pid(int pid)
{
  int result  = is_usd_pid(pid);
  if(!result) return 0;
  task_t* task = &tasks[pid];
  if( task->status == WAITING && can_wake(task->wake_tick)){
    task->status = READY;
    return 1;
  } else if(task->status == READY){
    return 1;
  }
  return 0;
}

int unset_pid(int pid)
{
    if(!VALID_TASK(pid))
      return 0;

    pid_is_used &= (~(0x1<<pid));
    return pid;
}

int set_pid(int pid)
{
  if(!VALID_TASK(pid))
    return 0;

  pid_is_used |= (0x1<<pid);
  return pid;
}

int find_ff_pid()
{
  int i;
  for(i=1; i<16; i++)
  {
    if(!is_used_pid(i))
      return i;
  }
  return 0;
}

void initialise_processes()
{
    // Rather important stuff happening, no interrupts please!
    __asm__ volatile("cli");

    // Relocate the stack so we know where it is.
    move_stack((void*)0xE0000000, 0x10000);
    // Init structures
    for(int i=0; i<16; i++){
      memset(&tasks[i], 0, sizeof(task_t));
    }

    // Initialise the first task (kernel task)
    current_task = &tasks[0];
    current_task->id = 0;
    current_task->page_directory = current_directory;
    current_task->starting_priority = 10;
    current_task->priority = 10;
    current_task->status = RUNNING;
    pid_is_used = 0x1;

    __asm__ volatile("mov %%esp, %0" : "=r"(current_task->esp));
    __asm__ volatile("mov %%ebp, %0" : "=r"(current_task->ebp));
    current_task->kernel_stack = 0xE0000000;
    set_kernel_stack(0xE0000000);

    // Reenable interrupts.
    __asm__ volatile("sti");
}

void move_stack(void *new_stack_start, uint32_t size)
{
  // Commented out old move stack, it's (maybe) causing a heisenbug.
  uint32_t i;
  // Allocate some space for the new stack.
  for( i = (uint32_t)new_stack_start;
       i >= ((uint32_t)new_stack_start-size);
       i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
    alloc_frame( get_page(i, 1, current_directory), 0 /* User mode */, 0 /* Is writable */ );
  }


  // Flush the TLB by reading and writing the page directory address again.
  uint32_t pd_addr;
  __asm__ volatile("mov %%cr3, %0" : "=r" (pd_addr));
  __asm__ volatile("mov %0, %%cr3" : : "r" (pd_addr));

  // Old ESP and EBP, read from registers.
  uint32_t old_stack_pointer; __asm__ volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  uint32_t old_base_pointer;  __asm__ volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  uint32_t offset            = (uint32_t)new_stack_start - initial_esp;

  // New ESP and EBP.
  uint32_t new_stack_pointer = old_stack_pointer + offset;
  uint32_t new_base_pointer  = old_base_pointer  + offset;

  // Copy the stack.
  memcpy((void*)new_stack_pointer, (void*)old_stack_pointer, initial_esp-old_stack_pointer);

  // Backtrace through the original stack, copying new values into
  // the new stack.
  for(i = (uint32_t)new_stack_start; i > (uint32_t)new_stack_start-size; i -= 4)
  {
    uint32_t tmp = * (uint32_t*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_stack_pointer < tmp) && (tmp < initial_esp))
    {
      tmp = tmp + offset;
      uint32_t *tmp2 = (uint32_t*)i;
      *tmp2 = tmp;
    }
  }

  // Change stacks.
  __asm__ volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
  __asm__ volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

int task_switch()
{
    // Rather important stuff happening, no interrupts please!
    __asm__ volatile("cli");

    // If we haven't initialised tasking yet, just return.
    if (!current_task){
      // Reenable interrupts
      __asm__ volatile("sti");
      return 0;
    }

    //If we are still the highest priority task we stay current and return
    if (current_task->status == RUNNING){
      int current_is_high = 1;
      for(int i=0; i<16; i++){
        if(!is_used_pid(i)){
          continue;
        } else if(i==current_task->id){
          continue;
        } else if (tasks[i].priority <= current_task->priority){
          current_is_high = 0;
          break;
        }
      }
      if(current_is_high){
        // Reenable interrupts.
        __asm__ volatile("sti");
        return current_task->status;
      }
    }

    // Read esp, ebp now for saving later on.
    uint32_t esp, ebp, eip;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));

    // Read the instruction pointer. We do some cunning logic here:
    // One of two things could have happened when this function exits -
    //   (a) We called the function and it returned the EIP as requested.
    //   (b) We have just switched tasks, and because the saved EIP is essentially
    //       the instruction after read_eip(), it will seem as if read_eip has just
    //       returned.
    // In the second case we need to return immediately. To detect it we put a dummy
    // value in EAX further down at the end of this function. As C returns values in EAX,
    // it will look like the return value is this dummy value! (0x12345).
    eip = read_eip();

    // Have we just switched tasks?
    if (eip == 0x12345){
      __asm__ volatile("sti");
      return current_task->status;
    }

    // No, we didn't switch tasks. Let's save some register values and switch.
    current_task->eip = eip;
    current_task->esp = esp;
    current_task->ebp = ebp;

    // Switch current task here.
    // If current task is RUNNING then it needs to be made READY
    if(current_task->status == RUNNING)
      current_task->status = READY;

    // Starting with first READY task after current,
    task_t* ready_task = 0;
    for(uint32_t i=ring_index+1; i<16; i++){
      if(!is_used_pid(i)) continue;
      if(tasks[i].status == READY){
        ready_task = &tasks[i];
        break;
      }
    }
    if(!ready_task){
      for(uint32_t i=0; i<=ring_index; i++){
        if(!is_used_pid(i)) continue;
        if(tasks[i].status == READY){
          ready_task = &tasks[i];
          break;
        }
      }
    }
    if(!ready_task){
      if(current_task->status == DEAD){
        for(uint32_t i=ring_index+1; i<16; i++){
          if(!is_usd_pid(i)) continue;
          ready_task = &tasks[i];
          break;
        }
        if(!ready_task){
          for(uint32_t i=0; i<=ring_index; i++){
            if(!is_usd_pid(i)) continue;
            ready_task = &tasks[i];
            break;
          }
        }
      } else {
        __asm__ volatile("sti");
        return current_task->status;
      }
    }

    // find the highest prio task,
    // where highest is strictly lower priority values
    // (1 higher than 10).
    // strict ensures that we get the first
    // task of that priority after the starting point
    uint32_t ready_id = ready_task->id;
    for(uint32_t i=ready_id+1; i<16; i++){
      if(!is_used_pid(i)) continue;
      if(tasks[i].priority < ready_task->priority){
        ready_task = &tasks[i];
        ring_index = i;
      }
    }
    for(uint32_t i=0; i<ready_id; i++){
      if(!is_used_pid(i)) continue;
      if(tasks[i].priority < ready_task->priority){
        ready_task = &tasks[i];
        ring_index = i;
      }
    }
    current_task->priority = current_task->starting_priority;
    current_task = ready_task;
    current_task->status = (current_task->status != READY) ?
                            current_task->status :
                            RUNNING;

    eip = current_task->eip;
    esp = current_task->esp;
    ebp = current_task->ebp;

    // Make sure the memory manager knows we've changed page directory.
    current_directory = current_task->page_directory;

    // Change our kernel stack over.
    set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);
    // Here we:
    // * Stop interrupts so we don't get interrupted.
    // * Temporarily put the new EIP location in ECX.
    // * Load the stack and base pointers from the new task struct.
    // * Change page directory to the physical address (physicalAddr) of the new directory.
    // * Put a dummy value (0x12345) in EAX so that above we can recognise that we've just
    //   switched task.
    // * Restart interrupts. The STI instruction has a delay - it doesn't take effect until after
    //   the next instruction.
    // * Jump to the location in ECX (remember we put the new EIP in there).
    perform_task_switch(eip, current_directory->physicalAddr, ebp, esp);
    __asm__ volatile("sti");
    return current_task->status;
}

int fork_proc()
{
    // We are modifying kernel structures, and so cannot be interrupted.
    __asm__ volatile("cli");

    // Take a pointer to this process' task struct for later reference.
    task_t *parent_task = current_task;

    // Clone the address space.
    page_directory_t *directory = clone_directory(current_directory);

    // Create a new process.
    // Get next available pid
    int next_pid =  find_ff_pid();
    if(!next_pid)
      return -1; // ?? do more here!!!
    set_pid(next_pid);

    task_t* new_task = &tasks[next_pid];
    new_task->id = next_pid;
    new_task->esp = new_task->ebp = 0;
    new_task->eip = 0;
    new_task->page_directory = directory;
    new_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    new_task->starting_priority = parent_task->starting_priority;
    new_task->priority = parent_task->priority;
    new_task->status = READY;
    //current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    //^^ was previous line, I changed the current_task to new task.
    // Note to self: if shit breaks, come back to this.

    // This will be the entry point for the new process.
    uint32_t eip = read_eip();

    // We could be the parent or the child here - check.
    if (current_task == parent_task)
    {
        // We are the parent, so set up the esp/ebp/eip for our child.
        uint32_t esp; __asm__ volatile("mov %%esp, %0" : "=r"(esp));
        uint32_t ebp; __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
        new_task->esp = esp;
        new_task->ebp = ebp;
        new_task->eip = eip;
        // All finished: Reenable interrupts.

        // And by convention return the PID of the child.
        task_switch();
    }
    else
    {
        // We are the child - by convention return 0.
        next_pid = -1;
    }
    __asm__ volatile("sti");
    return next_pid+1;
}

void task_exit(){
  if(current_task->id == 0) return;
  unset_pid(current_task->id);
  current_task->status = DEAD;
  int res = task_switch();
}

int get_pid()
{
    return current_task->id+1;
}

int set_task_prio(int pid, int prio){
  if(!VALID_TASK(pid-1))
    return 0;
  tasks[pid-1].priority = prio;
  tasks[pid-1].starting_priority = prio;
  return prio;
}
