#include "contiki.h"
#include "dev/dht22.h"
#include "dev/adc-sensors.h"
#include "dev/leds.h"
#include <stdio.h>

/* --- Configurations --- */
#define ADC_PIN             2
#define LOOP_PERIOD         2
#define LOOP_INTERVAL       (CLOCK_SECOND * LOOP_PERIOD)
#define LEDS_PERIODIC       LEDS_GREEN

/* --- Globals --- */
static struct etimer et;
static uint16_t counter;

/* --- Process --- */
PROCESS(zol25_sensors_process, "Zol25 Sensors");
AUTOSTART_PROCESSES(&zol25_sensors_process);

PROCESS_THREAD(zol25_sensors_process, ev, data)
{
  static int temperature = 0;
  static int humidity_air = 0;

  PROCESS_BEGIN();

  counter = 0;

  /* Activate sensors */
  SENSORS_ACTIVATE(dht22);
  adc_sensors.configure(ANALOG_VAC_SENSOR, ADC_PIN);

  printf("Smart Coffee Gadget: Starting sensor readings...\n");
  leds_on(LEDS_PERIODIC);
  etimer_set(&et, LOOP_INTERVAL);

  while(1) {
    PROCESS_YIELD();

    if(ev == PROCESS_EVENT_TIMER) {
      leds_toggle(LEDS_PERIODIC);

      int dht_status = dht22_read_all(&temperature, &humidity_air);

      int adc_val = adc_sensors.value(ANALOG_VAC_SENSOR);
      float humidity_soil = (adc_val * 100.0f) / 240.0f;

      // Start JSON object
      printf("{\n");
      printf("  \"counter\": %u,\n", counter);

      if(dht_status != DHT22_ERROR) {
      printf("  \"temperature_celsius\": %d.%02d,\n", temperature / 10, abs(temperature % 10));
      printf("  \"humidity_air_rh\": %d.%02d,\n", humidity_air / 10, abs(humidity_air % 10));
    } else {
      printf("  \"temperature_celsius\": null,\n");
      printf("  \"humidity_air_rh\": null,\n");
    }

    int humidity10 = (adc_val * 1000) / 240;
    printf("  \"humidity_soil_percent\": %d.%d\n", humidity10 / 10, humidity10 % 10);


      etimer_reset(&et);
      counter++;
    }
  }

  PROCESS_END();
}
