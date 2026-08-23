# NetXMS Training Agenda

*Updated for NetXMS 6.2.*

## Basic NetXMS course

The Basic NetXMS course covers everything one has to know to install, configure and maintain the
NetXMS monitoring system. Your network administrators will learn about all the system's features
and basic integration options.

1. **Concepts and system overview**

2. **Server installation**
   - A. Prerequisites
   - B. Database preparation
   - C. Server installation
   - D. Web UI installation

3. **Server maintenance**
   - A. Start/stop
   - B. Database consistency check
   - C. Backups
   - D. Scheduled tasks
   - E. Basic troubleshooting

4. **Management console**
   - A. Installation
   - B. UI elements and concepts
   - C. Web UI

5. **Agent management**
   - A. Agent installation
   - B. Security: shared secrets and certificate-based agent tunnels
   - C. External parameters
   - D. Centralized upgrade
   - E. Actions
   - F. Remote file management

6. **User management**
   - A. Adding and removing users and groups
   - B. Access rights
   - C. Two-factor authentication
   - D. Audit log

7. **Object management**
   - A. Object hierarchy
   - B. Statuses
   - C. Access control
   - D. Object creation and deletion
   - E. Node communication settings
   - F. Polling
   - G. Maintenance mode and downtime tracking
   - H. Object tools

8. **Data collection**
   - A. Data sources
   - B. DCI configuration
   - C. Thresholds
   - D. Transformation
   - E. Performance tab
   - F. Templates
   - G. Charts
   - H. DCI summary tables

9. **Package deployment and inventory**
   - A. Software package deployment
   - B. Software and hardware inventory

10. **Notification channels**

11. **Event processing**
    - A. Event processing policy
    - B. Alarms, actions and notifications
    - C. Working with event and alarm logs

12. **SNMP**
    - A. Working with SNMP devices
    - B. SNMP trap processing
    - C. MIB explorer

13. **Network maps**
    - A. Map types and configuration
    - B. Geographical maps and geo-areas

14. **Dashboards**
    - A. Working with dashboards
    - B. Dashboard configuration
    - C. Access control

## Advanced NetXMS course

The Advanced NetXMS course goes deeper into data collection and event processing, as well as
tackles network discovery and topology topics. The course is essential for business service
providers and scattered network owners, who also rely on integrations with other systems.

15. **Agent management**
    - A. External subagents (agent chaining)
    - B. Centralized configuration
    - C. Configuration policies
    - D. Proxy configuration
    - E. Agent tunnels in depth: certificate provisioning and renewal
    - F. User support application and desktop notifications

16. **User management**
    - A. RADIUS and LDAP integration
    - B. API access tokens for integrations

17. **Object management**
    - A. Zones
    - B. Object categories and object queries
    - C. Wireless domains and access points; wireless controller integration (Aruba, Ruckus, UniFi)
    - D. Physical infrastructure: racks, chassis and physical links
    - E. Circuits, collectors and sensors

18. **Data collection**
    - A. Data sources — in depth (script, SSH, MQTT, Modbus TCP, EtherNet/IP, push)
    - B. Web service (REST/HTTP) data collection
    - C. OpenTelemetry (OTLP) ingestion
    - D. Advanced DCI configuration
    - E. Scripted thresholds
    - F. Transformation
    - G. Templates automation
    - H. Instance discovery
    - I. Data collection proxy
    - J. Agent cache mode
    - K. Anomaly detection and prediction engines
    - L. Configuration export and import
    - M. Access control

19. **Notification channels**

20. **Event processing**
    - A. Event processing policy — in depth
    - B. Alarms, actions and notifications
    - C. Escalation

21. **Incident management**
    - A. Incident lifecycle and workflow
    - B. Linking alarms to incidents

22. **SNMP**
    - A. Adding new MIBs

23. **Network discovery**
    - A. Principles of operation
    - B. Configuration
    - C. Troubleshooting

24. **Network topology**
    - A. How topology information is collected
    - B. Search for IP or MAC address
    - C. IP routing
    - D. Topology-based event correlation

25. **Dashboards**
    - A. Dashboard configuration — complicated layouts and scripted elements

26. **Business services**
    - A. Services and node links
    - B. Service checks
    - C. Business service prototypes (automatic instantiation)

27. **Asset management**
    - A. Asset attribute schema
    - B. Linking assets to nodes
    - C. Automatic attribute population and compliance

28. **Network device configuration backup**
    - A. Built-in device configuration backup
    - B. Oxidized integration

29. **NXSL (built-in scripting)**
    - A. Language basics
    - B. Standard library
    - C. Hooks and data transformation
    - D. Typical usage patterns

30. **Log monitoring**
    - A. Agent-side log parsing
    - B. Syslog server
    - C. Windows event log monitoring and synchronization

31. **Network service monitoring**

32. **AI assistant**
    - A. Configuration and AI providers
    - B. Working with the assistant
    - C. AI tasks and skills

33. **Integration with other systems**
    - A. REST API (WebAPI) and OpenAPI specification
    - B. Grafana
    - C. Helpdesk integration (Jira, Redmine)
    - D. Performance data forwarding (InfluxDB, ClickHouse, RRDtool)
    - E. Event forwarding between NetXMS servers
    - F. Automation with nxshell
    - G. MCP server and AI-driven integrations

34. **Reporting**
    - A. Reporting server installation and configuration
    - B. Execution and scheduling
    - C. Deployment of new or updated report definitions
    - D. Introduction to designing new reports

35. **Migration from earlier major releases**
    - A. Major differences between versions
    - B. Preparing for migration
    - C. Migration procedure
