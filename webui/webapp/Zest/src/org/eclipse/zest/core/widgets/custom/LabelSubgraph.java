/*******************************************************************************
 * Copyright (c) 2009-2010 Mateusz Matela and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: Mateusz Matela - initial API and implementation
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.core.widgets.custom;

import org.eclipse.draw2d.Label;
import org.eclipse.swt.graphics.Color;
import org.eclipse.zest.core.widgets.FigureSubgraph;
import org.eclipse.zest.core.widgets.internal.GraphLabel;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;

/**
 * A subgraph layout that displays a label showing number of items pruned within
 * it.
 * 
 * @since 2.0
 */
public class LabelSubgraph extends FigureSubgraph {

	private Color backgroundColor;
	private Color foregroundColor;

	/**
	 * Sets the foreground color of this subgraph (that is color of the text on
	 * the label).
	 * 
	 * @param color
	 *            color to set
	 */
	public void setForegroundColor(Color color) {
		figure.setForegroundColor(color);
	}

	/**
	 * Sets the background color of this subgraph's label.
	 * 
	 * @param color
	 *            color to set
	 */
	public void setBackgroundColor(Color color) {
		figure.setBackgroundColor(color);
	}

	protected void createFigure() {
		figure = new GraphLabel(false);
		figure.setForegroundColor(foregroundColor);
		figure.setBackgroundColor(backgroundColor);
		updateFigure();
	}

	protected void updateFigure() {
		((Label) figure).setText("" + nodes.size());
	}

	public LabelSubgraph(NodeLayout[] nodes, LayoutContext context,
			Color foregroundColor, Color backgroundColor) {
		super(nodes, context);
		this.foregroundColor = foregroundColor;
		this.backgroundColor = backgroundColor;
	}
}