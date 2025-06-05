#include "contiki.h"
#include "dev/leds.h"
#include "dev/dht22.h"
#include "dev/adc-zoul.h"
#include "dev/adc-sensors.h"
#include "dev/button-sensor.h"
#include <stdio.h>

/* --- Configuration --- */
#define SENSOR_READ_INTERVAL (CLOCK_SECOND * 2)

/* --- Process Declaration --- */
PROCESS(smart_coffee_gadget_process, "Smart Coffee Gadget");
AUTOSTART_PROCESSES(&smart_coffee_gadget_process);

/* --- Global Timer --- */
static struct etimer sensor_timer;

PROCESS_THREAD(smart_coffee_gadget_process, ev, data)
{
  int16_t temperature; // Default 24.5°C (represented as 245 to have 24.5)
  int16_t humidity;    // Default 65.0%
  uint16_t rotation = 150; // Default rotation raw value

  PROCESS_BEGIN();

  /* Activate sensors */
  SENSORS_ACTIVATE(dht22);
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC3);
  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    etimer_set(&sensor_timer, SENSOR_READ_INTERVAL);
    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER && etimer_expired(&sensor_timer)) {
      /* Read DHT22 sensor */
      if(dht22_read_all(&temperature, &humidity) != DHT22_ERROR) {
      	printf("Temperature %02d.%02d ºC, ", temperature / 10, temperature % 10);
      	printf("Humidity %02d.%02d RH\n", humidity / 10, humidity % 10);
      } else {
      	printf("Failed to read the sensor\n");
      }
      clock_delay_usec(1000);

      /* Read rotary angle sensor on ADC channel 3 (ZOUL_SENSORS_ADC5) */
      rotation = adc_zoul.value(ZOUL_SENSORS_ADC3);
      printf("Rotary angle raw value: %u\n", rotation);

      /* Optional LED feedback based on rotation value */
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

    /* Handle button press */
    if(ev == sensors_event && data == &button_sensor) {
      printf("[NOTIFICATION] Coffee storage is LOW! Please refill.\n");
      leds_on(LEDS_RED);
      clock_delay_usec(1000); // blink effect
      leds_off(LEDS_RED);
    }
  }

  PROCESS_END();
}
