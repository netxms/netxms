/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2026 Raden Solutions
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
** File: upgrade_v60.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>
#include <netxms-xml.h>

/**
 * Upgrade from 60.16 to 60.17
 */
static bool H_UpgradeFromV16()
{
   static const TCHAR *batch =
      _T("ALTER TABLE event_policy ADD incident_ai_depth integer\n")
      _T("ALTER TABLE event_policy ADD incident_ai_auto_assign char(1)\n")
      _T("ALTER TABLE event_policy ADD incident_ai_prompt varchar(2000)\n")
      _T("UPDATE event_policy SET incident_ai_depth=0,incident_ai_auto_assign='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("event_policy"), _T("incident_ai_depth")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("event_policy"), _T("incident_ai_auto_assign")));

   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 60.15 to 60.16 (Incident management)
 */
static bool H_UpgradeFromV15()
{
   // Create incidents table
   CHK_EXEC(SQLQuery(
      _T("CREATE TABLE incidents (")
      _T("id integer not null,")
      _T("creation_time integer not null,")
      _T("last_change_time integer not null,")
      _T("state integer not null,")
      _T("assigned_user_id integer,")
      _T("title varchar(255) not null,")
      _T("source_alarm_id integer,")
      _T("source_object_id integer not null,")
      _T("created_by_user integer not null,")
      _T("resolved_by_user integer,")
      _T("closed_by_user integer,")
      _T("resolve_time integer,")
      _T("close_time integer,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_incidents_state ON incidents(state)")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_incidents_source_object ON incidents(source_object_id)")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_incidents_assigned_user ON incidents(assigned_user_id)")));

   // Create incident_alarms table
   CHK_EXEC(SQLQuery(
      _T("CREATE TABLE incident_alarms (")
      _T("incident_id integer not null,")
      _T("alarm_id integer not null,")
      _T("linked_time integer not null,")
      _T("linked_by_user integer not null,")
      _T("PRIMARY KEY(incident_id, alarm_id))")));

   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_incident_alarms_alarm ON incident_alarms(alarm_id)")));

   // Create incident_comments table
   CHK_EXEC(SQLQuery(
      _T("CREATE TABLE incident_comments (")
      _T("id integer not null,")
      _T("incident_id integer not null,")
      _T("creation_time integer not null,")
      _T("user_id integer not null,")
      _T("comment_text $SQL:TEXT not null,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_incident_comments_incident ON incident_comments(incident_id)")));

   // Create incident_activity_log table
   CHK_EXEC(SQLQuery(
      _T("CREATE TABLE incident_activity_log (")
      _T("id integer not null,")
      _T("incident_id integer not null,")
      _T("timestamp integer not null,")
      _T("user_id integer not null,")
      _T("activity_type integer not null,")
      _T("old_value varchar(255),")
      _T("new_value varchar(255),")
      _T("details varchar(1000),")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_incident_activity_incident ON incident_activity_log(incident_id)")));

   // Add incident columns to event_policy table
   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD incident_delay integer")));
   CHK_EXEC(SQLQuery(_T("UPDATE event_policy SET incident_delay=0")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("event_policy"), _T("incident_delay")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD incident_title varchar(255)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD incident_description varchar(2000)")));

   // Create incident events
   CHK_EXEC(CreateEventTemplate(EVENT_INCIDENT_OPENED, _T("SYS_INCIDENT_OPENED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("a1b2c3d4-e5f6-7890-abcd-ef1234567890"),
      _T("Incident #%<incidentId> opened (%<title>)"),
      _T("Generated when a new incident is created.\r\n")
      _T("Parameters:\r\n")
      _T("   1) incidentId - Incident ID\r\n")
      _T("   2) title - Incident title\r\n")
      _T("   3) sourceAlarmId - Source alarm ID (0 if manual)\r\n")
      _T("   4) createdByUser - User ID who created (0 if auto)")));

   CHK_EXEC(CreateEventTemplate(EVENT_INCIDENT_STATE_CHANGED, _T("SYS_INCIDENT_STATE_CHANGED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("b2c3d4e5-f6a7-8901-bcde-f12345678901"),
      _T("Incident #%<incidentId> state changed from %<oldStateName> to %<newStateName>"),
      _T("Generated when incident state changes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) incidentId - Incident ID\r\n")
      _T("   2) title - Incident title\r\n")
      _T("   3) oldState - Previous state code\r\n")
      _T("   4) oldStateName - Previous state name\r\n")
      _T("   5) newState - New state code\r\n")
      _T("   6) newStateName - New state name\r\n")
      _T("   7) userId - User ID who changed state")));

   CHK_EXEC(CreateEventTemplate(EVENT_INCIDENT_ASSIGNED, _T("SYS_INCIDENT_ASSIGNED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("e5f6a7b8-c9d0-1234-efab-345678901234"),
      _T("Incident #%<incidentId> assigned to user %<assignedUserName>"),
      _T("Generated when an incident is assigned to a user.\r\n")
      _T("Parameters:\r\n")
      _T("   1) incidentId - Incident ID\r\n")
      _T("   2) title - Incident title\r\n")
      _T("   3) assignedUserId - Assigned user ID\r\n")
      _T("   4) assignedUserName - Assigned user name\r\n")
      _T("   5) assignedByUser - User ID who assigned")));

   CHK_EXEC(CreateEventTemplate(EVENT_INCIDENT_ALARM_LINKED, _T("SYS_INCIDENT_ALARM_LINKED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("ee499067-16c1-493d-8d1b-eefe02a4efb0"),
      _T("Alarm #%<alarmId> linked to incident #%<incidentId>"),
      _T("Generated when an alarm is linked to an incident.\r\n")
      _T("Parameters:\r\n")
      _T("   1) incidentId - Incident ID\r\n")
      _T("   2) title - Incident title\r\n")
      _T("   3) alarmId - Alarm ID\r\n")
      _T("   4) userId - User ID who linked")));

   CHK_EXEC(CreateEventTemplate(EVENT_INCIDENT_ALARM_UNLINKED, _T("SYS_INCIDENT_ALARM_UNLINKED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("7b5b0dff-0498-4342-a2ca-033a6f30d0a0"),
      _T("Alarm #%<alarmId> unlinked from incident #%<incidentId>"),
      _T("Generated when an alarm is unlinked from an incident.\r\n")
      _T("Parameters:\r\n")
      _T("   1) incidentId - Incident ID\r\n")
      _T("   2) title - Incident title\r\n")
      _T("   3) alarmId - Alarm ID\r\n")
      _T("   4) userId - User ID who unlinked")));

   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 60.14 to 60.15
 */
static bool H_UpgradeFromV14()
{
   // Add OBJECT_ACCESS_MANAGE_POLICIES (0x01000000) to all ACL entries with MODIFY access set for Admins group
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(_T("UPDATE acl SET access_rights=access_rights+16777216 WHERE user_id=1073741825 AND (BITAND(access_rights, 16777216)=0) AND (BITAND(access_rights, 2)<>0)")));
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("UPDATE acl SET access_rights=access_rights+16777216 WHERE user_id=1073741825 AND ((access_rights & 16777216)=0) AND ((access_rights & 2)<>0)")));
   }
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 60.13 to 60.14 (Oracle varchar to varchar2(N char) conversion)
 */
static bool H_UpgradeFromV13()
{
   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      // List of all varchar columns that need to be converted to varchar2(N char)
      // Format: { table_name, column_name, size }
      static const struct {
         const wchar_t *table;
         const wchar_t *column;
         int size;
      } s_columns[] = {
         // metadata
         { L"metadata", L"var_name", 63 },
         { L"metadata", L"var_value", 255 },
         // config
         { L"config", L"var_name", 63 },
         { L"config", L"var_value", 2000 },
         { L"config", L"default_value", 2000 },
         { L"config", L"description", 450 },
         { L"config", L"units", 36 },
         // config_clob
         { L"config_clob", L"var_name", 63 },
         // config_values
         { L"config_values", L"var_name", 63 },
         { L"config_values", L"var_value", 15 },
         { L"config_values", L"var_description", 255 },
         // users
         { L"users", L"guid", 36 },
         { L"users", L"name", 63 },
         { L"users", L"password", 127 },
         { L"users", L"ui_access_rules", 2000 },
         { L"users", L"full_name", 127 },
         { L"users", L"description", 255 },
         { L"users", L"email", 127 },
         { L"users", L"phone_number", 63 },
         { L"users", L"ldap_unique_id", 64 },
         // auth_tokens
         { L"auth_tokens", L"description", 127 },
         // user_groups
         { L"user_groups", L"guid", 36 },
         { L"user_groups", L"name", 63 },
         { L"user_groups", L"ui_access_rules", 2000 },
         { L"user_groups", L"description", 255 },
         { L"user_groups", L"ldap_unique_id", 64 },
         // user_profiles
         { L"user_profiles", L"var_name", 63 },
         // userdb_custom_attributes
         { L"userdb_custom_attributes", L"attr_name", 127 },
         // object_properties
         { L"object_properties", L"guid", 36 },
         { L"object_properties", L"name", 63 },
         { L"object_properties", L"alias", 255 },
         { L"object_properties", L"status_translation", 8 },
         { L"object_properties", L"status_thresholds", 8 },
         { L"object_properties", L"latitude", 20 },
         { L"object_properties", L"longitude", 20 },
         { L"object_properties", L"map_image", 36 },
         { L"object_properties", L"name_on_map", 63 },
         { L"object_properties", L"country", 63 },
         { L"object_properties", L"region", 63 },
         { L"object_properties", L"city", 63 },
         { L"object_properties", L"district", 63 },
         { L"object_properties", L"street_address", 255 },
         { L"object_properties", L"postcode", 31 },
         // object_categories
         { L"object_categories", L"name", 63 },
         { L"object_categories", L"icon", 36 },
         { L"object_categories", L"map_image", 36 },
         // object_urls
         { L"object_urls", L"url", 2000 },
         { L"object_urls", L"description", 2000 },
         // object_custom_attributes
         { L"object_custom_attributes", L"attr_name", 127 },
         // object_ai_data
         { L"object_ai_data", L"data_key", 127 },
         // mobile_devices
         { L"mobile_devices", L"device_id", 64 },
         { L"mobile_devices", L"vendor", 64 },
         { L"mobile_devices", L"model", 128 },
         { L"mobile_devices", L"serial_number", 64 },
         { L"mobile_devices", L"os_name", 32 },
         { L"mobile_devices", L"os_version", 64 },
         { L"mobile_devices", L"user_id", 64 },
         { L"mobile_devices", L"comm_protocol", 31 },
         { L"mobile_devices", L"speed", 20 },
         // access_points
         { L"access_points", L"mac_address", 16 },
         { L"access_points", L"vendor", 64 },
         { L"access_points", L"model", 128 },
         { L"access_points", L"serial_number", 64 },
         // radios
         { L"radios", L"name", 63 },
         { L"radios", L"ssid", 32 },
         // rack_passive_elements
         { L"rack_passive_elements", L"name", 255 },
         { L"rack_passive_elements", L"image_front", 36 },
         { L"rack_passive_elements", L"image_rear", 36 },
         // physical_links
         { L"physical_links", L"description", 255 },
         // chassis
         { L"chassis", L"rack_image_front", 36 },
         { L"chassis", L"rack_image_rear", 36 },
         // nodes
         { L"nodes", L"primary_name", 255 },
         { L"nodes", L"primary_ip", 48 },
         { L"nodes", L"tunnel_id", 36 },
         { L"nodes", L"agent_cert_subject", 500 },
         { L"nodes", L"agent_cert_mapping_data", 500 },
         { L"nodes", L"community", 127 },
         { L"nodes", L"usm_auth_password", 127 },
         { L"nodes", L"usm_priv_password", 127 },
         { L"nodes", L"snmp_oid", 255 },
         { L"nodes", L"snmp_engine_id", 255 },
         { L"nodes", L"snmp_context_engine_id", 255 },
         { L"nodes", L"secret", 88 },
         { L"nodes", L"agent_version", 63 },
         { L"nodes", L"agent_id", 36 },
         { L"nodes", L"hardware_id", 40 },
         { L"nodes", L"platform_name", 63 },
         { L"nodes", L"uname", 255 },
         { L"nodes", L"snmp_sys_name", 127 },
         { L"nodes", L"snmp_sys_contact", 127 },
         { L"nodes", L"snmp_sys_location", 255 },
         { L"nodes", L"bridge_base_addr", 15 },
         { L"nodes", L"lldp_id", 255 },
         { L"nodes", L"driver_name", 32 },
         { L"nodes", L"rack_image_front", 36 },
         { L"nodes", L"rack_image_rear", 36 },
         { L"nodes", L"node_subtype", 127 },
         { L"nodes", L"hypervisor_type", 31 },
         { L"nodes", L"hypervisor_info", 255 },
         { L"nodes", L"ssh_login", 63 },
         { L"nodes", L"ssh_password", 63 },
         { L"nodes", L"vnc_password", 63 },
         { L"nodes", L"chassis_placement_config", 2000 },
         { L"nodes", L"vendor", 127 },
         { L"nodes", L"product_name", 127 },
         { L"nodes", L"product_version", 15 },
         { L"nodes", L"product_code", 31 },
         { L"nodes", L"serial_number", 31 },
         { L"nodes", L"syslog_codepage", 15 },
         { L"nodes", L"snmp_codepage", 15 },
         { L"nodes", L"ospf_router_id", 15 },
         { L"nodes", L"last_events", 255 },
         // icmp_target_address_list
         { L"icmp_target_address_list", L"ip_addr", 48 },
         // software_inventory
         { L"software_inventory", L"name", 127 },
         { L"software_inventory", L"version", 63 },
         { L"software_inventory", L"vendor", 63 },
         { L"software_inventory", L"url", 255 },
         { L"software_inventory", L"description", 255 },
         { L"software_inventory", L"uninstall_key", 255 },
         // hardware_inventory
         { L"hardware_inventory", L"hw_type", 47 },
         { L"hardware_inventory", L"vendor", 127 },
         { L"hardware_inventory", L"model", 127 },
         { L"hardware_inventory", L"location", 63 },
         { L"hardware_inventory", L"part_number", 63 },
         { L"hardware_inventory", L"serial_number", 63 },
         { L"hardware_inventory", L"description", 255 },
         // node_components
         { L"node_components", L"name", 255 },
         { L"node_components", L"description", 255 },
         { L"node_components", L"model", 255 },
         { L"node_components", L"serial_number", 63 },
         { L"node_components", L"vendor", 63 },
         { L"node_components", L"firmware", 127 },
         // ospf_areas
         { L"ospf_areas", L"area_id", 15 },
         // ospf_neighbors
         { L"ospf_neighbors", L"router_id", 15 },
         { L"ospf_neighbors", L"area_id", 15 },
         { L"ospf_neighbors", L"ip_address", 48 },
         // cluster_sync_subnets
         { L"cluster_sync_subnets", L"subnet_addr", 48 },
         // cluster_resources
         { L"cluster_resources", L"resource_name", 255 },
         { L"cluster_resources", L"ip_addr", 48 },
         // subnets
         { L"subnets", L"ip_addr", 48 },
         // interfaces
         { L"interfaces", L"mac_addr", 16 },
         { L"interfaces", L"description", 255 },
         { L"interfaces", L"if_name", 255 },
         { L"interfaces", L"if_alias", 255 },
         { L"interfaces", L"iftable_suffix", 127 },
         { L"interfaces", L"ospf_area", 15 },
         { L"interfaces", L"state_before_maintenance", 2000 },
         // interface_address_list
         { L"interface_address_list", L"ip_addr", 48 },
         // network_services
         { L"network_services", L"ip_bind_addr", 48 },
         // vpn_connector_networks
         { L"vpn_connector_networks", L"ip_addr", 48 },
         // icmp_statistics
         { L"icmp_statistics", L"poll_target", 63 },
         // items
         { L"items", L"guid", 36 },
         { L"items", L"name", 1023 },
         { L"items", L"description", 255 },
         { L"items", L"polling_interval_src", 255 },
         { L"items", L"retention_time_src", 255 },
         { L"items", L"instance", 1023 },
         { L"items", L"system_tag", 63 },
         { L"items", L"user_tag", 63 },
         { L"items", L"units_name", 63 },
         { L"items", L"instd_data", 1023 },
         { L"items", L"npe_name", 15 },
         // thresholds
         { L"thresholds", L"fire_value", 255 },
         { L"thresholds", L"rearm_value", 255 },
         { L"thresholds", L"last_checked_value", 255 },
         { L"thresholds", L"last_event_message", 2000 },
         // dc_tables
         { L"dc_tables", L"guid", 36 },
         { L"dc_tables", L"name", 1023 },
         { L"dc_tables", L"description", 255 },
         { L"dc_tables", L"polling_interval_src", 255 },
         { L"dc_tables", L"retention_time_src", 255 },
         { L"dc_tables", L"system_tag", 63 },
         { L"dc_tables", L"user_tag", 63 },
         { L"dc_tables", L"instance", 1023 },
         { L"dc_tables", L"instd_data", 1023 },
         // dc_table_columns
         { L"dc_table_columns", L"column_name", 63 },
         { L"dc_table_columns", L"snmp_oid", 1023 },
         { L"dc_table_columns", L"display_name", 255 },
         // dct_threshold_conditions
         { L"dct_threshold_conditions", L"column_name", 63 },
         { L"dct_threshold_conditions", L"check_value", 255 },
         // dct_threshold_instances
         { L"dct_threshold_instances", L"instance", 255 },
         // dci_schedules
         { L"dci_schedules", L"schedule", 255 },
         // raw_dci_values
         { L"raw_dci_values", L"raw_value", 255 },
         { L"raw_dci_values", L"transformed_value", 255 },
         // idata (non-TSDB)
         { L"idata", L"idata_value", 255 },
         { L"idata", L"raw_value", 255 },
         // event_cfg
         { L"event_cfg", L"event_name", 63 },
         { L"event_cfg", L"guid", 36 },
         { L"event_cfg", L"message", 2000 },
         { L"event_cfg", L"tags", 2000 },
         // event_log
         { L"event_log", L"event_message", 2000 },
         { L"event_log", L"event_tags", 2000 },
         // notification_log
         { L"notification_log", L"notification_channel", 63 },
         { L"notification_log", L"recipient", 2000 },
         { L"notification_log", L"subject", 2000 },
         { L"notification_log", L"message", 2000 },
         { L"notification_log", L"rule_id", 36 },
         // server_action_execution_log
         { L"server_action_execution_log", L"action_name", 63 },
         { L"server_action_execution_log", L"channel_name", 63 },
         { L"server_action_execution_log", L"recipient", 2000 },
         { L"server_action_execution_log", L"subject", 2000 },
         { L"server_action_execution_log", L"action_data", 2000 },
         // actions
         { L"actions", L"guid", 36 },
         { L"actions", L"action_name", 63 },
         { L"actions", L"rcpt_addr", 255 },
         { L"actions", L"email_subject", 255 },
         { L"actions", L"channel_name", 63 },
         // event_policy
         { L"event_policy", L"rule_guid", 36 },
         { L"event_policy", L"alarm_message", 2000 },
         { L"event_policy", L"alarm_impact", 1000 },
         { L"event_policy", L"alarm_key", 255 },
         { L"event_policy", L"downtime_tag", 15 },
         { L"event_policy", L"rca_script_name", 255 },
         // policy_action_list
         { L"policy_action_list", L"timer_delay", 127 },
         { L"policy_action_list", L"timer_key", 127 },
         { L"policy_action_list", L"blocking_timer_key", 127 },
         { L"policy_action_list", L"snooze_time", 127 },
         // policy_timer_cancellation_list
         { L"policy_timer_cancellation_list", L"timer_key", 127 },
         // policy_pstorage_actions
         { L"policy_pstorage_actions", L"ps_key", 127 },
         { L"policy_pstorage_actions", L"value", 2000 },
         // policy_cattr_actions
         { L"policy_cattr_actions", L"attribute_name", 127 },
         { L"policy_cattr_actions", L"value", 2000 },
         // alarms
         { L"alarms", L"hd_ref", 63 },
         { L"alarms", L"rule_guid", 36 },
         { L"alarms", L"rule_description", 255 },
         { L"alarms", L"message", 2000 },
         { L"alarms", L"alarm_key", 255 },
         { L"alarms", L"alarm_category_ids", 255 },
         { L"alarms", L"event_tags", 2000 },
         { L"alarms", L"rca_script_name", 255 },
         { L"alarms", L"impact", 1000 },
         // alarm_events
         { L"alarm_events", L"event_name", 63 },
         { L"alarm_events", L"message", 2000 },
         // alarm_categories
         { L"alarm_categories", L"name", 63 },
         { L"alarm_categories", L"descr", 255 },
         // snmp_trap_cfg
         { L"snmp_trap_cfg", L"guid", 36 },
         { L"snmp_trap_cfg", L"snmp_oid", 255 },
         { L"snmp_trap_cfg", L"user_tag", 63 },
         { L"snmp_trap_cfg", L"description", 255 },
         // snmp_trap_pmap
         { L"snmp_trap_pmap", L"snmp_oid", 255 },
         { L"snmp_trap_pmap", L"description", 255 },
         // agent_pkg
         { L"agent_pkg", L"pkg_type", 15 },
         { L"agent_pkg", L"pkg_name", 63 },
         { L"agent_pkg", L"version", 31 },
         { L"agent_pkg", L"platform", 63 },
         { L"agent_pkg", L"pkg_file", 255 },
         { L"agent_pkg", L"command", 4000 },
         { L"agent_pkg", L"description", 255 },
         // package_deployment_jobs
         { L"package_deployment_jobs", L"failure_reason", 255 },
         // package_deployment_log
         { L"package_deployment_log", L"failure_reason", 255 },
         { L"package_deployment_log", L"pkg_type", 15 },
         { L"package_deployment_log", L"pkg_name", 63 },
         { L"package_deployment_log", L"version", 31 },
         { L"package_deployment_log", L"platform", 63 },
         { L"package_deployment_log", L"pkg_file", 255 },
         { L"package_deployment_log", L"command", 255 },
         { L"package_deployment_log", L"description", 255 },
         // object_tools
         { L"object_tools", L"guid", 36 },
         { L"object_tools", L"tool_name", 255 },
         { L"object_tools", L"description", 255 },
         { L"object_tools", L"confirmation_text", 255 },
         { L"object_tools", L"command_name", 255 },
         { L"object_tools", L"command_short_name", 31 },
         // object_tools_table_columns
         { L"object_tools_table_columns", L"col_name", 255 },
         { L"object_tools_table_columns", L"col_oid", 255 },
         // input_fields
         { L"input_fields", L"name", 31 },
         { L"input_fields", L"display_name", 127 },
         // syslog
         { L"syslog", L"hostname", 127 },
         { L"syslog", L"msg_tag", 32 },
         // script_library
         { L"script_library", L"guid", 36 },
         { L"script_library", L"script_name", 255 },
         // snmp_trap_log
         { L"snmp_trap_log", L"ip_addr", 48 },
         { L"snmp_trap_log", L"trap_oid", 255 },
         // agent_configs
         { L"agent_configs", L"config_name", 255 },
         // address_lists
         { L"address_lists", L"addr1", 48 },
         { L"address_lists", L"addr2", 48 },
         { L"address_lists", L"comments", 255 },
         // graphs
         { L"graphs", L"name", 255 },
         // certificate_action_log
         { L"certificate_action_log", L"node_guid", 36 },
         { L"certificate_action_log", L"subject", 255 },
         // audit_log
         { L"audit_log", L"subsystem", 32 },
         { L"audit_log", L"workstation", 63 },
         { L"audit_log", L"hmac", 64 },
         // persistent_storage
         { L"persistent_storage", L"entry_key", 127 },
         { L"persistent_storage", L"value", 2000 },
         // snmp_communities
         { L"snmp_communities", L"community", 255 },
         // ap_common
         { L"ap_common", L"guid", 36 },
         { L"ap_common", L"policy_name", 63 },
         { L"ap_common", L"policy_type", 31 },
         // usm_credentials
         { L"usm_credentials", L"user_name", 255 },
         { L"usm_credentials", L"auth_password", 255 },
         { L"usm_credentials", L"priv_password", 255 },
         { L"usm_credentials", L"comments", 255 },
         // well_known_ports
         { L"well_known_ports", L"tag", 15 },
         // network_maps
         { L"network_maps", L"background", 36 },
         { L"network_maps", L"bg_latitude", 20 },
         { L"network_maps", L"bg_longitude", 20 },
         // network_map_links
         { L"network_map_links", L"link_name", 255 },
         { L"network_map_links", L"connector_name1", 255 },
         { L"network_map_links", L"connector_name2", 255 },
         { L"network_map_links", L"color_provider", 255 },
         // images
         { L"images", L"guid", 36 },
         { L"images", L"name", 63 },
         { L"images", L"category", 63 },
         { L"images", L"mimetype", 32 },
         // dashboard_templates
         { L"dashboard_templates", L"name_template", 255 },
         // business_services
         { L"business_services", L"instance", 1023 },
         // business_service_prototypes
         { L"business_service_prototypes", L"instance_data", 1023 },
         // business_service_checks
         { L"business_service_checks", L"description", 1023 },
         { L"business_service_checks", L"failure_reason", 255 },
         // slm_agreements
         { L"slm_agreements", L"uptime", 63 },
         { L"slm_agreements", L"notes", 255 },
         // business_service_tickets
         { L"business_service_tickets", L"check_description", 1023 },
         { L"business_service_tickets", L"reason", 255 },
         // organizations
         { L"organizations", L"name", 63 },
         { L"organizations", L"description", 255 },
         // persons
         { L"persons", L"first_name", 63 },
         { L"persons", L"last_name", 63 },
         { L"persons", L"title", 255 },
         // licenses
         { L"licenses", L"guid", 36 },
         { L"licenses", L"content", 2000 },
         // mapping_tables
         { L"mapping_tables", L"name", 63 },
         { L"mapping_tables", L"description", 4000 },
         // mapping_data
         { L"mapping_data", L"md_key", 63 },
         { L"mapping_data", L"md_value", 4000 },
         { L"mapping_data", L"description", 4000 },
         // dci_summary_tables
         { L"dci_summary_tables", L"guid", 36 },
         { L"dci_summary_tables", L"menu_path", 255 },
         { L"dci_summary_tables", L"title", 127 },
         { L"dci_summary_tables", L"table_dci_name", 255 },
         // scheduled_tasks
         { L"scheduled_tasks", L"taskId", 255 },
         { L"scheduled_tasks", L"schedule", 127 },
         { L"scheduled_tasks", L"params", 1023 },
         { L"scheduled_tasks", L"comments", 255 },
         { L"scheduled_tasks", L"task_key", 255 },
         // currency_codes
         { L"currency_codes", L"description", 127 },
         // country_codes
         { L"country_codes", L"name", 127 },
         // config_repositories
         { L"config_repositories", L"url", 1023 },
         { L"config_repositories", L"auth_token", 63 },
         { L"config_repositories", L"description", 1023 },
         // port_layouts
         { L"port_layouts", L"device_oid", 127 },
         { L"port_layouts", L"layout_data", 4000 },
         // sensors
         { L"sensors", L"mac_address", 16 },
         { L"sensors", L"vendor", 127 },
         { L"sensors", L"model", 127 },
         { L"sensors", L"serial_number", 63 },
         { L"sensors", L"device_address", 127 },
         // responsible_users
         { L"responsible_users", L"tag", 31 },
         // user_agent_notifications
         { L"user_agent_notifications", L"message", 1023 },
         { L"user_agent_notifications", L"objects", 1023 },
         // notification_channels
         { L"notification_channels", L"name", 63 },
         { L"notification_channels", L"driver_name", 63 },
         { L"notification_channels", L"description", 255 },
         // nc_persistent_storage
         { L"nc_persistent_storage", L"channel_name", 63 },
         { L"nc_persistent_storage", L"entry_name", 127 },
         { L"nc_persistent_storage", L"entry_value", 2000 },
         // two_factor_auth_methods
         { L"two_factor_auth_methods", L"name", 63 },
         { L"two_factor_auth_methods", L"driver", 63 },
         { L"two_factor_auth_methods", L"description", 255 },
         // two_factor_auth_bindings
         { L"two_factor_auth_bindings", L"name", 63 },
         // websvc_definitions
         { L"websvc_definitions", L"guid", 36 },
         { L"websvc_definitions", L"name", 63 },
         { L"websvc_definitions", L"description", 2000 },
         { L"websvc_definitions", L"url", 4000 },
         { L"websvc_definitions", L"request_data", 4000 },
         { L"websvc_definitions", L"login", 255 },
         { L"websvc_definitions", L"password", 255 },
         // websvc_headers
         { L"websvc_headers", L"name", 63 },
         { L"websvc_headers", L"value", 2000 },
         // shared_secrets
         { L"shared_secrets", L"secret", 88 },
         // win_event_log
         { L"win_event_log", L"log_name", 63 },
         { L"win_event_log", L"event_source", 127 },
         { L"win_event_log", L"message", 2000 },
         // geo_areas
         { L"geo_areas", L"name", 127 },
         // dc_targets
         { L"dc_targets", L"geo_areas", 2000 },
         // ssh_keys
         { L"ssh_keys", L"name", 255 },
         // ssh_credentials
         { L"ssh_credentials", L"login", 63 },
         { L"ssh_credentials", L"password", 63 },
         // vnc_credentials
         { L"vnc_credentials", L"password", 63 },
         // object_queries
         { L"object_queries", L"guid", 36 },
         { L"object_queries", L"name", 63 },
         { L"object_queries", L"description", 255 },
         // am_attributes
         { L"am_attributes", L"attr_name", 63 },
         { L"am_attributes", L"display_name", 255 },
         // am_enum_values
         { L"am_enum_values", L"attr_name", 63 },
         { L"am_enum_values", L"value", 63 },
         { L"am_enum_values", L"display_name", 255 },
         // asset_properties
         { L"asset_properties", L"attr_name", 63 },
         { L"asset_properties", L"value", 2000 },
         // asset_change_log
         { L"asset_change_log", L"attribute_name", 63 },
         { L"asset_change_log", L"old_value", 2000 },
         { L"asset_change_log", L"new_value", 2000 },
         // active_downtimes
         { L"active_downtimes", L"downtime_tag", 15 },
         // downtime_log
         { L"downtime_log", L"downtime_tag", 15 },
         // ai_tasks
         { L"ai_tasks", L"description", 255 },
         // ai_task_execution_log
         { L"ai_task_execution_log", L"task_description", 255 },
         // End marker
         { nullptr, nullptr, 0 }
      };

      for (int i = 0; s_columns[i].table != nullptr; i++)
      {
         wchar_t query[256];
         nx_swprintf(query, 256, L"ALTER TABLE %s MODIFY %s varchar2(%d char)", s_columns[i].table, s_columns[i].column, s_columns[i].size);
         CHK_EXEC(SQLQuery(query));
      }

      IntegerArray<uint32_t> *dcTargets = GetDataCollectionTargets();
      for(int i = 0; i < dcTargets->size(); i++)
      {
         wchar_t query[256];
         nx_swprintf(query, 256, L"ALTER TABLE idata_%u MODIFY (idata_value varchar2(255 char), raw_value varchar2(255 char))", dcTargets->get(i));
         CHK_EXEC(SQLQuery(query));
      }
      delete dcTargets;

      CHK_EXEC(SQLQuery(L"UPDATE metadata SET var_value='CREATE TABLE idata_%d (item_id integer not null,idata_timestamp number(20) not null,idata_value varchar2(255 char) null,raw_value varchar2(255 char) null)' WHERE var_name='IDataTableCreationCommand'"));
   }

   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 60.12 to 60.13
 */
static bool H_UpgradeFromV12()
{
   static const TCHAR *batch =
      _T("ALTER TABLE object_properties ADD is_hidden integer\n")
      _T("UPDATE object_properties SET is_hidden=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"object_properties", L"is_hidden"));

   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 60.11 to 60.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE object_ai_data (")
      _T("   object_id integer not null,")
      _T("   data_key varchar(127) not null,")
      _T("   data_value $SQL:TEXT null,")
      _T("   PRIMARY KEY(object_id,data_key))")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 60.10 to 60.11
 */
static bool H_UpgradeFromV10()
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD decommission_time integer\n")
      _T("UPDATE nodes SET decommission_time=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"nodes", L"decommission_time"));

   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.DefaultDecommissionExpirationTime"), L"30",
         _T("Default expiration time in days for decommissioned nodes"),
         L"days", 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 60.9 to 60.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateEventTemplate(EVENT_NC_QUEUE_OVERFLOW, L"SYS_NC_QUEUE_OVERFLOW",
         EVENT_SEVERITY_WARNING, EF_LOG, _T("67b1aebf-3862-44fa-9ff8-9098c9e63a12"),
         _T("Notification channel \"%<channelName>\" queue overflow (size=%<queueSize>, limit=%<maxQueueSize>)"),
         _T("Generated when notification channel queue size exceeds configured threshold.\r\n")
         _T("Parameters:\r\n")
         _T("   1) channelName - Notification channel name\r\n")
         _T("   2) queueSize - Current queue size\r\n")
         _T("   3) maxQueueSize - Configured maximum queue size")
         ));

   CHK_EXEC(CreateConfigParam(_T("NotificationChannels.MaxQueueSize"), L"500",
         _T("Maximum size of notification channel queue. When exceeded, new messages are dropped and event is generated. Set to 0 for unlimited."),
         nullptr, 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Update log parser XML by adding GUID to each parser
 */
static bool UpdateLogXML(const TCHAR *logName)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT var_value FROM config_clob WHERE var_name='%s'"), logName);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         char *text = DBGetFieldUTF8(hResult, 0, 0, nullptr, 0);
         if (text != nullptr)
         {
            pugi::xml_document xml;
            if (!xml.load_buffer(text, strlen(text)))
            {
               _tprintf(_T("Failed to load XML. Ignore \"%s\" log parsed\n"), logName);
               MemFree(text);
               DBFreeResult(hResult);
               return false;
            }

            pugi::xml_node node = xml.select_node("/parser/rules").node();
            for (pugi::xml_node child : node.children())
            {
               uuid guid = uuid::generate();
               char *guidAttr = guid.toString().getUTF8String();
               child.append_attribute("guid").set_value(guidAttr);
               MemFree(guidAttr);
            }

            xml_string_writer writer;
            xml.print(writer);

            DB_STATEMENT hStmt = DBPrepare(g_dbHandle, L"UPDATE config_clob SET var_value=? WHERE var_name=?");
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_TEXT, writer.result, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, logName, DB_BIND_STATIC);
               if (DBExecute(hStmt))
               {
                  DBFreeStatement(hStmt);
               }
               else
               {
                  MemFree(text);
                  DBFreeResult(hResult);
                  DBFreeStatement(hStmt);
                  return false;
               }
            }
            else
            {
               MemFree(text);
               DBFreeResult(hResult);
               return false;
            }
            MemFree(text);
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      return false;
   }

   return true;
}

/**
 * Upgrade from 60.8 to 60.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(UpdateLogXML(L"SyslogParser"));
   CHK_EXEC(UpdateLogXML(L"WindowsEventParser"));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Delete duplicate records from data table
 */
void DeleteDuplicateDataRecords(const wchar_t *tableType, uint32_t objectId);

/**
 * Add primary key for data tables for objects selected by given query
 */
static bool ConvertDataTables(const wchar_t *query, bool dataTablesWithPK)
{
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return false;

   bool success = true;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      uint32_t id = DBGetFieldULong(hResult, i, 0);

      if (IsDataTableExist(L"idata_%u", id))
      {
         WriteToTerminalEx(L"Converting table \x1b[1midata_%u\x1b[0m\n", id);

         wchar_t table[64];
         nx_swprintf(table, 64, L"idata_%u", id);

         if (dataTablesWithPK)
         {
            CHK_EXEC_NO_SP(DBDropPrimaryKey(g_dbHandle, table));
         }
         else
         {
            DeleteDuplicateDataRecords(L"idata", id);

            wchar_t index[64];
            nx_swprintf(index, 64, L"idx_idata_%u_id_timestamp", id);
            DBDropIndex(g_dbHandle, table, index);
         }

         CHK_EXEC_NO_SP(ConvertColumnToInt64(table, L"idata_timestamp"));
         CHK_EXEC_NO_SP(SQLQueryFormatted(L"UPDATE %s SET idata_timestamp = idata_timestamp * 1000", table));
         DBAddPrimaryKey(g_dbHandle,table, _T("item_id,idata_timestamp"));
      }

      if (IsDataTableExist(L"tdata_%u", id))
      {
         WriteToTerminalEx(L"Converting table \x1b[1mtdata_%u\x1b[0m\n", id);

         wchar_t table[64];
         nx_swprintf(table, 64, L"tdata_%u", id);

         if (dataTablesWithPK)
         {
            CHK_EXEC_NO_SP(DBDropPrimaryKey(g_dbHandle, table));
         }
         else
         {
            DeleteDuplicateDataRecords(L"tdata", id);

            wchar_t index[64];
            nx_swprintf(index, 64, L"idx_tdata_%u", id);
            DBDropIndex(g_dbHandle, table, index);
         }

         CHK_EXEC_NO_SP(ConvertColumnToInt64(table, L"tdata_timestamp"));
         CHK_EXEC_NO_SP(SQLQueryFormatted(L"UPDATE %s SET tdata_timestamp = tdata_timestamp * 1000", table));
         DBAddPrimaryKey(g_dbHandle,table, L"item_id,tdata_timestamp");
      }
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Add primary key to all data tables for given object class
 */
static bool ConvertDataTablesForClass(const wchar_t *className, bool dataTablesWithPK)
{
   wchar_t query[1024];
   nx_swprintf(query, 256, L"SELECT id FROM %s", className);
   return ConvertDataTables(query, dataTablesWithPK);
}

/**
 * Upgrade from 60.7 to 60.8
 */
static bool H_UpgradeFromV7()
{
   if ((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB))
   {
      CHK_EXEC(SQLQuery(
         L"CREATE OR REPLACE FUNCTION ms_to_timestamptz(ms_timestamp BIGINT) "
         L"RETURNS TIMESTAMPTZ AS $$ "
         L"    SELECT timestamptz 'epoch' + ms_timestamp * interval '1 millisecond'; "
         L"$$ LANGUAGE sql IMMUTABLE;"));
      CHK_EXEC(SQLQuery(
         L"CREATE OR REPLACE FUNCTION timestamptz_to_ms(t timestamptz) "
         L"RETURNS BIGINT AS $$ "
         L"    SELECT (EXTRACT(epoch FROM t) * 1000)::bigint; "
         L"$$ LANGUAGE sql IMMUTABLE;"));
   }

   // Convert raw_dci_values timestamp columns to INT64 milliseconds
   CHK_EXEC(ConvertColumnToInt64(L"raw_dci_values", L"last_poll_time"));
   CHK_EXEC(ConvertColumnToInt64(L"raw_dci_values", L"cache_timestamp"));
   CHK_EXEC(SQLQuery(L"UPDATE raw_dci_values SET last_poll_time = last_poll_time * 1000, cache_timestamp = CASE WHEN cache_timestamp > 1 THEN cache_timestamp * 1000 ELSE cache_timestamp END"));

   // Check if idata_/tdata_ tables already use PKs
   bool dataTablesWithPK = false;
   if (g_dbSyntax == DB_SYNTAX_PGSQL)
   {
      if (IsOnlineUpgradePending())
      {
         WriteToTerminal(L"Pending online upgrades must be completed before this step\n");
         return false;
      }

      int schemaV52 = GetSchemaLevelForMajorVersion(52);
      if (((schemaV52 >= 21) && (schemaV52 < 24)) || (DBMgrMetaDataReadInt32(L"PostgreSQL.DataTablesHavePK", 0) == 1))
      {
         // idata/tdata tables already converted to PKs
         dataTablesWithPK = true;
      }
   }

   // Convert idata/tdata timestamp columns to INT64 milliseconds
   if (g_dbSyntax != DB_SYNTAX_TSDB)
   {
      CHK_EXEC(ConvertColumnToInt64(L"idata", L"idata_timestamp"));
      CHK_EXEC(ConvertColumnToInt64(L"tdata", L"tdata_timestamp"));
      CHK_EXEC(SQLBatch(
         _T("UPDATE idata SET idata_timestamp = idata_timestamp * 1000\n")
         _T("UPDATE tdata SET tdata_timestamp = tdata_timestamp * 1000\n")
         _T("<END>")));
   }

   // Update metadata for idata_nnn/tdata_nnn table creation
   wchar_t query[256];
   nx_swprintf(query, 256, L"CREATE TABLE idata_%%d (item_id integer not null,idata_timestamp %s not null,idata_value varchar(255) null,raw_value varchar(255) null)", GetSQLTypeName(SQL_TYPE_INT64));
   CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataTableCreationCommand", query));
   nx_swprintf(query, 256, L"CREATE TABLE tdata_%%d (item_id integer not null,tdata_timestamp %s not null,tdata_value %s null)",
         GetSQLTypeName(SQL_TYPE_INT64), GetSQLTypeName(SQL_TYPE_TEXT));
   CHK_EXEC(DBMgrMetaDataWriteStr(L"TDataTableCreationCommand_0", query));

   // Convert all idata_nnn/tdata_nnn tables to INT64 milliseconds
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"nodes", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"clusters", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"mobile_devices", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"access_points", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"chassis", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"sensors", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTables(L"SELECT id FROM object_containers WHERE object_class=29 OR object_class=30", dataTablesWithPK));

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 60.6 to 60.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateEventTemplate(EVENT_INTERFACE_UNMANAGED, L"SYS_IF_UNMANAGED",
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("dd449241-1886-41e3-a279-9a084ff64cec"),
         _T("Interface \"%<interfaceName>\" changed state to UNMANAGED (IP Addr: %<interfaceIpAddress>/%<interfaceNetMask>, IfIndex: %<interfaceIndex>)"),
         _T("Generated when interface status changed to unmanaged.\r\n")
         _T("Parameters:\r\n")
         _T("   1) interfaceObjectId - Interface object ID\r\n")
         _T("   2) interfaceName - Interface name\r\n")
         _T("   3) interfaceIpAddress - Interface IP address\r\n")
         _T("   4) interfaceNetMask - Interface netmask\r\n")
         _T("   5) interfaceIndex - Interface index")
      ));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 60.5 to 60.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE nodes ADD last_events varchar(255)"));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 60.4 to 60.5
 */
static bool H_UpgradeFromV4()
{
   static const wchar_t *batch =
      L"ALTER TABLE interfaces ADD max_speed $SQL:INT64\n"
      L"UPDATE interfaces SET max_speed=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"interfaces", L"max_speed"));

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"description='Generated when system detects interface speed change.\r\n"
      L"Parameters:\r\n"
      L"   1) ifIndex - Interface index\r\n"
      L"   2) ifName - Interface name\r\n"
      L"   3) oldSpeed - Old speed in bps\r\n"
      L"   4) oldSpeedText - Old speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n"
      L"   5) newSpeed - New speed in bps\r\n"
      L"   6) newSpeedText - New speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n"
      L"   7) maxSpeed - Maximum speed in bps\r\n"
      L"   8) maxSpeedText - Maximum speed in bps with optional multiplier (kbps, Mbps, etc.)'"
      L" WHERE event_code=141"));   // SYS_IF_SPEED_CHANGED

   CHK_EXEC(CreateEventTemplate(EVENT_IF_SPEED_BELOW_MAXIMUM, L"SYS_IF_SPEED_BELOW_MAXIMUM",
         EVENT_SEVERITY_WARNING, EF_LOG, L"c020c587-ab9f-4834-ba3c-91583c2871b1",
         _T("Interface %<ifName> speed %<speedText> is below maximum %<maxSpeedText>"),
         _T("Generated when system detects that interface speed is below maximum.\r\n")
         _T("Parameters:\r\n")
         _T("   1) ifIndex - Interface index\r\n")
         _T("   2) ifName - Interface name\r\n")
         _T("   3) speed - Current speed in bps\r\n")
         _T("   4) speedText - Current speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n")
         _T("   5) maxSpeed - Maximum speed in bps\r\n")
         _T("   6) maxSpeedText - Maximum speed in bps with optional multiplier (kbps, Mbps, etc.)")
      ));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 60.3 to 60.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold value reached for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) thresholdValue - Threshold value\r\n")
      _T("   4) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   5) dciId - Data collection item ID\r\n")
      _T("   6) instance - Instance\r\n")
      _T("   7) isRepeatedEvent - Repeat flag\r\n")
      _T("   8) dciValue - Last collected DCI value\r\n")
      _T("   9) operation - Threshold''s operation code\r\n")
      _T("   10) function - Threshold''s function code\r\n")
      _T("   11) pollCount - Threshold''s required poll count\r\n")
      _T("   12) thresholdDefinition - Threshold''s textual definition\r\n")
      _T("   13) instanceValue - Instance value\r\n")
      _T("   14) instanceName - Instance name same as instance\r\n")
      _T("   15) thresholdId - Threshold''s ID'")
      _T(" WHERE event_code=17")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>)\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) dciId - Data collection item ID\r\n")
      _T("   4) instance - Instance\r\n")
      _T("   5) thresholdValue - Threshold value\r\n")
      _T("   6) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   7) dciValue - Last collected DCI value\r\n")
      _T("   8) operation - Threshold''s operation code\r\n")
      _T("   9) function - Threshold''s function code\r\n")
      _T("   10) pollCount - Threshold''s required poll count\r\n")
      _T("   11) thresholdDefinition - Threshold''s textual definition\r\n")
      _T("   12) instanceValue - Instance value\r\n")
      _T("   13) instanceName - Instance name same as instance\r\n")
      _T("   14) thresholdId - Threshold''s ID'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 60.2 to 60.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ai_tasks ("
      L"   id integer not null,"
      L"   user_id integer not null,"
      L"   description varchar(255) null,"
      L"   prompt $SQL:TEXT null,"
      L"   memento $SQL:TEXT null,"
      L"   last_execution_time integer not null,"
      L"   next_execution_time integer not null,"
      L"   iteration integer not null,"
      L"   PRIMARY KEY(id))"));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_task_execution_log ("
         L"   record_id $SQL:INT64 not null,"
         L"   execution_timestamp timestamptz not null,"
         L"   task_id integer not null,"
         L"   task_description varchar(255) null,"
         L"   user_id integer not null,"
         L"   status char(1) not null,"
         L"   iteration integer not null,"
         L"   explanation $SQL:TEXT null,"
         L"   PRIMARY KEY(record_id,execution_timestamp))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('ai_task_execution_log', 'execution_timestamp', chunk_time_interval => interval '86400 seconds')"));
   }
   else
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_task_execution_log ("
         L"   record_id $SQL:INT64 not null,"
         L"   execution_timestamp integer not null,"
         L"   task_id integer not null,"
         L"   task_description varchar(255) null,"
         L"   user_id integer not null,"
         L"   status char(1) not null,"
         L"   iteration integer not null,"
         L"   explanation $SQL:TEXT null,"
         L"   PRIMARY KEY(record_id))"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_task_exec_log_timestamp ON ai_task_execution_log(execution_timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_task_exec_log_asset_id ON ai_task_execution_log(task_id)"));

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 60.1 to 60.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AITasks.BaseSize",
            L"4",
            L"Base size for AI tasks thread pool.",
            nullptr, 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AITasks.MaxSize",
            L"4",
            L"Maximum size for AI tasks thread pool.",
            nullptr, 'I', true, true, false, false));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD ai_instructions $SQL:TEXT")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 60.0 to 60.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(L"ReportingServer.ResultsRetentionTime",
            L"90",
            L"The retention time for report execution results.",
            L"days", 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(1));
   return true;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int nextMajor;
   int nextMinor;
   bool (*upgradeProc)();
} s_dbUpgradeMap[] = {
   { 16, 60, 17, H_UpgradeFromV16 },
   { 15, 60, 16, H_UpgradeFromV15 },
   { 14, 60, 15, H_UpgradeFromV14 },
   { 13, 60, 14, H_UpgradeFromV13 },
   { 12, 60, 13, H_UpgradeFromV12 },
   { 11, 60, 12, H_UpgradeFromV11 },
   { 10, 60, 11, H_UpgradeFromV10 },
   { 9,  60, 10, H_UpgradeFromV9  },
   { 8,  60, 9,  H_UpgradeFromV8  },
   { 7,  60, 8,  H_UpgradeFromV7  },
   { 6,  60, 7,  H_UpgradeFromV6  },
   { 5,  60, 6,  H_UpgradeFromV5  },
   { 4,  60, 5,  H_UpgradeFromV4  },
   { 3,  60, 4,  H_UpgradeFromV3  },
   { 2,  60, 3,  H_UpgradeFromV2  },
   { 1,  60, 2,  H_UpgradeFromV1  },
   { 0,  60, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V60()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 60) && (minor < DB_SCHEMA_VERSION_V60_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 53.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 60.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         WriteToTerminal(L"Rolling back last stage due to upgrade errors...\n");
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
