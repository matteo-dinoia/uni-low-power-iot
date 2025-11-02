/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "sys/node-id.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* Application Configuration */
#define UNICAST_CHANNEL 146
#define APP_TIMER_DELAY (CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2))
/*---------------------------------------------------------------------------*/
typedef struct ping_pong_msg {
	uint16_t sequence_number;
	int16_t noise_floor;
}
__attribute__((packed))
ping_pong_msg_t;
/*---------------------------------------------------------------------------*/
static linkaddr_t receiver = {{0xAA, 0xBB}};
/*---------------------------------------------------------------------------*/
static uint16_t ping_pong_number = 0; // You can use it to keep track of the current ping-pong sequence number.
/*---------------------------------------------------------------------------*/
process_event_t app_event;
/*---------------------------------------------------------------------------*/
/* Declaration of static functions */
static void print_rf_conf(void);
static void set_ping_pong_msg(ping_pong_msg_t *m);
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from);
static void sent_unicast(struct unicast_conn *c, int status, int num_tx);
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks uc_callbacks = {
	.recv     = recv_unicast,
	.sent     = sent_unicast
};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS(unicast_process, "Unicast Process");
AUTOSTART_PROCESSES(&unicast_process);
/*---------------------------------------------------------------------------*/
static void
print_rf_conf(void)
{
	radio_value_t rfval = 0;

	printf("RF Configuration:\n");
	NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &rfval);
	printf("\tRF Channel: %d\n", rfval);
	NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &rfval);
	printf("\tTX Power: %ddBm\n", rfval);
	NETSTACK_RADIO.get_value(RADIO_PARAM_CCA_THRESHOLD, &rfval);
	printf("\tCCA Threshold: %d\n", rfval);
}
/*---------------------------------------------------------------------------*/
static void
set_ping_pong_msg(ping_pong_msg_t *m)
{
	/* Use this function to set the values in the ping-pong message struct. This can
	 * also be done directly in the ct_cb function. */
	radio_value_t noise;
	NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &noise);
	m->noise_floor = noise;
	m->sequence_number = ++ping_pong_number;
}
/*---------------------------------------------------------------------------*/
static void
app_send_msg(void)
{
	ping_pong_msg_t msg; /* Message struct to be sent in unicast */

	/* TO DO: Send the message to the receiver.
	 * 1. Build the message to be sent using the set_ping_pong_msg function.
	 *    This can be also done directly in this function, if preferred
	 * 2. Prepare the packetbuf
	 * 3. Send the packet to the receiver
	 */
	set_ping_pong_msg(&msg);
	packetbuf_copyfrom(&msg, sizeof(msg));
	unicast_send(&uc, &receiver);
	printf("SENT\n");
}
/*---------------------------------------------------------------------------*/
static void
recv_unicast(struct unicast_conn *c, const linkaddr_t *from)
{
	/* Local variables */
	ping_pong_msg_t msg;
	radio_value_t rssi;

	/* TO DO:
	 * 1. Copy the received message from the packetbuf to the msg struct
	 * 2. [OPTIONAL] Get the RSSI measurement from the received packet
	 * 3. Print the received ping pong number and noise floor, together with the RSSI
	 * 4. Think about how you can make the ping-pong message exchange proceed
	 */

	packetbuf_copyto(&msg);
	radio_value_t noise_pkt;
	NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &noise_pkt);
	printf("Node received %d/%d [seq: %d]\n", msg.noise_floor, noise_pkt, msg.sequence_number);
	ping_pong_number++;
	process_post(&unicast_process, app_event, NULL);
}
/*---------------------------------------------------------------------------*/
static void
sent_unicast(struct unicast_conn *c, int status, int num_tx)
{
	const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
	if(linkaddr_cmp(dest, &linkaddr_null)) {
		return;
	}
	/* TO DO [OPTIONAL]:
	 * Print some info about the packet sent, e.g., ping-pong number, num_tx, etc.
	 */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data)
{
	static struct etimer et;

	PROCESS_BEGIN();

// #if CONTIKI_TARGET_SKY
	if(node_id == 1) {
		receiver.u8[0] = 0x02;
		receiver.u8[1] = 0x00;
	} else if(node_id == 2) {
		receiver.u8[0] = 0x01;
		receiver.u8[1] = 0x00;
	}
// #endif

	/* Print RF configuration */
	print_rf_conf();

	/* Print the node link layer address */
	printf("Node Link Layer Address: %02X:%02X\n",
		linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

	/* Print the receiver link layer address */
	printf("Node: receiver address %02X:%02X\n",
		receiver.u8[0], receiver.u8[1]);

	/* Print the size of the ping_pong_msg_t struct */
	printf("Sizeof struct: %u\n", sizeof(ping_pong_msg_t));


	/* TODO:
	 * Open the unicast connection using the defined uc_callbacks structure
	 */
	unicast_open(&uc, UNICAST_CHANNEL, &uc_callbacks);

	/* TODO:
	 * Set the etimer
	 */
	app_event = process_alloc_event();

	if(node_id == 1)
		etimer_set(&et, APP_TIMER_DELAY);

	/* TODO:
	 * Think about how you can exploit app_event. This event might be helpful
	 * to notify the process that a message has been received and therefore it is
	 * the node turn to increase the ping-pong number and send the message.
	 */

	while(1) {
		/* Wait for an event */
		PROCESS_WAIT_EVENT();
		if(ev == PROCESS_EVENT_TIMER && etimer_expired(&et)){
			app_send_msg();
		}else if(ev == app_event) {
			etimer_set(&et, APP_TIMER_DELAY);
		}


		/* TODO:
		 * Think how you can properly handle *and exploit* the etimer and app_event events.
		 * IF you are lost, check the hint on the slide.
		 */
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
