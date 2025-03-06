/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.client.users;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Abstract NetXMS user database object.
 */
public abstract class AbstractUserObject
{
   // Type strings
   public static final String USERDB_TYPE_USER = "user";
   public static final String USERDB_TYPE_GROUP = "group";

	// Object flags
	public static final int MODIFIED = 0x0001;
	public static final int DELETED = 0x0002;
	public static final int DISABLED = 0x0004;
	public static final int CHANGE_PASSWORD = 0x0008;
	public static final int CANNOT_CHANGE_PASSWORD = 0x0010;
	public static final int INTRUDER_LOCKOUT = 0x0020;
	public static final int PASSWORD_NEVER_EXPIRES = 0x0040;
   public static final int LDAP_USER = 0x0080;
   public static final int SYNC_EXCEPTION = 0x0100;
   public static final int CLOSE_OTHER_SESSIONS = 0x0200;
   public static final int TOKEN_AUTH_ONLY = 0x0400;

   // User object fields
   public static final int MODIFY_LOGIN_NAME        = 0x00000001;
   public static final int MODIFY_DESCRIPTION       = 0x00000002;
   public static final int MODIFY_FULL_NAME         = 0x00000004;
   public static final int MODIFY_FLAGS             = 0x00000008;
   public static final int MODIFY_ACCESS_RIGHTS     = 0x00000010;
   public static final int MODIFY_MEMBERS           = 0x00000020;
   public static final int MODIFY_CERT_MAPPING      = 0x00000040;
   public static final int MODIFY_AUTH_METHOD       = 0x00000080;
   public static final int MODIFY_PASSWD_LENGTH     = 0x00000100;
   public static final int MODIFY_TEMP_DISABLE      = 0x00000200;
   public static final int MODIFY_CUSTOM_ATTRIBUTES = 0x00000400;
   public static final int MODIFY_UI_ACCESS_RULES   = 0x00000800;
   public static final int MODIFY_GROUP_MEMBERSHIP  = 0x00001000;
   public static final int MODIFY_EMAIL             = 0x00002000;
   public static final int MODIFY_PHONE_NUMBER      = 0x00004000;
   public static final int MODIFY_2FA_BINDINGS      = 0x00008000;

   // Well-known IDs
   public static final int WELL_KNOWN_ID_SYSTEM = 0;
   public static final int WELL_KNOWN_ID_EVERYONE = 1073741824;

   protected String type;  // "user" or "group", used by REST API
   protected int id;
	protected String name;
	protected UUID guid;
	protected long systemRights;
   protected String uiAccessRules;
	protected int flags;
	protected String description;
	protected String ldapDn;
	protected String ldapId;
   protected Map<String, String> customAttributes;
   private Date created = null;

	/**
	 * Create new user object.
	 *
	 * @param name object name
	 * @param type object type for REST API (should be "user" or "group")
	 */
	public AbstractUserObject(String name, String type)
	{
	   this.type = type;
		this.name = name;
		description = "";
		guid = UUID.randomUUID();
      customAttributes = new HashMap<String, String>(0);
	}

	/**
	 * Copy constructor
	 *
	 * @param src source object
	 */
	public AbstractUserObject(final AbstractUserObject src)
	{
	   this.type = src.type;
		this.id = src.id;
		this.name = new String(src.name);
		this.guid = UUID.fromString(src.guid.toString());
		this.systemRights = src.systemRights;
      this.uiAccessRules = src.uiAccessRules;
		this.flags = src.flags;
		this.description = src.description;
		this.ldapDn = src.ldapDn;
		this.ldapId = src.ldapId;
		this.created = src.created;
      this.customAttributes = new HashMap<String, String>(src.customAttributes);
	}

	/**
	 * Create object from NXCP message
	 * @param msg Message containing object's data
	 * @param type object type
	 */
	public AbstractUserObject(final NXCPMessage msg, String type)
	{
	   this.type = type;
      id = msg.getFieldAsInt32(NXCPCodes.VID_USER_ID);
		name = msg.getFieldAsString(NXCPCodes.VID_USER_NAME);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_USER_FLAGS);
		systemRights = msg.getFieldAsInt64(NXCPCodes.VID_USER_SYS_RIGHTS);
      uiAccessRules = msg.getFieldAsString(NXCPCodes.VID_UI_ACCESS_RULES);
		description = msg.getFieldAsString(NXCPCodes.VID_USER_DESCRIPTION);
		guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
		ldapDn = msg.getFieldAsString(NXCPCodes.VID_LDAP_DN);
      ldapId = msg.getFieldAsString(NXCPCodes.VID_LDAP_ID);
      created = msg.getFieldAsDate(NXCPCodes.VID_CREATION_TIME);
      customAttributes = msg.getStringMapFromFields(NXCPCodes.VID_CUSTOM_ATTRIBUTES_BASE, NXCPCodes.VID_NUM_CUSTOM_ATTRIBUTES);
	}

	/**
	 * Fill NXCP message with object data
	 * @param msg destination message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, id);
		msg.setField(NXCPCodes.VID_USER_NAME, name);
		msg.setFieldInt16(NXCPCodes.VID_USER_FLAGS, flags);
		msg.setFieldInt64(NXCPCodes.VID_USER_SYS_RIGHTS, systemRights);
      msg.setField(NXCPCodes.VID_UI_ACCESS_RULES, uiAccessRules);
		msg.setField(NXCPCodes.VID_USER_DESCRIPTION, description);
      msg.setFieldsFromStringMap(customAttributes, NXCPCodes.VID_CUSTOM_ATTRIBUTES_BASE, NXCPCodes.VID_NUM_CUSTOM_ATTRIBUTES);
	}

	/**
	 * @return true if user is marked as deleted
	 */
	public boolean isDeleted()
	{
		return (flags & DELETED) == DELETED;
	}

	/**
	 * @return the id
	 */
   public int getId()
	{
		return id;
	}

	/**
	 * @param id the id to set
	 */
   public void setId(int id)
	{
		this.id = id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the guid
	 */
	public UUID getGuid()
	{
		return guid;
	}

	/**
    * @return the ldapDn
    */
   public String getLdapDn()
   {
      return ldapDn;
   }

   /**
    * @return the ldapId
    */
   public String getLdapId()
   {
      return ldapId;
   }

   /**
	 * @return the systemRights
	 */
	public long getSystemRights()
	{
		return systemRights;
	}

	/**
	 * @param systemRights the systemRights to set
	 */
	public void setSystemRights(long systemRights)
	{
		this.systemRights = systemRights;
	}

	/**
    * @return the uiAccessRules
    */
   public String getUIAccessRules()
   {
      return uiAccessRules;
   }

   /**
    * @param uiAccessRules the uiAccessRules to set
    */
   public void setUIAccessRules(String uiAccessRules)
   {
      this.uiAccessRules = uiAccessRules;
   }

   /**
    * @return the flags
    */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
	}

	/**
    * Get display label for this object. Label includes object name and description (if present). Subclasses may add additional
    * fields to the label.
    * 
    * @return display label for this object
    */
   public String getLabel()
   {
      return description.isEmpty() ? name : (name + " (" + description + ")");
   }

   /**
    * Get custom attribute
    * 
    * @param name Name of the attribute
    * @return Custom attribute value
    */
	public String getCustomAttribute(final String name)
	{
		return customAttributes.get(name);
	}
	
	/**
	 * Set custom attribute's value
	 * @param name Name of the attribute
	 * @param value New value for attribute
	 */
	public void setCustomAttribute(final String name, final String value)
	{
		customAttributes.put(name, value);
	}

	/**
    * @return the customAttributes
    */
   public Map<String, String> getCustomAttributes()
   {
      return customAttributes;
   }

   /**
    * @param customAttributes the customAttributes to set
    */
   public void setCustomAttributes(Map<String, String> customAttributes)
   {
      this.customAttributes = customAttributes;
   }

   /**
    * Check if object is disabled
    * 
    * @return true if object is disabled
    */
	public boolean isDisabled()
	{
		return ((flags & DISABLED) == DISABLED);
	}
	
	/**
	 * Check if password should be changed at next logon
	 * @return true if password should be changed at next logon
	 */
	public boolean isPasswordChangeNeeded()
	{
		return ((flags & CHANGE_PASSWORD) == CHANGE_PASSWORD);
	}
	
	/**
	 * Check if password change is forbidden
	 * @return true if password change is forbidden
	 */
	public boolean isPasswordChangeForbidden()
	{
		return ((flags & CANNOT_CHANGE_PASSWORD) == CANNOT_CHANGE_PASSWORD);
	}

	/**
	 * Get creation date
	 * @return creation date
	 */
	public Date getCreationTime()
	{
	   return created;
	}
}
