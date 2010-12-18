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

import org.eclipse.zest.core.widgets.GraphItem;
import org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.InternalNode;
import org.eclipse.zest.layouts.dataStructures.InternalRelationship;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Manual layout of graph nodes
 *
 */
public class ManualLayout extends AbstractLayoutAlgorithm
{
	/**
	 * @param styles
	 */
	public ManualLayout(int styles)
	{
		super(styles);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#setLayoutArea(double, double, double, double)
	 */
	@Override
	public void setLayoutArea(double x, double y, double width, double height)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#isValidConfiguration(boolean, boolean)
	 */
	@Override
	protected boolean isValidConfiguration(boolean asynchronous, boolean continuous)
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#applyLayoutInternal(org.eclipse.zest.layouts.dataStructures.InternalNode[], org.eclipse.zest.layouts.dataStructures.InternalRelationship[], double, double, double, double)
	 */
	@Override
	protected void applyLayoutInternal(InternalNode[] entitiesToLayout, InternalRelationship[] relationshipsToConsider,
			double boundsX, double boundsY, double boundsWidth, double boundsHeight)
	{
		for(int i = 0; i < entitiesToLayout.length; i++)
		{
			GraphItem item = (GraphItem)entitiesToLayout[i].getLayoutEntity().getGraphData();
			if ((item != null) && (item.getData() instanceof NetworkMapElement))
			{
				NetworkMapElement mapObject = (NetworkMapElement)item.getData();
				entitiesToLayout[i].setLocation(mapObject.getX(), mapObject.getY());
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#preLayoutAlgorithm(org.eclipse.zest.layouts.dataStructures.InternalNode[], org.eclipse.zest.layouts.dataStructures.InternalRelationship[], double, double, double, double)
	 */
	@Override
	protected void preLayoutAlgorithm(InternalNode[] entitiesToLayout, InternalRelationship[] relationshipsToConsider, double x,
			double y, double width, double height)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#postLayoutAlgorithm(org.eclipse.zest.layouts.dataStructures.InternalNode[], org.eclipse.zest.layouts.dataStructures.InternalRelationship[])
	 */
	@Override
	protected void postLayoutAlgorithm(InternalNode[] entitiesToLayout, InternalRelationship[] relationshipsToConsider)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#getTotalNumberOfLayoutSteps()
	 */
	@Override
	protected int getTotalNumberOfLayoutSteps()
	{
		return 0;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#getCurrentLayoutStep()
	 */
	@Override
	protected int getCurrentLayoutStep()
	{
		return 0;
	}
}
