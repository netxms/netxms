# NetXMS as a Data Center Infrastructure Management (DCIM) Platform

**Applies to:** NetXMS 6.2 (current master) plus the upcoming **3D rack visualization**
feature (development branch `claude/ecstatic-noether-VgijM`, scheduled for the next release).

---

## 1. Introduction

NetXMS is an enterprise-grade, open-source network and infrastructure monitoring
platform. While it is most widely known for network and server monitoring, its object
model, physical-layout features, environmental sensor support, asset register, and
visualization tools make it a capable **Data Center Infrastructure Management (DCIM)**
solution.

This document describes how to use NetXMS to model, monitor, and manage a data center:
the physical infrastructure (racks, chassis, cabling), the powered equipment (servers,
network gear, PDUs, UPS), the environment (temperature, humidity, leak, smoke, power),
the asset register, and the dashboards/maps that tie it all together.

A DCIM deployment with NetXMS typically combines the following building blocks:

| DCIM concern | NetXMS capability |
|---|---|
| Physical layout (racks, units, elevation) | Rack objects + 2D and **3D** rack views |
| Equipment placement | Nodes / Chassis bound to racks with U-position, height, orientation |
| Passive infrastructure | Passive rack elements (PDU, patch panel, organiser, filler) |
| Cabling / patching | Physical Link objects (port-to-port and patch-panel mapping) |
| Asset register | Asset Management module (schema-driven custom attributes) |
| Hardware inventory | Automatic hardware component collection |
| Environmental monitoring | Sensor objects (temperature, humidity, CO₂, leak, smoke, …) |
| Power monitoring | UPS subagent, PDU/SNMP drivers, MODBUS power meters |
| Industrial / metering protocols | Native MODBUS TCP/RTU support |
| Geographic placement | World map / geolocation per object |
| Logical topology | Automatic L2/L3 network maps |
| Server-room floor plan | Network map in floor-plan mode: clickable racks + live metrics |
| Availability / SLA | Business Services with composite checks |
| Visualization | Dashboards (gauges, charts, rack diagrams, geo/status maps) |
| Alerting | Events, thresholds, alarms, notification channels |

---

## 2. Modeling the physical data center

### 2.1 Racks

Racks are first-class objects in the NetXMS object model. A **Rack** object represents a
physical cabinet and defines its vertical capacity and numbering convention.

Server implementation: `src/server/core/rack.cpp`, class definition in
`src/server/include/nms_objects.h`.

Rack attributes include:

- **Height** in rack units (U) — default 42U.
- **Numbering direction** — units numbered top-to-bottom or bottom-to-top.
- A collection of **passive rack elements** (see below).

Active equipment (Nodes and Chassis) is bound into a rack with these object-level
placement fields (e.g., on a Node: `physical_container_id`, `rack_position`,
`rack_height`, `rack_orientation`, `rack_image_front`, `rack_image_rear`):

- **Physical container** — the rack (or chassis) that houses the device.
- **Rack position** (the starting U-number of the device).
- **Height** in U occupied by the device.
- **Orientation** — `FRONT`, `REAR`, or `FILL` (full depth).
- **Front and rear images** (referenced from the image library by UUID) so the elevation
  shows a realistic faceplate.

Rack and passive-element data is persisted in the `racks` and `rack_passive_elements`
tables and exposed over the REST API at `/v1/objects/:object-id/rack-layout`, which
returns the rack metadata, its passive elements, and the placed child objects with their
geometry — useful for external DCIM integrations and reporting.

### 2.2 Passive rack elements

Not everything in a rack is a managed device. NetXMS models passive infrastructure as
**passive rack elements** attached directly to the rack. Element types (`RackElementType`):

| Type | Purpose |
|---|---|
| `PATCH_PANEL` | Patch panels (with a configurable port count, used by Physical Links) |
| `FILLER_PANEL` | Blanking / filler panels |
| `ORGANISER` | Cable organisers |
| `PDU` | Power distribution units (as a passive layout element) |

Each passive element carries a name, type, U-position, height, orientation, port count,
and front/rear image references — so the rack elevation reflects the real cabinet contents
including non-powered hardware.

### 2.3 Chassis

A **Chassis** object (`src/server/core/chassis.cpp`) models a multi-slot enclosure such as
a blade chassis or modular switch. A chassis has a **controller node**, can be bound to a
rack with its own position, height, orientation, and front/rear images, and groups the
member nodes (blades/modules) contained within it (optionally bound under the controller).
This lets you represent stacked and blade systems accurately in the physical layout. The
chassis component layout is available over REST at
`/v1/objects/:object-id/chassis-layout`.

Interfaces additionally carry their **physical location** (chassis / module / PIC / port),
so switch and chassis port positions are tracked down to the individual port.

### 2.4 Sites, rooms, and floors (containers)

Above the rack level, the physical hierarchy is modeled with **Container** objects
(`OBJECT_CONTAINER`). Containers are nestable and support **auto-bind** rules, so you can
build a site → building → floor → room → rack tree and have equipment placed into the
right branch automatically based on attributes. Every object (containers included) can also
carry a **postal address** (country, region, city, district, street address, postcode) and
a geolocation (§9), so facilities carry real-world location metadata.

### 2.5 Physical links (cabling and patching)

NetXMS documents the **physical cabling plant** with **Physical Link** objects
(`src/server/core/physical_link.cpp`). Each link records both endpoints, where an endpoint
can be a device interface or a specific patch-panel port:

- Object ID and parent interface for each side,
- Patch panel ID and port number,
- Front/back side of the panel,
- A free-text description.

This supports port-to-port cabling documentation, patch-panel cross-connect mapping, and
bidirectional link tracking, all persisted in the database and exportable as JSON. It is
the basis for answering "what is plugged into what" across the data center.

---

## 3. Rack visualization

### 3.1 2D rack view

The management console renders racks as a realistic front/rear elevation. Devices are
drawn at their correct U-position and height using their library images, and selecting a
device shows its details. In 6.2 the selection model was unified so that clicking an
element highlights it and surfaces an information card (see §3.3).

The rack diagram is also available as a **dashboard element** (`RackDiagramElement`,
`src/client/nxmc/java/.../dashboards/widgets/RackDiagramElement.java`), so rack elevations
can be embedded in operational dashboards.

### 3.2 3D rack view *(upcoming — branch `claude/ecstatic-noether-VgijM`)*

The upcoming release adds an interactive **3D rack view** alongside the existing 2D view.

**Access and persistence**
- A **"3D view"** toggle (checkbox) is added to the rack view toolbar.
- The 2D/3D choice is stored per user in the preference store
  (`RackView.Use3D`) and remembered across sessions.
- Both the 2D and 3D views feed the same shared information card, so the experience is
  consistent regardless of mode.

**Technology**
- Rendered with **WebGL via Three.js** inside the standard SWT/RWT `Browser` widget — so
  it works in **both the desktop (SWT) and web (RWT) clients**, using the same pattern as
  other browser-hosted widgets.
- Java owns the model (rack contents, status colors, theme colors); JavaScript owns the
  geometry, camera, lighting, and click-picking. They communicate through browser-function
  callbacks (`nxSetScene` Java→JS; `nxOnSelect`, `nxGetTexture` JS→Java).
- The Three.js library is bundled separately under
  `src/client/nxmc/java/src/main/resources/webgl/`; if it is not present the widget shows a
  graceful "library not bundled" placeholder rather than failing.

**Visual features**
- **Cabinet rendering** — the rack is drawn as a cabinet with semi-transparent
  (glass-like) side/top/bottom panels and subtle themed edge outlines; the front and rear
  faces are open so device faces and rear images are visible.
- **Device boxes** at correct rack coordinates for both active devices and passive
  elements, with front/rear images mapped onto the corresponding faces.
- **Depth derived from orientation/type** — `FRONT`/`REAR` devices occupy half the rack
  depth at the matching end, `FILL` devices span the full depth; passive elements use
  type-specific depths (filler ≈ 15 mm, patch panel ≈ 40 mm, organiser ≈ 60 mm, PDU ≈
  100 mm).
- **Unit numbers** rendered on the front rail, honoring the rack's numbering direction.
- **Status markers** — color-coded warning triangles (based on the Lucide *triangle-alert*
  icon) appear on the rail for abnormal devices (WARNING…CRITICAL), so problem equipment is
  identifiable at a glance without selecting it.
- **Selection highlighting** — the selected element gets a status-colored bright edge and a
  subtle emissive glow; its label (name + status) is shown.
- **Orbit controls** — pan, zoom, and rotate the camera without affecting the selection.
- **Theme aware** — background, foreground, edge, and text colors are queried from the
  "Rack" theme element and update dynamically with light/dark theme changes.

### 3.3 Shared selection information card

Both views present selection details in a fixed-width floating information card so the
layout does not shift when the selection changes:

- **Active devices (Node/Chassis):** object name (title), a colored status pill, primary
  IP address, hardware vendor/model/version/serial, SNMP location and contact, and rack
  position / height / side.
- **Passive elements:** name or type (title), element type, position, height, and side.

Empty fields are omitted automatically.

> **Scope note (3D MVP):** the first 3D release renders `FRONT`/`REAR`/`FILL` orientations
> and type-based depths. True per-device width/depth, cable/physical-link visualization, and
> arbitrary rotation are out of scope for the initial version.

---

## 4. Asset management (asset register)

NetXMS includes a dedicated **Asset Management** subsystem
(`src/server/core/asset_management.cpp`, `src/server/core/asset.cpp`,
`src/server/include/asset_management.h`) that acts as the data center's asset register.

**Schema-driven attributes** — administrators define the asset attribute schema centrally.
Each attribute has:

- A **data type**: String, Integer, Number, Boolean, Enum, MAC Address, IP Address, UUID,
  Object Reference, or Date.
- Optional **system type** semantics for common fields: Serial, IP Address, MAC Address,
  Vendor, Model.
- **Constraints**: mandatory, unique, hidden; min/max range; enumeration value lists.
- **Autofill** via NXSL script, so attribute values can be populated automatically from the
  linked object (e.g., pull serial number / vendor / model from collected hardware data).

**Asset objects** track the custom property values, the **linked object** (the
network/device object the asset corresponds to), and the last-update timestamp and user.
Assets can be organized into **asset groups** (`OBJECT_ASSETGROUP`) under an asset-tree root
(`OBJECT_ASSETROOT`), and the server can **automatically link** an asset to its monitored
object by matching MAC address, IP address, or serial number.

Every change to an asset is recorded in an **asset change log** (audit history) — old/new
value, attribute, operation, user, and timestamp — which is important for compliance and
inventory governance. Data is persisted in the `assets`, `asset_properties`, and
`asset_change_log` tables, and asset CRUD plus schema management are available over the
REST API.

The management console provides full asset views, property editors, and a schema manager
under `src/client/nxmc/java/.../modules/assetmanagement/`. This gives data center operators
a customizable CMDB-style register (serial, vendor, model, owner, location, purchase/warranty
dates, etc.) directly linked to the live monitored objects.

---

## 5. Hardware inventory

For agent- and SNMP-managed devices, NetXMS automatically collects a **hardware component
inventory** (`src/server/core/hwcomponent.cpp`, stored in the `hardware_inventory` table).
Components are categorized (processors, memory, storage, power supplies, fans,
network/expansion cards, etc.) and each carries index, type, vendor, model, part number,
serial number, location, capacity, and description. The server performs **change
detection** (added/removed) between polls, so you can track hardware additions and
removals over time.

For SNMP devices, NetXMS additionally parses the **ENTITY-MIB** physical component tree
(`src/server/libnxsrv/entity_mib.cpp`, stored in `node_components`), capturing the
chassis/module/port hierarchy with class, name, description, model, serial number, vendor,
and firmware. Together these complement the asset register with automatically-discovered
ground truth about what is physically inside each device.

---

## 6. Environmental monitoring

### 6.1 Sensor objects

NetXMS models environmental and metering devices as **Sensor** objects
(`src/server/core/sensor.cpp`). Sensors are pollable objects (status and configuration
polling) and support the following device classes:

| Class | Measures |
|---|---|
| `SENSOR_TEMPERATURE` | Temperature |
| `SENSOR_HUMIDITY` | Humidity |
| `SENSOR_TEMPERATURE_HUMIDITY` | Combined temperature + humidity |
| `SENSOR_CO2` | CO₂ concentration |
| `SENSOR_SMOKE` | Smoke detection |
| `SENSOR_WATER_LEAK` | Water/leak detection |
| `SENSOR_WATER_METER` | Water consumption |
| `SENSOR_ELECTRICITY_METER` | Electrical energy |
| `SENSOR_CURRENT` | Current draw |
| `SENSOR_POWER_SUPPLY` | Power supply status |
| `SENSOR_UPS` | UPS |
| `SENSOR_OTHER` | Generic / other |

Each sensor records identity metadata (MAC address, vendor, model, serial number, device
address), and integration parameters such as **MODBUS unit ID** and the **gateway node**
through which it is reached. This is the core of data center environmental monitoring:
rack-level and room-level temperature/humidity, leak detection under raised floors, smoke
detection, and energy/current metering.

### 6.2 Rittal CMC / LCP and other environmental controllers

NetXMS ships a dedicated driver for **Rittal CMC and LCP** devices
(`src/server/drivers/rittal/`) — the widely used data center environmental and cooling
monitoring/control units (CMC = Computer Multi Control). Combined with the generic
`net-snmp` driver and vendor drivers, this allows direct monitoring of rack-mounted
environmental gateways and their attached temperature/humidity/leak/door/power probes.

---

## 7. Power monitoring

### 7.1 UPS

The **UPS subagent** (`src/agent/subagents/ups/`) monitors uninterruptible power supplies
over multiple protocols and vendor families, including:

- **APC** (`apc.cpp`),
- **Megatec** (`megatec.cpp`),
- **BCM/XCP** (`bcmxcp.cpp`),
- **Metasys** (`metasys.cpp`),
- **Microdowell** (`microdowell.cpp`),
- **MEC0003** (`mec0003.cpp`),
- Generic **serial** UPS (`serial.cpp`).

This exposes battery charge, runtime, input/output voltage, load, and status as data
collection metrics that can be charted, thresholded, and alarmed.

### 7.2 PDUs and power metering

Power distribution can be monitored several ways:

- **SNMP** — managed/metered PDUs are monitored via the generic `net-snmp` driver (and
  vendor drivers where applicable), collecting per-outlet and aggregate load, voltage, and
  energy.
- **MODBUS** — power meters and energy analyzers are read directly via NetXMS' native
  MODBUS support (§8).
- Within the rack model, PDUs also appear as **passive rack elements** so they are
  represented in the physical elevation, and **dell-pwc** provides Dell PowerConnect
  coverage among the device drivers.

---

## 8. MODBUS and industrial protocols

NetXMS has **native MODBUS** support (`src/server/libnxsrv/modbus.cpp`,
`src/server/include/nxsrvapi.h`), exposed as a first-class **data origin** (`MODBUS`)
for data collection items — alongside Agent, SNMP, SSH, Script, MQTT, EtherNet/IP, and
device-driver origins.

Supported operations:

- Read holding registers (with numeric format conversion),
- Read input registers,
- Read coils (boolean),
- Read discrete inputs,
- Device identification (FC 43): vendor name, product code, revision, vendor URL, product
  name, model name.

Transport modes:

- **`ModbusDirectTransport`** — direct MODBUS/TCP connection to the device,
- **`ModbusProxyTransport`** — connection through a NetXMS agent acting as a proxy
  (MODBUS/RTU gateways, segmented networks).

This makes NetXMS able to read power meters, energy analyzers, environmental controllers,
and other industrial equipment common in data center facilities infrastructure directly,
without an intermediate gateway.

---

## 9. Location and topology

### 9.1 Geographic placement

Every object can carry a **geolocation**, and NetXMS provides a **world map** view and
geolocation cache (`src/client/nxmc/java/.../modules/worldmap/`). For multi-site data
centers, this places facilities on a real-world map with status roll-up. A **Geo Map**
dashboard element (`GeoMapElement`) embeds the same view into dashboards. Geographic areas
can also be defined for grouping and alerting.

### 9.2 Network / topology maps

NetXMS automatically builds **L2 and L3 topology maps** and supports predefined and
ad-hoc maps (`src/client/nxmc/java/.../modules/networkmaps/views/`): L2 topology, IP
topology, internal topology, VLAN maps, and predefined/context maps. Combined with
physical-link data, this connects the logical network picture to the physical cabling
plant.

### 9.3 Floor plan view (2D server-room plan)

A predefined network map can be switched into **floor plan display mode** — set the map
object display mode to `FLOOR_PLAN`
(`MapObjectDisplayMode.FLOOR_PLAN`; rendered by
`src/client/nxmc/java/.../networkmaps/widgets/helpers/ObjectFloorPlan.java`). In this mode
objects are drawn as positioned, resizable rectangles over the map canvas, so you can lay
out a **2D plan of a server room** by placing **rack objects** on it at their real floor
positions.

Two things make this especially useful for DCIM:

- **Clickable racks with drill-down.** Rack objects placed on the floor plan are live and
  clickable — opening a rack drills straight down into its rack view (2D or, with the
  upcoming feature, **3D**), so the floor plan becomes a navigable map of the room.
  Each rack figure also reflects the object's current status color, giving an at-a-glance
  health overview of the whole room.
- **Vital metrics placed directly on the plan.** Besides racks, you can drop live data
  onto the same map using map decoration elements:
  `NetworkMapDCIContainer` (a box showing one or more DCI values),
  `NetworkMapDCIImage` (a threshold-driven image/indicator),
  and `NetworkMapTextBox`. This lets you show, for example, the **server-room temperature**
  (or humidity, total power draw, UPS load, …) right on the floor plan next to the racks it
  describes, with color/threshold reaction.

The result is a single interactive room view that combines spatial layout, per-rack status
and drill-down, and live environmental/power readings — exactly the "control-room" picture
a data center operator expects.

## 10. Availability and SLA (Business Services)

NetXMS **Business Services** (`src/server/core/bizservice.cpp`,
`src/server/core/bizsvccheck.cpp`) model the availability of data center services as
composite health checks. Check types:

- **OBJECT** — based on a related object's status,
- **DCI** — based on a data collection item / threshold,
- **SCRIPT** — arbitrary NXSL evaluation.

Services support prototype-based definitions with **instance discovery** (agent list/table,
SNMP walk, script, or manual), auto-bind, configurable status thresholds, failure-reason
tracking, and ticketing integration. Availability is tracked over time, enabling SLA
reporting for hosted services, customer environments, and critical infrastructure.

---

## 11. Data collection, alerting, and visualization

The DCIM-specific objects above plug into NetXMS' standard monitoring engine:

- **Data collection** — metrics from agents, SNMP, MODBUS, scripts, SSH, etc., with
  thresholds, instance discovery, and historical storage.
- **Events and alarms** — threshold violations and status changes generate events that are
  correlated into alarms.
- **Notifications** — alarms drive notification channels (email, SMS, messaging,
  webhooks, etc.).
- **Dashboards** — assemble operational views from a rich element set:
  - `RackDiagramElement` — embed rack elevations,
  - `GeoMapElement` / `StatusMapElement` / `NetworkMapElement` — spatial and topology views,
  - `GaugeElement`, line/bar/pie/comparison charts (including scripted and table-based),
  - `AvailabilityChartElement` — service availability,
  - `ObjectStatusChartElement`, `StatusIndicatorElement`, `DciSummaryTableElement`,
    alarm/event/syslog/SNMP-trap monitors, and more.

A typical DCIM dashboard combines a rack diagram, environmental gauges (temperature/
humidity), power charts (UPS/PDU load), a geo or status map of facilities, and an alarm
list.

---

## 12. Integration (REST API and scripting)

DCIM data is not locked inside the console. The NetXMS **WebAPI** (`src/server/webapi/`)
exposes the physical model over REST — including rack layout
(`/v1/objects/:object-id/rack-layout`), chassis layout
(`/v1/objects/:object-id/chassis-layout`), objects, data collection values, and asset
data — so external CMDBs, capacity-planning tools, and reporting systems can consume the
data center model and live metrics. **NXSL** scripting provides the same reach internally
for autofill, auto-bind, business-service checks, and automation. The Java client library
offers programmatic access to racks, chassis, sensors, assets, and physical links.

## 13. Putting it together — a DCIM rollout outline

1. **Model the facility.** Create container objects for sites/rooms; create **Rack**
   objects with correct height and numbering.
2. **Place equipment.** Add Nodes/Chassis, bind them to racks with U-position, height, and
   orientation, and assign front/rear images for realistic elevations. Add **passive
   elements** (PDUs, patch panels, organisers, fillers).
3. **Document cabling.** Create **Physical Link** objects for port-to-port and
   patch-panel connections.
4. **Add environment.** Create **Sensor** objects (temperature, humidity, leak, smoke,
   CO₂, metering); for Rittal CMC/LCP and SNMP gateways use the relevant drivers; for power
   meters use MODBUS.
5. **Add power.** Monitor UPS via the UPS subagent and PDUs via SNMP/MODBUS.
6. **Build the asset register.** Define the asset schema (serial, vendor, model, owner,
   warranty, location…), enable autofill from hardware inventory, and link assets to
   objects.
7. **Set geolocation** for each facility for the world map.
8. **Define services and SLAs** with Business Services.
9. **Create dashboards and a floor plan.** Combine rack diagrams (2D/3D), environmental and
   power gauges, maps, and alarm lists; build a **floor-plan network map** of each server
   room with clickable racks (drill-down to rack view) and live metrics (room temperature,
   humidity, power) placed directly on the plan.
10. **Configure alerting** — thresholds, events, alarms, and notification channels for
    temperature, humidity, leak, power, and equipment-status conditions.

---

*Document version: NetXMS 6.2 baseline, plus the 3D rack visualization feature from branch
`claude/ecstatic-noether-VgijM` (next release).*
