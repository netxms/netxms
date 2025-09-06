
# NetXMS — enterprise-grade monitoring

NetXMS is an open-source network and infrastructure monitoring and management solution, providing performance and availability monitoring with flexible event processing, alerting, reporting and graphing for all layers of IT infrastructure. It’s a solution for every type of device — it can monitor and manage your entire IT infrastructure — from network switches to apps — all in one place.

## Architecture

![Architecture](doc/Architecture.png)

## Features

### Network monitoring

* Builds network topology and maps automatically
* Collects information via ARP caches, routing tables, LLDP, CDP, STP, switch forwarding databases
* Automatically updates peer information for all registered hosts and devices
* Provides searches for specific MAC or IP address and information about wireless access points and wireless clients
* Offers easy access to routing tables, MAC tables, and VLAN information
* Visualises IP routes
* Enables topology-based event correlations
* Supports all SNMP versions
* Has configurable routing change detection
* Provides a mechanism for handling vendor- or device-specific information and presenting it in a unified way
* Collects data via SSH
* Collects data via web services in XML, JSON and plain text format.

### User interface

* Choose between the desktop or web-based version, or use both simultaneously
* Desktop version available for Windows, Max OS X, and Linux
* Offers graphical network maps and user-configurable dashboards
* Can be integrated with [Grafana](https://grafana.com/)

### Server and workstation monitoring

* Provides agents for all popular platforms and operating systems — centralises configuration and upgrades, uses the minimum system resources, acts as proxy for other agents and SNMP devices if necessary, communicates in a firewall-friendly way, offers local cache for unstable connections
* Monitors log file contents
* Wide range of metrics out of the box: 
   * network and I/O performance; 
   * process, CPU, and memory consumption; 
   * network services; 
   * hardware sensors; 
   * application-level metrics for various applications; 
   * and many others
* File transfer capabilities built into the NetXMS agent
* Can be utilised for user support — a low-footprint application that presents users with preconfigured actions and basic information for helpdesk, configurable actions via NetXMS policies, screenshots and screencast

### Distributed monitoring

* Divides networks into zones with overlapping subnets and proxy agents for logical grouping or distributing data collection load
* Ensures automatic load balancing and failover with multiple proxy agents for each zone
* Continues to collect data in autonomous mode when the central management server is not available
* Receives syslog messages and SNMP traps from monitored devices to forward them to the central management server or local storage if the connection is down
* Eliminates the need to connect directly to each device in remote locations — a single TCP port open in either direction is enough for server-to-proxy-agent communication

### Built-in scripting engine 

* Allows for advanced automation and management
* “Hook” scripts can be called from many places within the system for custom processing
* Can be used for data transformation, complex thresholds, complex event processing rules, SNMP trap transformation, and many other purposes
* Uses easy-to-earn non-strict typed interpreted language
* Optimised for speed and low memory footprint so that a server can run hundreds of scripts simultaneously
* Each script runs inside its own VM with no access to anything outside the server process other than through well-defined APIs
* Additional security mechanisms are available to prevent unauthorised data access via scripts

### Integration

* Full Java API allows users to do everything that can be done from UI
* Permits partial or complete replacement of UI
* Rest API provides access to collected data and NetXMS configuration
* Has modular agents and servers so that their functionality can be extended by writing additional modules (plugins)
* Python-based scripting language provides access to full Java API
* Built-in integration with helpdesk systems

### Security

* Offers an internal user database or integration with an external directory using LDAP (both can be used simultaneously)
* Supports authentication with passwords, X.509 certificates, smart cards, RADIUS or LDAP server
* Uses two-factor authentication with TOTP or one-time codes sent via SMS or instant message
* Encrypts all communications
* Enables fine-grained access control configuration
* Offers an extensive audit log with optional sending to an external system

### Data and event processing

* Offers flexible policy-based event processing
* Enables alarm creation and termination, internal script execution, command execution on a management server or on a remote host via a NetXMS agent and other configurable commands
* Supports notifications via email, MS Teams, Telegram, Slack, SMS via GSM modem or SMS gateway
* Offers support for problem escalation
* Has a flexible threshold system for data collection
* Configurable with templates for simplified management of large networks

### Business services

* Translates metrics collected by NetXMS to business language.
* View vital SLA information at a glance.
* The status of the service is determined based on the status of monitored objects (servers, network devices, etc.) or metric thresholds.
* Calculate business service availability for the arbitrary time range.
* View detailed information about business service downtime, including start time, end time and cause.

## Installation

Full installation information is available [there](https://www.netxms.org/documentation/adminguide/installation.html#).

### Installing from deb repository

We host public APT repository http://packages.netxms.org/ for all deb-based distributions (Debian, Ubuntu, Mint, Raspbian, etc.). Packages are signed, and you’ll need to install additional encryption key for signature verification.

1. Download and install netxms-release-latest.deb package, which contain source list file of the repository as well as signing key:

```
wget http://packages.netxms.org/netxms-release-latest.deb
sudo dpkg -i netxms-release-latest.deb
sudo apt-get update
```

2. Then you can install the required components, e.g. to install NetXMS server for use with PostgreSQL:

```
sudo apt-get install netxms-server netxms-dbdrv-pgsql
```

3. Amend database connection details in server configuration file (/etc/netxmsd.conf).

4. Initialize database schema. 

```
nxdbmgr init
```

5. Start the NetXMS server

```
sudo systemctl start netxmsd
```

### Other options

Installers for other platforms (Windows, Aix, Solaris...) are available on [netxms.org](https://www.netxms.org/download/).


## Documentaion 

* [Administration Guide](https://www.netxms.org/documentation/adminguide/)
* [Change log](https://github.com/netxms/changelog/blob/master/ChangeLog)
* [NetXMS Scripting Language](https://www.netxms.org/documentation/nxsl-latest/)
* [Data dictionary](https://www.netxms.org/documentation/datadictionary-latest/)
* [Javadoc (NxShell and Java API)](https://www.netxms.org/documentation/javadoc/latest/)


## Support

Community support: 

* [Forum](https://www.netxms.org/forum)
* [Telegram](https://telegram.me/netxms)
* [X/Twitter](https://github.com/netxms/netxms)
* [Issue tracker](https://dev.raden.solutions/projects/netxms/)
    
Commercial support: [Raden Solutions](https://www.radensolutions.com/)
Additional professional services: [Raden Solutions](https://www.radensolutions.com/)

## License 

Most parts of NetXMS are licensed under the GNU General Public License version 2, but there are some exclusions. See [COPYING](COPYING) for more information.

