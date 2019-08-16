/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.util.Locale;
import java.util.MissingResourceException;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;
import org.netxms.base.CommonRCC;

/**
 * This class represents request completion codes (RCC) sent by NetXMS server.
 */
public final class RCC extends CommonRCC
{
   public static final int ACCESS_DENIED = 2;
	public static final int INVALID_OBJECT_ID = 7;
   public static final int CANT_CREATE_OBJECT = 8;
	public static final int DUPLICATE_DCI = 13;
	public static final int INVALID_DCI_ID = 14;
	public static final int OBJECT_CREATION_FAILED = 18;
	public static final int OBJECT_LOOP = 19;
	public static final int INVALID_OBJECT_NAME = 20;
	public static final int INVALID_ALARM_ID = 21;
	public static final int INVALID_ACTION_ID = 22;
	public static final int DCI_COPY_ERRORS = 24;
	public static final int INVALID_EVENT_CODE = 25;
	public static final int NO_WOL_INTERFACES = 26;
	public static final int NO_MAC_ADDRESS = 27;
	public static final int INVALID_TRAP_ID = 29;
	public static final int DCI_NOT_SUPPORTED = 30;
	public static final int NPI_PARSE_ERROR = 32;
	public static final int DUPLICATE_PACKAGE = 33;
	public static final int PACKAGE_FILE_EXIST = 34;
	public static final int RESOURCE_BUSY = 35;
	public static final int INVALID_PACKAGE_ID = 36;
	public static final int INVALID_IP_ADDR = 37;
	public static final int ACTION_IN_USE = 38;
	public static final int VARIABLE_NOT_FOUND = 39;
	public static final int ADDRESS_IN_USE = 41;
	public static final int EXEC_FAILED = 47;
	public static final int INVALID_TOOL_ID = 48;
	public static final int SNMP_ERROR = 49;
	public static final int BAD_REGEXP = 50;
	public static final int UNKNOWN_PARAMETER = 51;
	public static final int FILE_IO_ERROR = 52;
	public static final int CORRUPTED_MIB_FILE = 53;
	public static final int INVALID_JOB_ID = 55;
	public static final int INVALID_SCRIPT_ID = 56;
	public static final int INVALID_SCRIPT_NAME = 57;
	public static final int UNKNOWN_MAP_NAME = 58;
	public static final int INVALID_MAP_ID = 59;
	public static final int ACCOUNT_DISABLED = 60;
	public static final int NO_GRACE_LOGINS = 61;
	public static final int CONNECTION_BROKEN = 62;
	public static final int INVALID_CONFIG_ID = 63;
	public static final int DB_CONNECTION_LOST = 64;
	public static final int ALARM_OPEN_IN_HELPDESK = 65;
	public static final int ALARM_NOT_OUTSTANDING = 66;
	public static final int NOT_PUSH_DCI = 67;
	public static final int CONFIG_PARSE_ERROR = 68;
	public static final int CONFIG_VALIDATION_ERROR = 69;
	public static final int INVALID_GRAPH_ID = 70;
	public static final int SNMP_FAILURE = 75;
	public static final int NO_L2_TOPOLOGY_SUPPORT = 76;
	public static final int RCC_INVALID_PSTORAGE_KEY = 77;
	public static final int NO_SUCH_INSTANCE = 78;
	public static final int INVALID_EVENT_ID = 79;
	public static final int AGENT_ERROR = 80;
	public static final int UNKNOWN_VARIABLE = 81;
	public static final int RESOURCE_NOT_AVAILABLE = 82;
	public static final int JOB_CANCEL_FAILED = 83;
	public static final int INVALID_POLICY_ID = 84;
	public static final int UNKNOWN_LOG_NAME = 85;
	public static final int INVALID_LOG_HANDLE = 86;
	public static final int INVALID_SESSION_HANDLE = 89;
	public static final int CLUSTER_MEMBER_ALREADY = 90;
	public static final int JOB_HOLD_FAILED = 91;
	public static final int JOB_UNHOLD_FAILED = 92;
	public static final int ZONE_ID_ALREADY_IN_USE = 93;
	public static final int INVALID_ZONE_ID = 94;
	public static final int ZONE_NOT_EMPTY = 95;
	public static final int NO_COMPONENT_DATA = 96;
	public static final int INVALID_ALARM_NOTE_ID = 97;
	public static final int INVALID_MAPPING_TABLE_ID = 99;
	public static final int NO_SOFTWARE_PACKAGE_DATA = 100;
	public static final int INVALID_SUMMARY_TABLE_ID = 101;
	public static final int USER_LOGGED_IN = 102;
   public static final int NO_HDLINK = 108;
   public static final int HDLINK_COMM_FAILURE = 109;
   public static final int HDLINK_ACCESS_DENIED = 110;
   public static final int HDLINK_INTERNAL_ERROR = 111;
   public static final int NO_LDAP_CONNECTION = 112;
   public static final int NO_ROUTING_TABLE = 113;
   public static final int NO_FDB = 114;
   public static final int NO_LOCATION_HISTORY = 115;
   public static final int NXSL_COMPILATION_ERROR = 117;
   public static final int NXSL_EXECUTION_ERROR = 118;
   public static final int UNKNOWN_CONFIG_VARIABLE = 119;
   public static final int UNSUPPORTED_AUTH_METHOD = 120;
   public static final int NAME_ALEARDY_EXISTS = 121;
   public static final int CATEGORY_IN_USE = 122;
   public static final int CATEGORY_NAME_EMPTY = 123;
   public static final int AGENT_FILE_DOWNLOAD_ERROR = 124;
   public static final int INVALID_TUNNEL_ID = 125;
   public static final int FILE_ALREADY_EXISTS = 126;
   public static final int FOLDER_ALREADY_EXIST = 127;
   public static final int NO_SUCH_POLICY = 128;
   public static final int NO_HARDWARE_DATA = 129;
   public static final int CHANNEL_ALREADY_EXIST = 130;
   public static final int NO_CHANNEL_NAME = 131;
   public static final int CHANNEL_IN_USE = 132;
	
	// SNMP-specific, has no corresponding RCC_xxx constants in C library
	public static final int BAD_MIB_FILE_HEADER = 1001;
	public static final int BAD_MIB_FILE_DATA = 1002;
	
	/**
	 * Get message text for given RCC
	 * 
	 * @param code request completion code
	 * @param lang language code
	 * @param additionalInfo additional info (to be included into message text)
	 * @return formatted message text
	 */
	public static String getText(int code, String lang, String additionalInfo)
	{
      try
      {
         ResourceBundle bundle = PropertyResourceBundle.getBundle("messages", new Locale(lang));
         try
         {
            return String.format(bundle.getString(String.format("RCC_%04d", code)), additionalInfo);
         }
         catch(MissingResourceException e)
         {
            return String.format(bundle.getString("RCC_UNKNOWN"), code);
         }
      }
      catch(Exception e)
      {
         return "Error " + Integer.toString(code);
      }
	}
}
