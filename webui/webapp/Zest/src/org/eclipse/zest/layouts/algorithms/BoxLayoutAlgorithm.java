/*******************************************************************************
 * Copyright (c) 2009-2010 Mateusz Matela and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: Mateusz Matela - initial API and implementation
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.layouts.algorithms;

/**
 * Layout algorithm that places all elements in one column or one row, depending
 * on set orientation.
 */
public class BoxLayoutAlgorithm extends GridLayoutAlgorithm {

	public static final int HORIZONTAL = 1;

	public static final int VERTICAL = 2;

	private int orientation = HORIZONTAL;

	public BoxLayoutAlgorithm() {
	}

	public BoxLayoutAlgorithm(int orientation) {
		setOrientation(orientation);
	}

	public int getOrientation() {
		return orientation;
	}

	public void setOrientation(int orientation) {
		if (orientation == HORIZONTAL || orientation == VERTICAL)
			this.orientation = orientation;
		else
			throw new RuntimeException("Invalid orientation: " + orientation);
	}

	protected int[] calculateNumberOfRowsAndCols(int numChildren,
			double boundX, double boundY, double boundWidth, double boundHeight) {
		if (orientation == HORIZONTAL)
			return new int[] { numChildren, 1 };
		else
			return new int[] { 1, numChildren };
	}
}
