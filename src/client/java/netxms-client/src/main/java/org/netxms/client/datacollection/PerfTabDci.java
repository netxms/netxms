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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * DCI information for performance tab in console
 */
public class PerfTabDci
{
   private long id; // DCI ID
   private long nodeId;
   private long templateDciId; // ID of related template DCI, or 0 for standalone DCIs
   private long rootTemplateDciId; // Root template DCI ID (for DCI created by instance discovery from template)
   private int status;
   private String description;
   private String perfTabSettings;
   private String instance;
   private String instanceName;

   /**
    * Create performance tab DCI information object from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    * @param nodeId owning node ID
    */
   public PerfTabDci(NXCPMessage msg, long baseId, long nodeId)
	{
      this.nodeId = nodeId;
		id = msg.getFieldAsInt64(baseId);
		description = msg.getFieldAsString(baseId + 1);
		status = msg.getFieldAsInt32(baseId + 2);
		perfTabSettings = msg.getFieldAsString(baseId + 3);
		templateDciId = msg.getFieldAsInt64(baseId + 5);
		instance = msg.getFieldAsString(baseId + 6);
		if (instance == null)
			instance = "";
      instanceName = msg.getFieldAsString(baseId + 7);
      if (instanceName == null)
         instanceName = instance;
      rootTemplateDciId = msg.getFieldAsInt64(baseId + 8);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the status
    */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the perfTabSettings
	 */
	public String getPerfTabSettings()
	{
		return perfTabSettings;
	}

	/**
	 * @return the templateDciId
	 */
	public final long getTemplateDciId()
	{
		return templateDciId;
	}

	/**
    * @return the rootTemplateDciId
    */
   public long getRootTemplateDciId()
   {
      return rootTemplateDciId;
   }

   /**
	 * @return the instance
	 */
	public final String getInstance()
	{
		return instance;
	}

   /**
    * @return the instanceName
    */
   public String getInstanceName()
   {
      return instanceName;
   }
}
