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
 * This is a dimension that isn't dependent on awt, swt, or any other library,
 * except layout.
 * 
 * @author Casey Best
 */
public class DisplayIndependentDimension {
	public double width, height;

	public DisplayIndependentDimension(double width, double height) {
		this.width = width;
		this.height = height;
	}

	public DisplayIndependentDimension(DisplayIndependentDimension dimension) {
		this.width = dimension.width;
		this.height = dimension.height;
	}

	public String toString() {
		return "(" + width + ", " + height + ")";
	}
}
