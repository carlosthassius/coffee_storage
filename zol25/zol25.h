#ifndef ZOL25_H
#define ZOL25_H

#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "simple-udp.h"

void receiver(struct simple_udp_connection *c,
              const uip_ipaddr_t *sender_addr,
              uint16_t sender_port,
              const uip_ipaddr_t *receiver_addr,
              uint16_t receiver_port,
              const uint8_t *data,
              uint16_t datalen);
int sensor_data(char *buffer, size_t buffer_size, uint32_t pub_id);
void set_radio_default_parameters(void);
void init_sensors(void);

#endif
