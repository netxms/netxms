/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.client;

import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.users.AbstractAccessListElement;

/**
 * Access list element for NetXMS objects
 */
public class AccessListElement extends AbstractAccessListElement
{
   private boolean inherited;

	/**
	 * Create new ACL element with given user ID and rights
	 * 
	 * @param userId user id
	 * @param accessRights bit mask
	 */
   public AccessListElement(int userId, int accessRights)
	{
		super(userId, accessRights);
      inherited = false;
	}

	/**
	 * Copy constructor
	 * 
	 * @param src Source ACL element
	 */
	public AccessListElement(AccessListElement src)
	{
		super(src);
      inherited = src.inherited;
	}

   /**
    * Create copy of existing ACL entry with possibly changed inheritance flag.
    * 
    * @param src Source ACL element
    * @param inherited inheritance flag
    */
   public AccessListElement(AccessListElement src, boolean inherited)
   {
      super(src);
      this.inherited = inherited;
   }

	/**
    * Check if this access list element was inherited.
    *
    * @return true if this access list element was inherited
    */
   public boolean isInherited()
   {
      return inherited;
   }

   /**
    * @return true if READ access granted
    */
	public boolean hasRead()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_READ) != 0;
	}
	
   /**
    * @return true if READ AGENT access granted
    */
   public boolean hasReadAgent()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_READ_AGENT) != 0;
   }

   /**
    * @return true if READ SNMP access granted
    */
   public boolean hasReadSNMP()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_READ_SNMP) != 0;
   }

	/**
	 * @return true if MODIFY access granted
	 */
	public boolean hasModify()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_MODIFY) != 0;
	}
	
   /**
    * @return true if EDIT COMMENTS access granted
    */
   public boolean hasEditComments()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_EDIT_COMMENTS) != 0;
   }

   /**
    * @return true if EDIT RESPONSIBLE USERS access granted
    */
   public boolean hasEditResponsibleUsers()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_EDIT_RESP_USERS) != 0;
   }

   /**
    * @return true if CREATE access granted
    */
   public boolean hasCreate()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_CREATE) != 0;
   }

	/**
	 * @return true if DELETE access granted
	 */
	public boolean hasDelete()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_DELETE) != 0;
	}
	
   /**
    * @return true if CONTROL access granted
    */
   public boolean hasControl()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_CONTROL) != 0;
   }

   /**
    * @return true if SEND EVENTS access granted
    */
   public boolean hasSendEvents()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_SEND_EVENTS) != 0;
   }

	/**
	 * @return true if READ ALARMS access granted
	 */
	public boolean hasReadAlarms()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_READ_ALARMS) != 0;
	}
	
	/**
	 * @return true if ACK ALARMS access granted
	 */
	public boolean hasAckAlarms()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_UPDATE_ALARMS) != 0;
	}
	
	/**
	 * @return true if TERMINATE ALARMS access granted
	 */
	public boolean hasTerminateAlarms()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_TERM_ALARMS) != 0;
	}
	
   /**
    * @return true if CREATE ISSUE access granted
    */
   public boolean hasCreateIssue()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_CREATE_ISSUE) != 0;
   }

   /**
    * @return true if PUSH DATA access granted
    */
   public boolean hasPushData()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_PUSH_DATA) != 0;
   }

	/**
	 * @return true if ACCESS CONTROL access granted
	 */
	public boolean hasAccessControl()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_ACL) != 0;
	}
	
   /**
    * @return true if DOWNLOAD access granted
    */
   public boolean hasDownload()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_DOWNLOAD) != 0;
   }

   /**
    * @return true if UPLOAD access granted
    */
   public boolean hasUpload()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_UPLOAD) != 0;
   }

   /**
    * @return true if MANAGE FILES access granted
    */
   public boolean hasManage()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_MANAGE_FILES) != 0;
   }

   /**
    * @return true if CONFIGURE AGENT access granted
    */
   public boolean hasConfigureAgent()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_CONFIGURE_AGENT) != 0;
   }

   /**
    * @return true if SCREENSHOT access granted
    */
   public boolean hasTakeScreenshot()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_SCREENSHOT) != 0;
   }

   /**
    * @return true if MAINTENANCE access granted
    */
   public boolean hasControlMaintenance()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_MAINTENANCE) != 0;
   }

   /**
    * @return true if EDIT MAINTENANCE JOURNAL access granted
    */
   public boolean hasEditMaintenbanceJournal()
   {
      return (accessRights & UserAccessRights.OBJECT_ACCESS_EDIT_MNT_JOURNAL) != 0;
   }
}
