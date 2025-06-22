#include "contiki.h"
#include "simple-udp.h"
#include "./example.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/packetbuf.h"

#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"
#include "dev/dht22.h"

#include "sys/etimer.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/
#define SEND_INTERVAL       (3 * CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
typedef struct {
  uint8_t id;
  uint16_t counter;
  uint16_t adc1_value;
  uint16_t adc3_value;
  int angle1;
  int angle3;
} my_msg_t;

/*---------------------------------------------------------------------------*/
static struct simple_udp_connection mcast_connection;
static my_msg_t msg;
static struct etimer et;

/* new global for storing received message */
static my_msg_t last_received_msg;
static int msg_received_flag = 0;

/*---------------------------------------------------------------------------*/
PROCESS(mcast_and_sensor_process, "UDP receiver + local sensors");
AUTOSTART_PROCESSES(&mcast_and_sensor_process);
/*---------------------------------------------------------------------------*/
void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  my_msg_t *msgPtr = (my_msg_t *)data;

  leds_toggle(LEDS_GREEN);
  memcpy(&last_received_msg, msgPtr, sizeof(my_msg_t));
  msg_received_flag = 1;

  printf("[INFO] Message received from: ");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
}
/*---------------------------------------------------------------------------*/
int sensor_data(char *buffer, size_t buffer_size, uint32_t pub_id) {
  static int temperature = 0;
  static int humidity_air = 0;
  int len;
  // Base unix time for 20 jun 2025 00:00:00 UTC
  static unsigned long base_unix_time = 1737110400UL;
  unsigned long now = base_unix_time + clock_seconds();

  int dht_status = dht22_read_all(&temperature, &humidity_air);
  int raw = adc_zoul.value(ZOUL_SENSORS_ADC1);

  // normalize
  int humidity_soil = (raw * 1000) / 16000;
  // ensure that the value does not exceed the percentage range
  if (humidity_soil > 1000) humidity_soil = 1000;
  if (humidity_soil < 0) humidity_soil = 0;

  printf("{\n");
  printf("  \"id\": \"%lu\",\n", pub_id);

  if (dht_status != DHT22_ERROR) {
    printf("  \"temperature_celsius\": %d.%01d,\n", temperature / 10, abs(temperature % 10));
    printf("  \"humidity_air_rh\": %d.%02d,\n", humidity_air / 10, abs(humidity_air % 10));
  } else {
    printf("  \"temperature_celsius\": null,\n");
    printf("  \"humidity_air_rh\": null,\n");
  }

  printf("  \"humidity_soil_percent\": %d.%d,\n", humidity_soil / 10, humidity_soil % 10);
  printf("  \"angle1\": %d,\n", last_received_msg.angle1);
  printf("  \"angle3\": %d\n", last_received_msg.angle3);
  printf("}\n");

  len = snprintf(buffer, buffer_size,
  "{"
  "\"id\":\"%lu\","
  "\"ts\":\"%lu\","
  "\"temperature_celsius\":%d.%02d,"
  "\"humidity_air_rh\":%d.%02d,"
  "\"humidity_soil_percent\":%d.%02d,"
  "\"angle1\":%d.%02d,"
  "\"angle3\":%d.%02d"
  "}",
  pub_id,
  now,
  temperature / 10, abs(temperature % 10),
  humidity_air / 10, abs(humidity_air % 10),
  humidity_soil / 10, abs(humidity_soil % 10),
  last_received_msg.angle1 / 100, abs(last_received_msg.angle1 % 100),
  last_received_msg.angle3 / 100, abs(last_received_msg.angle3 % 100));

  msg_received_flag = 0;
  return len; 
}

/*---------------------------------------------------------------------------*/
void
set_radio_default_parameters(void)
{
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, EXAMPLE_TX_POWER);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, EXAMPLE_CHANNEL);
}

void init_sensors(void){
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1);
  SENSORS_ACTIVATE(dht22);
}
