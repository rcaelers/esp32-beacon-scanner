FROM ubuntu:18.04

LABEL maintainer "Rob Caelers <rob.caelers@gmail.com>"

RUN \
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get update -y && \
  apt-get install -y \
       bison \
       curl \
       flex \
       git \
       golang \
       gperf \
       libncurses-dev \
       make \
       python \
       python-serial && \
  apt-get clean

RUN mkdir /esp32 && \
    mkdir /esp32/gcc-5 && \
    mkdir /esp32/gcc-8 && \
    mkdir /esp32/esp-idf && \
    mkdir /esp32/project

RUN curl https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz | tar xzf - -C /esp32/gcc5

RUN curl https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_2_0-esp32-2018r1-linux-amd64.tar.xz  | tar jzf - -C /esp32/gcc8
RUN cd /esp32 && \
    git clone --recursive https://github.com/espressif/esp-idf.git

RUN mkdir -p /esp32/src/github.com/square && \
    cd /esp32/src/github.com/square && \
    git clone https://github.com/square/certstrap && \
    cd certstrap && \
    GOPATH=/esp32/ GOARCH=amd64 GOOS=linux go build -o bin/certstrap

ENV PATH $PATH:/esp32/src/github.com/square/certstrap/bin
ENV IDF_PATH /esp32/esp-idf

WORKDIR /esp32/project

CMD ["/bin/bash"]
