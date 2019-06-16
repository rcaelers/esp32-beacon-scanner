FROM ubuntu:18.10

LABEL maintainer "Rob Caelers <rob.caelers@gmail.com>"

RUN \
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get update -y && \
  apt-get install -y \
       bison \
       cmake \
       curl \
       flex \
       git \
       golang \
       gperf \
       libncurses-dev \
       make \
       ninja-build \
       python \
       python-pip \
       python-pyparsing \
       python-serial && \
  apt-get clean

RUN mkdir /build && \
    mkdir /build/gcc5 && \
    mkdir /build/gcc8 && \
    mkdir /build/esp-idf && \
    mkdir /build/project

RUN curl https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz | tar xzf - -C /build/gcc5

RUN curl https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_2_0-esp32-2019r1-rc2-linux-amd64.tar.xz | tar xJf - -C /build/gcc8
RUN cd /build && \
    git clone --recursive https://github.com/espressif/esp-idf.git

RUN pip install --user -r /build/esp-idf/requirements.txt

RUN mkdir -p /build/src/github.com/square && \
    cd /build/src/github.com/square && \
    git clone https://github.com/square/certstrap && \
    cd certstrap && \
    GOPATH=/build/ GOARCH=amd64 GOOS=linux go build -o bin/certstrap

ENV PATH $PATH:/build/src/github.com/square/certstrap/bin
ENV IDF_PATH /build/esp-idf

WORKDIR /build/project

CMD ["/bin/bash"]
