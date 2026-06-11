# Application Monitoring with NetXMS

*Capabilities overview — source material for marketing brochures and white papers.*

---

## Positioning

NetXMS is a unified monitoring platform: network, infrastructure, and applications are
monitored by one system, stored in one database, correlated by one event engine, and
presented in one console. When an application slows down, NetXMS shows the application
metrics, the server it runs on, the database behind it, and the network path in between —
without stitching together separate tools.

"Application monitoring" means different things to different customers. NetXMS covers the
full spectrum:

1. **Is the application up?** — availability and synthetic checks
2. **Is the application healthy?** — metrics from processes, databases, middleware, JVMs
3. **What is the application saying?** — log and event monitoring
4. **What does the application measure about itself?** — ingestion of metrics from
   instrumented applications (OpenTelemetry, Prometheus)
5. **Does it matter to the business?** — business services, SLA, impact analysis

---

## 1. Availability and Synthetic Checks

**HTTP/HTTPS monitoring**
- Response time measurement, HTTP status code validation
- Content checks: match response body against regular expressions, detect defacement or
  error pages via response checksums
- TLS certificate monitoring: expiration, validity, chain verification — alarms before
  certificates expire
- Full REST API polling: GET/POST/PUT/DELETE/PATCH/HEAD with custom headers, request
  bodies, and Basic/Bearer/NTLM/Digest authentication; JSON, XML, and plain-text response
  parsing turns any API response field into a monitored metric

**Network service checks**
- TCP connect, SMTP, POP3, FTP, SSH, Telnet, TLS, and custom request/response protocols
- Response time tracked and alarmed like any other metric

**Distributed vantage points**
- Any NetXMS agent can act as a checking probe, so services can be tested from where the
  users are — branch offices, DMZ, cloud regions — through secure agent tunnels and zones

**Coming in the next release**
- **Scripted multi-step transactions**: an HTTP client built into the NXSL scripting
  engine enables true synthetic journeys — log in, perform an action, verify the result —
  with cookies and session state carried across steps and per-step timing

## 2. Process and OS-Level Application Monitoring

- Per-process CPU, memory (RSS/virtual), threads, handles, open file descriptors, I/O —
  on Windows, Linux, macOS, AIX, Solaris, and the BSD family
- Process counting and existence checks with pattern matching; aggregate (sum/min/max/avg)
  across all instances of an application
- Windows-specific depth: any Performance Counter, WMI queries, service state monitoring —
  covering IIS, .NET CLR, Exchange, and any application exposing perfmon counters

## 3. Database Monitoring

Native agent-side monitors with dedicated subagents:

| Database | Coverage highlights |
|---|---|
| Oracle | Sessions, tablespaces, performance and health metrics |
| MySQL / MariaDB | Connections, InnoDB, replication, performance schema |
| PostgreSQL | Database/table statistics, transactions, cache hit ratios, replication |
| MongoDB | Collections, document and storage statistics, index metrics |
| Redis | Memory, clients, persistence, replication, keyspace |
| IBM DB2 | 50+ metrics: connections, cache, I/O, locks, transactions |
| IBM Informix | Server and database health metrics |

In addition, the **generic database query subagent** runs any SQL query against any
ODBC-accessible database (including Microsoft SQL Server) on a schedule and publishes the
results as metrics or tables — application-specific KPIs straight from the application's
own database.

## 4. Java, Middleware, and Application Servers

- **JMX monitoring**: read any MBean attribute from any JVM — application servers
  (Tomcat, JBoss/WildFly, Jetty), Kafka, and any Java application exposing JMX
- **Java subagent plugin framework** for custom Java-based monitors
- **MQTT**: connect to brokers, subscribe to topics, extract metrics from messages
  (including JSON payloads)
- **Oracle Tuxedo**: machines, servers, queues, services, clients
- Application-specific monitors: **Asterisk PBX** (SIP peers, channels, registration
  tests), **BIND DNS** (query and zone statistics)

## 5. Instrumented and Cloud-Native Applications

**OpenTelemetry (OTLP) — native ingestion**
- NetXMS accepts OTLP **metrics** (gauge, sum, histogram) and **logs** directly from
  applications instrumented with standard OpenTelemetry SDKs and collectors
- Flexible matching rules map telemetry to NetXMS nodes by resource attributes (service
  name, host, IP, custom attributes)
- Automatic instance discovery: metric attributes become DCI instances on the fly, with a
  browsable catalog of every metric each application reports
- Log severity maps to NetXMS event severity, feeding the standard alarm pipeline
- Works in reverse too: NetXMS events can be forwarded to OTLP-compatible backends

**Prometheus**
- Agent receives metrics via the Prometheus **remote write** protocol; selected metrics
  are mapped to NetXMS metric names with label-based arguments
- **Coming in the next release: active scraping** of Prometheus/OpenMetrics `/metrics`
  endpoints — unlocking the entire exporter ecosystem (Kafka, RabbitMQ, HAProxy, nginx,
  Elasticsearch, etcd, and hundreds more) with label-based instance discovery

**Push model**
- Applications can push metrics directly to NetXMS via the agent, the native push
  protocol, or the REST API — no polling required

**Containers and virtualization**
- Docker: container discovery, state tracking, CPU/memory/network metrics — agent-side or
  agentless via the Docker Engine API (cloud connector)
- Virtualization via libvirt (KVM/QEMU and others) and Xen: per-VM inventory, state, CPU,
  and network metrics

## 6. Log and Event Monitoring

- **Agent-side log parsing**: watch any application log file, match patterns, and turn log
  lines into structured NetXMS events in real time — close to the source, with only events
  crossing the network
- **Windows Event Log synchronization**: subscribe to any event log with filtering by
  source, ID, and severity
- **Built-in syslog server**: RFC 3164 and RFC 5424, automatic mapping of messages to
  monitored nodes, storage and event generation
- **OTLP log ingestion** from instrumented applications
- All sources converge in the **Event Processing Policy**: correlation, filtering,
  enrichment with NXSL scripts, alarm generation, and escalation

## 7. Custom Monitoring — Monitor Anything

When no ready-made monitor exists, NetXMS provides multiple extension points, from a
one-line config entry to a full plugin:

- **External parameters**: any script or command output becomes a metric
- **Script metrics (NXSL)**: server-side scripted data collection and transformation with
  full access to the object model
- **SSH metrics**: run commands on remote systems over SSH and collect the output —
  agentless
- **Python subagent** and **agent extension API** (JSON-RPC) for richer integrations in
  any language
- **Web service metrics**: poll any REST/XML API and extract values by path

## 8. Built-In Intelligence

- **Flexible thresholds**: last value, average, sum, deviation, delta-based, and fully
  scripted threshold conditions; multi-level thresholds with automatic rearm
- **Anomaly detection**: machine-learning based (isolation forest) detection of abnormal
  metric behavior, plus AI-assisted anomaly profiles — catch problems that fixed
  thresholds miss
- **Automatic instance discovery**: new databases, queues, endpoints, or label sets are
  discovered and monitored as they appear — across agents, SNMP, web services, scripts,
  and OpenTelemetry attributes
- **Data management**: per-metric retention, transformation scripts, and automatic
  hourly/daily aggregation for long-term capacity views

## 9. The Business View

- **Business services**: model an application as the sum of its components, with health
  checks based on object status, metric thresholds, or custom scripts
- **SLA tracking**: uptime calculation against defined service levels, with historical
  availability reporting
- **Impact analysis**: component failures propagate through the service model, so
  operations sees which business functions are affected — not just which server is down
- Dashboards and maps present application health to both engineers and management

## 10. Open Integration

- **REST API** for objects, metrics, alarms, events, and automation
- **Grafana** datasource integration for alarms and time-series data
- **Time-series export** to InfluxDB and ClickHouse for analytics pipelines
- **25+ notification channels**: Slack, Microsoft Teams, Telegram, Mattermost, Matrix,
  Google Chat, XMPP, e-mail, SMS gateways, shell scripts, and more
- **Event forwarding** to external systems, including OTLP backends
- **Help desk integration**: Jira and Redmine
- Open source (GPL), with commercial support available

---

## Summary Matrix

| Capability | Mechanism | Status |
|---|---|---|
| HTTP/HTTPS and REST API checks | Agent + server web service engine | Available |
| TLS certificate expiry monitoring | Agent network service checks | Available |
| TCP/SMTP/POP3/SSH/custom protocol checks | Agent + server | Available |
| Multi-step synthetic transactions | NXSL HTTP client | Next release |
| Process monitoring (all major OSes) | Platform subagents | Available |
| Windows perfmon / WMI / services | winperf, wmi subagents | Available |
| Oracle, MySQL, PostgreSQL, MongoDB, Redis, DB2, Informix | Dedicated subagents | Available |
| Any ODBC database (incl. MS SQL Server) | dbquery subagent | Available |
| Java/JVM via JMX | jmx subagent | Available |
| MQTT, Tuxedo, Asterisk, BIND | Dedicated subagents | Available |
| OpenTelemetry metrics and logs | Native OTLP ingestion | Available |
| Prometheus remote write | prometheus subagent | Available |
| Prometheus/OpenMetrics scraping | prometheus subagent | Next release |
| Docker containers | Subagent + cloud connector | Available |
| Log file / Windows Event Log / syslog | logwatch, wineventsync, syslog server | Available |
| Custom scripts, SSH, Python, push | Multiple extension points | Available |
| Anomaly detection (ML + AI) | Server data collection engine | Available |
| Business services and SLA | Server object model | Available |
| REST API, Grafana, InfluxDB/ClickHouse | Server integrations | Available |
