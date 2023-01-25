# Introduction

This package provides web interface for NetXMS, both legacy and new one, as well as API module.
It's build on top of Jetty10.

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

You can replace certificate in file `jetty-base/etc/keystore.jks` with your own.
Change password in file `jetty-base/start.d/ssl.ini` (property `jetty.sslContext.keyStorePassword`) if required.

It's highly recommended to use certificate issued by trusted CA like LetsEncrypt, your internal CA,
or any commecial CA instead of self-signed.

To simplify inital setup, SNI check is disabled. It's recommended to renable it by setting
property `jetty.sslContext.sniHostCheck` in file `jetty-base/start.d/ssl.ini` to `true`.


## Changing IP address of the NetXMS server

Both management console and API module are configured to connect to the NetXMS server at `127.0.0.1:4701`. If you want to change this, modify property `server` in files `jetty-base/resources/nxmc.properties` (management console) and `jetty-base/resources/nxapisrv.properties` (API).


# Configuration files

jetty-base/etc/keystore.p12 - keystore for HTTPS connector
jetty-base/etc/launcher.conf - configuration file for Jetty service launcher (e.g. memory limits)
jetty-base/start.d/http.ini - http connector configuration
jetty-base/start.d/ssl.ini - https connector configuration
jetty-base/resources/nxmc.properties - management console configuration (both legacy and new one)
jetty-base/resources/nxapisrv.properties - API configuration

# Additional information:

NetXMS administrator guide: https://netxms.org/documentation/adminguide/
Jetty10 operations guide: https://www.eclipse.org/jetty/documentation/jetty-10/operations-guide/
