Certificates
===============

https://github.com/square/certstrap

certstrap init --common-name "CA"
certstrap request-cert -cn <hostname> --ip <ip>
certstrap sign <hostname> --CA "CA"
