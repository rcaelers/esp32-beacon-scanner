Certificates
===============

https://github.com/square/certstrap

certstrap init --common-name "CA"

Generate certificates for MQTT server:

certstrap request-cert -cn <fqdn_mqtt_server>
certstrap sign <fqdn_mqtt_server> --CA "CA"

Generate certificates for ESP32:

certstrap request-cert -cn esp32
certstrap sign esp32 --CA "CA"
