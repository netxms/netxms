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
import org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.InternalNode;
import org.eclipse.zest.layouts.dataStructures.InternalRelationship;

/**
 * Sparse tree (prevent node overlapping) after TreeLayout
 *
 */
public class SparseTree extends AbstractLayoutAlgorithm
{
	private static final int ROW_BINDING_DELTA = 10;
	private static final int MINIMAL_DISTANCE = 10;
	
	/**
	 * @param styles
	 */
	public SparseTree(int styles)
	{
		super(styles);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#applyLayoutInternal(org.eclipse.zest.layouts.dataStructures.InternalNode[], org.eclipse.zest.layouts.dataStructures.InternalRelationship[], double, double, double, double)
	 */
	@Override
	protected void applyLayoutInternal(InternalNode[] entitiesToLayout, InternalRelationship[] relationshipsToConsider,
			double boundsX, double boundsY, double boundsWidth, double boundsHeight)
	{
		ArrayList<ArrayList<InternalNode>> rows = new ArrayList<ArrayList<InternalNode>>();
		for(int i = 0; i < entitiesToLayout.length; i++)
			addToRow(rows, entitiesToLayout[i]);

		// Sort rows by vertical position
		Collections.sort(rows, new Comparator<ArrayList<InternalNode>>() {
			@Override
			public int compare(ArrayList<InternalNode> o1, ArrayList<InternalNode> o2)
			{
				return (int)(o1.get(0).getLayoutEntity().getYInLayout() - o2.get(0).getLayoutEntity().getYInLayout());
			}
		});
		
		// Sort elements within rows by horizontal position and shift
		// any overlapping elements to the right
		for(int currRowIdx = 0; currRowIdx < rows.size(); currRowIdx++)
		{
			final ArrayList<InternalNode> row = rows.get(currRowIdx);
			Collections.sort(row, new Comparator<InternalNode>() {
				@Override
				public int compare(InternalNode node1, InternalNode node2)
				{
					return (int)(node1.getLayoutEntity().getXInLayout() - node2.getLayoutEntity().getXInLayout());
				}
			});
			
			for(int currNodeIdx = 1; currNodeIdx < row.size(); currNodeIdx++)
			{
				final InternalNode currNode = row.get(currNodeIdx);
				final InternalNode prevNode = row.get(currNodeIdx - 1);
				double currNodePos = currNode.getLayoutEntity().getXInLayout();
				double prevNodePos = prevNode.getLayoutEntity().getXInLayout();
				double minimalPos = prevNodePos + prevNode.getLayoutEntity().getWidthInLayout() + MINIMAL_DISTANCE;
				if (currNodePos < minimalPos)
				{
					currNode.setLocation(minimalPos, currNode.getLayoutEntity().getYInLayout());
				}
			}
		}

		// Center parent nodes relatively to child nodes
		for(int currRowIdx = rows.size() - 1; currRowIdx > 0; currRowIdx--)
		{
			final ArrayList<InternalNode> row = rows.get(currRowIdx);
			double leftmostX = row.get(0).getLayoutEntity().getXInLayout();
			int firstChildIdx = 0;
			InternalNode parent = getParentNode(row.get(0), relationshipsToConsider);
			for(int currNodeIdx = 1; currNodeIdx < row.size(); currNodeIdx++)
			{
				InternalNode currNode = row.get(currNodeIdx);
				InternalNode nextParent = getParentNode(currNode, relationshipsToConsider);
				if (parent != nextParent)
				{
					if (parent != null)
						centerNode(rows, currRowIdx, firstChildIdx, parent, relationshipsToConsider,
								leftmostX, row.get(currNodeIdx - 1).getLayoutEntity().getXInLayout() + row.get(currNodeIdx - 1).getLayoutEntity().getWidthInLayout());
					parent = nextParent;
					leftmostX = currNode.getLayoutEntity().getXInLayout();
					firstChildIdx = currNodeIdx;
				}
			}
			if (parent != null)
			{
				InternalNode currNode = row.get(row.size() - 1);
				centerNode(rows, currRowIdx, firstChildIdx, parent, relationshipsToConsider,
						leftmostX, currNode.getLayoutEntity().getXInLayout() + currNode.getLayoutEntity().getWidthInLayout());
			}
		}
	}
	
	/**
	 * Center parent node relatively to child nodes
	 * 
	 * @param rows row list
	 * @param currRowIdx current row (contains child nodes)
	 * @param firstChildIdx index of first child node
	 * @param node current node
	 * @param relationships graph's relationships
	 * @param leftmostX position of leftmost child node
	 * @param rightmostX position of rightmost child node + it's length
	 */
	private void centerNode(ArrayList<ArrayList<InternalNode>> rows, int currRowIdx, int firstChildIdx,
			InternalNode node, InternalRelationship[] relationships, double leftmostX, double rightmostX)
	{
		double newX = leftmostX + (rightmostX - leftmostX) / 2 - node.getLayoutEntity().getWidthInLayout() / 2;
		double shift = newX - node.getLayoutEntity().getXInLayout();
		if (shift > 0) 
		{
			final ArrayList<InternalNode> row = rows.get(currRowIdx - 1);
			node.setLocation(newX, node.getLayoutEntity().getYInLayout());
			int index = row.indexOf(node);
			if (index != -1)
			{
				for(int i = index + 1; i < row.size(); i++)
				{
					InternalNode n = row.get(i);
					n.setLocation(n.getLayoutEntity().getXInLayout() + shift, n.getLayoutEntity().getYInLayout());
				}
			}
		}
		else
		{
			// Child nodes are to the left of parent, shift them right
			final ArrayList<InternalNode> row = rows.get(currRowIdx);
			for(int i = firstChildIdx; i < row.size(); i++)
			{
				InternalNode n = row.get(i);
				n.setLocation(n.getLayoutEntity().getXInLayout() - shift, n.getLayoutEntity().getYInLayout());
				shiftChildNodes(rows, currRowIdx + 1, n, relationships, -shift);
			}
		}
	}

	/**
	 * Shift recursively all children of given node
	 * 
	 * @param rows row list
	 * @param currRowIdx current row
	 * @param parent parent node
	 * @param relationships graph's relationships
	 * @param shift shift value
	 */
	private void shiftChildNodes(ArrayList<ArrayList<InternalNode>> rows, int currRowIdx,
			InternalNode parent, InternalRelationship[] relationships, double shift)
	{
		if (rows.size() <= currRowIdx)
			return;
		
		final ArrayList<InternalNode> row = rows.get(currRowIdx);
		for(InternalNode n : row)
		{
			if (getParentNode(n, relationships) == parent)
			{
				n.setLocation(n.getLayoutEntity().getXInLayout() + shift, n.getLayoutEntity().getYInLayout());
				shiftChildNodes(rows, currRowIdx + 1, n, relationships, shift);
			}
		}
	}
	
	/**
	 * Find parent node for given node
	 * 
	 * @param node
	 * @param relationships
	 * @return
	 */
	private InternalNode getParentNode(InternalNode node, InternalRelationship[] relationships)
	{
		for(InternalRelationship r : relationships)
		{
			if (r.getDestination() == node)
				return r.getSource();
		}
		return null;
	}
	
	/**
	 * Add node to appropriate row
	 * @param rows rows
	 * @param node node
	 */
	private void addToRow(ArrayList<ArrayList<InternalNode>> rows, InternalNode node)
	{
		double nodeY = node.getLayoutEntity().getYInLayout();
		for(ArrayList<InternalNode> row : rows)
		{
			double rowY = row.get(0).getLayoutEntity().getYInLayout();
			if (Math.abs(nodeY - rowY) <= ROW_BINDING_DELTA)
			{
				row.add(node);
				return;
			}
		}
		
		ArrayList<InternalNode> newRow = new ArrayList<InternalNode>();
		newRow.add(node);
		rows.add(newRow);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#getCurrentLayoutStep()
	 */
	@Override
	protected int getCurrentLayoutStep()
	{
		return 0;
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
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#isValidConfiguration(boolean, boolean)
	 */
	@Override
	protected boolean isValidConfiguration(boolean asynchronous, boolean continuous)
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#postLayoutAlgorithm(org.eclipse.zest.layouts.dataStructures.InternalNode[], org.eclipse.zest.layouts.dataStructures.InternalRelationship[])
	 */
	@Override
	protected void postLayoutAlgorithm(InternalNode[] entitiesToLayout, InternalRelationship[] relationshipsToConsider)
	{
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
	 * @see org.eclipse.zest.layouts.algorithms.AbstractLayoutAlgorithm#setLayoutArea(double, double, double, double)
	 */
	@Override
	public void setLayoutArea(double x, double y, double width, double height)
	{
	}
}
