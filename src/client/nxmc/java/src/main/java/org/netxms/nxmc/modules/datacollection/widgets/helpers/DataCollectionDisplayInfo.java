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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.netxms.client.constants.DataType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Data collection display information
 */
public class DataCollectionDisplayInfo
{
   private static I18n i18n = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class);
	private static String dciDataTypes[] = new String[9];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
      dciDataTypes[DataType.COUNTER32.getValue()] = i18n.tr("Counter 32-bit");
      dciDataTypes[DataType.COUNTER64.getValue()] = i18n.tr("Counter 64-bit");
		dciDataTypes[DataType.INT32.getValue()] = i18n.tr("Integer");
		dciDataTypes[DataType.UINT32.getValue()] = i18n.tr("Unsigned Integer");
		dciDataTypes[DataType.INT64.getValue()] = i18n.tr("Integer 64-bit");
		dciDataTypes[DataType.UINT64.getValue()] = i18n.tr("Unsigned Integer 64-bit");
		dciDataTypes[DataType.FLOAT.getValue()] = i18n.tr("Float");
		dciDataTypes[DataType.STRING.getValue()] = i18n.tr("String");
		dciDataTypes[DataType.NULL.getValue()] = i18n.tr("Null");
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
			return dciDataTypes[dt.getValue()];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return i18n.tr("<unknown>");
		}
	}
}
