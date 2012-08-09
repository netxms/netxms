/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.algorithms;

import org.eclipse.swt.widgets.Item;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.interfaces.EntityLayout;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Manual layout of graph nodes
 *
 */
public class ManualLayout implements LayoutAlgorithm
{
	private LayoutContext context;

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.LayoutAlgorithm#setLayoutContext(org.eclipse.zest.layouts.interfaces.LayoutContext)
	 */
	@Override
	public void setLayoutContext(LayoutContext context)
	{
		this.context = context;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.LayoutAlgorithm#applyLayout(boolean)
	 */
	@Override
	public void applyLayout(boolean clean)
	{
		EntityLayout[] entitiesToLayout = context.getEntities();
		for(int i = 0; i < entitiesToLayout.length; i++)
		{
			Item[] items = entitiesToLayout[i].getItems();
			if ((items.length > 0) && (items[0].getData() instanceof NetworkMapElement))
			{
				NetworkMapElement mapObject = (NetworkMapElement)items[0].getData();
				entitiesToLayout[i].setLocation(mapObject.getX(), mapObject.getY());
			}
		}
	}
}
