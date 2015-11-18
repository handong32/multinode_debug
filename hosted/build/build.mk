MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

app_sources := AppMain.cc EbbRTCoeffInit.cc
target := AppMain

EBBRT_APP_LINK := -lm -lboost_system -lboost_serialization -lgsl -lgslcblas

include $(abspath $(EBBRT_SRCDIR)/apps/ebbrthosted.mk)

-include $(shell find -name '*.d')
