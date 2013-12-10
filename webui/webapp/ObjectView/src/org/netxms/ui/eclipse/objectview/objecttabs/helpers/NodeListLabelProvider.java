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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.NodesTab;

/**
 * Label provider for node list
 */
public class NodeListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
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
		AbstractNode node = (AbstractNode)element;
		switch(columnIndex)
		{
			case NodesTab.COLUMN_SYS_DESCRIPTION:
				return node.getSystemDescription();
			case NodesTab.COLUMN_ID:
				return Long.toString(node.getObjectId());
			case NodesTab.COLUMN_NAME:
				return node.getObjectName();
			case NodesTab.COLUMN_STATUS:
				return StatusDisplayInfo.getStatusText(node.getStatus());
			case NodesTab.COLUMN_IP_ADDRESS:
				return node.getPrimaryIP().isAnyLocalAddress() ? null : node.getPrimaryIP().getHostAddress();
         case NodesTab.COLUMN_PLATFORM:
            return node.getPlatformName();
         case NodesTab.COLUMN_AGENT_VERSION:
            return node.getAgentVersion();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
	 */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
		AbstractNode node = (AbstractNode)element;
		switch(columnIndex)
		{
			case NodesTab.COLUMN_STATUS:
				return StatusDisplayInfo.getStatusColor(node.getStatus());
			default:
				return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
	 */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
	}
}
