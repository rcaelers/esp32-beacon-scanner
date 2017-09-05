COMPONENT_EMBED_TXTFILES := certs/CA.crt certs/ESP32.crt certs/ESP32.key

COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_SRCDIRS += os

CXXFLAGS+=-std=gnu++14 -fexceptions
