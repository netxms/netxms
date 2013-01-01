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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.StackLayout;
import org.eclipse.draw2d.text.FlowPage;
import org.eclipse.draw2d.text.TextFlow;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Tooltip for object on map
 */
public class ObjectTooltip extends Figure
{
	/**
	 * @param object
	 */
	public ObjectTooltip(GenericObject object)
	{
		setBorder(new MarginBorder(3));
		setLayoutManager(new StackLayout());
		
		FlowPage page = new FlowPage();
		add(page);
		
		TextFlow text = new TextFlow();
		StringBuilder sb = new StringBuilder(object.getObjectName());
		sb.append("\nStatus: ");
		sb.append(StatusDisplayInfo.getStatusText(object.getStatus()));
		if ((object instanceof Node) && !((Node)object).getPrimaryIP().isAnyLocalAddress())
		{
			sb.append("\nPrimary IP: ");
			sb.append(((Node)object).getPrimaryIP().getHostAddress());
		}
		if (!object.getComments().isEmpty())
		{
			sb.append("\n\n");
			sb.append(object.getComments());
		}
		text.setText(sb.toString());
		page.add(text);
	}
}
