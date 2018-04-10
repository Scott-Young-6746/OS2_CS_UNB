// timer.h -- Defines the interface for all PIT-related functions.
//            Written for JamesM's kernel development tutorials.

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

void init_timer(uint32_t frequency);
uint32_t sleep_to_tick(unsigned int seconds);
int can_wake(uint32_t end_tick);

#endif
