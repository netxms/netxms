/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.propertypages.helpers;

import java.util.HashMap;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.snmp.SnmpObjectId;

/**
 * Label provider for list of table column definitions
 */
public class TableColumnLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final long serialVersionUID = 1L;

	private HashMap<Integer, String> dtTexts = new HashMap<Integer, String>();
	
	/**
	 * Default constructor
	 */
	public TableColumnLabelProvider()
	{
		dtTexts.put(DataCollectionItem.DT_INT, "Integer");
		dtTexts.put(DataCollectionItem.DT_UINT, "Unsigned Integer");
		dtTexts.put(DataCollectionItem.DT_INT64, "Int64");
		dtTexts.put(DataCollectionItem.DT_UINT64, "Unsigned Int64");
		dtTexts.put(DataCollectionItem.DT_FLOAT, "Float");
		dtTexts.put(DataCollectionItem.DT_STRING, "String");
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case 0:
			case 1:
				return ((ColumnDefinition)element).getName();
			case 2:
				return dtTexts.get(((ColumnDefinition)element).getDataType());
			case 3:
				SnmpObjectId oid = ((ColumnDefinition)element).getSnmpObjectId();
				return (oid != null) ? oid.toString() : null;
		}
		return null;
	}
}
