/*******************************************************************************
 * Copyright (c) 2005-2010 The Chisel Group and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group - initial API and implementation
 *               Mateusz Matela 
 *               Ian Bull
 *               Miles Parker - optional node space configuration
 ******************************************************************************/
package org.eclipse.zest.layouts.algorithms;

import java.util.Iterator;

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.TreeLayoutObserver.TreeNode;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.EntityLayout;
import org.eclipse.zest.layouts.interfaces.LayoutContext;

/**
 * The TreeLayoutAlgorithm class implements a simple algorithm to arrange graph
 * nodes in a layered tree-like layout.
 * 
 * @version 3.0
 * @author Mateusz Matela
 * @author Casey Best and Rob Lintern (version 2.0)
 * @author Jingwei Wu (version 1.0)
 */
public class TreeLayoutAlgorithm implements LayoutAlgorithm {

	/**
	 * Tree direction constant for which root is placed at the top and branches
	 * spread downwards
	 */
	public final static int TOP_DOWN = 1;

	/**
	 * Tree direction constant for which root is placed at the bottom and
	 * branches spread upwards
	 */
	public final static int BOTTOM_UP = 2;

	/**
	 * Tree direction constant for which root is placed at the left and branches
	 * spread to the right
	 */
	public final static int LEFT_RIGHT = 3;

	/**
	 * Tree direction constant for which root is placed at the right and
	 * branches spread to the left
	 */
	public final static int RIGHT_LEFT = 4;

	private int direction = TOP_DOWN;

	private boolean resize = false;

	private LayoutContext context;

	private DisplayIndependentRectangle bounds;

	private double leafSize, layerSize;

	private TreeLayoutObserver treeObserver;

	private Dimension nodeSpace;

	/**
	 * Create a default Tree Layout.
	 */
	public TreeLayoutAlgorithm() {
	}

	/**
	 * Create a Tree Layout with a specified direction.
	 * 
	 * @param direction
	 *            The direction, one of {@link TreeLayoutAlgorithm#BOTTOM_UP},
	 *            {@link TreeLayoutAlgorithm#LEFT_RIGHT},
	 *            {@link TreeLayoutAlgorithm#RIGHT_LEFT},
	 *            {@link TreeLayoutAlgorithm#TOP_DOWN}
	 */
	public TreeLayoutAlgorithm(int direction) {
		this(direction, null);
	}

	/**
	 * Create a Tree Layout with fixed size spacing around nodes. If nodeSpace
	 * is not null, the layout will size the container to the ideal space to
	 * just contain all nodes of fixed size without any overlap. Otherwise, the
	 * algorithm will size for the container's available space.
	 * 
	 * @param direction
	 *            The direction, one of {@link TreeLayoutAlgorithm#BOTTOM_UP},
	 *            {@link TreeLayoutAlgorithm#LEFT_RIGHT},
	 *            {@link TreeLayoutAlgorithm#RIGHT_LEFT},
	 *            {@link TreeLayoutAlgorithm#TOP_DOWN}
	 * @param nodeSpace
	 *            the size to make each node. May be null.
	 */
	public TreeLayoutAlgorithm(int direction, Dimension nodeSpace) {
		setDirection(direction);
		this.nodeSpace = nodeSpace;
	}

	/**
	 * @param nodeSpaceSize
	 *            the nodeSpaceSize to set
	 */
	public void setNodeSpace(Dimension nodeSpace) {
		this.nodeSpace = nodeSpace;
	}

	public int getDirection() {
		return direction;
	}

	public void setDirection(int direction) {
		if (direction == TOP_DOWN || direction == BOTTOM_UP
				|| direction == LEFT_RIGHT || direction == RIGHT_LEFT)
			this.direction = direction;
		else
			throw new IllegalArgumentException("Invalid direction: "
					+ direction);
	}

	/**
	 * 
	 * @return true if this algorithm is set to resize elements
	 */
	public boolean isResizing() {
		return resize;
	}

	/**
	 * 
	 * @param resizing
	 *            true if this algorithm should resize elements (default is
	 *            false)
	 */
	public void setResizing(boolean resizing) {
		resize = resizing;
	}

	public void setLayoutContext(LayoutContext context) {
		if (treeObserver != null) {
			treeObserver.stop();
		}
		this.context = context;
		if (context != null) {
			treeObserver = new TreeLayoutObserver(context, null);
		}
	}

	public void applyLayout(boolean clean) {
		if (!clean)
			return;

		internalApplyLayout();

		EntityLayout[] entities = context.getEntities();

		if (resize)
			AlgorithmHelper.maximizeSizes(entities);
		scaleEntities(entities);
	}

	private void scaleEntities(EntityLayout[] entities) {
		if (nodeSpace == null) {
			DisplayIndependentRectangle bounds2 = new DisplayIndependentRectangle(
					bounds);
			int insets = 4;
			bounds2.x += insets;
			bounds2.y += insets;
			bounds2.width -= 2 * insets;
			bounds2.height -= 2 * insets;
			AlgorithmHelper.fitWithinBounds(entities, bounds2, resize);
		}
	}

	void internalApplyLayout() {
		TreeNode superRoot = treeObserver.getSuperRoot();
		bounds = context.getBounds();
		updateLeafAndLayerSizes();
		int leafCountSoFar = 0;
		for (Iterator iterator = superRoot.getChildren().iterator(); iterator
				.hasNext();) {
			TreeNode rootInfo = (TreeNode) iterator.next();
			computePositionRecursively(rootInfo, leafCountSoFar);
			leafCountSoFar = leafCountSoFar + rootInfo.numOfLeaves;
		}
	}

	private void updateLeafAndLayerSizes() {
		if (nodeSpace != null) {
			if (getDirection() == TOP_DOWN || getDirection() == BOTTOM_UP) {
				leafSize = nodeSpace.preciseWidth();
				layerSize = nodeSpace.preciseHeight();
			} else {
				leafSize = nodeSpace.preciseHeight();
				layerSize = nodeSpace.preciseWidth();
			}
		} else {
			TreeNode superRoot = treeObserver.getSuperRoot();
			if (direction == TOP_DOWN || direction == BOTTOM_UP) {
				leafSize = bounds.width / superRoot.numOfLeaves;
				layerSize = bounds.height / superRoot.height;
			} else {
				leafSize = bounds.height / superRoot.numOfLeaves;
				layerSize = bounds.width / superRoot.height;
			}
		}
	}

	/**
	 * Computes positions recursively until the leaf nodes are reached.
	 */
	private void computePositionRecursively(TreeNode entityInfo,
			int relativePosition) {
		double breadthPosition = relativePosition + entityInfo.numOfLeaves
				/ 2.0;
		double depthPosition = (entityInfo.depth + 0.5);

		switch (direction) {
		case TOP_DOWN:
			entityInfo.getNode().setLocation(breadthPosition * leafSize,
					depthPosition * layerSize);
			break;
		case BOTTOM_UP:
			entityInfo.getNode().setLocation(breadthPosition * leafSize,
					bounds.height - depthPosition * layerSize);
			break;
		case LEFT_RIGHT:
			entityInfo.getNode().setLocation(depthPosition * layerSize,
					breadthPosition * leafSize);
			break;
		case RIGHT_LEFT:
			entityInfo.getNode().setLocation(
					bounds.width - depthPosition * layerSize,
					breadthPosition * leafSize);
			break;
		}

		for (Iterator iterator = entityInfo.children.iterator(); iterator
				.hasNext();) {
			TreeNode childInfo = (TreeNode) iterator.next();
			computePositionRecursively(childInfo, relativePosition);
			relativePosition += childInfo.numOfLeaves;
		}
	}
}
