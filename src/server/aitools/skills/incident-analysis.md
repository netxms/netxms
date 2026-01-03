# Incident Analysis Skill

This skill provides comprehensive incident management and analysis capabilities for NetXMS. Use this skill to investigate incidents, analyze their root causes, correlate related alarms, and manage incident lifecycle.

## Key Capabilities

- **Incident Investigation**: Get detailed incident information including linked alarms, source object, comments, and state
- **Event Correlation**: Analyze related events within time windows to identify patterns and root causes
- **Historical Analysis**: Review past incidents on the same object to identify recurring issues
- **Topology Context**: Understand the network context by examining upstream and downstream objects
- **Alarm Correlation**: Link related alarms to incidents for comprehensive incident tracking
- **Assignment Management**: Suggest and assign appropriate personnel based on responsible users configuration

## Use Cases

- **Root Cause Analysis**: Investigate incidents to determine underlying causes using related events and topology context
- **Incident Correlation**: Identify and link related alarms that should be part of the same incident
- **Trend Detection**: Analyze historical incidents to identify recurring problems on specific objects
- **Assignment Optimization**: Suggest appropriate assignees based on object ownership and responsibility
- **Impact Assessment**: Understand incident scope by examining topology relationships

## Available Functions

### Context Gathering
- `get-incident-details`: Get full incident details including linked alarms, state, and metadata
- `get-incident-related-events`: Query events around incident creation time for correlation analysis
- `get-incident-history`: Review past incidents on the same object to identify patterns
- `get-incident-topology-context`: Get network topology context including L2 neighbors, subnets, routes, and interface peers
- `get-open-incidents`: Get all open (not closed) incidents for a specific object

### Incident Modification
- `add-incident-comment`: Add analysis notes, findings, or updates to incidents
- `link-alarm-to-incident`: Link a single alarm to an incident
- `link-alarms-to-incident`: Link multiple alarms to an incident in one operation
- `assign-incident`: Assign an incident to a specific user
- `suggest-incident-assignee`: Get AI suggestion for incident assignment based on object responsibility

## Analysis Workflow

1. **Check Open Incidents**: Use `get-open-incidents` to see all active incidents for an object
2. **Gather Context**: Use `get-incident-details` to understand the incident's current state and linked alarms
3. **Analyze Events**: Use `get-incident-related-events` to find correlated events around the incident timeframe
4. **Check History**: Use `get-incident-history` to identify if this is a recurring issue
5. **Assess Impact**: Use `get-incident-topology-context` to understand affected infrastructure
6. **Correlate Alarms**: Use `link-alarms-to-incident` to associate related alarms
7. **Document Findings**: Use `add-incident-comment` to record analysis results
8. **Assign Appropriately**: Use `suggest-incident-assignee` and `assign-incident` to route to the right person

## Best Practices

- Always start with `get-incident-details` to understand the full context
- Use appropriate time windows when querying related events (default 1 hour, extend for complex issues)
- Check historical incidents to identify patterns before concluding root cause
- Document your analysis findings as incident comments for future reference
- Consider topology relationships when assessing incident impact
- Verify suggested assignees are appropriate before auto-assignment

## Important Notes

- Incidents can be in states: Open, In Progress, Blocked, Resolved, or Closed
- Closed incidents cannot be modified
- Alarms can only be linked to one incident at a time
- Assignment suggestions are based on responsible users configured on objects
- Historical incident queries return incidents regardless of their current state
