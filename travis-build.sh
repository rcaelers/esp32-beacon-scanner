#!/bin/bash

cd main/certs

if [ -f CA.crt -o -f esp32.crt -o -f esp32.key ]; then
    echo "Certificates already exist. Not overwriting"
else
    certstrap init --common-name "CA" --passphrase "test"
    certstrap request-cert --passphrase "test" -cn esp32
    certstrap sign esp32 --passphrase "test" --CA "CA"

    cp out/CA.crt .
    cp out/esp32.crt .
    cp out/esp32.key .
    chmod u+w,a+r *.crt *.key
fi

cd ../..

cp main/user_config.h.example main/user_config.h

make defconfig
make all
