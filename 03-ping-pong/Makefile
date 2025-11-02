# For TMote Sky (emulated in Cooja) use the following target
TARGET ?= sky

# Project
DEFINES=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = uc-ctimer uc-etimer uc-runicast

all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RIME = 1
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
