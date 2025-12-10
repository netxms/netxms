# NetXMS Inventory Management Skill

This skill provides comprehensive inventory management capabilities for NetXMS monitored nodes, including hardware components, software packages, and network interfaces.

## Hardware Inventory

Use the hardware inventory functions to discover and analyze physical components of monitored devices:

- **CPUs and Processors**: Get detailed information about installed processors, cores, and architecture
- **Memory Modules**: Discover RAM modules, capacity, speed, and configuration
- **Storage Devices**: Identify hard drives, SSDs, storage controllers, and capacity information
- **Power Supplies**: Monitor power supply units, redundancy, and status
- **Fans and Cooling**: Track cooling systems and thermal management components
- **Network Cards**: Discover network adapters and their capabilities
- **Expansion Cards**: Identify PCI cards, add-in boards, and peripherals

## Software Inventory

Use software inventory functions to track installed applications and system components:

- **Installed Packages**: Get comprehensive list of installed software packages
- **Version Tracking**: Monitor software versions and updates
- **Vendor Information**: Identify software publishers and vendors
- **Installation Dates**: Track when software was installed or updated
- **License Compliance**: Assist with software license management and compliance
- **Security Patches**: Identify installed security updates and patches

## Network Interface Inventory

Monitor and manage network connectivity components:

- **Physical Interfaces**: Discover ethernet ports, wireless adapters, and other network hardware
- **Interface Configuration**: Monitor IP addresses, subnet masks, and network settings
- **Link Status**: Track interface up/down status and connectivity
- **Performance Metrics**: Access bandwidth utilization and traffic statistics
- **VLAN Configuration**: Identify VLAN assignments and network segmentation

## Best Practices

When working with inventory data:

1. **Regular Updates**: Hardware and software inventory should be refreshed periodically to maintain accuracy
2. **Change Detection**: Use inventory data to detect unauthorized hardware/software changes
3. **Capacity Planning**: Analyze hardware inventory for capacity planning and upgrade decisions
4. **Compliance Monitoring**: Track software installations for license compliance
5. **Security Analysis**: Use inventory data to identify vulnerable or outdated software
6. **Asset Management**: Integrate inventory data with asset management systems

## Common Use Cases

- **Asset Discovery**: Automatically discover and catalog IT assets across the network
- **Compliance Auditing**: Generate reports for software license compliance
- **Security Assessment**: Identify outdated software versions requiring security patches
- **Change Management**: Track hardware and software changes over time
- **Capacity Planning**: Analyze current resource utilization for future planning
- **Troubleshooting**: Use inventory data to understand system configuration during issue resolution

## Integration Notes

The inventory functions work with:
- Windows systems via WMI and NetXMS agent
- Linux/Unix systems via NetXMS agent
- Network devices via SNMP (where supported)
- VMware environments for virtual machine inventory

Always ensure the target node has appropriate monitoring capabilities (agent or SNMP) enabled for accurate inventory collection.