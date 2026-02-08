# Data Collection Management

This skill provides comprehensive data collection management capabilities for NetXMS monitored infrastructure. Use this skill to:

- **Create new metrics**: Set up data collection items (DCIs) to monitor specific parameters on nodes, devices, and other objects
- **Monitor current values**: Retrieve real-time metric values and status information  
- **Analyze historical data**: Access time series data for trend analysis, anomaly detection, and performance monitoring
- **Performance optimization**: Identify performance bottlenecks and capacity planning opportunities

## Key Capabilities

### Metric Creation
- Support for multiple data sources: SNMP, agent-based monitoring, and custom scripts
- Flexible data types: integers, counters, floats, strings
- Automatic threshold configuration and alerting setup

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

### Real-time Monitoring  
- Current metric values with timestamps
- Data collection status and error information
- Filtering capabilities for specific metrics

### Historical Analysis
- Time series data retrieval with flexible time ranges
- Support for trend analysis and anomaly detection
- Data aggregation and statistical analysis
- Performance baseline establishment

Use this skill when you need to monitor system performance, track resource utilization, analyze trends, detect anomalies, or establish monitoring for new infrastructure components.