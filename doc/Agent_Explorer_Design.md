# Agent Explorer Design (NX-879)

## Overview

The Agent Explorer is a dedicated view for browsing and interacting with NetXMS agent data (parameters, lists, and tables). It provides a unified interface similar to the MIB Explorer, solving current UX issues where viewing agent data requires multiple tools (nxget, NXSL) and creating DCIs is disconnected from data exploration.

## Goals

- **Unified exploration** - Browse all agent parameters, lists, and tables in one place
- **Live querying** - View current values without creating DCIs or using command-line tools
- **Streamlined DCI creation** - Create DCIs directly from discovered items
- **Instance discovery integration** - Connect parameterized metrics with their related lists

## User Interface

### View Structure

The Agent Explorer is an `AdHocObjectView` functioning both as:
- A **Tool view** accessible from the Tools perspective
- A **Node context view** that opens for a specific node

### Layout

Three-pane split layout (similar to MIB Explorer):

```
┌─────────────────────────────────────────────────────────────────┐
│  [Node selector dropdown]  [Refresh] [Walk] [Query]             │
├──────────────────────┬──────────────────────────────────────────┤
│                      │                                          │
│   Agent Data Tree    │         Details Panel                    │
│                      │                                          │
│   ▼ Parameters       │   Name: System.CPU.Usage                 │
│     ▼ System         │   Type: Float                            │
│       ▼ CPU          │   Description: Average CPU usage for     │
│         Usage        │                last minute               │
│         Usage5       │                                          │
│         Usage15      │   Last Queried Value: 23.5               │
│       Hostname       │   Query Time: 2026-01-14 10:23:45        │
│     ▼ Agent          │                                          │
│   ▼ Lists            │                                          │
│   ▼ Tables           │                                          │
│                      │                                          │
├──────────────────────┴──────────────────────────────────────────┤
│                     Query Results Table                         │
│  Name                      │ Type   │ Value          │ Status   │
│  System.CPU.Usage          │ Float  │ 23.5           │ OK       │
│  System.CPU.Usage5         │ Float  │ 18.2           │ OK       │
│  System.Memory.Physical... │ UInt64 │ 8520343552     │ OK       │
└─────────────────────────────────────────────────────────────────┘
```

### Tree Structure

**Three root nodes:**
- Parameters (icon: gauge/metric)
- Lists (icon: list/bullet points)
- Tables (icon: grid/table)

**Smart grouping algorithm:**

Items are organized hierarchically based on their dot-notation names with intelligent grouping:

1. First segment becomes top-level category (`System`, `Agent`, `FileSystem`, etc.)
2. Second segment becomes subcategory only if 2+ items share it
3. Single-child intermediate nodes are collapsed to avoid excessive depth

Example transformation:

```
Input parameters:
  System.CPU.Usage
  System.CPU.Usage5
  System.CPU.Usage15
  System.Hostname
  System.Memory.Physical.Free
  System.Memory.Physical.Total
  Agent.Version

Result tree:
▼ Parameters
  ▼ System
    ▼ CPU
        Usage
        Usage5
        Usage15
      Hostname
    ▼ Memory
      ▼ Physical
          Free
          Total
  ▼ Agent
      Version
```

Parameterized items like `FileSystem.Free(*)` display with the `(*)` visible, indicating they require an instance.

### Details Panel

Displays for selected item:
- Name
- Data type (for parameters)
- Description
- Column definitions (for tables)
- Related list (for parameterized items, if detected)
- Last queried value (persists until next query)
- Query timestamp

## Value Querying

### Single Item Query

- Select item in tree, click "Query" button (or press Enter/double-click)
- Result displays in details panel and persists until next query
- Uses existing API methods:
  - Parameters: `NXCSession.queryMetric(nodeId, origin, name)`
  - Tables: `NXCSession.queryTable(nodeId, name)`
  - Lists: `NXCSession.queryList(nodeId, name)` (new method)

### Bulk Walk

- Select a category node (e.g., `System` or `System.CPU`)
- Click "Walk" button
- Queries all leaf items under that node in parallel batches
- Results populate the bottom results table
- Columns: Name, Type, Value, Status (OK/Error/Timeout)
- Progress indicator with cancellation support
- Results stream to table as they arrive

### Parameterized Query

For items with `(*)` placeholder (e.g., `FileSystem.Free(*)`):

1. Query action opens "Select instance" dialog
2. Dialog shows dropdown populated from related list (auto-detected)
3. Auto-detection logic:
   - `FileSystem.Free(*)` → looks for `FileSystem.MountPoints` or `FileSystem.*` lists
   - `Net.Interface.BytesIn(*)` → looks for `Net.InterfaceList` or `Net.Interface.*` lists
4. User can also type custom instance value
5. Query executes with substituted instance: `FileSystem.Free(/home)`

## Lists Handling

### Display

When querying a list:
- Results show in details panel as scrollable list
- Each value on its own line
- "Copy All" button for clipboard
- Item count displayed

```
┌─ List: FileSystem.MountPoints ──────────────┐
│ /                                           │
│ /home                                       │
│ /boot                                       │
│ /var                                        │
│                                             │
│ Items: 4                     [Copy All]     │
└─────────────────────────────────────────────┘
```

### List-Parameter Connection

When selecting a parameterized parameter:
- Details panel shows "Related list: FileSystem.MountPoints" (if detected)
- Clicking related list name navigates to it in tree
- "Query with instance..." button opens picker pre-populated with list values

## Tables Handling

### Display

- Bottom results panel switches to table mode
- Reuses existing table value display widget
- Full column headers from table definition
- Sortable, filterable grid
- Export to CSV via context menu

## DCI Creation

### Context Menu Actions

Right-click on item(s) in tree or results table:

```
├─ Create DCI...              (opens dialog)
├─ Create DCI with Defaults   (immediate creation)
└─ Create DCIs for Selected   (batch, when multiple selected)
```

### Single DCI Dialog

Fields:
- Description (pre-filled from item description)
- Polling Interval (default: 60s)
- Retention Time (default: 30 days)
- Delta Calculation (None/Simple/Average per second/Average per minute)
- Data Type (pre-filled, editable for parameters)

For parameterized items `(*)`:
- Additional checkbox: "Enable instance discovery"
- If checked: Select discovery list from dropdown
- Creates DCI with instance discovery configuration

### Batch DCI Creation

When multiple items selected:
- Dialog shows count: "Create N DCIs"
- Shared settings: Polling Interval, Retention Time
- Individual descriptions auto-generated
- Preview list of items to be created
- "Create All" button

### Create with Defaults

Immediate creation with:
- Polling: 60 seconds
- Retention: 30 days
- Delta: None
- Description: Item description or name
- Opens Data Collection Configuration after creation

### Post-Creation

- Success notification with link to open DCI properties
- Option to "Create another" staying in explorer

## Implementation

### New Client Files (Java)

```
src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/agentmanagement/
├── views/
│   └── AgentExplorer.java              # Main view (AdHocObjectView)
├── widgets/
│   ├── AgentDataTree.java              # Tree with smart grouping
│   ├── AgentItemDetails.java           # Details panel
│   └── helpers/
│       ├── AgentTreeContentProvider.java
│       ├── AgentTreeLabelProvider.java
│       └── AgentTreeNode.java          # Tree node model
├── dialogs/
│   ├── CreateAgentDciDialog.java       # Single DCI creation
│   ├── CreateAgentDciBatchDialog.java  # Batch DCI creation
│   └── SelectInstanceDialog.java       # Instance picker for (*) params
├── actions/
│   └── CreateAgentDci.java             # DCI creation action
└── AgentExplorerToolDescriptor.java    # Tool registration
```

### Client Library Changes

```
src/client/java/netxms-client/.../NXCSession.java
  + queryList(long nodeId, String listName) → String[]
  + getSupportedLists(long nodeId) → List<AgentList>

src/client/java/netxms-client/.../AgentList.java   # New class: name, description
```

### Server Changes

**Configuration Poll:**
- Collect supported lists (similar to parameters/tables)
- Store in node object or database
- Expose via NXCP protocol

**NXCP Additions:**
- `CMD_GET_SUPPORTED_LISTS` - Retrieve list of supported lists for a node
- `CMD_QUERY_LIST` - Query list values (if not already existing)

### Caching Strategy

- Cache supported parameters/lists/tables per node
- Refresh button clears cache and re-fetches
- Cache expires on node reconnect or manual refresh
- Queried values are NOT cached (always live)

### Performance Considerations

For bulk walk with many items:
- Execute queries in parallel batches (e.g., 10 concurrent)
- Show progress indicator
- Allow cancellation
- Stream results to table as they arrive

## Summary

| Aspect | Decision |
|--------|----------|
| Entry point | Both tool view and node context view |
| Layout | Three-pane: tree, details panel, results table |
| Tree structure | Three root nodes (Parameters, Lists, Tables) with smart grouping |
| Value querying | Hybrid: single query + bulk walk |
| DCI creation | Dialog + defaults + batch |
| List integration | Instance discovery + parameterized query picker |
| Table display | Reuse existing table value widget |
| Details panel | Basic info + last queried value retained |
