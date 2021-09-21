/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.businessservices;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.BusinessChecksType;

/**
 * Business service check
 */
public class ServiceCheck 
{	
	private long id;
	private BusinessChecksType checkType;
   private String description;
	private String script;
   private long objectId;
   private long dciId;
	private int threshold;
   private boolean violated = false;
	private String failureReason;

   /**
    * Default constructor 
    */
   public ServiceCheck()
   {
      id = 0;
      checkType = BusinessChecksType.DCI;
      description = null;
      script = null; 
      objectId = 0;
      dciId = 0;
      threshold = 0;
   }

   /**
    * Copy constructor
    * 
    * @param check check to copy
    */
   public ServiceCheck(ServiceCheck check)
   {
      id = check.id;
      checkType = check.checkType;
      description = check.description;
      script = check.script; 
      objectId = check.objectId;
      dciId = check.dciId;
      threshold = check.threshold;
   }

	/**
	 * Constructor to create object form message 
	 * 
	 * @param msg NXCPmessage from server with data
	 * @param base base id for object
	 */
	public ServiceCheck(NXCPMessage msg, long base)
	{
	   id = msg.getFieldAsInt64(base);
		checkType = BusinessChecksType.getByValue(msg.getFieldAsInt32(base + 1));
      failureReason = msg.getFieldAsString(base + 2);
      dciId = msg.getFieldAsInt64(base + 3);
      objectId = msg.getFieldAsInt64(base + 4);
      threshold = msg.getFieldAsInt32(base + 5); 
		description = msg.getFieldAsString(base + 6); 
		script = msg.getFieldAsString(base + 7); 
		if (failureReason != null && !failureReason.isEmpty())
		{
		   violated = true;
		}
	}
	
	/**
	 * Fill message
	 * 
	 * @param msg NXCPMessage to fill data
	 */
	public void fillMessage(NXCPMessage msg)
	{
	   msg.setFieldInt32(NXCPCodes.VID_BUSINESS_SERVICE_CHECK_ID, (int)id);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
	   msg.setFieldInt32(NXCPCodes.VID_BUSINESS_SERVICE_CHECK_TYPE, checkType.getValue());
      msg.setFieldInt32(NXCPCodes.VID_BUSINESS_SERVICE_CHECK_RELATED_OBJECT, (int)objectId);
      msg.setFieldInt32(NXCPCodes.VID_BUSINESS_SERVICE_CHECK_RELATED_DCI, (int)dciId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setFieldInt32(NXCPCodes.VID_THRESHOLD, threshold);
	}

	/**
	 * @return the checkType
	 */
	public BusinessChecksType getCheckType()
	{
		return checkType;
	}

	/**
	 * @return the script
	 */
	public String getScript()
	{
		return script == null ? "" : script;
	}

	/**
	 * @return the threshold
	 */
	public int getThreshold()
	{
		return threshold;
	}

	/**
	 * @return the failureReason
	 */
	public String getFailureReason()
	{
		return failureReason;
	}

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description == null ? "" : description;
   }

   /**
    * @return the objectId
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @return the dciId
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @return the status
    */
   public boolean isViolated()
   {
      return violated;
   }

   /**
    * @param checkType the checkType to set
    */
   public void setCheckType(BusinessChecksType checkType)
   {
      this.checkType = checkType;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @param script the script to set
    */
   public void setScript(String script)
   {
      this.script = script;
   }

   /**
    * @param objectId the objectId to set
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   }

   /**
    * @param dciId the dciId to set
    */
   public void setDciId(long dciId)
   {
      this.dciId = dciId;
   }

   /**
    * @param threshold the threshold to set
    */
   public void setThreshold(int threshold)
   {
      this.threshold = threshold;
   }
}
