/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Flooding example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/

static size_t SIZE = 5;
static unsigned char MAX_HOPS = 2;

static void forward(struct broadcast_conn *c, linkaddr_t* src,
                    linkaddr_t* dest, unsigned char hops) {

  char* data = malloc(SIZE);
  *data = src->u8[0];
  *(data+1) = src->u8[1];
  *(data+2) = dest->u8[0];
  *(data+3) = dest->u8[1];
  *(data+4) = hops;

  packetbuf_copyfrom((void*) data, SIZE);
  broadcast_send(c);
  printf("Packet from %d.%d forwarded\n", src->u8[0], src->u8[1]);

}

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  char* data = (char*) packetbuf_dataptr();
  unsigned char src_1 = *data;
  unsigned char src_2 = *(data+1);
  unsigned char dest_1 = *(data+2);
  unsigned char dest_2 = *(data+3);
  unsigned char hops = *(data+4);
  
  // Source address
  linkaddr_t* src = (linkaddr_t*) malloc(sizeof(unsigned char)*LINKADDR_SIZE);
  src->u8[0] = src_1;
  src->u8[1] = src_2;

  // Destination address
  linkaddr_t* dest = (linkaddr_t*) malloc(sizeof(unsigned char)*LINKADDR_SIZE);
  dest->u8[0] = dest_1;
  dest->u8[1] = dest_2;  

  // Packet went back to source
  if (linkaddr_cmp(src, &linkaddr_node_addr)) {
    printf("Packet went back to source, dropped\n");

  // Packet has reached destination
  } else if (linkaddr_cmp(dest, &linkaddr_node_addr)) {
    printf("Packet received from %d.%d\n",
            src->u8[0], src->u8[1]);

  // Packet is dropped because number of hops has reached 0
  } else if (hops <= 1) {
    printf("Packet dropped\n");

  // Packet forwarded  
  } else {
    forward(c, src, dest, hops-1);
  }

}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    linkaddr_t* src = (linkaddr_t*) malloc(sizeof(unsigned char)*LINKADDR_SIZE);
    linkaddr_copy(src, &linkaddr_node_addr);

    // Destination is mote with address 3.0
    linkaddr_t* dest = (linkaddr_t*) malloc(sizeof(unsigned char)*LINKADDR_SIZE);
    dest->u8[0] = 3;
    dest->u8[1] = 0;

    char* data = malloc(SIZE);
    *data = src->u8[0];
    *(data+1) = src->u8[1];
    *(data+2) = dest->u8[0];
    *(data+3) = dest->u8[1];
    *(data+4) = MAX_HOPS;

    packetbuf_copyfrom((void*) data, SIZE);
    broadcast_send(&broadcast);
    printf("Packet sent to %d.%d\n", dest->u8[0], dest->u8[1]);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
