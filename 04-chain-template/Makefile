# For TMote Sky (emulated in Cooja) use the following target
TARGET ?= sky

DEFINES=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = chain

all: $(CONTIKI_PROJECT)

CFLAGS=-Wall -Wextra

CONTIKI_WITH_RIME = 1
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
