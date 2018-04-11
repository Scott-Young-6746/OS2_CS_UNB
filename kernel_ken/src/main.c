// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "multiboot.h"
#include "task.h"
#include "syscall.h"
#include "kernel_ken.h"
#include "semaphores.h"
#include "pipes.h"
#include "app.h"

extern uint32_t placement_address;

uint32_t initial_esp;

int kernel_main(struct multiboot *mboot_ptr, uint32_t initial_stack)
{
    initial_esp = initial_stack;
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();
    // Initialise the screen (by clearing it)
    monitor_clear();

    // Initialise the PIT to 100Hz
    __asm__ volatile("sti");

    // Start paging.
    initialise_paging();

    // Start multitasking.
    initialise_tasking();
    init_timer(50);

    // Start semaphores.
    initialise_semaphores();

    // Start IPC
    initialise_pipes();

    my_app();

    return 0;
}
