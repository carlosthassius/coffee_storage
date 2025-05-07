#include "contiki.h"
#include "dev/dht22.h"
#include "dev/leds.h"
#include "dev/adc-zoul.h"
#include <stdio.h>

#define ROTARY_SENSOR_PIN ADC_LINE_1  // A1 on Zolertia RE-Mote
#define ADC_REF_VOLTAGE 3300          // millivolts
#define GROVE_VCC 5000                // millivolts
#define FULL_ANGLE 300                // degrees
#define USAGE_THRESHOLD_DAYS 3        // alert after 3 days of no use

PROCESS(smart_coffee_gadget_process, "Smart Coffee Gadget");
AUTOSTART_PROCESSES(&smart_coffee_gadget_process);

static struct etimer sensor_timer;
static int days_since_last_open = 0;

PROCESS_THREAD(smart_coffee_gadget_process, ev, data)
{
  static int16_t temperature, humidity;
  static uint16_t adc_val;
  static float voltage, degrees;

  PROCESS_BEGIN();

  // Activate sensors
  SENSORS_ACTIVATE(dht22);
  adc_zoul.configure(SENSORS_HW_INIT, ROTARY_SENSOR_PIN);

  while(1) {
    etimer_set(&sensor_timer, CLOCK_SECOND * 10); // Check every 10 seconds
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sensor_timer));

    printf("Reading sensors...\n");

    // Read DHT22 Sensor
    if(dht22_read_all(&temperature, &humidity) != DHT22_ERROR) {
      printf("Temperature: %02d.%d °C, Humidity: %02d.%d%% RH\n",
             temperature / 10, temperature % 10,
             humidity / 10, humidity % 10);
    } else {
      printf("Failed to read DHT22 sensor.\n");
    }

    // Read rotary sensor (angle)
    adc_val = adc_zoul.value(ROTARY_SENSOR_PIN);
    voltage = (adc_val * ADC_REF_VOLTAGE) / 4096.0;  // 12-bit ADC
    degrees = (voltage * FULL_ANGLE) / GROVE_VCC;

    printf("Rotary angle: %.2f°\n", degrees);

    // Simulate usage tracking
    if(degrees > 10) {
      printf("Container opened. Resetting usage counter.\n");
      days_since_last_open = 0;
    } else {
      days_since_last_open++;
    }

    if(days_since_last_open >= USAGE_THRESHOLD_DAYS) {
      printf("ALERT: Coffee container hasn't been opened for %d days. Beans might be stale.\n", days_since_last_open);
    }

    // Simulate LED feedback based on usage (brightness concept)
    if(degrees > 150) {
      leds_on(LEDS_GREEN);
      leds_off(LEDS_RED);
    } else {
      leds_on(LEDS_RED);
      leds_off(LEDS_GREEN);
    }
  }

  PROCESS_END();
}
