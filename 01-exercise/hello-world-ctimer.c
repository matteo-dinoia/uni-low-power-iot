#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> 

#define PERIOD CLOCK_SECOND

static struct ctimer timer;

static void timer_callback(void* ptr) {
  printf("Hello, world! I am 0x%04x (callback timer)\n", node_id);
  leds_toggle(LEDS_RED|LEDS_GREEN);
  ctimer_set(&timer, PERIOD, timer_callback, ptr); 
  // set the same timer again
}

/*--------------------------------------------------------------*/
// Declare a process and list it to start at boot
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);

/*--------------------------------------------------------------*/
// Implement the process thread function
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  
  // Set the callback timer
  ctimer_set(&timer, PERIOD, timer_callback, NULL);
  
  PROCESS_END();
}