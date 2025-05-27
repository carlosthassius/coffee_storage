#include "contiki.h"
#include "dev/leds.h"
#include "dev/dht22.h"
#include "dev/adc-sensors.h"
#include "dev/button-sensor.h"
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

  int temperature = 24; // Default 24.5°C
  int humidity = 650;    // Default 65.0%
  uint16_t rotation = 150; // Default rotation value

  /* Activate sensors */
  SENSORS_ACTIVATE(dht22);
  adc_zoul.configure(ANALOG_PHIDGET_ROTATION_1109, ROTARY_SENSOR_PIN);
  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    etimer_set(&sensor_timer, SENSOR_READ_INTERVAL);
    PROCESS_WAIT_EVENT();

    /* --- Read DHT22 Sensor --- */
    if(ev == PROCESS_EVENT_TIMER && etimer_expired(&sensor_timer)) {
      if(dht22_read_all(&temperature, &humidity) == DHT22_ERROR) {
        temperature=245;
        humidity=650;
        printf("[Mock] Temperature: %d.%d °C, Humidity: %d.%d %%RH\n",
               temperature / 10, temperature % 10,
               humidity / 10, humidity % 10);
      } else {
        printf("Temperature: %d.%d °C, Humidity: %d.%d %%RH\n",
               temperature / 10, temperature % 10,
               humidity / 10, humidity % 10);
      }

      /* Small delay to avoid conflicts */
      clock_delay_usec(1000); // 100 ms

      /* --- Read Rotary Sensor --- */
      rotation = adc_zoul.value(ANALOG_PHIDGET_ROTATION_1109);
      if(rotation == 0 || rotation == ADC_WRAPPER_ERROR) {
        rotation=150;
        printf("[Mock] Rotary angle raw value: %u\n", rotation);
      } else {
        printf("Rotary angle raw value: %u\n", rotation);
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

    /* --- Handle Button Press --- */
    if(ev == sensors_event && data == &button_sensor) {
      printf("[NOTIFICATION] Coffee storage is LOW! Please refill.\n");
      leds_on(LEDS_RED);
      clock_delay_usec(1000); // Blink effect
      leds_off(LEDS_RED);
    }
  }

  PROCESS_END();
}
