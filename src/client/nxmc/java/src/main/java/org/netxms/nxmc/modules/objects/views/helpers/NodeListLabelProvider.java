/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.NodesView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for node list
 */
public class NodeListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(NodeListLabelProvider.class);
   private NXCSession session = Registry.getSession();
   
   /**
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
			case NodesView.COLUMN_SYS_DESCRIPTION:
				return node.getSystemDescription();
			case NodesView.COLUMN_ID:
				return Long.toString(node.getObjectId());
			case NodesView.COLUMN_NAME:
            return node.getNameWithAlias();
			case NodesView.COLUMN_STATUS:
				return StatusDisplayInfo.getStatusText(node.getStatus());
			case NodesView.COLUMN_IP_ADDRESS:
				return node.getPrimaryIP().isValidUnicastAddress() ? node.getPrimaryIP().getHostAddress() : null;
         case NodesView.COLUMN_PLATFORM:
            return node.getPlatformName();
         case NodesView.COLUMN_AGENT_VERSION:
            return node.getAgentVersion();
         case NodesView.COLUMN_RACK:
            if (node.getPhysicalContainerId() != 0)
            {
               Rack rack = session.findObjectById(node.getPhysicalContainerId(), Rack.class);
               if (rack != null)
               {
                  return String.format(i18n.tr("%s (units %d-%d)"), rack.getObjectName(),
                          rack.isTopBottomNumbering() ? node.getRackPosition() : node.getRackPosition() - node.getRackHeight() + 1,
                          rack.isTopBottomNumbering() ? node.getRackPosition() + node.getRackHeight() - 1 : node.getRackPosition());
               }
            }
            return ""; //$NON-NLS-1$
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
			case NodesView.COLUMN_STATUS:
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
