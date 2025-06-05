#include "contiki.h"
#include "dev/leds.h"
#include "dev/adc-zoul.h"
#include "dev/button-sensor.h"
#include <stdio.h>

/* --- Configuration --- */
#define SENSOR_READ_INTERVAL (CLOCK_SECOND * 2)

/* --- Process Declaration --- */
PROCESS(rotary_angle_monitor_process, "Rotary Angle Monitor");
AUTOSTART_PROCESSES(&rotary_angle_monitor_process);

/* --- Global Timer --- */
static struct etimer sensor_timer;

int normalize_adc (int adc_value, int channel) {
    
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

PROCESS_THREAD(rotary_angle_monitor_process, ev, data)
{
  uint16_t rotation_adc1 = 0;
  uint16_t rotation_adc3 = 0;

  PROCESS_BEGIN();

  /* Activate sensors */
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);
  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    etimer_set(&sensor_timer, SENSOR_READ_INTERVAL);
    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER && etimer_expired(&sensor_timer)) {
      rotation_adc1 = adc_zoul.value(ZOUL_SENSORS_ADC1);
      rotation_adc3 = adc_zoul.value(ZOUL_SENSORS_ADC3);
      
      printf("ADC1 raw value: %u\n", normalize_adc(rotation_adc1,1));
      printf("ADC3 raw value: %u\n", normalize_adc(rotation_adc3,3));

      /* Simple LED feedback */
      if(rotation_adc1 < 100 || rotation_adc3 < 100) {
        leds_on(LEDS_GREEN);
        leds_off(LEDS_RED | LEDS_BLUE);
      } else if(rotation_adc1 < 300 || rotation_adc3 < 300) {
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
      clock_delay_usec(1000);
      leds_off(LEDS_RED);
    }
  }

  PROCESS_END();
}
