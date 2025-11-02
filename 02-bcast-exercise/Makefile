# For TMote Sky (emulated in Cooja) use the following target
TARGET ?= sky

# Project
DEFINES=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = node

# Add custom compiler flags
CFLAGS += -Wall -mmcu=msp430g2553 # -pedantic

all: $(CONTIKI_PROJECT)


CONTIKI_WITH_RIME = 1
CONTIKI ?= ../contiki
include $(CONTIKI)/Makefile.include

