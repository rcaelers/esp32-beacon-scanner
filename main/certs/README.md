Certificates
===============

https://github.com/square/certstrap

certstrap init --common-name "CA"
certstrap request-cert -cn <username> --ip <ip>
certstrap sign <username> --CA "CA"
