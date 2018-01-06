PROJECT_NAME := beacon-scanner

include $(IDF_PATH)/make/project.mk

CXXFLAGS += -std=gnu++14 -D_GLIBCXX_USE_C99
