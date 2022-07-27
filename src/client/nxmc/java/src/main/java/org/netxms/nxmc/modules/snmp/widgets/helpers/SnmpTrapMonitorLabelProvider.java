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
package org.netxms.nxmc.modules.snmp.widgets.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.snmp.SnmpTrapLogRecord;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.widgets.SnmpTrapTraceWidget;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for SNMP trap monitor
 */
public class SnmpTrapMonitorLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(SnmpTrapMonitorLabelProvider.class);

   private NXCSession session = Registry.getSession();

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
		SnmpTrapLogRecord record = (SnmpTrapLogRecord)element;
		switch(columnIndex)
		{
			case SnmpTrapTraceWidget.COLUMN_TIMESTAMP:
            return DateFormatFactory.getDateTimeFormat().format(record.getTimestamp());
			case SnmpTrapTraceWidget.COLUMN_SOURCE_IP:
				return record.getSourceAddress().getHostAddress();
			case SnmpTrapTraceWidget.COLUMN_SOURCE_NODE:
				final AbstractObject object = session.findObjectById(record.getSourceNode());
            return (object != null) ? object.getObjectName() : i18n.tr("<unknown>");
			case SnmpTrapTraceWidget.COLUMN_OID:
				return record.getTrapObjectId();
			case SnmpTrapTraceWidget.COLUMN_VARBINDS:
				return record.getVarbinds();
		}
		return null;
	}
}
