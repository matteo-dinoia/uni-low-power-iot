/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define UNUSED __attribute__((unused))
/*---------------------------------------------------------------------------*/
/* Application Configuration */
#define BROADCAST_CHANNEL 0xAA
#define UNICAST_CHANNEL 0xBB
#define BEACON_INTERVAL (5*CLOCK_SECOND)
#define MAX_HOPS 20
#define NBR_TABLE_SIZE 30
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS(button_process, "Button Process");
PROCESS(beacon_process, "Beacon Process");
AUTOSTART_PROCESSES(&beacon_process, &button_process); /* Start both processes when the node boots */
/*---------------------------------------------------------------------------*/
/* For the button_process (message forwarding) */
static struct unicast_conn uc_conn;
static void recv_uc(struct unicast_conn *c, const linkaddr_t *from);
static const struct unicast_callbacks uc_callbacks = {
	.recv     = recv_uc,
	.sent     = NULL,
};
/*---------------------------------------------------------------------------*/
/* For the beacon_process (neighbour discovery) */
static struct broadcast_conn bc_conn;
static void recv_bc(struct broadcast_conn *c, const linkaddr_t *from);
static const struct broadcast_callbacks bc_callbacks = {
	.recv     = recv_bc,
	.sent     = NULL,
};
/*---------------------------------------------------------------------------*/
/* "Helper" functions */
static void send_msg();
static bool get_random_neighbor(linkaddr_t* addr);
static void nbr_table_add(const linkaddr_t* addr);
/*---------------------------------------------------------------------------*/
/* Send a message in unicast upon button press */
PROCESS_THREAD(button_process, ev, UNUSED data)
{
	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_sensor);
	unicast_open(&uc_conn, UNICAST_CHANNEL, &uc_callbacks);

	while(1) {
		PROCESS_WAIT_EVENT();
		if(ev == sensors_event) {
			send_msg();
		}
	}
	PROCESS_END();
}

/* Periodically send beacon messages in broadcast */
PROCESS_THREAD(beacon_process, ev, UNUSED data)
{
	static struct etimer et;
	PROCESS_BEGIN();
	printf("Node Link Layer Address: %X:%X\n",
		linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

	broadcast_open(&bc_conn, BROADCAST_CHANNEL, &bc_callbacks);

	while(1) {
		etimer_set(&et, BEACON_INTERVAL);
		PROCESS_WAIT_EVENT();
		if(ev == PROCESS_EVENT_TIMER && etimer_expired(&et)) {
			packetbuf_clear();
			broadcast_send(&bc_conn);
			/* We just send an empty packet in broadcast... Why?!
			 * NB: We rely on the identified sender broadcast primitive of RIME.
			 * Therefore, RIME will add the sender address as an attribute to our payload.
			 */
		}
	}
	PROCESS_END();
}

/* ################## Message format definition: ##################
 *
 *   octet |  0  |  1  |   2  |   3  |   4  |
 *         +-----+-----+------+------+------+-
 *   field |   seqn    | hops |  string  ...
 *         +-----+-----+------+------+------+-
 *    size |  2 octets |1 oct.| variable size
 *
 * Numbers *should be* stored with network byte order (big-endian)
 *
 * ################################################################
 */

static void send_msg() {
	uint8_t* payload;
	static uint16_t seqn = 0; /* Increase this number by 1 upon every button press */
	uint8_t hops = 0;
	char str[] = "Hello from Matteo";
	size_t str_length = strlen(str);

	packetbuf_clear(); // Always clear before use!
	payload = packetbuf_dataptr();

	/* TODO 1: SERIALIZE THE DATA TO BE SENT!
	 * 1. Fill in the seqn in the message payload
	 * 2. Fill in the hops in the message payload
	 * 3. Fill in the string in the message payload (Hint: think about what a string is!)
	 * 4. Specify the size of the message to be sent (use packetbuf_set_datalen())
	 * 5. Increase the seqn by 1
	 */
	payload[0] = seqn >> 8;
	payload[1] = seqn;

	payload[2] = hops;
	memcpy(&payload[3], str, str_length);
	packetbuf_set_datalen(str_length + 3);

	linkaddr_t nexthop;
	if (get_random_neighbor(&nexthop)) {
		printf("Send to %02x:%02x\n", nexthop.u8[0], nexthop.u8[1]);
		unicast_send(&uc_conn, &nexthop);
	}
	else
		printf("No neighbors to send packet to \n");
}

static void
recv_uc(UNUSED struct unicast_conn *c, const linkaddr_t *from)
{
	uint8_t* payload = packetbuf_dataptr();
	uint16_t seqn;
	uint8_t hops;

	/* TODO 2: DESERIALIZE THE RECEIVED MESSAGE!
	 * 1. Extract the seqn field
	 * 2. Extract the hops field
	 * 3. Extract the string to a variable called str
	 * (If you have problems, check what you have done in Lab 2!)
	 */


	seqn = (uint16_t) payload[0] << 8;
	seqn |= (uint16_t) payload[1];

	hops = payload[2];

	const size_t str_length = packetbuf_datalen() - 3;
	char str[str_length + 1];
	memcpy(str, &payload[3], str_length);
	str[str_length] = '\0';

	printf("Recv from %02x:%02x length=%u seqn=%u hopcount=%u\n",
		from->u8[0], from->u8[1], packetbuf_datalen(), seqn, hops);
	printf("%s\n", str);
	free(str);

	if (hops < MAX_HOPS) {
		/* Forward the message with the incremented hopcount */
		hops += 1;
		payload[2] = hops;
		linkaddr_t nexthop;
		if (get_random_neighbor(&nexthop)) {
			printf("Forward to %02x:%02x\n", nexthop.u8[0], nexthop.u8[1]);
			unicast_send(&uc_conn, &nexthop);
		}
		else
			printf("No neighbors to forward packet to\n");
	}
}

static void recv_bc(UNUSED struct broadcast_conn *c, const linkaddr_t *from) {
	printf("Beacon from %02x:%02x\n", from->u8[0], from->u8[1]);
	nbr_table_add(from);
}

/*---------------------------------------------------------------------------*/
linkaddr_t nbr_table[NBR_TABLE_SIZE]; // Simplistic neighbor table (just an array of addresses)
int cur_nbr_idx = 0;                  // Current index in the neighbor table

static void nbr_table_add(const linkaddr_t* addr) {
	int i;
	// Check if the neighbor is already present in the table. If yes, do nothing.
	for (i=0; i<NBR_TABLE_SIZE; i++) {
		if (linkaddr_cmp(addr, &nbr_table[i]))
			return;
	}
	nbr_table[cur_nbr_idx] = *addr; // Store the new neighbor, potentially in place of an old one
	cur_nbr_idx = (cur_nbr_idx+1) % NBR_TABLE_SIZE; // Increment and wrap around
}

static bool get_random_neighbor(linkaddr_t* addr) {
	int num_nbr = 0;
	int i;

	// Count neighbors
	for (i=0; i<NBR_TABLE_SIZE; i++) {
		if (nbr_table[i].u8[0] !=0 || nbr_table[i].u8[1] !=0)
			num_nbr ++;
	}

	if (num_nbr == 0) // No neighbors
		return false;

	// Choose a random neighbor
	int idx = random_rand() % num_nbr;
	memcpy(addr, &nbr_table[idx], sizeof(linkaddr_t));
	return true;
}
/*---------------------------------------------------------------------------*/

