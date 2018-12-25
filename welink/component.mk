#
# Component Makefile
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

ifdef CONFIG_TARGET_PLATFORM_ESP8266
LIBS += txdevicesdk8266
else
LIBS += txdevicesdk32
endif

COMPONENT_ADD_LDFLAGS += -L $(COMPONENT_PATH)/lib \
                           $(addprefix -l,$(LIBS))
