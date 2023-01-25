SSL certificate: jetty-base/etc/keystore.p12

generate self-signed certificate:

    openssl genrsa -out server-key.pem
    openssl req -new -key server-key.pem -out server-cert.req
    openssl x509 -req -signkey server-key.pem -in server-cert.req -days 365 -out server-cert.pem
    openssl pkcs12 -export -inkey server-key.pem -in server-cert.pem -out jetty-base/etc/keystore.p12 -password pass:example1

Custom logo:

jetty-base/resources/logo.jpg

API configuration:

jetty-base/resources/nxapisrv.properties

NXMC configuration:

jetty-base/resources/nxmc.properties

HTTP port:

"jetty.http.port" in jetty-base/start.d/http.ini

to disable http connector - delete http.ini

SSL configuration:

jetty-base/start.d/ssl.ini

Parameters jetty.ssl.port, jetty.sslContext.keyStorePath, jetty.sslContext.keyStorePassword

Web apps should be placed in jetty-base/webapps
