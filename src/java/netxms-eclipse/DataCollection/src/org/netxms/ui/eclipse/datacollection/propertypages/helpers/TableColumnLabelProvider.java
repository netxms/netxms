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
package org.netxms.ui.eclipse.datacollection.propertypages.helpers;

import java.util.HashMap;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.ui.eclipse.datacollection.Messages;

/**
 * Label provider for list of table column definitions
 */
public class TableColumnLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private HashMap<Integer, String> dtTexts = new HashMap<Integer, String>();
	private HashMap<Integer, String> afTexts = new HashMap<Integer, String>();
	
	/**
	 * Default constructor
	 */
	public TableColumnLabelProvider()
	{
		dtTexts.put(DataCollectionItem.DT_INT, Messages.get().TableColumnLabelProvider_in32);
		dtTexts.put(DataCollectionItem.DT_UINT, Messages.get().TableColumnLabelProvider_uint32);
		dtTexts.put(DataCollectionItem.DT_INT64, Messages.get().TableColumnLabelProvider_int64);
		dtTexts.put(DataCollectionItem.DT_UINT64, Messages.get().TableColumnLabelProvider_uint64);
		dtTexts.put(DataCollectionItem.DT_FLOAT, Messages.get().TableColumnLabelProvider_float);
		dtTexts.put(DataCollectionItem.DT_STRING, Messages.get().TableColumnLabelProvider_string);
		
		afTexts.put(DataCollectionItem.DCF_FUNCTION_AVG, Messages.get().TableColumnLabelProvider_AVG);
		afTexts.put(DataCollectionItem.DCF_FUNCTION_MAX, Messages.get().TableColumnLabelProvider_MAX);
		afTexts.put(DataCollectionItem.DCF_FUNCTION_MIN, Messages.get().TableColumnLabelProvider_MIN);
		afTexts.put(DataCollectionItem.DCF_FUNCTION_SUM, Messages.get().TableColumnLabelProvider_SUM);
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
				return ((ColumnDefinition)element).getName();
			case 1:
				return ((ColumnDefinition)element).getDisplayName();
			case 2:
				return dtTexts.get(((ColumnDefinition)element).getDataType());
			case 3:
				return ((ColumnDefinition)element).isInstanceColumn() ? Messages.get().TableColumnLabelProvider_Yes : Messages.get().TableColumnLabelProvider_No;
			case 4:
				return afTexts.get(((ColumnDefinition)element).getAggregationFunction());
			case 5:
				SnmpObjectId oid = ((ColumnDefinition)element).getSnmpObjectId();
				return (oid != null) ? oid.toString() : null;
		}
		return null;
	}
}
