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
package org.netxms.ui.eclipse.objecttools.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objecttools.ObjectToolTableColumn;

/**
 * Label provider for ObjectToolTableColumn objects
 *
 */
public class ToolColumnLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] formatName = { "String", "Integer", "Float", "IP Address", "MAC Address", "IfIndex" };
	
	private boolean snmpTable;
	
	/**
	 * The constructor
	 * 
	 * @param snmpTable
	 */
	public ToolColumnLabelProvider(boolean snmpTable)
	{
		this.snmpTable = snmpTable;
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
				return ((ObjectToolTableColumn)element).getName();
			case 1:
				return formatName[((ObjectToolTableColumn)element).getFormat()];
			case 2:
				return snmpTable ? ((ObjectToolTableColumn)element).getSnmpOid() : Integer.toString(((ObjectToolTableColumn)element).getSubstringIndex());
		}
		return null;
	}
}
