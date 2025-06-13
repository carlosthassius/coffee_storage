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

/*---------------------------------------------------------------------------*/
#define SEND_INTERVAL (3 * CLOCK_SECOND)
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
static uint16_t counter = 0;
/*---------------------------------------------------------------------------*/
PROCESS(mcast_example_process, "UDP multicast example process");
AUTOSTART_PROCESSES(&mcast_example_process);
/*---------------------------------------------------------------------------*/
int normalize_rotary (int adc_value, int channel) {
    
    float rotation;
    float ang_min = 0.0f;
    float ang_max = 225.0f;

    if(channel==1) {
        float min_bruto = 0.0f;
        float max_bruto = 32764.0f;
        rotation = ((float)adc_value - min_bruto) * ((ang_max - ang_min) / (max_bruto - min_bruto) ) + ang_min;

        if (rotation < ang_min) {
            rotation = ang_min;
        }
        if (rotation > ang_max) {
            rotation = ang_max;
        }
    }
    else if(channel==3) {
        float min_bruto_ch3 = 100.0f;
            float max_bruto_ch3 = 24000.0f;

            if (adc_value <= (int)min_bruto_ch3) {
                rotation = ang_min; 
            } else if (adc_value >= (int)max_bruto_ch3) {
                rotation = ang_max;
            } else {
                rotation = ((float)adc_value - min_bruto_ch3) * ((ang_max - ang_min) / (max_bruto_ch3 - min_bruto_ch3)) + ang_min;
            }
            
            if (rotation < ang_min) {
                rotation = ang_min;
            }
            if (rotation > ang_max) {
                rotation = ang_max;
            }
    }
    return (int)rotation;
}

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
print_radio_values(void)
{
  radio_value_t aux;

  printf("\n* Radio parameters:\n");

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &aux);
  printf("   Channel %u", aux);

  NETSTACK_RADIO.get_value(RADIO_CONST_CHANNEL_MIN, &aux);
  printf(" (Min: %u, ", aux);

  NETSTACK_RADIO.get_value(RADIO_CONST_CHANNEL_MAX, &aux);
  printf("Max: %u)\n", aux);

  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &aux);
  printf("   Tx Power %3d dBm", aux);

  NETSTACK_RADIO.get_value(RADIO_CONST_TXPOWER_MIN, &aux);
  printf(" (Min: %3d dBm, ", aux);

  NETSTACK_RADIO.get_value(RADIO_CONST_TXPOWER_MAX, &aux);
  printf("Max: %3d dBm)\n", aux);

  printf("   PAN ID: 0x%02X\n", IEEE802154_CONF_PANID);
}
/*---------------------------------------------------------------------------*/
static void
set_radio_default_parameters(void)
{
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, EXAMPLE_TX_POWER);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, EXAMPLE_CHANNEL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mcast_example_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  set_radio_default_parameters();
  print_radio_values();

  simple_udp_register(&mcast_connection, UDP_CLIENT_PORT, NULL,
                      UDP_CLIENT_PORT, receiver);

#if CONTIKI_TARGET_ZOUL
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);
#else
  SENSORS_ACTIVATE(adxl345);
  SENSORS_ACTIVATE(tmp102);
  SENSORS_ACTIVATE(battery_sensor);
#endif

  etimer_set(&periodic_timer, SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    msg.id = 1;
    msg.counter = counter++;

    #if CONTIKI_TARGET_ZOUL
        msg.adc1_value = adc_zoul.value(ZOUL_SENSORS_ADC1);
        msg.adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3);
        msg.angle1 = normalize_rotary(msg.adc1_value,1);
        msg.angle3 = normalize_rotary(msg.adc3_value,3);
    #endif

    uip_create_linklocal_allnodes_mcast(&mcast_connection.remote_addr);
    simple_udp_send(&mcast_connection, &msg, sizeof(msg));

    printf("Mensagem enviada: contador=%u, adc1=%u, adc3=%u, angle1=%d, angle3=%d\n",
           msg.counter, msg.adc1_value, msg.adc3_value, msg.angle1, msg.angle3);
  }

  PROCESS_END();
}
