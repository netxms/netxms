/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objecttools.ObjectToolTableColumn;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for ObjectToolTableColumn objects
 *
 */
public class ToolColumnLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ToolColumnLabelProvider.class);
	private final String[] formatName = { i18n.tr("String"), i18n.tr("Integer"), i18n.tr("Float"), i18n.tr("IP Address"), i18n.tr("MAC Address"), i18n.tr("IfIndex") };

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
	
	/**
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

   /**
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
