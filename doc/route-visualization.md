# Network Route Visualization

## Overview

The route visualization renders a calculated network path (trace path) as a vertical stepper/timeline. Each hop in the path is a known managed NetXMS node. The visualization is designed for both troubleshooting ("where does the path break?") and documentation ("what is the route between two nodes?").

## Data Format

The visualization receives a JSON object with the following structure:

```json
{
   "type": "route",
   "title": "Path: WebServer -> DatabaseServer",
   "isComplete": true,
   "hops": [
      {
         "objectId": 142,
         "objectName": "WebServer",
         "type": "ROUTE",
         "ifIndex": 3,
         "nextHop": "10.0.1.1",
         "route": "10.0.2.0/24",
         "name": "eth0"
      },
      {
         "objectId": 85,
         "objectName": "CoreRouter",
         "type": "VPN",
         "vpnConnectorId": 201,
         "vpnConnectorName": "HQ-Branch VPN"
      }
   ]
}
```

### Top-Level Fields

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | Always `"route"` |
| `title` | string | Display title (e.g., "Path: NodeA -> NodeB"). Falls back to "Network Path" if absent. |
| `isComplete` | boolean | Whether the full path was resolved. If `false`, the visualization shows a broken-path indicator at the end. |
| `hops` | array | Ordered list of hop objects (see below). |

### Hop Object Fields

Every hop has these common fields:

| Field | Type | Description |
|-------|------|-------------|
| `objectId` | integer | NetXMS node object ID |
| `objectName` | string | Node display name |
| `type` | string | One of: `ROUTE`, `VPN`, `PROXY`, `DESTINATION`, `L2_LINK` |
| `name` | string | Interface or element name (present on most types) |

Additional fields vary by hop type:

**ROUTE** — standard IP routing hop:
- `ifIndex` (integer, optional) — outbound interface index
- `nextHop` (string) — next hop IP address
- `route` (string) — matching route prefix (e.g., "10.0.2.0/24")

**VPN** — VPN tunnel hop:
- `vpnConnectorId` (integer) — VPN connector object ID
- `vpnConnectorName` (string) — VPN connector display name

**PROXY** — proxy forwarding hop:
- `proxyNodeId` (integer) — proxy node object ID
- `proxyNodeName` (string) — proxy node display name

**L2_LINK** — layer 2 link:
- `ifIndex` (integer, optional) — interface index

**DESTINATION** — marks the final destination node (no additional fields, always the last hop if present)

## Visual Layout

### Structure

The visualization uses a vertical timeline layout:

```
[Header: title | Complete/Incomplete badge | hop count]
─────────────────────────────────────────────────────
  O  NodeName [objectId]
  |  ROUTE  eth0 -- next hop 10.0.1.1 -- via 10.0.2.0/24
  |
  O  NodeName [objectId]
  :  VPN  HQ-Branch VPN       (dashed line for VPN)
  :
  O  NodeName [objectId]
  |  L2_LINK  ge-0/0/1
  |
  O  NodeName [objectId]
     DESTINATION  Destination
```

Each hop row consists of two parts side by side:

1. **Spine** (left) — a colored circle (dot) connected to the next hop by a vertical line
2. **Content** (right) — node name with object ID, and a detail line with a type badge and type-specific information

### Header

A horizontal bar at the top containing:
- **Title** — bold, left-aligned (e.g., "Path: WebServer -> DatabaseServer")
- **Status badge** — "Complete" (green) or "Incomplete" (orange/warning)
- **Hop count** — right-aligned, muted text (e.g., "12 hops")

### Hop Row

Each hop is rendered as a row with a fixed-width spine column on the left and a content area on the right.

**Spine column:**
- A filled circle (12px diameter), colored by hop type
- A vertical line (2px wide) extending down to the next hop's dot, colored to match the current hop type
- For VPN and PROXY hops, the connecting line is dashed instead of solid, visually indicating a non-direct link
- The last hop in a complete path has no line below its dot
- If the path is incomplete, the line continues from the last hop to a broken-path indicator

**Content area:**
- **First line:** node name (bold, 0.9rem) followed by object ID in brackets (muted, 0.75rem)
- **Second line:** a type badge (small uppercase label with colored background/border) followed by type-specific detail text (muted, 0.8rem)

### Detail Text by Hop Type

- **ROUTE:** `{interfaceName} -- next hop {nextHop} -- via {route}` (parts joined with em-dash, omitting absent fields)
- **VPN:** `{vpnConnectorName}` or fallback `VPN connector {vpnConnectorId}`
- **PROXY:** `{proxyNodeName}` or fallback `Proxy node {proxyNodeId}`
- **L2_LINK:** `{name}` or fallback `L2 link`
- **DESTINATION:** `Destination`

### Incomplete Path Indicator

When `isComplete` is `false`, a final pseudo-hop is appended after the last real hop:
- Spine dot rendered as a hollow circle with a dashed orange border
- Node name: "Path incomplete" (orange, italic)
- Detail text: "Next hop could not be resolved" (orange, italic)

## Color Scheme

Each hop type has a distinct color used for the spine dot, connecting line, and type badge:

| Hop Type | Color | Usage |
|----------|-------|-------|
| ROUTE | Blue | Most common, standard routing |
| VPN | Purple | VPN tunnel traversal, dashed connecting line |
| PROXY | Orange | Proxy-forwarded hop, dashed connecting line |
| DESTINATION | Green | Final destination node |
| L2_LINK | Teal | Layer 2 connectivity |

The type badge for each hop uses a light tinted background with a matching border and darker text in the same hue (e.g., ROUTE badge: light blue background, blue border, dark blue text).

## CSV Export

The visualization supports exporting path data as CSV with the following columns:

```
Hop,Node,Node ID,Type,Details
1,WebServer,142,ROUTE,eth0 -- next hop 10.0.1.1 -- via 10.0.2.0/24
2,CoreRouter,85,VPN,HQ-Branch VPN
```

## Scrolling Behavior

The path list scrolls vertically, making it suitable for long paths (10+ hops) without horizontal overflow. The header remains fixed at the top.
