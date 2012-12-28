/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

//
// Event flags
//

#define EF_LOG                   0x00000001


//
// Event flags used by client library (has no meaning for server)
//

#define EF_MODIFIED              0x01000000
#define EF_DELETED               0x02000000

#define SERVER_FLAGS_BITS        0x00FFFFFF


//
// Event severity codes
//

#define EVENT_SEVERITY_NORMAL    0
#define EVENT_SEVERITY_WARNING   1
#define EVENT_SEVERITY_MINOR     2
#define EVENT_SEVERITY_MAJOR     3
#define EVENT_SEVERITY_CRITICAL  4


//
// System-defined events
//

#define EVENT_NODE_ADDED                  1
#define EVENT_SUBNET_ADDED                2
#define EVENT_INTERFACE_ADDED             3
#define EVENT_INTERFACE_UP                4
#define EVENT_INTERFACE_DOWN              5
#define EVENT_NODE_NORMAL                 6
#define EVENT_NODE_WARNING                7
#define EVENT_NODE_MINOR                  8
#define EVENT_NODE_MAJOR                  9
#define EVENT_NODE_CRITICAL               10
#define EVENT_NODE_UNKNOWN                11
#define EVENT_NODE_UNMANAGED              12
#define EVENT_NODE_FLAGS_CHANGED          13
#define EVENT_SNMP_FAIL                   14
#define EVENT_AGENT_FAIL                  15
#define EVENT_INTERFACE_DELETED           16
#define EVENT_THRESHOLD_REACHED           17
#define EVENT_THRESHOLD_REARMED           18
#define EVENT_SUBNET_DELETED              19
#define EVENT_THREAD_HANGS                20
#define EVENT_THREAD_RUNNING              21
#define EVENT_SMTP_FAILURE                22
#define EVENT_MAC_ADDR_CHANGED            23
#define EVENT_INCORRECT_NETMASK           24
#define EVENT_SERVICE_DOWN                25
#define EVENT_SERVICE_UP                  26
#define EVENT_SERVICE_UNKNOWN             27
#define EVENT_NODE_DOWN                   28
#define EVENT_NODE_UP                     29
#define EVENT_SMS_FAILURE                 30
#define EVENT_SNMP_OK                     31
#define EVENT_AGENT_OK                    32
#define EVENT_SCRIPT_ERROR                33
#define EVENT_CONDITION_ACTIVATED         34
#define EVENT_CONDITION_DEACTIVATED       35
#define EVENT_DB_CONNECTION_LOST          36
#define EVENT_DB_CONNECTION_RESTORED      37
#define EVENT_CLUSTER_RESOURCE_MOVED      38
#define EVENT_CLUSTER_RESOURCE_DOWN       39
#define EVENT_CLUSTER_RESOURCE_UP         40
#define EVENT_CLUSTER_DOWN                41
#define EVENT_CLUSTER_UP                  42
#define EVENT_ALARM_TIMEOUT               43
#define EVENT_LOG_RECORD_MATCHED          44
#define EVENT_INTERFACE_UNKNOWN           45
#define EVENT_INTERFACE_DISABLED          46
#define EVENT_INTERFACE_TESTING           47
#define EVENT_EVENT_STORM_DETECTED        48
#define EVENT_EVENT_STORM_ENDED           49
#define EVENT_NETWORK_CONNECTION_LOST     50
#define EVENT_NETWORK_CONNECTION_RESTORED 51
#define EVENT_DB_QUERY_FAILED             52
#define EVENT_DCI_UNSUPPORTED             53
#define EVENT_DCI_DISABLED                54
#define EVENT_DCI_ACTIVE                  55
#define EVENT_IP_ADDRESS_CHANGED          56
#define EVENT_8021X_PAE_STATE_CHANGED     57
#define EVENT_8021X_BACKEND_STATE_CHANGED 58
#define EVENT_8021X_PAE_FORCE_UNAUTH      59
#define EVENT_8021X_AUTH_FAILED           60
#define EVENT_8021X_AUTH_TIMEOUT          61
#define EVENT_INTERFACE_UNEXPECTED_UP     62
#define EVENT_INTERFACE_EXPECTED_DOWN     63
#define EVENT_CONTAINER_AUTOBIND          64
#define EVENT_CONTAINER_AUTOUNBIND        65
#define EVENT_TEMPLATE_AUTOAPPLY          66
#define EVENT_TEMPLATE_AUTOREMOVE         67
#define EVENT_NODE_UNREACHABLE            68

#define EVENT_SNMP_UNMATCHED_TRAP         500
#define EVENT_SNMP_COLD_START             501
#define EVENT_SNMP_WARM_START             502
#define EVENT_SNMP_LINK_DOWN              503
#define EVENT_SNMP_LINK_UP                504
#define EVENT_SNMP_AUTH_FAILURE           505
#define EVENT_SNMP_EGP_NL                 506

#define FIRST_USER_EVENT_ID               100000

#endif
