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
package org.netxms.client.packages;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NetXMS package info
 */
public class PackageInfo
{
	private long id;
	private String name;
	private String description;
	private String fileName;
	private String platform;
	private String version;
	
	/**
	 * Create package information from NPI file
	 * 
	 * @param npiFile
	 * @throws IOException 
	 */
	public PackageInfo(File npiFile) throws IOException
	{
		id = 0;
		BufferedReader reader = null;
		try
		{
			reader = new BufferedReader(new FileReader(npiFile));
			while(true)
			{
				String line = reader.readLine();
				if (line == null)
					break;
				
				line = line.trim();
				if (line.startsWith("NAME "))
				{
					name = line.substring(5).trim();
				}
				else if (line.startsWith("PLATFORM "))
				{
					platform = line.substring(9).trim();
				}
				else if (line.startsWith("VERSION "))
				{
					version = line.substring(8).trim();
				}
				else if (line.startsWith("DESCRIPTION "))
				{
					description = line.substring(12).trim();
				}
				else if (line.startsWith("FILE "))
				{
					fileName = line.substring(5).trim();
				}
			}
		}
		finally
		{
			if (reader != null)
				reader.close();
		}
	}
	
	/**
	 * Create package information from NXCP message
	 * 
	 * @param msg
	 */
	public PackageInfo(NXCPMessage msg)
	{
		id = msg.getFieldAsInt64(NXCPCodes.VID_PACKAGE_ID);
		name = msg.getFieldAsString(NXCPCodes.VID_PACKAGE_NAME);
		description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
		fileName = msg.getFieldAsString(NXCPCodes.VID_FILE_NAME);
		platform = msg.getFieldAsString(NXCPCodes.VID_PLATFORM_NAME);
		version = msg.getFieldAsString(NXCPCodes.VID_PACKAGE_VERSION);
	}
	
	/**
	 * Fill NXCP message with package information
	 *  
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
		msg.setFieldInt32(NXCPCodes.VID_PACKAGE_ID, (int)id);
		msg.setField(NXCPCodes.VID_PACKAGE_NAME, name);
		msg.setField(NXCPCodes.VID_DESCRIPTION, description);
		msg.setField(NXCPCodes.VID_FILE_NAME, fileName);
		msg.setField(NXCPCodes.VID_PLATFORM_NAME, platform);
		msg.setField(NXCPCodes.VID_PACKAGE_VERSION, version);
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the fileName
	 */
	public String getFileName()
	{
		return fileName;
	}

	/**
	 * @return the platform
	 */
	public String getPlatform()
	{
		return platform;
	}

	/**
	 * @return the version
	 */
	public String getVersion()
	{
		return version;
	}
}
