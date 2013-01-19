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
package org.eclipse.zest.layouts.dataStructures;

/**
 * This is a rectangle that isn't dependent on awt, swt, or any other library,
 * except layout.
 * 
 * @author Casey Best
 */
public class DisplayIndependentRectangle {

	public double x, y, width, height;

	public DisplayIndependentRectangle() {
		this.x = 0;
		this.y = 0;
		this.width = 0;
		this.height = 0;
	}

	public DisplayIndependentRectangle(double x, double y, double width,
			double height) {
		this.x = x;
		this.y = y;
		this.width = width;
		this.height = height;
	}

	public DisplayIndependentRectangle(DisplayIndependentRectangle rect) {
		this.x = rect.x;
		this.y = rect.y;
		this.width = rect.width;
		this.height = rect.height;
	}

	public String toString() {
		return "(" + x + ", " + y + ", " + width + ", " + height + ")";
	}

	public boolean intersects(DisplayIndependentRectangle rect) {
		return rect.x < x + width && rect.y < y + height
				&& rect.x + rect.width > x && rect.y + rect.height > y;
	}
}
