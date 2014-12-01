/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Software package information
 */
public class SoftwarePackage
{
	private String name;
	private String description;
	private String version;
	private String vendor;
	private String supportUrl;
	private Date installDate;
	private Object data;
	
	/**
	 * Create software package information from NXCP message
	 * 
	 * @param msg
	 * @param baseId
	 */
	protected SoftwarePackage(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		version = msg.getFieldAsString(baseId + 1);
		vendor = msg.getFieldAsString(baseId + 2);
		installDate = msg.getFieldAsDate(baseId + 3);
		supportUrl = msg.getFieldAsString(baseId + 4);
		description = msg.getFieldAsString(baseId + 5);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return (name != null) ? name : "";
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return (description != null) ? description : "";
	}

	/**
	 * @return the version
	 */
	public String getVersion()
	{
		return (version != null) ? version : "";
	}

	/**
	 * @return the vendor
	 */
	public String getVendor()
	{
		return (vendor != null) ? vendor : "";
	}

	/**
	 * @return the supportUrl
	 */
	public String getSupportUrl()
	{
		return (supportUrl != null) ? supportUrl : "";
	}

	/**
	 * @return the installDate
	 */
	public Date getInstallDate()
	{
		return installDate;
	}
	
	/**
	 * @return
	 */
	public long getInstallDateMs()
	{
		return (installDate != null) ? installDate.getTime() : 0;
	}

	/**
	 * Get value of custom data field.
	 * 
	 * @return the data
	 */
	public Object getData()
	{
		return data;
	}

	/**
	 * Set custom data field.
	 * 
	 * @param data the data to set
	 */
	public void setData(Object data)
	{
		this.data = data;
	}
}
