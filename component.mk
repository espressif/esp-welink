COMPONENT_ADD_INCLUDEDIRS := include adaptation/include
COMPONENT_SRCDIRS := adaptation

LIBS += txdevicesdk

COMPONENT_ADD_LDFLAGS += -L $(COMPONENT_PATH)/lib \
                           $(addprefix -l,$(LIBS))

