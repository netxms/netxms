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
package org.netxms.ui.eclipse.console.resources;

import org.netxms.client.datacollection.DataCollectionItem;

/**
 * Data collection display information
 *
 */
public class DataCollectionDisplayInfo
{
	private static String dciDataTypes[] = new String[7];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
		dciDataTypes[DataCollectionItem.DT_INT] = "Integer";
		dciDataTypes[DataCollectionItem.DT_UINT] = "Unsigned Integer";
		dciDataTypes[DataCollectionItem.DT_INT64] = "Integer 64-bit";
		dciDataTypes[DataCollectionItem.DT_UINT64] = "Unsigned Integer 64-bit";
		dciDataTypes[DataCollectionItem.DT_FLOAT] = "Float";
		dciDataTypes[DataCollectionItem.DT_STRING] = "String";
		dciDataTypes[DataCollectionItem.DT_NULL] = "Null";
	}

	/**
	 * Get name for given data type
	 * @param dt Data type ID
	 * @return data type name
	 */
	public static String getDataTypeName(int dt)
	{
		try
		{
			return dciDataTypes[dt];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "Unknown";
		}
	}
}
