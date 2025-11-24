#ifndef __MY_COLLECT_H__
#define __MY_COLLECT_H__
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
/* Callback structure of our Rime collection primitive */
struct my_collect_callbacks {
  void (* recv)(const linkaddr_t *originator, uint8_t hops);
};
/*---------------------------------------------------------------------------*/
/* Connection object of our Rime collection primitive */
struct my_collect_conn {
  struct broadcast_conn bc; // Connection object of the identified sender broadcast primitive, used in LAB 6 to build the tree  
  struct unicast_conn uc;   // Connection object of the identified receiver unicast primitive, used in LAB 7 to forward data packets
  const struct my_collect_callbacks* callbacks;
  struct ctimer beacon_timer;
  linkaddr_t parent;        // Address of the current parent
  uint16_t metric;          // Current hop count of the node
  uint16_t beacon_seqn;     // Highest beacon sequence number the node has seen 
  bool is_sink;
};
/*---------------------------------------------------------------------------*/
/* Initialize your RIME collection primitive (i.e., open a collect connection)
 *  - conn      -- a pointer to the collect connection object 
 *  - channels  -- starting channel C (my_collect uses two: C and C+1)
 *  - is_sink   -- initialise in either sink or forwarder mode by the application
 *  - callbacks -- a pointer to the collect callback structure
 */
void my_collect_open(
    struct my_collect_conn* conn, 
    uint16_t channels, 
    bool is_sink,
    const struct my_collect_callbacks *callbacks);
/*---------------------------------------------------------------------------*/
/* [Lab 7] Send a data packet to the sink */
int my_collect_send(struct my_collect_conn *c);
/*---------------------------------------------------------------------------*/
#endif /* __MY_COLLECT_H__ */
