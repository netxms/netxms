/*******************************************************************************
 * Copyright (c) 2011 Simon Templer.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    Simon Templer - introduced this interface to allow custom figures being 
 *        configured with foreground/background/border colors, font and border 
 *        width through the GraphNode class, associated to bug 335136  
 *******************************************************************************/
package org.eclipse.zest.core.widgets;

import org.eclipse.draw2d.IFigure;
import org.eclipse.swt.graphics.Color;

/**
 * Marks a figure that allows its style being configured through a
 * {@link GraphNode} from the model.<br>
 * <br>
 * If a figure implementing this interface is associated to a {@link GraphNode},
 * the foreground, background and border colors will be configured according to
 * the model, as well as the font and the border width.
 * 
 * @see IFigure#setFont(org.eclipse.swt.graphics.Font)
 * @see IFigure#setForegroundColor(Color)
 * @see IFigure#setBackgroundColor(Color)
 * 
 * @author Simon Templer
 */
public interface IStyleableFigure extends IFigure {

	/**
	 * Set the border color
	 * 
	 * @param borderColor
	 *            the border color
	 */
	void setBorderColor(Color borderColor);

	/**
	 * Set the border width
	 * 
	 * @param borderWidth
	 *            the border width
	 */
	void setBorderWidth(int borderWidth);

}
