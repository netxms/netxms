### NetXMS Overview

#### Architecture
- **Three-Tier System**:
  - **Agents**: Collect data (NetXMS or SNMP).
  - **Server**: Processes and stores data.
  - **Clients**: Access via Desktop Client, Web Client, or mobile app.

#### Objects
- **Object Classes**:
  - **Entire Network**: Root of IP topology.
  - **Zone**: Group of interconnected networks.
  - **Subnet**: Represents IP subnets.
  - **Node**: Physical hosts or devices.
  - **Interface**: Network interfaces of nodes.
  - **Service Root**: Infrastructure service root.
  - **Container**: Groups objects logically.
  - **Cluster**: Aggregates information from nodes.
  - **Chassis**: Represents enclosures like blade servers.
  - **Rack**: Visual representation of equipment.
  - **Condition**: Complex conditions with scripts.
  - **Network Service**: Services like HTTP, SSH.
  - **VPN Connector**: Represents VPN endpoints.
  - **Sensor**: Logical data collection points.
  - **Wireless Domain**: Represents wireless networks.
  - **Access Point**: Thin wireless access points.
  - **Template Root**: Root of templates.
  - **Asset Root**: Root of hardware assets.
  - **Network Map Root**: Root of network maps.
  - **Dashboard Root**: Root of dashboards.
  - **Business Service Root**: Root of business services.

#### Object Status
- **Status Levels**: Normal, Warning, Minor, Major, Critical, Unknown, Unmanaged, Disabled, Testing.
- **Maintenance Mode**: Prevents event processing but continues data collection.

#### Event Processing
- **Sources**: Polling processes, SNMP traps, external applications.
- **Event Queue**: Sequential or parallel processing with defined policies.

#### Polling
- **Types**:
  - **Status Polling**: Check object status.
  - **Configuration Polling**: Gather configuration details.
  - **Topology Polling**: Discover network topology.
  - **ICMP Polling**: Ping for response statistics.

#### Data Collection
- **Sources**: Internal, agents, SNMP, web services, push, Windows counters, etc.
- **Metrics**: Single-value, list, table.

#### Discovery
- **Network Discovery**:
  - **Passive Mode**: Non-intrusive, querying ARP/routing tables.
  - **Active Mode**: Uses ICMP for scanning.
- **Topology Discovery**: Identifies network link layer topology and IP routing.

#### Security
- **Encryption**: AES-256, AES-128, Blowfish.
- **Authentication**: IP whitelist, preshared keys.
- **Password Protection**: Salted hash with SHA-256.

