COMPONENT_ADD_INCLUDEDIRS := include boost boost/ext
COMPONENT_SRCDIRS = src/core src/utils src/net src/ble src/http src/mqtt src/ota src/drivers boost/ext/libs/regex/src

CXXFLAGS += -Wno-error=switch
