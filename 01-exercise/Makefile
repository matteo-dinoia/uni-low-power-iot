# For TMote Sky (emulated in Cooja) use the following target
TARGET ?= sky

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

CONTIKI_PROJECT = hello-world hello-world-ctimer
all: $(CONTIKI_PROJECT)

CONTIKI_WITH_IPV6=0
CONTIKI_WITH_RIME=1

CONTIKI ?= ../contiki-uwb/contiki
include $(CONTIKI)/Makefile.include
