/*******************************************************************************
 * Copyright (c) 2011 Simon Templer.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    Simon Templer - introduced this interface to allow custom figures being 
 *        configured with text and icon through the GraphNode class, associated 
 *        to bug 335136  
 *******************************************************************************/
package org.eclipse.zest.core.widgets;

import org.eclipse.draw2d.IFigure;
import org.eclipse.swt.graphics.Image;

/**
 * Marks a figure that allows being configured with a text and icon through a
 * {@link GraphNode} from the model.
 * 
 * @author Simon Templer
 */
public interface ILabeledFigure extends IFigure {

	/**
	 * Set the label text. If needed, the figure should adjust its size.
	 * 
	 * @param text
	 *            the text
	 */
	public void setText(String text);

	/**
	 * Get the label text.
	 * 
	 * @return the label text
	 */
	public String getText();

	/**
	 * Set the label icon. If needed, the figure should adjust its size.
	 * 
	 * @param icon
	 *            the icon image or <code>null</code> for no icon
	 */
	public void setIcon(Image icon);

	/**
	 * Get the label icon.
	 * 
	 * @return the icon image
	 */
	public Image getIcon();

}
