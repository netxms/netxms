# Device Configuration Backup Skill

This skill provides device configuration backup management capabilities for NetXMS monitored network devices. Use this skill to check backup status, retrieve and compare device configurations, and trigger on-demand backups.

## Key Capabilities

- **Check Backup Status**: Verify if a device is registered for backup and view last backup job result
- **List Backups**: Browse available backup snapshots with timestamps, sizes, and SHA-256 hashes
- **Retrieve Configuration**: Get the full running or startup configuration content from any backup
- **Start Backup**: Trigger an immediate on-demand backup job for a registered device
- **Compare Configurations**: Retrieve two backup versions side-by-side for change analysis

## Concepts

### Running vs Startup Configuration
- **Running config**: The active configuration currently in use on the device
- **Startup config**: The saved configuration that will be loaded on reboot
- Differences between running and startup configs indicate unsaved changes

### Change Detection
- Each backup stores SHA-256 hashes of both running and startup configurations
- Comparing hashes across backups quickly identifies when changes occurred
- Use the backup list to find configuration drift over time

### Backup Workflow
- Devices must be registered for backup during configuration polling
- Backups run on a schedule (configured server-side) and can be triggered on-demand
- Each backup captures both running and startup configurations when available

## Use Cases

- **Change Audit**: Compare configurations before and after a maintenance window to verify only intended changes were made
- **Troubleshooting**: Review recent configuration changes when a device starts misbehaving
- **Compliance**: Verify device configurations match organizational standards
- **Disaster Recovery**: Retrieve the last known good configuration for a failed device
- **Drift Detection**: Identify unauthorized or unexpected configuration changes

## Best Practices

- Use `get-backup-status` first to verify a device is registered before attempting other operations
- Use `get-backup-list` to browse available backups and identify relevant timestamps before retrieving full content
- When comparing configs, use the backup list hashes to identify which backups actually differ before fetching full content
- For large configurations, the content may be truncated at 64KB - check for truncation notes in the response
- Binary configurations (firmware images) are returned as base64-encoded data
