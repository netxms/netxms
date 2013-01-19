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

import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentDimension;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.EntityLayout;
import org.eclipse.zest.layouts.interfaces.LayoutContext;

/**
 * @version 2.0
 * @author Ian Bull
 * @author Casey Best and Rob Lintern
 */
public class GridLayoutAlgorithm implements LayoutAlgorithm {

	private static final double PADDING_PERCENTAGE = 0.95;
	private static final int MIN_ENTITY_SIZE = 5;

	protected double aspectRatio = 1.0;
	protected int rowPadding = 0;
	private boolean resize = false;
	protected int rows, cols, numChildren;
	protected double colWidth, rowHeight, offsetX, offsetY;
	protected double childrenHeight, childrenWidth;

	private LayoutContext context;

	/**
	 * @deprecated Since Zest 2.0, use {@link #GridLayoutAlgorithm()}.
	 */
	public GridLayoutAlgorithm(int style) {
		this();
		setResizing(style != LayoutStyles.NO_LAYOUT_NODE_RESIZING);
	}

	public GridLayoutAlgorithm() {
	}

	public void setLayoutContext(LayoutContext context) {
		this.context = context;
	}

	public void applyLayout(boolean clean) {
		if (!clean)
			return;
		DisplayIndependentRectangle bounds = context.getBounds();
		calculateGrid(bounds);
		applyLayoutInternal(context.getEntities(), bounds);
	}

	/**
	 * Calculates all the dimensions of grid that layout entities will be fit
	 * in. The following fields are set by this method: {@link #numChildren},
	 * {@link #rows}, {@link #cols}, {@link #colWidth}, {@link #rowHeight},
	 * {@link #offsetX}, {@link #offsetY}
	 * 
	 * @param bounds
	 */
	protected void calculateGrid(DisplayIndependentRectangle bounds) {
		numChildren = context.getNodes().length;
		int[] result = calculateNumberOfRowsAndCols(numChildren, bounds.x,
				bounds.y, bounds.width, bounds.height);
		cols = result[0];
		rows = result[1];

		colWidth = bounds.width / cols;
		rowHeight = bounds.height / rows;

		double[] nodeSize = calculateNodeSize(colWidth, rowHeight);
		childrenWidth = nodeSize[0];
		childrenHeight = nodeSize[1];
		offsetX = (colWidth - childrenWidth) / 2.0; // half of the space between
													// columns
		offsetY = (rowHeight - childrenHeight) / 2.0; // half of the space
														// between rows
	}

	/**
	 * Use this algorithm to layout the given entities and bounds. The entities
	 * will be placed in the same order as they are passed in, unless a
	 * comparator is supplied.
	 * 
	 * @param entitiesToLayout
	 *            apply the algorithm to these entities
	 * @param bounds
	 *            the bounds in which the layout can place the entities.
	 */
	protected synchronized void applyLayoutInternal(
			EntityLayout[] entitiesToLayout, DisplayIndependentRectangle bounds) {

		int index = 0;
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				if ((i * cols + j) < numChildren) {
					EntityLayout node = entitiesToLayout[index++];
					if (resize && node.isResizable())
						node.setSize(Math.max(childrenWidth, MIN_ENTITY_SIZE),
								Math.max(childrenHeight, MIN_ENTITY_SIZE));
					DisplayIndependentDimension size = node.getSize();
					double xmove = bounds.x + j * colWidth + offsetX
							+ size.width / 2;
					double ymove = bounds.y + i * rowHeight + offsetY
							+ size.height / 2;
					if (node.isMovable())
						node.setLocation(xmove, ymove);
				}
			}
		}
	}

	/**
	 * Calculates and returns an array containing the number of columns,
	 * followed by the number of rows
	 */
	protected int[] calculateNumberOfRowsAndCols(int numChildren,
			double boundX, double boundY, double boundWidth, double boundHeight) {
		if (aspectRatio == 1.0) {
			return calculateNumberOfRowsAndCols_square(numChildren, boundX,
					boundY, boundWidth, boundHeight);
		} else {
			return calculateNumberOfRowsAndCols_rectangular(numChildren);
		}
	}

	protected int[] calculateNumberOfRowsAndCols_square(int numChildren,
			double boundX, double boundY, double boundWidth, double boundHeight) {
		int rows = Math.max(1,
				(int) Math.sqrt(numChildren * boundHeight / boundWidth));
		int cols = Math.max(1,
				(int) Math.sqrt(numChildren * boundWidth / boundHeight));

		// if space is taller than wide, adjust rows first
		if (boundWidth <= boundHeight) {
			// decrease number of rows and columns until just enough or not
			// enough
			while (rows * cols > numChildren) {
				if (rows > 1)
					rows--;
				if (rows * cols > numChildren)
					if (cols > 1)
						cols--;
			}
			// increase number of rows and columns until just enough
			while (rows * cols < numChildren) {
				rows++;
				if (rows * cols < numChildren)
					cols++;
			}
		} else {
			// decrease number of rows and columns until just enough or not
			// enough
			while (rows * cols > numChildren) {
				if (cols > 1)
					cols--;
				if (rows * cols > numChildren)
					if (rows > 1)
						rows--;
			}
			// increase number of rows and columns until just enough
			while (rows * cols < numChildren) {
				cols++;
				if (rows * cols < numChildren)
					rows++;
			}
		}
		int[] result = { cols, rows };
		return result;
	}

	protected int[] calculateNumberOfRowsAndCols_rectangular(int numChildren) {
		int rows = Math.max(1, (int) Math.ceil(Math.sqrt(numChildren)));
		int cols = Math.max(1, (int) Math.ceil(Math.sqrt(numChildren)));
		int[] result = { cols, rows };
		return result;
	}

	protected double[] calculateNodeSize(double colWidth, double rowHeight) {
		double childW = Math
				.max(MIN_ENTITY_SIZE, PADDING_PERCENTAGE * colWidth);
		double childH = Math.max(MIN_ENTITY_SIZE, PADDING_PERCENTAGE
				* (rowHeight - rowPadding));
		double whRatio = colWidth / rowHeight;
		if (whRatio < aspectRatio) {
			childH = childW / aspectRatio;
		} else {
			childW = childH * aspectRatio;
		}
		double[] result = { childW, childH };
		return result;
	}

	/**
	 * Sets the padding between rows in the grid
	 * 
	 * @param rowPadding
	 *            padding - should be greater than or equal to 0
	 */
	public void setRowPadding(int rowPadding) {
		if (rowPadding >= 0) {
			this.rowPadding = rowPadding;
		}
	}

	/**
	 * Sets the preferred aspect ratio for layout entities. The default aspect
	 * ratio is 1.
	 * 
	 * @param aspectRatio
	 *            aspect ratio - should be greater than 0
	 */
	public void setAspectRatio(double aspectRatio) {
		if (aspectRatio > 0) {
			this.aspectRatio = aspectRatio;
		}
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

}
