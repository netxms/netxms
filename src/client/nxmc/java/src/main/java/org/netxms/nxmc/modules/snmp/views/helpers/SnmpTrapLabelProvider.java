/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.views.SnmpTrapEditor;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for SNMP trap configuration editor
 *
 */
public class SnmpTrapLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private I18n i18n = LocalizationHelper.getI18n(SnmpTrapLabelProvider.class);
   private NXCSession session;
	
	/**
	 * The constructor
	 */
	public SnmpTrapLabelProvider()
	{
      session = Registry.getSession();
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
		SnmpTrap trap = (SnmpTrap)element;
		switch(columnIndex)
		{
			case SnmpTrapEditor.COLUMN_ID:
				return Long.toString(trap.getId());
			case SnmpTrapEditor.COLUMN_TRAP_OID:
				return trap.getObjectId().toString();
			case SnmpTrapEditor.COLUMN_EVENT:
				EventTemplate evt = session.findEventTemplateByCode(trap.getEventCode());
            return (evt != null) ? evt.getName() : i18n.tr("Unknown");
			case SnmpTrapEditor.COLUMN_DESCRIPTION:
				return trap.getDescription();
		}
		return null;
	}
}
