/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
 * This class represents request completion codes (RCC) sent by server.
 * 
 * @author Victor Kirhenshtein
 *
 */
public final class RCC
{
	public static final int SUCCESS = 0;
	public static final int COMPONENT_LOCKED = 1;
	public static final int ACCESS_DENIED = 2;
	public static final int INVALID_REQUEST = 3;
	public static final int TIMEOUT = 4;
	public static final int OUT_OF_STATE_REQUEST = 5;
	public static final int DB_FAILURE = 6;
	public static final int INVALID_OBJECT_ID = 7;
	public static final int ALREADY_EXIST = 8;
	public static final int COMM_FAILURE = 9;
	public static final int SYSTEM_FAILURE = 10;
	public static final int INVALID_USER_ID = 11;
	public static final int INVALID_ARGUMENT = 12;
	public static final int DUPLICATE_DCI = 13;
	public static final int INVALID_DCI_ID = 14;
	public static final int OUT_OF_MEMORY = 15;
	public static final int IO_ERROR = 16;
	public static final int INCOMPATIBLE_OPERATION = 17;
	public static final int OBJECT_CREATION_FAILED = 18;
	public static final int OBJECT_LOOP = 19;
	public static final int INVALID_OBJECT_NAME = 20;
	public static final int INVALID_ALARM_ID = 21;
	public static final int INVALID_ACTION_ID = 22;
	public static final int OPERATION_IN_PROGRESS = 23;
	public static final int DCI_COPY_ERRORS = 24;
	public static final int INVALID_EVENT_CODE = 25;
	public static final int NO_WOL_INTERFACES = 26;
	public static final int NO_MAC_ADDRESS = 27;
	public static final int NOT_IMPLEMENTED = 28;
	public static final int INVALID_TRAP_ID = 29;
	public static final int DCI_NOT_SUPPORTED = 30;
	public static final int VERSION_MISMATCH = 31;
	public static final int NPI_PARSE_ERROR = 32;
	public static final int DUPLICATE_PACKAGE = 33;
	public static final int PACKAGE_FILE_EXIST = 34;
	public static final int RESOURCE_BUSY = 35;
	public static final int INVALID_PACKAGE_ID = 36;
	public static final int INVALID_IP_ADDR = 37;
	public static final int ACTION_IN_USE = 38;
	public static final int VARIABLE_NOT_FOUND = 39;
	public static final int BAD_PROTOCOL = 40;
	public static final int ADDRESS_IN_USE = 41;
	public static final int NO_CIPHERS = 42;
	public static final int INVALID_PUBLIC_KEY = 43;
	public static final int INVALID_SESSION_KEY = 44;
	public static final int NO_ENCRYPTION_SUPPORT = 45;
	public static final int INTERNAL_ERROR = 46;
	public static final int EXEC_FAILED = 47;
	public static final int INVALID_TOOL_ID = 48;
	public static final int SNMP_ERROR = 49;
	public static final int BAD_REGEXP = 50;
	public static final int UNKNOWN_PARAMETER = 51;
	public static final int FILE_IO_ERROR = 52;
	public static final int CORRUPTED_MIB_FILE = 53;
	public static final int TRANSFER_IN_PROGRESS = 54;
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
	public static final int LOCAL_CRYPTO_ERROR = 71;
	public static final int UNSUPPORTED_AUTH_TYPE = 72;
	public static final int BAD_CERTIFICATE = 73;
	public static final int INVALID_CERT_ID = 74;
	public static final int SNMP_FAILURE = 75;
	public static final int NO_L2_TOPOLOGY_SUPPORT = 76;
	public static final int INVALID_SITUATION_ID = 77;
	public static final int INSTANCE_NOT_FOUND = 78;
	public static final int INVALID_EVENT_ID = 79;
	public static final int AGENT_ERROR = 80;
	public static final int UNKNOWN_VARIABLE = 81;
	public static final int RESOURCE_NOT_AVAILABLE = 82;
	public static final int JOB_CANCEL_FAILED = 83;
	public static final int INVALID_POLICY_ID = 84;
	public static final int UNKNOWN_LOG_NAME = 85;
	public static final int INVALID_LOG_HANDLE = 86;
	
	// SNMP-specific, has no corresponding RCC_xxx constants in C library
	public static final int BAD_MIB_FILE_HEADER = 1001;
	public static final int BAD_MIB_FILE_DATA = 1002;
}
