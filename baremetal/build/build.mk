MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

EBBRT_APP_OBJECTS := AppMain.o EbbRTCoeffInit.o 

EBBRT_TARGET := AppMain
EBBRT_APP_VPATH := $(abspath $(MYDIR)../src)
EBBRT_CONFIG := $(abspath $(MYDIR)../src/ebbrtcfg.h)

EBBRT_APP_INCLUDES := -I $(abspath $(MYDIR)../ext)

EBBRT_APP_LINK := -L $(MYDIR)lib -lgsl -lgslcblas -lboost_system-gcc-1_54 -lboost_serialization-gcc-1_54

# FIXME:  For the moment we are using EBBRT_OPTFLAGS probably want to look at an APP secific variable

EBBRT_OPTFLAGS = -Wno-unused-local-typedefs -O2

include $(abspath $(EBBRT_SRCDIR)/apps/ebbrtbaremetal.mk)

-include $(shell find -name '*.d')
