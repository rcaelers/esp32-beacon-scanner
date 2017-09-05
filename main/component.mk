# TODO:
# COMPONENT_EMBED_TXTFILES := certs/aws-root-ca.pem certs/certificate.pem.crt certs/private.pem.key

COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_SRCDIRS += os

CXXFLAGS+=-std=gnu++14 -fexceptions
