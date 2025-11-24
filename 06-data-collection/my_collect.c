#include <stdbool.h>
#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
#include "my_collect.h"
/*---------------------------------------------------------------------------*/
#define BEACON_INITIAL_INTERVAL (CLOCK_SECOND * 1)
#define BEACON_INTERVAL (CLOCK_SECOND * 6000) /* Time the sink should wait before rebuilding the tree from scratch. \
											 * [Lab 7] Try to change this period to analyse                       \
											 * how it affects the radio-on time (i.e., energy                     \
											 * consumption) of you solution and ContikiMac.                       \
											 */
#define BEACON_FORWARD_DELAY (random_rand() % CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
#define RSSI_THRESHOLD -100 // Links with RSSI < RSSI_THRESHOLD should be neglected!
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
void beacon_timer_cb(void *ptr);
/*---------------------------------------------------------------------------*/
/* Initilization of Rime broadcast and unicast callback structures */
struct broadcast_callbacks bc_cb = {
	.recv = bc_recv,
	.sent = NULL};
struct unicast_callbacks uc_cb = {
	.recv = uc_recv,
	.sent = NULL};
/*---------------------------------------------------------------------------*/
void my_collect_open(struct my_collect_conn *conn, uint16_t channels,
					 bool is_sink, const struct my_collect_callbacks *callbacks)
{
	/* TODO 1.1: Initialize the connection structure.
	 * 1. Set the parent address (suggestions:
	 *    - [logic] The node has not discovered its parent yet;
	 *    - [implementation] Check in contiki/core/net/linkaddr.h
	 *             how to copy a RIME address);
	 * 2. Check if the node is the sink;
	 * 3. Set the metric field (suggestion: the node is *not* connected yet, remember the node's
	 *    logic to accept or discard a beacon based on the metric field);
	 * 4. Set beacon_seqn (suggestion: no beacon has been received yet);
	 * 5. Set the callbacks field.
	 */

	//  = ; not important
	linkaddr_copy(&conn->parent, &linkaddr_null);
	conn->metric = 0;
	conn->is_sink = is_sink;
	conn->beacon_seqn = 0;
	conn->callbacks = callbacks;

	/* Open the underlying Rime primitives */
	broadcast_open(&conn->bc, channels, &bc_cb);
	unicast_open(&conn->uc, channels + 1, &uc_cb);

	/* TODO 1.2: SINK ONLY
	 * 1. Make the sink send beacons periodically to (re)build the tree.
	 *    (Tip 1: use the beacon_timer in my_collect_conn;
	 *     Tip 2: the beacon_timer_cb callback needs to access the connection object!
	 *            How can you pass a pointer to it?).
	 *    The FIRST time make the sink TX a beacon after 1 second, after that the sink
	 *    should send beacons with a period equal to BEACON_INTERVAL.
	 * 2. Does the sink need to change/update any field in the connection object
	 *    w.r.t. the value set for a non-sink node? (You may have already addressed
	 *    this in TODO 1.1.)
	 */

	if (is_sink)
	{
		ctimer_set(&conn->beacon_timer, BEACON_INITIAL_INTERVAL, &beacon_timer_cb, conn);
	}
}
/*---------------------------------------------------------------------------*/
/*                              Beacon Handling                              */
/*---------------------------------------------------------------------------*/
/* Beacon message structure */
struct beacon_msg
{
	uint16_t seqn;
	uint16_t metric;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn and metric */
void send_beacon(struct my_collect_conn *conn)
{
	/* Prepare the beacon message */
	struct beacon_msg beacon = {
		.seqn = conn->beacon_seqn, .metric = conn->metric};

	/* Send the beacon message in broadcast */
	packetbuf_clear();
	packetbuf_copyfrom(&beacon, sizeof(beacon));
	printf("my_collect: sending beacon: seqn %u metric %u\n",
		   conn->beacon_seqn, conn->metric);
	broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Beacon timer callback */
void beacon_timer_cb(void *ptr)
{
	struct my_collect_conn *conn = (struct my_collect_conn *)ptr;
	/* TODO 2: Implement the beacon callback.
	 * 1. Send a beacon in broadcast (use send_beacon());
	 * 2. Should the sink do anything else?
	 * 3. Think who will exploit this callback (only
	 *    the sink or also common nodes?).
	 */

	if (conn->is_sink)
	{
		conn->beacon_seqn++;
		send_beacon(conn);
		ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, &beacon_timer_cb, conn);
	}
	else
	{
		send_beacon(conn);
	}
}
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender)
{
	struct beacon_msg beacon;
	radio_value_t rssi;

	/* Get the pointer to the overall structure my_collect_conn from its field bc */
	struct my_collect_conn *conn = (struct my_collect_conn *)(((uint8_t *)bc_conn) -
															  offsetof(struct my_collect_conn, bc));

	/* Check if the received broadcast packet looks legitimate */
	if (packetbuf_datalen() != sizeof(struct beacon_msg))
	{
		printf("my_collect: broadcast of wrong size\n");
		return;
	}
	memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));

	/* TODO 3.0:
	 * Read the RSSI of the *last* reception
	 */
	NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);

	printf("my_collect: recv beacon from %02x:%02x seqn %u metric %u rssi %d\n",
		   sender->u8[0], sender->u8[1],
		   beacon.seqn, beacon.metric, rssi);

	/* TODO 3:
	 * 1. Analyze the received beacon; check: RSSI, seqn, and metric.
	 * 2. Update (if needed) the node's current routing info (parent, metric, beacon_seqn).
	 *    Tip: when you update the node's current routing info add a debug print, e.g.,
	 *         printf("my_collect: new parent %02x:%02x, my metric %d, my seqn %d\n",
	 *              sender->u8[0], sender->u8[1], conn->metric, conn->beacon_seqn);
	 */
	uint16_t lower_bound = conn->beacon_seqn - UINT16_MAX / 2;


	if ((lower_bound < beacon.seqn && beacon.seqn < conn->beacon_seqn) // low < new_beacon < curr_beacon
			|| (lower_bound > conn->beacon_seqn &&  lower_bound < beacon.seqn) // new_beacon < curr_beacon or lower_bound{wrapped} < new_beacon
			|| rssi < RSSI_THRESHOLD)
	{
		// Old beacon or bad message discarded
		return;
	}

	if (beacon.seqn != conn->beacon_seqn)
	{
		// New beacon accepted
		conn->beacon_seqn = beacon.seqn;
	}
	else if (beacon.metric + 1 >= conn->metric)
	{
		// Ignore worst connection
		return;
	}

	linkaddr_copy(&conn->parent, sender);
	conn->metric = beacon.metric + 1;

	/* TODO 4:
	 * IF you have received a new beacon or a beacon with a better metric send, *after a small, random
	 * delay* (BEACON_FORWARD_DELAY), a beacon in broadcast to update the neighbouring nodes about the
	 * changes.
	 * Tip: you can use the beacon_timer in my_collect_conn.
	 */
	ctimer_set(&conn->beacon_timer, BEACON_FORWARD_DELAY, &beacon_timer_cb, conn);
}
/*---------------------------------------------------------------------------*/
/*                     Data Handling --- LAB 7                               */
/*---------------------------------------------------------------------------*/
/* Header structure for data packets */
struct collect_header
{
	linkaddr_t source;
	uint8_t hops;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Data Collection: send function */
int my_collect_send(struct my_collect_conn *conn)
{
	printf("DEBUG REQ TO SEND\n");
	/* TODO 5:
	 * 1. Check if the node is connected (has a parent), IF NOT return -1;
	 * 2. If possible, allocate space for the data collection header. If this is
	 *    not possible, return -2;
	 * 3. Prepare and insert the header in the packet buffer;
	 *    Tip: The Rime address of a node is stored in linkaddr_node_addr!
	 *         (check contiki/core/net/linkaddr.h for additional details);
	 * 4. Send the packet to the parent using unicast and return the status
	 *    of unicast_send() to the application.
	 */
	if (linkaddr_cmp(&conn->parent, &linkaddr_null)){
		return -1; // If no parent
	}

	int success = packetbuf_hdralloc(sizeof(struct collect_header));
	if (success == 0){
		return -2; // Could not allocate memory
	}

	struct collect_header *header = packetbuf_hdrptr();
	header->hops = 0;
	linkaddr_set_node_addr(&header->source);


	printf("DEBUG REQ TO SEND DONE %X:%X\n", conn->parent.u8[0], conn->parent.u8[1]);
	return unicast_send(&conn->uc, &conn->parent);
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from)
{

	/* Get the pointer to the overall structure my_collect_conn from its field uc */
	struct my_collect_conn *conn = (struct my_collect_conn *)(((uint8_t *)uc_conn) -
															  offsetof(struct my_collect_conn, uc));

	struct collect_header hdr;
	printf("DEBUG Received\n");

	/* Check if the received unicast message looks legitimate */
	if (packetbuf_datalen() < sizeof(struct collect_header))
	{
		printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
		return;
	}

	/* TODO 6:
	 * 1. Extract the header;
	 * 2. On the sink, remove the header and call the application callback;
	 *    [TBC] - Should we update any field of hdr?
	 *          - What about packetbuf_dataptr() and packetbuf_hdrptr()? Does the
	 *            application recv callback rely on any of them? Should we take any action?
	 * 3. On a forwarder, update the header and forward the packet to the parent (IF ANY)
	 *    using unicast.
	 */
	struct collect_header *header = packetbuf_dataptr();
	header->hops++;
	printf("DEBUG Received %d %X:%X\n", header->hops, header->source.u8[0], header->source.u8[1]);

	// Sink pass to callback and return
	if(conn->is_sink){
		printf("DEBUG Delivered %d %X:%X\n", header->hops, header->source.u8[0], header->source.u8[1]);
		packetbuf_hdrreduce(sizeof(struct collect_header));
		conn->callbacks->recv(&header->source, header->hops);
		return;
	}

	if(linkaddr_cmp(&conn->parent, &linkaddr_null)){
		// Shouldn't happen -> message is lost
		return;
	}

	printf("DEBUG Passed %d %X:%X\n", header->hops, header->source.u8[0], header->source.u8[1]);
	unicast_send(&conn->uc, &conn->parent);
}
/*---------------------------------------------------------------------------*/
