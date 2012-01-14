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
package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.ui.eclipse.snmp.SnmpConstants;
import org.netxms.ui.eclipse.snmp.views.MibExplorer;

/**
 * Label provider for SNMP values
 *
 */
public class SnmpValueLabelProvider extends LabelProvider implements ITableLabelProvider
{
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
		SnmpValue value = (SnmpValue)element;
		switch(columnIndex)
		{
			case MibExplorer.COLUMN_NAME:
				return value.getName();
			case MibExplorer.COLUMN_TYPE:
				if (value.getType() == 0xFFFF)
					return "Hex-STRING";
				return SnmpConstants.getAsnTypeName(value.getType());
			case MibExplorer.COLUMN_VALUE:
				return value.getValue();
		}
		return null;
	}
}
