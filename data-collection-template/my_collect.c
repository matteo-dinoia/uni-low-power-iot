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
#define BEACON_INTERVAL (CLOCK_SECOND * 60) /* Amount of time the sink should wait before 
                                             * rebuilding the tree from scratch.
                                             * [Lab 7] Try to change this period and analyse
                                             * how it affects the radio-on time (i.e., energy
                                             * consumption) of your solution with ContikiMac.
                                             */ 
#define BEACON_FORWARD_DELAY (random_rand() % CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
#define RSSI_THRESHOLD -95 // Links with RSSI < RSSI_THRESHOLD should be neglected!
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender); 
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);        
void beacon_timer_cb(void* ptr);                                     
/*---------------------------------------------------------------------------*/
/* Initilization of Rime broadcast and unicast callback structures */
struct broadcast_callbacks bc_cb = {
  .recv = bc_recv,
  .sent = NULL
};
struct unicast_callbacks uc_cb = {
  .recv = uc_recv,
  .sent = NULL
};
/*---------------------------------------------------------------------------*/
void
my_collect_open(struct my_collect_conn* conn, uint16_t channels, 
                bool is_sink, const struct my_collect_callbacks *callbacks)
{
  /* TODO 1.1: Initialize the connection structure.
   * 1. Set the parent address (suggestions: 
   *                            - [logic] The node has not discovered its parent yet;
   *                            - [implementation] Check in contiki/core/net/linkaddr.h 
   *                                               how to copy a RIME address);
   * 2. Check if the node is the sink;
   * 3. Set the metric field (suggestion: the node is *not* connected yet, remember the node's
   *    logic to accept or discard a beacon based on the metric field);
   * 4. Set beacon_seqn (suggestion: no beacon has been received yet);
   * 5. Set the callbacks field.
   */

  linkaddr_copy(&conn->parent, &linkaddr_null);
  conn->is_sink = is_sink;
  conn->metric = 65535;
  conn->beacon_seqn = 0;
  conn->callbacks = callbacks;

  /* Open the underlying Rime primitives */
  broadcast_open(&conn->bc, channels,     &bc_cb);
  unicast_open  (&conn->uc, channels + 1, &uc_cb);

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

  if (conn->is_sink) {
    conn->metric = 0; /* The sink hop count is (by definition) *always* equal to 0.
                       * Remember to update this field *before sending the first*
                       * beacon message in broadcast! */
    /* Schedule the first beacon message flood after 1 second */
    ctimer_set(&conn->beacon_timer, CLOCK_SECOND, beacon_timer_cb, conn);
  }
}
/*---------------------------------------------------------------------------*/
/*                              Beacon Handling                              */
/*---------------------------------------------------------------------------*/
/* Beacon message structure */
struct beacon_msg {
  uint16_t seqn;
  uint16_t metric;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn and metric */
void
send_beacon(struct my_collect_conn* conn)
{
  /* Prepare the beacon message */
  struct beacon_msg beacon = {
    .seqn = conn->beacon_seqn, .metric = conn->metric};

  /* Send the beacon message in broadcast */
  packetbuf_clear();
  packetbuf_copyfrom(&beacon, sizeof(beacon));
  printf("my_collect: sending beacon: seqn %d metric %d\n",
    conn->beacon_seqn, conn->metric);
  broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Beacon timer callback */
void
beacon_timer_cb(void* ptr)
{
  /* TO DO 2: Implement the beacon callback.
   * 1. Send a beacon in broadcast (use send_beacon());
   * 2. Should the sink do anything else?
   * 3. Think who will exploit this callback (only 
   *    the sink or also common nodes?).
   */
  struct my_collect_conn* conn = (struct my_collect_conn*)ptr; /* [Side note] Even an implicit cast, i.e.,
                                                                * struct my_collect_conn* conn = ptr;
                                                                * works correctly */
  send_beacon(conn);

  /* --- Sink-only logic: Rebuild the tree from scratch after BEACON_INTERVAL --- */
  if (conn->is_sink) {
    conn->beacon_seqn ++; /* Before beginning a new beacon flood, the sink 
                           * should *always* increase the beacon sequence number! */
    /* Schedule the next beacon message flood */
    ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, beacon_timer_cb, conn);
  }
}
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void
bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender)
{
  struct beacon_msg beacon;
  int16_t rssi;

  /* Get the pointer to the overall structure my_collect_conn from its field bc */
  struct my_collect_conn* conn = (struct my_collect_conn*)(((uint8_t*)bc_conn) - 
    offsetof(struct my_collect_conn, bc));

  /* Check if the received broadcast packet looks legitimate */
  if (packetbuf_datalen() != sizeof(struct beacon_msg)) {
    printf("my_collect: broadcast of wrong size\n");
    return;
  }
  memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));

  /* TO DO 3.0:
   * Read the RSSI of the *last* reception
   */
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  printf("my_collect: recv beacon from %02x:%02x seqn %u metric %u rssi %d\n", 
      sender->u8[0], sender->u8[1], 
      beacon.seqn, beacon.metric, rssi);

  /* TO DO 3:
   * 1. Analyze the received beacon; check: RSSI, seqn, and metric.
   * 2. Update (if needed) the node's current routing info (parent, metric, beacon_seqn).
   *    Tip: when you update the node's current routing info add a debug print, e.g.,
   *         printf("my_collect: new parent %02x:%02x, my metric %d, my seqn %d\n", 
   *              sender->u8[0], sender->u8[1], conn->metric, conn->beacon_seqn);
   */
  
  if (rssi < RSSI_THRESHOLD || beacon.seqn < conn->beacon_seqn)
    return; // The beacon is either too weak or too old, ignore it!
  if (beacon.seqn == conn->beacon_seqn) { // The beacon is not new, check the metric
    if (beacon.metric >= (conn->metric - 1))
      return; // Worse or equal path length than the route we have already discovered; ignore it!
  }
  /* Otherwise, memorize the new parent, the metric, and the seqn */
  linkaddr_copy(&conn->parent, sender); // NB: This information is *fundamental* for data collection/forwarding!
  conn->metric = beacon.metric + 1;
  conn->beacon_seqn = beacon.seqn;
  printf("my_collect: new parent %02x:%02x, my metric %d, my seqn %d\n", 
      sender->u8[0], sender->u8[1], conn->metric, conn->beacon_seqn);

  /* TO DO 4:
   * IF you have received a new beacon or a beacon with a better metric send, *after a small, random 
   * delay* (BEACON_FORWARD_DELAY), a beacon in broadcast to update the neighbouring nodes about the 
   * changes.
   * Tip: you can use the beacon_timer in my_collect_conn.
   */
  ctimer_set(&conn->beacon_timer, BEACON_FORWARD_DELAY, beacon_timer_cb, conn);
}
/*---------------------------------------------------------------------------*/
/*                     Data Handling --- LAB 7                               */
/*---------------------------------------------------------------------------*/
/* Header structure for data packets */
struct collect_header {
  linkaddr_t source;
  uint8_t hops;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Data Collection: send function */
int
my_collect_send(struct my_collect_conn *conn)
{
  /* TO DO 5:
   * 1. Check if the node is connected (has a parent), IF NOT return -1;
   * 2. If possible, allocate space for the data collection header. If this is
   *    not possible, return -2;
   * 3. Prepare and insert the header in the packet buffer;
   *    Tip: The Rime address of a node is stored in linkaddr_node_addr!
   *         (check contiki/core/net/linkaddr.h for additional details);
   * 4. Send the packet to the parent using unicast and return the status
   *    of unicast_send() to the application.
   */

  struct collect_header hdr;
  /* 5.1 */
  if (linkaddr_cmp(&conn->parent, &linkaddr_null)) // The node is still disconnected (no parent identified)
    return -1; // Inform the app that my_collect is currently unable to forward/deliver the packet

  /* 5.2 */
  if (!packetbuf_hdralloc(sizeof(hdr))) // Allocate space for the header in the packet buffer
    return -2; // Packetbuf_hdralloc() was unable to allocate sufficient header space (payload too big)
  
  /* 5.3 */
  // Prepare the collection header
  linkaddr_copy(&hdr.source, &linkaddr_node_addr); // Store the node address in the source field
  hdr.hops = 0; // The message has not yet been sent: no hop has been travelled so far
  // Alternative way: struct collect_header hdr = {.source=linkaddr_node_addr, .hops=0};

  // Copy the collection header in front of the application payload (at the beginning of the packet buffer)
  memcpy(packetbuf_hdrptr(), &hdr, sizeof(hdr));

  /* 5.4 */
  return unicast_send(&conn->uc, &conn->parent);
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void
uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from)
{
  /* Get the pointer to the overall structure my_collect_conn from its field uc */
  struct my_collect_conn* conn = (struct my_collect_conn*)(((uint8_t*)uc_conn) - 
    offsetof(struct my_collect_conn, uc));

  struct collect_header hdr;

  /* Check if the received unicast message looks legitimate */
  if (packetbuf_datalen() < sizeof(struct collect_header)) {
    printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
    return;
  }

  /* TO DO 6:
   * 1. Extract the header;
   * 2. On the sink, remove the header and call the application callback; 
   *    [TBC] - Should we update any field of hdr? 
   *          - What about packetbuf_dataptr() and packetbuf_hdrptr()? Does the 
   *            application recv callback rely on any of them? Should we take any action?
   * 3. On a forwarder, update the header and forward the packet to the parent (IF ANY) 
   *    using unicast.
   */

  /* 6.1 */
  memcpy(&hdr, packetbuf_dataptr(), sizeof(hdr));
  hdr.hops += 1; /* Upon receiving a data packet, the hop count stored in the collection
                  * header needs to be *always* increased by 1 (both at the sink and at forwarders) */  
  
  /* Potential debug print, to help identify misbehaviours in your protocol: 
   * printf("my_collect: data packet rcvd -- source: %02x:%02x, hops: %u\n",
   *   hdr.source.u8[0], hdr.source.u8[1], hdr.hops);
   */

  /* 6.2 */
  if (conn->is_sink) { /* The destination has been reached, no more forwarding is needed! 
                        * Let's inform the application about the received packet. */
    /* Remove the header, to make packetbuf_dataptr() point to the beginning of the application payload */
    if (packetbuf_hdrreduce(sizeof(struct collect_header)))
      conn->callbacks->recv(&hdr.source, hdr.hops); // Call the sink recv callback function (inform the app about the received message)
    else
      printf("my_collect: ERROR, the header could not be reduced!\n");
  }
  /* 6.3 */
  else {/* Non-sink node acting as a forwarder. Send the received packet to the node's parent in unicast */
    if (linkaddr_cmp(&conn->parent, &linkaddr_null)) {  /* Just to be sure, and to detect potential bugs. 
                                                         * If the node is disconnected, my-collect will be 
                                                         * unable to forward the data packet upwards */
      printf("my_collect: ERROR, unable to forward data packet -- "
        "source: %02x:%02x, hops: %u\n", hdr.source.u8[0], hdr.source.u8[1], hdr.hops);
      return;
    }
    memcpy(packetbuf_dataptr(), &hdr, sizeof(hdr)); // Copy the updated my-collect header in the packet buffer
    unicast_send(&conn->uc, &conn->parent); // Send the updated message to the node's parent in unicast
  }
}
/*---------------------------------------------------------------------------*/

