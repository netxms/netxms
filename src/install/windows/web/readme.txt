# Introduction

This package provides web interface for NetXMS.
It's built on top of Jetty 12.

Default setup have both HTTP and HTTPS connectors enabled, with self-signed certificate.

HTTP connector is available at http://localhost:4788/
HTTPS connector is available at https://localhost:4733/

Certificate is issued to localhost and not trusted by any CA, so you will get warning in browser.
It's worth to mention that SNI check is disabled in default setup, so you can access HTTPS connector
using any hostname (or even IP address).

# Configuration

## Changing ports

HTTP connector - modify `jetty.http.port` in file `jetty-base/start.d/http.ini`.
HTTPS connector - modify `jetty.ssl.port` in file `jetty-base/start.d/ssl.ini`.

If you want to disable http connector entirely, comment out line `--module=http`
in file `jetty-base/start.d/http.ini`.


## Changing certificate

You can replace certificate in file `jetty-base/etc/keystore.p12` with your own.
Change password in file `jetty-base/start.d/ssl.ini` (property `jetty.sslContext.keyStorePassword`) if required.

It's highly recommended to use certificate issued by trusted CA like LetsEncrypt, your internal CA,
or any commecial CA instead of self-signed.

To simplify initial setup, SNI check is disabled. It's recommended to re-enable it by setting
properties `jetty.ssl.sniHostCheck` and `jetty.ssl.sniRequired` in file `jetty-base/start.d/ssl.ini` to `true`.


## Changing IP address of the NetXMS server

Management console is configured to connect to the NetXMS server at `127.0.0.1:4701`. If you want to change this, modify property `server` in file `jetty-base/resources/nxmc.properties`.


# Configuration files

jetty-base/etc/keystore.p12 - keystore for HTTPS connector
etc/launcher.conf - configuration file for Jetty service launcher (e.g. memory limits)
jetty-base/start.d/http.ini - http connector configuration
jetty-base/start.d/ssl.ini - https connector configuration
jetty-base/resources/nxmc.properties - management console configuration

# Additional information:

NetXMS administrator guide: https://netxms.org/documentation/adminguide/
Jetty 12 operations guide: https://jetty.org/docs/jetty/12/operations-guide/index.html
