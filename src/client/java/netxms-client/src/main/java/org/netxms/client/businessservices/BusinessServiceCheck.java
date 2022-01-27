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
package org.netxms.client.businessservices;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.BusinessServiceCheckType;
import org.netxms.client.constants.BusinessServiceState;
import org.netxms.client.objects.interfaces.NodeItemPair;

/**
 * Business service check
 */
public class BusinessServiceCheck implements NodeItemPair
{	
	private long id;
   private long serviceId;
   private long prototypeServiceId;
   private long prototypeCheckId;
	private BusinessServiceCheckType type;
   private String description;
	private String script;
   private long objectId;
   private long dciId;
	private int threshold;
   private BusinessServiceState state;
	private String failureReason;

   /**
    * Default constructor 
    */
   public BusinessServiceCheck()
   {
      id = 0;
      serviceId = 0;
      prototypeServiceId = 0;
      prototypeCheckId = 0;
      type = BusinessServiceCheckType.NONE;
      description = null;
      script = null; 
      objectId = 0;
      dciId = 0;
      threshold = 0;
      state = BusinessServiceState.OPERATIONAL;
   }

   /**
    * Copy constructor
    * 
    * @param src check to copy
    */
   public BusinessServiceCheck(BusinessServiceCheck src)
   {
      id = src.id;
      serviceId = src.serviceId;
      prototypeServiceId = src.prototypeServiceId;
      prototypeCheckId = src.prototypeCheckId;
      type = src.type;
      description = src.description;
      script = src.script; 
      objectId = src.objectId;
      dciId = src.dciId;
      threshold = src.threshold;
      state = src.state;
   }

	/**
    * Constructor to create check from message
    * 
    * @param msg NXCPmessage from server with data
    * @param baseId base field ID for check
    */
   public BusinessServiceCheck(NXCPMessage msg, long baseId)
	{
	   id = msg.getFieldAsInt64(baseId);
		type = BusinessServiceCheckType.getByValue(msg.getFieldAsInt32(baseId + 1));
      failureReason = msg.getFieldAsString(baseId + 2);
      dciId = msg.getFieldAsInt64(baseId + 3);
      objectId = msg.getFieldAsInt64(baseId + 4);
      threshold = msg.getFieldAsInt32(baseId + 5); 
		description = msg.getFieldAsString(baseId + 6); 
      script = msg.getFieldAsString(baseId + 7);
      state = BusinessServiceState.getByValue(msg.getFieldAsInt32(baseId + 8));
      prototypeServiceId = msg.getFieldAsInt64(baseId + 9);
      prototypeCheckId = msg.getFieldAsInt64(baseId + 10);
      serviceId = msg.getFieldAsInt64(baseId + 11);
	}

	/**
	 * Fill message
	 * 
	 * @param msg NXCPMessage to fill data
	 */
	public void fillMessage(NXCPMessage msg)
	{
	   msg.setFieldInt32(NXCPCodes.VID_CHECK_ID, (int)id);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setFieldInt16(NXCPCodes.VID_BIZSVC_CHECK_TYPE, type.getValue());
      msg.setFieldInt32(NXCPCodes.VID_RELATED_OBJECT, (int)objectId);
      msg.setFieldInt32(NXCPCodes.VID_RELATED_DCI, (int)dciId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setFieldInt32(NXCPCodes.VID_THRESHOLD, threshold);
	}

	/**
	 * @return the checkType
	 */
	public BusinessServiceCheckType getCheckType()
	{
		return type;
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
    * @return the serviceId
    */
   public long getServiceId()
   {
      return serviceId;
   }

   /**
    * Get ID of business service prototype this check was created from. Will be equal to owning service ID if check was created by
    * instance discovery.
    *
    * @return ID of business service prototype or 0
    */
   public long getPrototypeServiceId()
   {
      return prototypeServiceId;
   }

   /**
    * Get ID of business service check from business service prototype this check was created from.
    *
    * @return ID of business service check from business service prototype or 0
    */
   public long getPrototypeCheckId()
   {
      return prototypeCheckId;
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
    * Get check state.
    *
    * @return check state
    */
   public BusinessServiceState getState()
   {
      return state;
   }

   /**
    * @param checkType the checkType to set
    */
   public void setCheckType(BusinessServiceCheckType checkType)
   {
      this.type = checkType;
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

   /**
    * @see org.netxms.client.objects.interfaces.NodeItemPair#getNodeId()
    */
   @Override
   public long getNodeId()
   {
      return objectId;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }
}
