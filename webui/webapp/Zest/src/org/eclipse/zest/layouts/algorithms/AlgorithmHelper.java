/*******************************************************************************
 * Copyright (c) 2005-2010 The Chisel Group and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group - initial API and implementation
 *               Mateusz Matela 
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.layouts.algorithms;

import org.eclipse.zest.layouts.dataStructures.DisplayIndependentDimension;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentPoint;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.EntityLayout;

public class AlgorithmHelper {

	public static int MIN_NODE_SIZE = 8;

	public static double PADDING_PERCENT = 0.8;

	/**
	 * Fits given entities within given bounds, preserving their relative
	 * locations.
	 * 
	 * @param entities
	 * @param destinationBounds
	 * @param resize
	 */
	public static void fitWithinBounds(EntityLayout[] entities,
			DisplayIndependentRectangle destinationBounds, boolean resize) {
		DisplayIndependentRectangle startingBounds = getLayoutBounds(entities,
				false);
		double sizeScale = Math.min(destinationBounds.width
				/ startingBounds.width, destinationBounds.height
				/ startingBounds.height);
		if (entities.length == 1) {
			fitSingleEntity(entities[0], destinationBounds, resize);
			return;
		}
		for (int i = 0; i < entities.length; i++) {
			EntityLayout entity = entities[i];
			DisplayIndependentDimension size = entity.getSize();
			if (entity.isMovable()) {
				DisplayIndependentPoint location = entity.getLocation();
				double percentX = (location.x - startingBounds.x)
						/ (startingBounds.width);
				double percentY = (location.y - startingBounds.y)
						/ (startingBounds.height);

				if (resize && entity.isResizable()) {
					size.width *= sizeScale;
					size.height *= sizeScale;
					entity.setSize(size.width, size.height);
				}

				location.x = destinationBounds.x + size.width / 2 + percentX
						* (destinationBounds.width - size.width);
				location.y = destinationBounds.y + size.height / 2 + percentY
						* (destinationBounds.height - size.height);
				entity.setLocation(location.x, location.y);

			} else if (resize && entity.isResizable()) {
				entity.setSize(size.width * sizeScale, size.height * sizeScale);
			}
		}
	}

	private static void fitSingleEntity(EntityLayout entity,
			DisplayIndependentRectangle destinationBounds, boolean resize) {
		if (entity.isMovable()) {
			entity.setLocation(destinationBounds.x + destinationBounds.width
					/ 2, destinationBounds.y + destinationBounds.height / 2);
		}
		if (resize && entity.isResizable()) {
			double width = destinationBounds.width;
			double height = destinationBounds.height;
			double preferredAspectRatio = entity.getPreferredAspectRatio();
			if (preferredAspectRatio > 0) {
				DisplayIndependentDimension fixedSize = fixAspectRatio(width,
						height, preferredAspectRatio);
				entity.setSize(fixedSize.width, fixedSize.height);
			} else {
				entity.setSize(width, height);
			}
		}
	}

	/**
	 * Resizes the nodes so that they have a maximal area without overlapping
	 * each other, with additional empty space of 20% of node's width (or
	 * height, if bigger). It does nothing if there's less than two nodes.
	 * 
	 * @param entities
	 */
	public static void maximizeSizes(EntityLayout[] entities) {
		if (entities.length > 1) {
			DisplayIndependentDimension minDistance = getMinimumDistance(entities);
			double nodeSize = Math.max(minDistance.width, minDistance.height)
					* PADDING_PERCENT;
			double width = nodeSize;
			double height = nodeSize;
			for (int i = 0; i < entities.length; i++) {
				EntityLayout entity = entities[i];
				if (entity.isResizable()) {
					double preferredRatio = entity.getPreferredAspectRatio();
					if (preferredRatio > 0) {
						DisplayIndependentDimension fixedSize = fixAspectRatio(
								width, height, preferredRatio);
						entity.setSize(fixedSize.width, fixedSize.height);
					} else {
						entity.setSize(width, height);
					}
				}
			}
		}
	}

	private static DisplayIndependentDimension fixAspectRatio(double width,
			double height, double preferredRatio) {
		double actualRatio = width / height;
		if (actualRatio > preferredRatio) {
			width = height * preferredRatio;
			if (width < MIN_NODE_SIZE) {
				width = MIN_NODE_SIZE;
				height = width / preferredRatio;
			}
		}
		if (actualRatio < preferredRatio) {
			height = width / preferredRatio;
			if (height < MIN_NODE_SIZE) {
				height = MIN_NODE_SIZE;
				width = height * preferredRatio;
			}
		}
		return new DisplayIndependentDimension(width, height);
	}

	/**
	 * Find the bounds in which the nodes are located. Using the bounds against
	 * the real bounds of the screen, the nodes can proportionally be placed
	 * within the real bounds. The bounds can be determined either including the
	 * size of the nodes or not. If the size is not included, the bounds will
	 * only be guaranteed to include the center of each node.
	 */
	public static DisplayIndependentRectangle getLayoutBounds(
			EntityLayout[] entities, boolean includeNodeSize) {
		double rightSide = Double.NEGATIVE_INFINITY;
		double bottomSide = Double.NEGATIVE_INFINITY;
		double leftSide = Double.POSITIVE_INFINITY;
		double topSide = Double.POSITIVE_INFINITY;
		for (int i = 0; i < entities.length; i++) {
			EntityLayout entity = entities[i];
			DisplayIndependentPoint location = entity.getLocation();
			DisplayIndependentDimension size = entity.getSize();
			if (includeNodeSize) {
				leftSide = Math.min(location.x - size.width / 2, leftSide);
				topSide = Math.min(location.y - size.height / 2, topSide);
				rightSide = Math.max(location.x + size.width / 2, rightSide);
				bottomSide = Math.max(location.y + size.height / 2, bottomSide);
			} else {
				leftSide = Math.min(location.x, leftSide);
				topSide = Math.min(location.y, topSide);
				rightSide = Math.max(location.x, rightSide);
				bottomSide = Math.max(location.y, bottomSide);
			}
		}
		return new DisplayIndependentRectangle(leftSide, topSide, rightSide
				- leftSide, bottomSide - topSide);
	}

	/**
	 * minDistance is the closest that any two points are together. These two
	 * points become the center points for the two closest nodes, which we wish
	 * to make them as big as possible without overlapping. This will be the
	 * maximum of minDistanceX and minDistanceY minus a bit, lets say 20%
	 * 
	 * We make the recommended node size a square for convenience.
	 * 
	 * <pre>
	 *    _______
	 *   |       | 
	 *   |       |
	 *   |   +   |
	 *   |   |\  |
	 *   |___|_\_|_____
	 *       | | \     |
	 *       | |  \    |
	 *       +-|---+   |
	 *         |       |
	 *         |_______|
	 * </pre>
	 * 
	 * 
	 */
	public static DisplayIndependentDimension getMinimumDistance(
			EntityLayout[] entities) {
		DisplayIndependentDimension horAndVertdistance = new DisplayIndependentDimension(
				Double.MAX_VALUE, Double.MAX_VALUE);
		double minDistance = Double.MAX_VALUE;

		// TODO: Very Slow!
		for (int i = 0; i < entities.length; i++) {
			DisplayIndependentPoint location1 = entities[i].getLocation();
			for (int j = i + 1; j < entities.length; j++) {
				DisplayIndependentPoint location2 = entities[j].getLocation();
				double distanceX = location1.x - location2.x;
				double distanceY = location1.y - location2.y;
				double distance = distanceX * distanceX + distanceY * distanceY;

				if (distance < minDistance) {
					minDistance = distance;
					horAndVertdistance.width = Math.abs(distanceX);
					horAndVertdistance.height = Math.abs(distanceY);
				}
			}
		}
		return horAndVertdistance;
	}
}
