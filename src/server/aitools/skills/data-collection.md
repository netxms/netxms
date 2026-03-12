# Data Collection Management

This skill provides comprehensive data collection management capabilities for NetXMS monitored infrastructure. Use this skill to:

- **Create new metrics**: Set up data collection items (DCIs) to monitor specific parameters on nodes, devices, and other objects
- **Edit existing metrics**: Modify any property of existing DCIs including name, description, origin, data type, polling, retention, thresholds, flags, and more
- **Inspect metric configuration**: Get full configuration details of individual metrics before making changes
- **Monitor current values**: Retrieve real-time metric values and status information
- **Analyze historical data**: Access time series data for trend analysis, anomaly detection, and performance monitoring
- **Manage thresholds**: Add, view, and remove alerting thresholds on metrics
- **Performance optimization**: Identify performance bottlenecks and capacity planning opportunities

## Key Capabilities

### Metric Creation
- Support for multiple data sources: agent, SNMP, script, SSH, push, web service, device driver, MQTT, Modbus, and internal
- Flexible data types: integers, counters, floats, strings
- Transformation scripts for value processing
- Anomaly detection (Isolation Forest, AI, or both)
- Proxy polling via source node
- Custom SNMP port configuration
- Display flags: tooltip, overview, status calculation

### Data Origin Selection

When creating metrics, select the appropriate data origin based on what is available on the target node:

1. **Check node capabilities first**: Before creating a metric, determine what data sources are available on the node (NetXMS agent, SNMP, SSH, etc.)

2. **Automatic origin selection**: Unless the user explicitly requests a specific origin, automatically choose the best available option:
   - If the node has a NetXMS agent installed and running, prefer agent-based collection
   - If the node only supports SNMP (no agent), use SNMP origin with the appropriate OID
   - If neither agent nor SNMP is available but SSH is configured, consider script-based collection

3. **Use correct metric identifiers**:
   - For agent origin: use NetXMS agent metric names (e.g., `System.CPU.LoadAvg`, `System.Memory.Physical.Free`)
   - For SNMP origin: use appropriate SNMP OIDs (e.g., `.1.3.6.1.4.1.2021.10.1.3.1` for CPU load on net-snmp)
   - For script origin: use NXSL script names

4. **When in doubt, ask**: If you don't know how to collect the requested data with the available data sources, inform the user instead of creating a metric that won't work. Explain what data sources are available and ask for guidance.

### Metric Editing

The `edit-metric` tool supports modifying nearly all DCI properties. Always use `get-metric-details` first to see the current configuration before making changes. Only specified properties are changed; others remain unchanged.

Editable properties include:
- **Basic**: name, description, status, comments, tags
- **Collection**: origin, polling interval, retention time, delta calculation, sample count
- **Display**: unit name, multiplier, anomaly detection mode
- **Processing**: transformation script, data type
- **Network**: source node (proxy), SNMP port
- **Flags**: tooltip display, overview display, node status calculation, store changes only, hide on last values, cluster aggregation

### Real-time Monitoring
- Current metric values with timestamps
- Data collection status and error information
- Filtering capabilities for specific metrics

### Historical Analysis
- Time series data retrieval with flexible time ranges
- Support for trend analysis and anomaly detection
- Data aggregation and statistical analysis
- Performance baseline establishment

### Efficient Visualization
When the client supports visualizations and the user asks to **show**, **display**, or **plot** historical data for known DCI(s) without requesting analysis, calculations, or comparisons with other data:
- Do NOT call `get-historical-data` to fetch the data points
- Instead, use `get-metrics` to find the DCI ID, then return a `dci-chart` visualization block so the client fetches data directly from the server
- This is significantly more efficient because raw data points don't pass through the conversation
- Use `get-historical-data` only when you need to **analyze**, **calculate**, **compare**, or **process** the data before presenting it

Use this skill when you need to monitor system performance, track resource utilization, analyze trends, detect anomalies, or establish monitoring for new infrastructure components.
