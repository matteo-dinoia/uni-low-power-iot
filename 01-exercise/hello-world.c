#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> 
#include <stdbool.h>

#define PERIOD CLOCK_SECOND
/*--------------------------------------------------------------*/
// Declare a process and list it to start at boot
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);

// Implement the process thread function
PROCESS_THREAD(hello_world_process, ev, data)
{ 
  // Timer object
  static struct etimer timer; // ALWAYS use static variables in processes!


  PROCESS_BEGIN();            // All processes should start with PROCESS_BEGIN()

  while (1) {
    // Set a timer and wait for expiration
    etimer_set(&timer, PERIOD);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));


    printf("Hello, world! I am 0x%04x\n", node_id);

    // Toggle red and green LEDs
    leds_toggle(LEDS_RED|LEDS_GREEN); 
    // See also leds_on(), leds_off()
  }
  
  PROCESS_END();              // All processes should end with PROCESS_END()
}
/*---------------------------------------------------------------------------*/
