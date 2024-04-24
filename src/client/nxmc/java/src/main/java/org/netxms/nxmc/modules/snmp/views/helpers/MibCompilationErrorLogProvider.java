/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.snmp.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.Severity;
import org.netxms.client.snmp.MibCompilationLogEntry;
import org.netxms.client.snmp.MibCompilationLogEntry.MessageType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.views.MibFileManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for SNMP trap configuration editor
 *
 */
public class MibCompilationErrorLogProvider extends LabelProvider implements ITableLabelProvider
{
   private I18n i18n = LocalizationHelper.getI18n(MibCompilationErrorLogProvider.class);

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
	   if (columnIndex == 0)
         return (((MibCompilationLogEntry)element).getType() == MessageType.ERROR) ? StatusDisplayInfo.getStatusImage(Severity.CRITICAL) : StatusDisplayInfo.getStatusImage(Severity.MINOR);
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
	   MibCompilationLogEntry entry = (MibCompilationLogEntry)element;
		switch(columnIndex)
		{
			case MibFileManager.ERROR_COLUMN_SEVERITY:
            return entry.getType() == MessageType.ERROR ? i18n.tr("Error") : i18n.tr("Warning");
			case MibFileManager.ERROR_COLUMN_LOCATION:
				return entry.getFileName();
			case MibFileManager.ERROR_COLUMN_MESSAGE:
            return entry.getText();
		}
		return null;
	}
}
