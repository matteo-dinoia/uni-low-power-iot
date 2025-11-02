/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	/* TODO:
	* - Get the string from the received message
	* - Print the sender address and the received message
	*   Expected print: Recv from XX:XX - Message: 'Hello Word!"
	*/
	const int len = packetbuf_datalen();
	char msg[len];

	memcpy(msg, packetbuf_dataptr(), len);

	printf("Recv from  %02X:%02X - Message: '%s'\n", from->u8[0], from->u8[1], msg);

}

static void
broadcast_sent(struct broadcast_conn *c, int status, int num_tx)
{
	printf("INFO: %d %d\n", status, num_tx);
}

/* TODO - STEP 2: (To be implemented *ONLY when* all other TODOs work as expected!)
* - Implement the sent callback function (broadcast_sent): print status and number of TX
*/

static const struct broadcast_callbacks broadcast_cb = {
	// TODO: Assign pointers to your callback functions
	.recv = broadcast_recv,
	.sent = broadcast_sent
};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
	static struct etimer et;

	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_sensor);

	/* TODO:
	* - Initialize the Rime broadcast primitive
	*/
	broadcast_open(&broadcast, 666, &broadcast_cb);

	/* Print node link layer address */
	printf("Node Link Layer Address: %02X:%02X\n",
		linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);


	/* Delay 5-8 seconds */
	etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));

	static char msg1[]="Hello world!";
	static char msg2[]="Hello Matteo!";
	static char *msg_selected = msg1;

	while(1) {


		/* Wait for the timer to expire */
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) || data==&button_sensor);

		if(etimer_expired(&et)){

			packetbuf_copyfrom(msg_selected, strlen(msg_selected) + 1);
			broadcast_send(&broadcast);
			/* Delay 5-8 seconds */
			etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));
		}else{
			if(msg_selected == msg1){
				msg_selected = msg2;
			} else {
				msg_selected = msg1;
			}
		}

		/* TODO:
		* - Create a text message
		* - Fill the message in the packet buffer
		* - Send!
		*/

	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
