/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxevent.h
**
**/

#ifndef _nxevent_h_
#define _nxevent_h_

/**
 * Event flags
 */
#define EF_LOG                   0x00000001
#define EF_DO_NOT_MONITOR        0x00000002

/**
 * Event flags used by client library (has no meaning for server)
 */
#define EF_MODIFIED              0x01000000
#define EF_DELETED               0x02000000

/**
 * Bit mask for server-side flags
 */
#define SERVER_FLAGS_BITS        0x00FFFFFF

/**
 * Event severity codes
 */
#define EVENT_SEVERITY_NORMAL    0
#define EVENT_SEVERITY_WARNING   1
#define EVENT_SEVERITY_MINOR     2
#define EVENT_SEVERITY_MAJOR     3
#define EVENT_SEVERITY_CRITICAL  4

/**
 * System-defined events
 */
#define EVENT_NODE_ADDED                            1
#define EVENT_SUBNET_ADDED                          2
#define EVENT_INTERFACE_ADDED                       3
#define EVENT_INTERFACE_UP                          4
#define EVENT_INTERFACE_DOWN                        5
#define EVENT_NODE_NORMAL                           6
#define EVENT_NODE_WARNING                          7
#define EVENT_NODE_MINOR                            8
#define EVENT_NODE_MAJOR                            9
#define EVENT_NODE_CRITICAL                         10
#define EVENT_NODE_UNKNOWN                          11
#define EVENT_NODE_UNMANAGED                        12
#define EVENT_NODE_CAPABILITIES_CHANGED             13
#define EVENT_SNMP_FAIL                             14
#define EVENT_AGENT_FAIL                            15
#define EVENT_INTERFACE_DELETED                     16
#define EVENT_THRESHOLD_REACHED                     17
#define EVENT_THRESHOLD_REARMED                     18
#define EVENT_SUBNET_DELETED                        19
#define EVENT_THREAD_HANGS                          20
#define EVENT_THREAD_RUNNING                        21
#define EVENT_NOTIFICATION_FAILURE                  22
#define EVENT_MAC_ADDR_CHANGED                      23
#define EVENT_INCORRECT_NETMASK                     24
#define EVENT_SERVICE_DOWN                          25
#define EVENT_SERVICE_UP                            26
#define EVENT_SERVICE_UNKNOWN                       27
#define EVENT_NODE_DOWN                             28
#define EVENT_NODE_UP                               29
#define EVENT_ALL_THRESHOLDS_REARMED                30
#define EVENT_SNMP_OK                               31
#define EVENT_AGENT_OK                              32
#define EVENT_SCRIPT_ERROR                          33
#define EVENT_CONDITION_ACTIVATED                   34
#define EVENT_CONDITION_DEACTIVATED                 35
#define EVENT_DB_CONNECTION_LOST                    36
#define EVENT_DB_CONNECTION_RESTORED                37
#define EVENT_CLUSTER_RESOURCE_MOVED                38
#define EVENT_CLUSTER_RESOURCE_DOWN                 39
#define EVENT_CLUSTER_RESOURCE_UP                   40
#define EVENT_CLUSTER_DOWN                          41
#define EVENT_CLUSTER_UP                            42
#define EVENT_ALARM_TIMEOUT                         43
#define EVENT_LOG_RECORD_MATCHED                    44
#define EVENT_INTERFACE_UNKNOWN                     45
#define EVENT_INTERFACE_DISABLED                    46
#define EVENT_INTERFACE_TESTING                     47
#define EVENT_EVENT_STORM_DETECTED                  48
#define EVENT_EVENT_STORM_ENDED                     49
#define EVENT_NETWORK_CONNECTION_LOST               50
#define EVENT_NETWORK_CONNECTION_RESTORED           51
#define EVENT_DB_QUERY_FAILED                       52
#define EVENT_DCI_UNSUPPORTED                       53
#define EVENT_DCI_DISABLED                          54
#define EVENT_DCI_ACTIVE                            55
#define EVENT_IP_ADDRESS_CHANGED                    56
#define EVENT_8021X_PAE_STATE_CHANGED               57
#define EVENT_8021X_BACKEND_STATE_CHANGED           58
#define EVENT_8021X_PAE_FORCE_UNAUTH                59
#define EVENT_8021X_AUTH_FAILED                     60
#define EVENT_8021X_AUTH_TIMEOUT                    61
#define EVENT_INTERFACE_UNEXPECTED_UP               62
#define EVENT_INTERFACE_EXPECTED_DOWN               63
#define EVENT_CONTAINER_AUTOBIND                    64
#define EVENT_CONTAINER_AUTOUNBIND                  65
#define EVENT_TEMPLATE_AUTOAPPLY                    66
#define EVENT_TEMPLATE_AUTOREMOVE                   67
#define EVENT_NODE_UNREACHABLE                      68
#define EVENT_TABLE_THRESHOLD_ACTIVATED             69
#define EVENT_TABLE_THRESHOLD_DEACTIVATED           70
#define EVENT_IF_PEER_CHANGED                       71
#define EVENT_AP_UP                                 72
#define EVENT_AP_UNPROVISIONED                      73
#define EVENT_AP_DOWN                               74
#define EVENT_IF_MASK_CHANGED                       75
#define EVENT_IF_IPADDR_ADDED                       76
#define EVENT_IF_IPADDR_DELETED                     77
#define EVENT_MAINTENANCE_MODE_ENTERED              78
#define EVENT_MAINTENANCE_MODE_LEFT                 79
#define EVENT_LDAP_SYNC_ERROR                       80
#define EVENT_AGENT_LOG_PROBLEM                     81
#define EVENT_AGENT_LOCAL_DATABASE_PROBLEM          82
#define EVENT_IF_EXPECTED_STATE_UP                  83
#define EVENT_IF_EXPECTED_STATE_DOWN                84
#define EVENT_IF_EXPECTED_STATE_IGNORE              85
#define EVENT_ROUTING_LOOP_DETECTED                 86
#define EVENT_PACKAGE_INSTALLED                     87
#define EVENT_PACKAGE_UPDATED                       88
#define EVENT_PACKAGE_REMOVED                       89
#define EVENT_POLICY_AUTODEPLOY                     90
#define EVENT_POLICY_AUTOUNINSTALL                  91
#define EVENT_UNBOUND_TUNNEL                        92
#define EVENT_AGENT_ID_CHANGED                      93
#define EVENT_TUNNEL_OPEN                           94
#define EVENT_TUNNEL_CLOSED                         95
#define EVENT_TUNNEL_AGENT_ID_MISMATCH              96
#define EVENT_SERVER_STARTED                        97
#define EVENT_HARDWARE_COMPONENT_ADDED              98
#define EVENT_HARDWARE_COMPONENT_REMOVED            99
#define EVENT_DUPLICATE_NODE_DELETED                100
#define EVENT_DUPLICATE_IP_ADDRESS                  101
#define EVENT_ETHERNET_IP_UNREACHABLE               102
#define EVENT_ETHERNET_IP_OK                        103
#define EVENT_DBWRITER_QUEUE_OVERFLOW               104
#define EVENT_DBWRITER_QUEUE_NORMAL                 105
#define EVENT_HOUSEKEEPER_STARTED                   106
#define EVENT_HOUSEKEEPER_COMPLETED                 107
#define EVENT_LICENSE_PROBLEM                       108
#define EVENT_SNMP_TRAP_FLOOD_DETECTED              109
#define EVENT_SNMP_TRAP_FLOOD_ENDED                 110
#define EVENT_GEOLOCATION_CHANGED                   111
#define EVENT_GEOLOCATION_INSIDE_RESTRICTED_AREA    112
#define EVENT_GEOLOCATION_OUTSIDE_ALLOWED_AREA      113
#define EVENT_CLUSTER_AUTOADD                       114
#define EVENT_CLUSTER_AUTOREMOVE                    115
#define EVENT_TUNNEL_HOST_DATA_MISMATCH             116
#define EVENT_POLICY_VALIDATION_ERROR               117
#define EVENT_TUNNEL_SETUP_ERROR                    118
#define EVENT_DUPLICATE_MAC_ADDRESS                 119
#define EVENT_BUSINESS_SERVICE_OPERATIONAL          120
#define EVENT_BUSINESS_SERVICE_FAILED               121
#define EVENT_ICMP_UNREACHABLE                      122
#define EVENT_ICMP_OK                               123
#define EVENT_BUSINESS_SERVICE_DEGRADED             124
#define EVENT_NOTIFICATION_CHANNEL_DOWN             125
#define EVENT_NOTIFICATION_CHANNEL_UP               126
#define EVENT_AGENT_FILE_CHANGED                    127
#define EVENT_AGENT_FILE_ADDED                      128
#define EVENT_AGENT_FILE_DELETED                    129
#define EVENT_TOO_MANY_SCRIPT_ERRORS                130
#define EVENT_SSH_OK                                131
#define EVENT_SSH_UNREACHABLE                       132
#define EVENT_ASSET_AUTO_UPDATE_FAILED              133
#define EVENT_ASSET_LINK                            134
#define EVENT_ASSET_UNLINK                          135
#define EVENT_ASSET_LINK_CONFLICT                   136
#define EVENT_CONFIGURATION_ERROR                   137
#define EVENT_MODBUS_UNREACHABLE                    138
#define EVENT_MODBUS_OK                             139
#define EVENT_IF_STP_STATE_CHANGED                  140
#define EVENT_IF_SPEED_CHANGED                      141
#define EVENT_AP_UNKNOWN                            142
#define EVENT_CIRCUIT_AUTOBIND                      143
#define EVENT_CIRCUIT_AUTOUNBIND                    144
#define EVENT_RESPONSIBLE_USER_ADDED                145
#define EVENT_RESPONSIBLE_USER_REMOVED              146
#define EVENT_RESPONSIBLE_USER_MODIFIED             147
#define EVENT_IF_SPEED_BELOW_MAXIMUM                148
#define EVENT_INTERFACE_UNMANAGED                   149

#define EVENT_SNMP_UNMATCHED_TRAP                   500
#define EVENT_SNMP_COLD_START                       501
#define EVENT_SNMP_WARM_START                       502
#define EVENT_SNMP_LINK_DOWN                        503
#define EVENT_SNMP_LINK_UP                          504
#define EVENT_SNMP_AUTH_FAILURE                     505
#define EVENT_SNMP_EGP_NL                           506
#define EVENT_SNMP_DISCARD_TRAP                     507

#define FIRST_USER_EVENT_ID                         100000

#endif
