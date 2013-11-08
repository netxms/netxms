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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import org.eclipse.gef4.zest.layouts.LayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.interfaces.EntityLayout;
import org.eclipse.gef4.zest.layouts.interfaces.LayoutContext;

/**
 * Sparse tree (prevent node overlapping) after TreeLayout
 *
 */
public class SparseTree implements LayoutAlgorithm
{
	private static final int ROW_BINDING_DELTA = 10;
	private static final int MINIMAL_DISTANCE = 10;
	
	private LayoutContext context;

	/* (non-Javadoc)
	 * @see org.eclipse.gef4.zest.layouts.LayoutAlgorithm#setLayoutContext(org.eclipse.gef4.zest.layouts.interfaces.LayoutContext)
	 */
	@Override
	public void setLayoutContext(LayoutContext context)
	{
		this.context = context;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.gef4.zest.layouts.LayoutAlgorithm#applyLayout(boolean)
	 */
	@Override
	public void applyLayout(boolean clean)
	{
		EntityLayout[] entitiesToLayout = context.getEntities();

		ArrayList<ArrayList<EntityLayout>> rows = new ArrayList<ArrayList<EntityLayout>>();
		for(int i = 0; i < entitiesToLayout.length; i++)
			addToRow(rows, entitiesToLayout[i]);

		// Sort rows by vertical position
		Collections.sort(rows, new Comparator<ArrayList<EntityLayout>>() {
			@Override
			public int compare(ArrayList<EntityLayout> o1, ArrayList<EntityLayout> o2)
			{
				return (int)(o1.get(0).getLocation().y - o2.get(0).getLocation().y);
			}
		});
		
		// Sort elements within rows by horizontal position and shift
		// any overlapping elements to the right
		for(int currRowIdx = 0; currRowIdx < rows.size(); currRowIdx++)
		{
			final ArrayList<EntityLayout> row = rows.get(currRowIdx);
			Collections.sort(row, new Comparator<EntityLayout>() {
				@Override
				public int compare(EntityLayout node1, EntityLayout node2)
				{
					return (int)(node1.getLocation().x - node2.getLocation().x);
				}
			});
			
			for(int currNodeIdx = 1; currNodeIdx < row.size(); currNodeIdx++)
			{
				final EntityLayout currNode = row.get(currNodeIdx);
				final EntityLayout prevNode = row.get(currNodeIdx - 1);
				double currNodePos = currNode.getLocation().x;
				double prevNodePos = prevNode.getLocation().x;
				double minimalPos = prevNodePos + prevNode.getSize().width + MINIMAL_DISTANCE;
				if (currNodePos < minimalPos)
				{
					currNode.setLocation(minimalPos, currNode.getLocation().y);
				}
			}
		}

		// Center parent nodes relatively to child nodes
		for(int currRowIdx = rows.size() - 1; currRowIdx > 0; currRowIdx--)
		{
			final ArrayList<EntityLayout> row = rows.get(currRowIdx);
			double leftmostX = row.get(0).getLocation().x;
			int firstChildIdx = 0;
			EntityLayout parent = getParentNode(row.get(0));
			for(int currNodeIdx = 1; currNodeIdx < row.size(); currNodeIdx++)
			{
				EntityLayout currNode = row.get(currNodeIdx);
				EntityLayout nextParent = getParentNode(currNode);
				if (parent != nextParent)
				{
					if (parent != null)
						centerNode(rows, currRowIdx, firstChildIdx, parent,
								leftmostX, row.get(currNodeIdx - 1).getLocation().x + row.get(currNodeIdx - 1).getSize().width);
					parent = nextParent;
					leftmostX = currNode.getLocation().x;
					firstChildIdx = currNodeIdx;
				}
			}
			if (parent != null)
			{
				EntityLayout currNode = row.get(row.size() - 1);
				centerNode(rows, currRowIdx, firstChildIdx, parent, leftmostX, currNode.getLocation().x + currNode.getSize().width);
			}
		}
	}
	
	/**
	 * @param e
	 * @return
	 */
	private EntityLayout getParentNode(EntityLayout e)
	{
		EntityLayout[] parents = e.getPredecessingEntities();
		return (parents.length > 0) ? parents[0] : null;
	}
	
	/**
	 * Center parent node relatively to child nodes
	 * 
	 * @param rows row list
	 * @param currRowIdx current row (contains child nodes)
	 * @param firstChildIdx index of first child node
	 * @param node current node
	 * @param leftmostX position of leftmost child node
	 * @param rightmostX position of rightmost child node + it's length
	 */
	private void centerNode(ArrayList<ArrayList<EntityLayout>> rows, int currRowIdx, int firstChildIdx,
			EntityLayout node, double leftmostX, double rightmostX)
	{
		double newX = leftmostX + (rightmostX - leftmostX) / 2 - node.getSize().width / 2;
		double shift = newX - node.getLocation().x;
		if (shift > 0) 
		{
			final ArrayList<EntityLayout> row = rows.get(currRowIdx - 1);
			node.setLocation(newX, node.getLocation().y);
			int index = row.indexOf(node);
			if (index != -1)
			{
				for(int i = index + 1; i < row.size(); i++)
				{
					EntityLayout n = row.get(i);
					n.setLocation(n.getLocation().x + shift, n.getLocation().y);
				}
			}
		}
		else
		{
			// Child nodes are to the left of parent, shift them right
			final ArrayList<EntityLayout> row = rows.get(currRowIdx);
			for(int i = firstChildIdx; i < row.size(); i++)
			{
				EntityLayout n = row.get(i);
				n.setLocation(n.getLocation().x - shift, n.getLocation().y);
				shiftChildNodes(rows, currRowIdx + 1, n, -shift);
			}
		}
	}

	/**
	 * Shift recursively all children of given node
	 * 
	 * @param rows row list
	 * @param currRowIdx current row
	 * @param parent parent node
	 * @param shift shift value
	 */
	private void shiftChildNodes(ArrayList<ArrayList<EntityLayout>> rows, int currRowIdx, EntityLayout parent, double shift)
	{
		if (rows.size() <= currRowIdx)
			return;
		
		final ArrayList<EntityLayout> row = rows.get(currRowIdx);
		for(EntityLayout n : row)
		{
			if (getParentNode(n) == parent)
			{
				n.setLocation(n.getLocation().x + shift, n.getLocation().y);
				shiftChildNodes(rows, currRowIdx + 1, n, shift);
			}
		}
	}
	
	/**
	 * Add node to appropriate row
	 * @param rows rows
	 * @param node node
	 */
	private void addToRow(ArrayList<ArrayList<EntityLayout>> rows, EntityLayout node)
	{
		double nodeY = node.getLocation().y;
		for(ArrayList<EntityLayout> row : rows)
		{
			double rowY = row.get(0).getLocation().y;
			if (Math.abs(nodeY - rowY) <= ROW_BINDING_DELTA)
			{
				row.add(node);
				return;
			}
		}
		
		ArrayList<EntityLayout> newRow = new ArrayList<EntityLayout>();
		newRow.add(node);
		rows.add(newRow);
	}
}
