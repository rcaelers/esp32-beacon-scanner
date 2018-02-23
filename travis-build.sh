#!/bin/bash

cd main/certs

if [ -f CA.crt -o -f esp32.crt -o -f esp32.key ]; then
   echo "Certificates already exist. Not overwriting"
   exit 0;
fi

certstrap init --common-name "CA" --passphrase "test"
certstrap request-cert --passphrase "test" -cn esp32
certstrap sign esp32 --passphrase "test" --CA "CA"

cp out/CA.crt .
cp out/esp32.crt .
cp out/esp32.key .

cd ../..

make defconfig
make all
