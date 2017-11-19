Certificates
===============

https://github.com/square/certstrap

certstrap init --common-name "CA"

certstrap request-cert -cn <fqdn_mqtt_server>
certstrap sign <fqdn_mqtt_server> --CA "CA"

certstrap request-cert -cn <username>
certstrap sign <username> --CA "CA"
