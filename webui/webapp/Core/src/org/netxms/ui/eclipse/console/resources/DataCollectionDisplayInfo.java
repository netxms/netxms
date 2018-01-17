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

import org.netxms.client.constants.DataType;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Data collection display information
 *
 */
public class DataCollectionDisplayInfo
{
	private String dciDataTypes[] = new String[9];
	
   /**
    * Get status display instance for current display
    * 
    * @return
    */
   private static DataCollectionDisplayInfo getInstance()
   {
      DataCollectionDisplayInfo instance = (DataCollectionDisplayInfo)ConsoleSharedData.getProperty("DataCollectionDisplayInfo");
      if (instance == null)
      {
         instance = new DataCollectionDisplayInfo();
         ConsoleSharedData.setProperty("DataCollectionDisplayInfo", instance);
      }
      return instance;
   }

   /**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	private DataCollectionDisplayInfo()
	{
      dciDataTypes[DataType.COUNTER32.getValue()] = Messages.get().DataCollectionDisplayInfo_Counter32;
      dciDataTypes[DataType.COUNTER64.getValue()] = Messages.get().DataCollectionDisplayInfo_Counter64;
		dciDataTypes[DataType.INT32.getValue()] = Messages.get().DataCollectionDisplayInfo_Integer;
		dciDataTypes[DataType.UINT32.getValue()] = Messages.get().DataCollectionDisplayInfo_UInteger;
		dciDataTypes[DataType.INT64.getValue()] = Messages.get().DataCollectionDisplayInfo_Integer64;
		dciDataTypes[DataType.UINT64.getValue()] = Messages.get().DataCollectionDisplayInfo_UInteger64;
		dciDataTypes[DataType.FLOAT.getValue()] = Messages.get().DataCollectionDisplayInfo_Float;
		dciDataTypes[DataType.STRING.getValue()] = Messages.get().DataCollectionDisplayInfo_String;
		dciDataTypes[DataType.NULL.getValue()] = Messages.get().DataCollectionDisplayInfo_Null;
	}

	/**
	 * Get name for given data type
	 * @param dt Data type ID
	 * @return data type name
	 */
	public static String getDataTypeName(DataType dt)
	{
		try
		{
			return getInstance().dciDataTypes[dt.getValue()];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return Messages.get().DataCollectionDisplayInfo_Unknown;
		}
	}
}
