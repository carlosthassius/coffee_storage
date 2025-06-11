#include "contiki.h"
#include "dev/dht22.h"
#include "dev/adc-sensors.h"
#include "dev/leds.h"
#include <stdio.h>
#include <stdint.h>

/* --- Configurações --- */
#define ADC_PIN             2
#define LOOP_PERIOD         2
#define LOOP_INTERVAL       (CLOCK_SECOND * LOOP_PERIOD)
#define LEDS_PERIODIC       LEDS_GREEN

/* --- Variáveis Globais --- */
static struct etimer et;
static uint16_t counter;

/* --- Processos --- */
PROCESS(zol25_sensors_process, "Zol25 Sensors");
AUTOSTART_PROCESSES(&zol25_sensors_process);

PROCESS_THREAD(zol25_sensors_process, ev, data)
{
  static int temperature;
  static int humidity_air; // Umidade do DHT22

  PROCESS_BEGIN();

  counter = 0;

  /* Ativar sensores */
  SENSORS_ACTIVATE(dht22);
  adc_sensors.configure(ANALOG_VAC_SENSOR, ADC_PIN);

  printf("Smart Coffee Gadget: Leitura de sensores iniciada\n");
  leds_on(LEDS_PERIODIC);
  etimer_set(&et, LOOP_INTERVAL);

  while(1) {
    PROCESS_YIELD();

    if(ev == PROCESS_EVENT_TIMER) {
      leds_toggle(LEDS_PERIODIC);

      printf("-----------------------------------------\n");
      printf("Contador = 0x%08x\n", counter);

      /* Leitura do DHT22 */
      if(dht22_read_all(&temperature, &humidity_air) != DHT22_ERROR) {
        printf("Temperatura: %02d.%02d ºC\n", temperature / 10, temperature % 10);
        printf("Umidade (ar): %02d.%02d %%RH\n", humidity_air / 10, humidity_air % 10);
      } else {
        printf("Falha na leitura do sensor DHT22\n");
      }

      /* Leitura do sensor analógico de umidade */
      int val = adc_sensors.value(ANALOG_VAC_SENSOR);
      int humidity10 = (val * 1000) / 240; // em décimos de porcentagem

      int inteiro = humidity10 / 10;
      int decimal = humidity10 % 10;

      printf("Umidade (sensor analógico): %d.%d%%\n", inteiro, decimal); 

      /* Reinicia o temporizador */
      etimer_set(&et, LOOP_INTERVAL);
      counter++;
    }
  }

  PROCESS_END();
}
