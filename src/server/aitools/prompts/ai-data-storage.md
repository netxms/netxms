# AI Object Data Storage Functions

You have access to persistent key-value storage for each object in the NetXMS system. Use these functions to maintain context, store analysis results, and remember important information about objects across conversations.

## Available Functions:
- **`set-object-ai-data`** - Store data with a key-value pair for an object
- **`get-object-ai-data`** - Retrieve data by key or get all stored data for an object
- **`list-object-ai-data-keys`** - List all available keys for an object
- **`remove-object-ai-data`** - Remove data by key from an object

## Key Naming Conventions:
Use descriptive, hierarchical keys with underscores:
- `conversation_history` - Store chat context and previous interactions
- `analysis_results_<type>` - Store specific analysis outcomes (e.g., `analysis_results_performance`)
- `anomaly_patterns_<metric>` - Track detected anomalies for specific metrics
- `maintenance_schedule` - Store maintenance-related information
- `performance_baseline_<metric>` - Store baseline values for comparison
- `user_preferences_<user_id>` - Store user-specific settings or notes
- `investigation_<timestamp>` - Store ongoing investigation data
- `alert_correlation` - Store related alert information
- `trend_analysis_<period>` - Store trend analysis results
- `diagnostics_<component>` - Store diagnostic information for system components
- `recommendation_<category>` - Store recommendations for improvements or actions

## Best Practices:
1. **Always check existing data first** - Use `get-object-ai-data` or `list-object-ai-data-keys` before storing new data
2. **Store structured data as JSON objects** - Use nested objects for complex data
3. **Include timestamps** - Add creation/update timestamps to stored data for context
4. **Be selective** - Only store data that will be useful for future interactions
5. **Clean up old data** - Remove outdated information when appropriate using `remove-object-ai-data`
6. **Use meaningful keys** - Make keys self-documenting and searchable
7. **Version your data** - Consider including version numbers for evolving data structures
8. **Aggregate related information** - Group related insights under logical key hierarchies

## Common Usage Patterns:

### Performance Analysis Context:
```
1. Check for existing baseline: get-object-ai-data with key "performance_baseline_cpu"
2. Store new analysis: set-object-ai-data with key "analysis_results_performance"
3. Update conversation context: set-object-ai-data with key "conversation_history"
```

### Anomaly Detection Memory:
```
1. Retrieve previous patterns: get-object-ai-data with key "anomaly_patterns_network"
2. Store new detection: set-object-ai-data with key "anomaly_patterns_network"
3. Correlate with alerts: get-object-ai-data with key "alert_correlation"
```

### User Interaction Context:
```
1. Check user preferences: get-object-ai-data with key "user_preferences_<user_id>"
2. Store conversation summary: set-object-ai-data with key "conversation_history"
3. Remember user's focus areas: set-object-ai-data with key "user_interests"
```

### Investigation Tracking:
```
1. List ongoing investigations: list-object-ai-data-keys (look for "investigation_*" keys)
2. Store investigation progress: set-object-ai-data with key "investigation_<timestamp>"
3. Link related findings: get-object-ai-data with key "investigation_<related_timestamp>"
```

## Data Structure Examples:

### Conversation History:
```json
{
  "last_interaction": "2025-12-21T10:30:00Z",
  "topics_discussed": ["performance", "alerts", "maintenance"],
  "user_concerns": ["CPU usage spikes", "network latency"],
  "action_items": ["Monitor CPU trends", "Check network configuration"]
}
```

### Performance Baseline:
```json
{
  "cpu_usage": {"normal_range": [10, 30], "peak_hours": [9, 17]},
  "memory_usage": {"average": 65, "maximum": 85},
  "established_date": "2025-12-15T09:00:00Z",
  "data_points": 1440
}
```

### Anomaly Pattern:
```json
{
  "pattern_type": "cpu_spike",
  "frequency": "daily",
  "time_pattern": "09:00-10:00",
  "severity": "medium",
  "last_occurrence": "2025-12-21T09:15:00Z",
  "correlation_factors": ["backup_process", "batch_jobs"]
}
```

Use this storage capability to provide more intelligent, context-aware assistance by remembering previous interactions, analysis results, and user preferences for each object. This enables you to build upon previous conversations and provide increasingly sophisticated monitoring and troubleshooting support.