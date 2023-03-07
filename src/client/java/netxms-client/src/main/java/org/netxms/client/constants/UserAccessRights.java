/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.constants;

/**
 * User access rights
 */
public final class UserAccessRights
{
	// System-wide rights
	public static final long SYSTEM_ACCESS_MANAGE_USERS            = 0x00000000000001L;
	public static final long SYSTEM_ACCESS_SERVER_CONFIG           = 0x00000000000002L;
	public static final long SYSTEM_ACCESS_CONFIGURE_TRAPS         = 0x00000000000004L;
	public static final long SYSTEM_ACCESS_MANAGE_SESSIONS         = 0x00000000000008L;
	public static final long SYSTEM_ACCESS_VIEW_EVENT_DB           = 0x00000000000010L;
	public static final long SYSTEM_ACCESS_EDIT_EVENT_DB           = 0x00000000000020L;
	public static final long SYSTEM_ACCESS_EPP                     = 0x00000000000040L;
	public static final long SYSTEM_ACCESS_MANAGE_ACTIONS          = 0x00000000000080L;
	public static final long SYSTEM_ACCESS_DELETE_ALARMS           = 0x00000000000100L;
	public static final long SYSTEM_ACCESS_MANAGE_PACKAGES         = 0x00000000000200L;
	public static final long SYSTEM_ACCESS_VIEW_EVENT_LOG          = 0x00000000000400L;
	public static final long SYSTEM_ACCESS_MANAGE_TOOLS            = 0x00000000000800L;
	public static final long SYSTEM_ACCESS_MANAGE_SCRIPTS          = 0x00000000001000L;
	public static final long SYSTEM_ACCESS_VIEW_TRAP_LOG           = 0x00000000002000L;
	public static final long SYSTEM_ACCESS_VIEW_AUDIT_LOG          = 0x00000000004000L;
	public static final long SYSTEM_ACCESS_MANAGE_AGENT_CFG        = 0x00000000008000L;
	public static final long SYSTEM_ACCESS_PERSISTENT_STORAGE      = 0x00000000010000L;
	public static final long SYSTEM_ACCESS_SEND_NOTIFICATION       = 0x00000000020000L;
	public static final long SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN     = 0x00000000040000L;
	public static final long SYSTEM_ACCESS_REGISTER_AGENTS         = 0x00000000080000L;
	public static final long SYSTEM_ACCESS_READ_SERVER_FILES       = 0x00000000100000L;
	public static final long SYSTEM_ACCESS_SERVER_CONSOLE          = 0x00000000200000L;
	public static final long SYSTEM_ACCESS_MANAGE_SERVER_FILES     = 0x00000000400000L;
	public static final long SYSTEM_ACCESS_MANAGE_MAPPING_TBLS     = 0x00000000800000L;
   public static final long SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS     = 0x00000001000000L;
   public static final long SYSTEM_ACCESS_REPORTING_SERVER        = 0x00000002000000L;
   //public static final long SYSTEM_ACCESS_XMPP_COMMANDS         = 0x00000004000000L; reserved for future reuse
   public static final long SYSTEM_ACCESS_MANAGE_IMAGE_LIB        = 0x00000008000000L;
   public static final long SYSTEM_ACCESS_UNLINK_ISSUES           = 0x00000010000000L;
   public static final long SYSTEM_ACCESS_VIEW_SYSLOG             = 0x00000020000000L;
   public static final long SYSTEM_ACCESS_USER_SCHEDULED_TASKS    = 0x00000040000000L;
   public static final long SYSTEM_ACCESS_OWN_SCHEDULED_TASKS     = 0x00000080000000L;
   public static final long SYSTEM_ACCESS_ALL_SCHEDULED_TASKS     = 0x00000100000000L;
   public static final long SYSTEM_ACCESS_SCHEDULE_SCRIPT         = 0x00000200000000L;
   public static final long SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD    = 0x00000400000000L;
   public static final long SYSTEM_ACCESS_SCHEDULE_MAINTENANCE    = 0x00000800000000L;
   public static final long SYSTEM_ACCESS_MANAGE_REPOSITORIES     = 0x00001000000000L;
   public static final long SYSTEM_ACCESS_VIEW_REPOSITORIES       = 0x00002000000000L;
   public static final long SYSTEM_ACCESS_VIEW_ALL_ALARMS         = 0x00004000000000L;
   public static final long SYSTEM_ACCESS_EXTERNAL_INTEGRATION    = 0x00008000000000L;
   public static final long SYSTEM_ACCESS_SETUP_TCP_PROXY         = 0x00010000000000L;
   public static final long SYSTEM_ACCESS_IMPORT_CONFIGURATION    = 0x00020000000000L;
   public static final long SYSTEM_ACCESS_UA_NOTIFICATIONS        = 0x00040000000000L;
   public static final long SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS = 0x00080000000000L;
   public static final long SYSTEM_ACCESS_OBJECT_CATEGORIES       = 0x00100000000000L;
   public static final long SYSTEM_ACCESS_MANAGE_GEO_AREAS        = 0x00200000000000L;
   public static final long SYSTEM_ACCESS_SSH_KEY_CONFIGURATION   = 0x00400000000000L;
   public static final long SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES   = 0x00800000000000L;
   public static final long SYSTEM_ACCESS_MANAGE_2FA_METHODS      = 0x01000000000000L;
   public static final long SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE     = 0x02000000000000L;

	// Object access rights
	public static final int OBJECT_ACCESS_READ          = 0x00000001;
	public static final int OBJECT_ACCESS_MODIFY        = 0x00000002;
	public static final int OBJECT_ACCESS_CREATE        = 0x00000004;
	public static final int OBJECT_ACCESS_DELETE        = 0x00000008;
	public static final int OBJECT_ACCESS_READ_ALARMS   = 0x00000010;
	public static final int OBJECT_ACCESS_ACL           = 0x00000020;
	public static final int OBJECT_ACCESS_UPDATE_ALARMS = 0x00000040;
	public static final int OBJECT_ACCESS_SEND_EVENTS   = 0x00000080;
	public static final int OBJECT_ACCESS_CONTROL       = 0x00000100;
	public static final int OBJECT_ACCESS_TERM_ALARMS   = 0x00000200;
	public static final int OBJECT_ACCESS_PUSH_DATA     = 0x00000400;
   public static final int OBJECT_ACCESS_CREATE_ISSUE  = 0x00000800;
   public static final int OBJECT_ACCESS_DOWNLOAD      = 0x00001000;
   public static final int OBJECT_ACCESS_UPLOAD        = 0x00002000;
   public static final int OBJECT_ACCESS_MANAGE_FILES  = 0x00004000;
   public static final int OBJECT_ACCESS_MAINTENANCE   = 0x00008000;
   public static final int OBJECT_ACCESS_READ_AGENT    = 0x00010000;
   public static final int OBJECT_ACCESS_READ_SNMP     = 0x00020000;
   public static final int OBJECT_ACCESS_SCREENSHOT    = 0x00040000;
   public static final int OBJECT_ACCESS_EDIT_MNT_JOURNAL = 0x00080000; 
}
