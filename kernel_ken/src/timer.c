// timer.c -- Initialises the PIT, and handles clock updates.
//            Written for JamesM's kernel development tutorials.

#include "timer.h"
#include "isr.h"
#include "monitor.h"
#include "task.h"

uint32_t tick = 0;
uint32_t freq = 0;

uint32_t sleep_to_tick(unsigned int seconds){
  if(seconds > 60*60*24*30*12) return 0xFFFFFFFF;
  return tick + ((uint32_t)seconds * freq);
}

int can_wake(uint32_t end_tick){
  return tick >= end_tick;
}

static void timer_callback(registers_t *regs)
{
    tick++;
    int res = WAITING;
    while(res == WAITING){
      res = task_switch();
      if(res == WAITING)
        asm volatile("hlt");
    }
}

void init_timer(uint32_t frequency)
{
    // Firstly, register our timer callback.
    register_interrupt_handler(IRQ0, &timer_callback);
    if(!frequency) return;
    freq = frequency;

    // The value we send to the PIT is the value to divide it's input clock
    // (processor dependant (1193180 in tutorial) Hz) by,
    // to get our required frequency. Important to note is
    // that the divisor must be small enough to fit into 16-bits.
    uint32_t divisor = 1193180 / frequency;

    // Send the command byte.
    outb(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    outb(0x40, l);
    outb(0x40, h);
}
