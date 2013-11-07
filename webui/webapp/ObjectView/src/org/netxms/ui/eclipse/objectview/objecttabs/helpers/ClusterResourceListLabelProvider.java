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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.ClusterTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for cluster resource list
 */
public class ClusterResourceListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session;
	
	/**
	 * Default constructor
	 */
	public ClusterResourceListLabelProvider()
	{
		session = (NXCSession)ConsoleSharedData.getSession();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == 0)
			return StatusDisplayInfo.getStatusImage((((ClusterResource)element).getCurrentOwner() != 0) ? Severity.NORMAL : Severity.MAJOR); 
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
			case ClusterTab.COLUMN_NAME:
				return ((ClusterResource)element).getName();
			case ClusterTab.COLUMN_VIP:
				return ((ClusterResource)element).getVirtualAddress().getHostAddress();
			case ClusterTab.COLUMN_OWNER:
				long ownerId = ((ClusterResource)element).getCurrentOwner();
				if (ownerId > 0)
				{
					Node owner = (Node)session.findObjectById(ownerId, Node.class);
					return (owner != null) ? owner.getObjectName() : "<" + Long.toString(ownerId) + ">";
				}
				else
				{
					return "NONE";
				}
		}
		return null;
	}
}
