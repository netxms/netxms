/**
 * 
 */
package org.netxms.api.client.constants;

/**
 * User access rights
 *
 */
public class UserAccessRights
{
	// System-wide rights
	public static final int SYSTEM_ACCESS_MANAGE_USERS        = 0x00000001;
	public static final int SYSTEM_ACCESS_SERVER_CONFIG       = 0x00000002;
	public static final int SYSTEM_ACCESS_CONFIGURE_TRAPS     = 0x00000004;
	public static final int SYSTEM_ACCESS_MANAGE_SESSIONS     = 0x00000008;
	public static final int SYSTEM_ACCESS_VIEW_EVENT_DB       = 0x00000010;
	public static final int SYSTEM_ACCESS_EDIT_EVENT_DB       = 0x00000020;
	public static final int SYSTEM_ACCESS_EPP                 = 0x00000040;
	public static final int SYSTEM_ACCESS_MANAGE_ACTIONS      = 0x00000080;
	public static final int SYSTEM_ACCESS_DELETE_ALARMS       = 0x00000100;
	public static final int SYSTEM_ACCESS_MANAGE_PACKAGES     = 0x00000200;
	public static final int SYSTEM_ACCESS_VIEW_EVENT_LOG      = 0x00000400;
	public static final int SYSTEM_ACCESS_MANAGE_TOOLS        = 0x00000800;
	public static final int SYSTEM_ACCESS_MANAGE_SCRIPTS      = 0x00001000;
	public static final int SYSTEM_ACCESS_VIEW_TRAP_LOG       = 0x00002000;
	public static final int SYSTEM_ACCESS_VIEW_AUDIT_LOG      = 0x00004000;
	public static final int SYSTEM_ACCESS_MANAGE_AGENT_CFG    = 0x00008000;
	public static final int SYSTEM_ACCESS_MANAGE_SITUATIONS   = 0x00010000;
	public static final int SYSTEM_ACCESS_SEND_SMS            = 0x00020000;
	public static final int SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN = 0x00040000;
	public static final int SYSTEM_ACCESS_REGISTER_AGENTS     = 0x00080000;
	public static final int SYSTEM_ACCESS_READ_FILES          = 0x00100000;
	public static final int SYSTEM_ACCESS_SERVER_CONSOLE      = 0x00200000;
	public static final int SYSTEM_ACCESS_MANAGE_FILES        = 0x00400000;
	public static final int SYSTEM_ACCESS_MANAGE_MAPPING_TBLS = 0x00800000;
   public static final int SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS = 0x01000000;
   public static final int SYSTEM_ACCESS_REPORTING_SERVER    = 0x02000000;
   public static final int SYSTEM_ACCESS_XMPP_COMMANDS       = 0x04000000;
   public static final int SYSTEM_ACCESS_MANAGE_IMAGE_LIB    = 0x08000000;
	
	public static final int OBJECT_ACCESS_READ          = 0x00000001;
	public static final int OBJECT_ACCESS_MODIFY        = 0x00000002;
	public static final int OBJECT_ACCESS_CREATE        = 0x00000004;
	public static final int OBJECT_ACCESS_DELETE        = 0x00000008;
	public static final int OBJECT_ACCESS_READ_ALARMS   = 0x00000010;
	public static final int OBJECT_ACCESS_ACL           = 0x00000020;
	public static final int OBJECT_ACCESS_ACK_ALARMS    = 0x00000040;
	public static final int OBJECT_ACCESS_SEND_EVENTS   = 0x00000080;
	public static final int OBJECT_ACCESS_CONTROL       = 0x00000100;
	public static final int OBJECT_ACCESS_TERM_ALARMS   = 0x00000200;
	public static final int OBJECT_ACCESS_PUSH_DATA     = 0x00000400;
}
