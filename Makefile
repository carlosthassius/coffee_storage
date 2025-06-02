### Project that we will compile:
CONTIKI_PROJECT = project



all: $(CONTIKI_PROJECT)
TARGET = zoul
CONTIKI = ../../contiki/
include $(CONTIKI)/Makefile.include
CONTIKI_TARGET_SOURCEFILES +=  dht22.c adc-zoul.c
