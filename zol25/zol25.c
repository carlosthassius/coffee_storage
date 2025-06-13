#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "simple-udp.h"
#include "net/packetbuf.h"
#include "../example.h"

#if CONTIKI_TARGET_ZOUL
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"
#include "dev/adc-sensors.h"
#include "dev/dht22.h"
#else
#include "dev/adxl345.h"
#include "dev/battery-sensor.h"
#include "dev/i2cmaster.h"
#include "dev/tmp102.h"
#endif

#include "sys/etimer.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/
#define SEND_INTERVAL (3 * CLOCK_SECOND)
#define ADC_PIN           2
#define LOOP_INTERVAL       (CLOCK_SECOND * 3)
#define LEDS_PERIODIC       LEDS_GREEN
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
static uint16_t local_counter = 0;
static struct etimer et;
/*---------------------------------------------------------------------------*/
PROCESS(mcast_and_sensor_process, "UDP receiver + local sensors");
AUTOSTART_PROCESSES(&mcast_and_sensor_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  radio_value_t aux;
  my_msg_t *msgPtr = (my_msg_t *)data;

  leds_toggle(LEDS_GREEN);
  printf("\n***\nMessage from: ");
  uip_debug_ipaddr_print(sender_addr);

  printf("\nData received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &aux);
  printf("CH: %u ", (unsigned int) aux);

  aux = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  printf("RSSI: %ddBm ", (int8_t)aux);

  aux = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
  printf("LQI: %u\n", aux);

#if CONTIKI_TARGET_ZOUL
  printf("{\"node_id\":%u,\"sensors\":{\"adc1\":%u,\"adc3\":%u},\"angles\":{\"angle1\":%d,\"angle3\":%d},\"counter\":%u}\n",
         msgPtr->id, msgPtr->adc1_value, msgPtr->adc3_value, msgPtr->angle1,
         msgPtr->angle3, msgPtr->counter);
#else
  printf("ID: %u, temp: %u, x: %d, y: %d, z: %d, batt: %u, counter: %u\n",
         msgPtr->id, msgPtr->value1, msgPtr->value2, msgPtr->value3,
         msgPtr->value4, msgPtr->battery, msgPtr->counter);
#endif
}
/*---------------------------------------------------------------------------*/
static void
print_local_sensor_data(void)
{
  static int temperature = 0;
  static int humidity_air = 0;

  // Read temperature and humidity from DHT22
  int dht_status = dht22_read_all(&temperature, &humidity_air);

  // Read raw ADC value for soil moisture
  int adc_val = adc_sensors.value(ANALOG_VAC_SENSOR);
  int humidity10 = (adc_val * 1000) / 240;

  // Start JSON output
  printf("{\n");
  printf("  \"local_counter\": %u,\n", local_counter);

  if(dht_status != DHT22_ERROR) {
    printf("  \"temperature_celsius\": %d.%02d,\n", temperature / 10, abs(temperature % 10));
    printf("  \"humidity_air_rh\": %d.%02d,\n", humidity_air / 10, abs(humidity_air % 10));
  } else {
    printf("  \"temperature_celsius\": null,\n");
    printf("  \"humidity_air_rh\": null,\n");
  }

  printf("  \"humidity_soil_percent\": %d.%d,\n", humidity10 / 10, humidity10 % 10);
  printf("  \"raw_adc\": %d\n", adc_val);
  printf("}\n");
}


/*---------------------------------------------------------------------------*/
static void
set_radio_default_parameters(void)
{
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, EXAMPLE_TX_POWER);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, EXAMPLE_CHANNEL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mcast_and_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  set_radio_default_parameters();

  simple_udp_register(&mcast_connection, UDP_CLIENT_PORT, NULL,
                      UDP_CLIENT_PORT, receiver);


  adc_sensors.configure(ANALOG_VAC_SENSOR, ADC_PIN);
  SENSORS_ACTIVATE(dht22);
  //SENSORS_ACTIVATE(adc_sensors);
  //adc_sensors.configure(ANALOG_VAC_SENSOR, ADC_PIN);

  etimer_set(&et, LOOP_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    print_local_sensor_data();
    etimer_reset(&et);
    local_counter++;
  }

  PROCESS_END();
}
