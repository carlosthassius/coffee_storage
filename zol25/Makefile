all: zol25

# Linker optimizations
SMALL = 1

# Includes the project-conf configuration file
CFLAGS += -DPROJECT_CONF_H=\"./project-conf.h\"

CONTIKI = ../../../../..

# This flag includes the IPv6 libraries
CONTIKI_WITH_IPV6 = 1

# Add required source files for DHT22 and ADC support
PROJECT_SOURCEFILES += dht22.c adc-sensors.c

include $(CONTIKI)/Makefile.include
