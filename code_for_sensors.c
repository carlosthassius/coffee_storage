#include "contiki.h"
#include "dev/leds.h"
#include "dev/dht22.h"
#include "dev/adc-sensors.h"
#include <stdio.h>

/* --- Configuration --- */
#define ROTARY_SENSOR_PIN 5  // A5 on RE-Mote
#define SENSOR_READ_INTERVAL (CLOCK_SECOND * 2)

/* --- Process Declaration --- */
PROCESS(smart_coffee_gadget_process, "Smart Coffee Gadget");
AUTOSTART_PROCESSES(&smart_coffee_gadget_process);

/* --- Global Timer --- */
static struct etimer sensor_timer;

/* --- Main Process --- */
PROCESS_THREAD(smart_coffee_gadget_process, ev, data)
{
  PROCESS_BEGIN();

  int temperature, humidity;
  uint16_t rotation;

  /* Activate sensors */
  SENSORS_ACTIVATE(dht22);
  adc_sensors.configure(ANALOG_PHIDGET_ROTATION_1109, ROTARY_SENSOR_PIN);

  while(1) {
    etimer_set(&sensor_timer, SENSOR_READ_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sensor_timer));

    /* --- Read DHT22 Sensor --- */
    if(dht22_read_all(&temperature, &humidity) != DHT22_ERROR) {
      printf("Temperature: %d.%d °C, Humidity: %d.%d %%RH\n",
             temperature / 10, temperature % 10,
             humidity / 10, humidity % 10);
    } else {
      printf("DHT22 read failed!\n");
    }

    /* --- Read Rotary Sensor --- */
    rotation = adc_sensors.value(ANALOG_PHIDGET_ROTATION_1109);
    if(rotation != ADC_WRAPPER_ERROR) {
      printf("Rotary angle raw value: %u\n", rotation);
    } else {
      printf("Failed to read rotary sensor. Check wiring.\n");
    }

    /* Optional LED Feedback */
    if(rotation < 100) {
      leds_on(LEDS_GREEN);
      leds_off(LEDS_RED | LEDS_BLUE);
    } else if(rotation < 300) {
      leds_on(LEDS_BLUE);
      leds_off(LEDS_GREEN | LEDS_RED);
    } else {
      leds_on(LEDS_RED);
      leds_off(LEDS_GREEN | LEDS_BLUE);
    }
  }

  PROCESS_END();
}
